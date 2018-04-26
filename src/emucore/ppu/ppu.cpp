
#include <algorithm>
#include "ppu.h"
#include "cpu/cpu.h"
#include "internaldebug/internaldebug.h"
#include "dma/dmaunit.h"

int ppu_bgmode = 0;

namespace sch
{
    Ppu::Ppu()
    {
#ifdef IDBG_ENABLED
        InternalDebug.getPpuAddress = std::bind( &Ppu::getEffectiveVramAddr, this );
        InternalDebug.getPpuAddrInc = [this]() { return addrInc; };
        InternalDebug.getPpuPos = [this](int& h, int& v)
        {
            runTo(InternalDebug.getMainClock());
            h = curPos.H;
            v = curPos.V;
        };
        InternalDebug.getIrqPos = [this](int& h, int& v)
        {
            h = irqPos.H;
            v = irqPos.V;
            switch(irqMode)
            {
            case IrqMode::Disabled:
                h = v = -1;
                break;

            case IrqMode::V:        h = -1;     break;
            case IrqMode::H:        v = -1;     break;
            }
        };
#endif
    }

    void Ppu::reset(bool hard, EventManager* evt, DmaUnit* dma)
    {
        evtManager = evt;
        dmaUnit = dma;
        if(hard)
        {
            for(auto& i : bgLayers)
            {
                i.chrAddr = 0;
                i.scrollX = 0;
                i.scrollY = 0;
                i.tileMapAddr = 0;
                i.tileMapXOverflow = 0;
                i.tileMapYOverflow = 0;
                i.use16Tiles = false;
            }

            forceBlank = true;
            brightness_4bit = 0;
            brightnessMultiplier = 0;

            bgMode = 0;
            mode1AltPriority = false;
            scrollRegPrev = 0;
            
            addrIncOnHigh = true;
            addrInc = 1;
            vramRemapMode = 0;
            vramAddr = 0;

            cgAddr = 0;
            cgRegPrev = 0;
            cgRegToggle = false;

            manScrLayers = 0;
            subScrLayers = 0;

            for(auto& i : vram)
                i = 0;
            for(auto& i : cgRam)        i.reset();

            colorWindowClip = WindowMode::Never;
            colorWindowStopMath = WindowMode::Never;
            colorMathSubscr = false;
            direct256 = false;
            colorMathSubtract = false;
            colorHalfMath = false;
            colorMathLayers = 0;
            fixedColor.reset();

            ///////////////////////////
            objSizeMode = 0;
            objChrAddr[0] = objChrAddr[1] = 0;
            oamAddrLoad = 0;
            oamAddr = 0;
            oamBuffer = 0;
            useAltSpr0 = false;
            for(auto& i : oamLow)   i = 0;
            for(auto& i : oamHigh)  i = 0;
            for(auto& i : sprites)  i.reset();

            nmiEnabled = false;
            irqPending = false;
            irqMode = IrqMode::Disabled;
            vblReadFlag = false;

            irqPos.H = irqPos.V = 0;
            
            latchPos.H = latchPos.V = 0;
            latchPosToggle = false;

            ////////////////////////////////
            m7_ScrollX =            0;
            m7_ScrollY =            0;
            m7_ClipToTM =           false;
            m7_FillWithTile0 =      false;
            m7_FlipX =              false;
            m7_FlipY =              false;
            m7_WriteBuffer =        0;
            m7_Matrix[0] =          0;
            m7_Matrix[1] =          0;
            m7_Matrix[2] =          0;
            m7_Matrix[3] =          0;
            m7_CenterX =            0;
            m7_CenterY =            0;
            m7_MultB =              0;
            m7_MultResult =         0;
        }
    }

    void Ppu::frameStart(Cpu* c, const VideoSettings& vid)
    {
        evtManager->setLineCutoff(241);

        video = vid;
        cpu = c;

        linesRendered =     0;
        oddFrame =          !oddFrame;
        v0_time =           Time::Never;
        vEnd_time =         Time::Never;
        nmiHasHappened =    false;

        curTick =           0;
        curPos.V =          241;
        curPos.H =          0;

        // Set some events
        evtManager->addEvent( 1, 262, this, EventCode::CatchUp );   // possible time for v0_time to be set
        evtManager->addEvent( 1, 263, this, EventCode::CatchUp );   // The other possible time
        addIrqEvent();                                              // for when the next IRQ will happen
        evtManager->addEvent( 1, 224, this, EventCode::CatchUp );   // possible time for start of VBlank
        evtManager->addEvent( 1, 240, this, EventCode::CatchUp );   // The other possible time

        // HDMA events
        evtManager->addEvent( 6, 0, this, EventCode::HdmaStart );
        for(int i = 0; i <= 0xE0; ++i)
            evtManager->addEvent( 278, i, this, EventCode::HdmaLine );
        for(int i = 0xE1; i < 0xE0; ++i)
            evtManager->addEvent( 278, i, this, EventCode::HdmaLineOverscan );
    }


    inline u16 Ppu::getEffectiveVramAddr() const
    {
        /*
            00 = No remapping
            01 = Remap addressing aaaaaaaaBBBccccc => aaaaaaaacccccBBB
            10 = Remap addressing aaaaaaaBBBcccccc => aaaaaaaccccccBBB
            11 = Remap addressing aaaaaaBBBccccccc => aaaaaacccccccBBB
            */
        switch(vramRemapMode)
        {
        default:
        case 0:         return vramAddr & 0x7FFF;
        case 1:         return (vramAddr & 0x7F00) | ((vramAddr >> 5) & 7) | ((vramAddr & 0x1F) << 3);
        case 2:         return (vramAddr & 0x7E00) | ((vramAddr >> 6) & 7) | ((vramAddr & 0x3F) << 3);
        case 3:         return (vramAddr & 0x7C00) | ((vramAddr >> 7) & 7) | ((vramAddr & 0x7F) << 3);
        }

        return 0;       // should never reach here
    }
    
    void Ppu::regWrite(u16 a, u8 v)
    {
        unsigned tmp;

        switch(a)
        {
        case 0x2100:
            forceBlank = (v & 0x80) != 0;
            brightness_4bit = v & 0x0F;
            brightnessMultiplier = (0xFF00 * brightness_4bit) / (0x0F * 0x1F);
            break;
            
        case 0x2101:    w_2101(v);      break;
        case 0x2102:    w_2102(v);      break;
        case 0x2103:    w_2103(v);      break;
        case 0x2104:    w_2104(v);      break;

        case 0x2105:
            bgMode =                    v & 0x07;
            ppu_bgmode = bgMode;        // TODO remove this
            mode1AltPriority =          (v & 0x08) != 0;
            bgLayers[0].use16Tiles =    (v & 0x10) != 0;
            bgLayers[1].use16Tiles =    (v & 0x20) != 0;
            bgLayers[2].use16Tiles =    (v & 0x40) != 0;
            bgLayers[3].use16Tiles =    (v & 0x80) != 0;
            break;

        case 0x2106:
            // TODO
            break;

        case 0x2107: case 0x2108: case 0x2109: case 0x210A:
            tmp = a - 0x2107;
            bgLayers[tmp].tileMapAddr =         (v & 0xFC) << 8;
            bgLayers[tmp].tileMapXOverflow =    (v & 0x01) ? 0x400 : 0;
            bgLayers[tmp].tileMapYOverflow =    (v & 0x02) ? (bgLayers[tmp].tileMapXOverflow + 0x400) : 0;
            break;

        case 0x210B: case 0x210C:
            tmp = (a - 0x210B) << 1;
            bgLayers[tmp+0].chrAddr =           (v & 0x0F) << 12;
            bgLayers[tmp+1].chrAddr =           (v & 0xF0) << 8;
            break;

        case 0x210D:
            m7_ScrollX = (v << 8) | m7_WriteBuffer;
            m7_ScrollX = ((m7_ScrollX & 0x1FFF) ^ 0x1000) - 0x1000;
            m7_WriteBuffer = v;
                // no break
        case 0x210F: case 0x2111: case 0x2113:
            tmp = (a - 0x210D) >> 1;
            bgLayers[tmp].scrollX = ((bgLayers[tmp].scrollX >> 8) & 7)
                                        |   (scrollRegPrev & ~7)
                                        |   (v << 8);
            scrollRegPrev = v;
            break;
            
        case 0x210E:
            m7_ScrollY = (v << 8) | m7_WriteBuffer;
            m7_ScrollY = ((m7_ScrollY & 0x1FFF) ^ 0x1000) - 0x1000;
            m7_WriteBuffer = v;
                // no break
        case 0x2110: case 0x2112: case 0x2114:
            tmp = (a - 0x210E) >> 1;
            bgLayers[tmp].scrollY = scrollRegPrev | (v << 8);
            scrollRegPrev = v;
            break;

        case 0x2115:
            addrIncOnHigh =         (v & 0x80) != 0;
            if(v & 0x02)            addrInc = 128;
            else                    addrInc = (v & 0x01) ? 32 : 1;
            vramRemapMode =         (v >> 2) & 3;
            break;

        case 0x2116:
            vramAddr =              (vramAddr & 0xFF00) | v;
            break;
        case 0x2117:
            vramAddr =              (vramAddr & 0x00FF) | (v << 8);
            break;

        case 0x2118:
            tmp = getEffectiveVramAddr();
            vram[tmp] = (vram[tmp] & 0xFF00) | v;
            if(!addrIncOnHigh)
                vramAddr += addrInc;
            break;
        case 0x2119:
            tmp = getEffectiveVramAddr();
            vram[tmp] = (vram[tmp] & 0x00FF) | (v << 8);
            if(addrIncOnHigh)
                vramAddr += addrInc;
            break;
            
        case 0x211A:
            m7_ClipToTM =       !(v & 0x80);
            m7_FillWithTile0 =  (v & 0x40) != 0;
            m7_FlipY =          (v & 0x02) != 0;
            m7_FlipX =          (v & 0x01) != 0;
            break;

        case 0x211B: case 0x211C: case 0x211D: case 0x211E:
            a -= 0x211B;
            m7_Matrix[a] = (v << 8) | m7_WriteBuffer;
            m7_Matrix[a] = ((m7_Matrix[a] & 0xFFFF) ^ 0x8000) - 0x8000;
            m7_WriteBuffer = v;

            if(a == 1)      m7_MultB = v;
            m7_MultResult = m7_Matrix[0] * m7_MultB;
            break;

        case 0x211F:
            m7_CenterX = (v << 8) | m7_WriteBuffer;
            m7_CenterX = ((m7_CenterX & 0x1FFF) ^ 0x1000) - 0x1000;
            m7_WriteBuffer = v;
            break;
        case 0x2120:
            m7_CenterY = (v << 8) | m7_WriteBuffer;
            m7_CenterY = ((m7_CenterY & 0x1FFF) ^ 0x1000) - 0x1000;
            m7_WriteBuffer = v;
            break;

        case 0x2121:
            cgAddr = v;
            cgRegToggle = false;        // should this be done here?
            break;
        case 0x2122:
            if(cgRegToggle)     cgRam[cgAddr++].from15Bit( (v << 8) | cgRegPrev );
            else                cgRegPrev = v;
            cgRegToggle = !cgRegToggle;
            break;

            
        case 0x2123:
        case 0x2124:
        case 0x2125:
        case 0x2126:
        case 0x2127:
        case 0x2128:
        case 0x2129:
        case 0x212A:
        case 0x212B:
            // TODO Window bullshit
            break;
            
        case 0x212C:        manScrLayers = v;       break;
        case 0x212D:        subScrLayers = v;       break;
            
        case 0x212E:
        case 0x212F:
            // TODO window bullshit
            break;
            
        case 0x2130:
            colorWindowClip =       static_cast<WindowMode>((v >> 6) & 3);
            colorWindowStopMath =   static_cast<WindowMode>((v >> 4) & 3);
            colorMathSubscr =       (v & 0x02) != 0;
            direct256 =             (v & 0x01) != 0;
            break;

        case 0x2131:
            colorMathSubtract =     (v & 0x80) != 0;
            colorHalfMath =         (v & 0x40) != 0;
            colorMathLayers =       (v & 0x3F);
            break;

        case 0x2132:
            if(v & 0x20)        fixedColor.r = (v & 0x1F);
            if(v & 0x40)        fixedColor.g = (v & 0x1F);
            if(v & 0x80)        fixedColor.b = (v & 0x1F);
            break;

        case 0x2133:
            // TODO interlace and overscan and UGH
            break;

        case 0x4200:
            nmiEnabled =        (v & 0x80) != 0;

            irqMode = static_cast<IrqMode>((v >> 4) & 3);
            acknowledgeIrq();
            addIrqEvent();
            break;

        case 0x4207:
            irqPos.H &= 0x100;
            irqPos.H |= v;
            addIrqEvent();
            break;
        case 0x4208:
            irqPos.H &= 0x0FF;
            irqPos.H |= ((v & 0x01) << 8);
            addIrqEvent();
            break;
        case 0x4209:
            irqPos.V &= 0x100;
            irqPos.V |= v;
            addIrqEvent();
            break;
        case 0x420A:
            irqPos.V &= 0x0FF;
            irqPos.V |= ((v & 0x01) << 8);
            addIrqEvent();
            break;
        }
    }

    void Ppu::regRead(u16 a, u8& v)
    {
        static const Coord      vbl_start = {0xE1, 0x16};  // TODO this is hacky
        static const Coord      vbl_end   = {   0, 0x1E};  // TODO this is hacky
        // TODO
        switch(a)
        {
        case 0x2134:    v = static_cast<u8>( m7_MultResult        & 0xFF);      break;
        case 0x2135:    v = static_cast<u8>((m7_MultResult >>  8) & 0xFF);      break;
        case 0x2136:    v = static_cast<u8>((m7_MultResult >> 16) & 0xFF);      break;

        case 0x2137:
            latchPos = curPos;
            latchPosToggle = false;
            break;

        case 0x213C:
            if(latchPosToggle)      v = (v & 0xFE) | ((latchPos.H >> 8) & 1);
            else                    v = latchPos.H & 0xFF;
            latchPosToggle = !latchPosToggle;
            break;
            
        case 0x213D:
            if(latchPosToggle)      v = (v & 0xFE) | ((latchPos.V >> 8) & 1);
            else                    v = latchPos.V & 0xFF;
            latchPosToggle = !latchPosToggle;
            break;

        case 0x4210:
            v &= ~0x8F;
            v |= 2;
            if(vblReadFlag)     v |= 0x80;
            vblReadFlag = false;
            break;

        case 0x4211:
            v &= ~0x80;
            if(irqPending)      v |= 0x80;
            acknowledgeIrq();
            addIrqEvent();
            break;

        case 0x4212:
            // TODO this is a bit hacky.  Maybe touch this up later?
            v &= ~0xC1;
            if(curPos > vbl_start || curPos < vbl_end)      v |= 0x80;      // TODO this is hacky.  Slapped this in for Turtles in Time
            if(curPos.H < 13 || curPos.H > 121)             v |= 0x40;      // Is this right???
            break;
        }
    }
    
    void Ppu::runTo(timestamp_t runto)
    {
        if(frameIsOver())           // don't run past end of frame
            return;

        Coord   targetPos = getCoordFromTimestamp(runto);
        if(curPos >= targetPos) return; // we're already caught up

        // TODO put in IRQ stuff at some point

        // Scenario 1!  Partial scanline
        if((curPos.V == targetPos.V) && (curPos.H > 0))
        {
            // we already did the work for this line (at cyc 0).  Just count up the cycles and don't really do anything
            checkIrqOnLine(curPos.V, curPos.H, targetPos.H);
            advance(targetPos.H - curPos.H);
        }
        // Scenario 2!  We're either at the exact start of a line, or the target is on a different line
        else
        {
            // if we're not at the start, fill out the rest of the line
            if(curPos.H > 0)
            {
                checkIrqOnLine(curPos.V, curPos.H, 341);
                targetPos.V += advance(341 - curPos.H);
            }

            // Now start doing full scanlines!
            while((curPos.V < targetPos.V) && !frameIsOver())
            {
                doScanline(curPos.V);
                checkIrqOnLine(curPos.V, 0, 341);
                targetPos.V += advance(341);
            }
            // Now, curPos.V == targetPos.V .. run to proper H time
            //    curPos.H should be 0 at this time
            if(targetPos.H > 0 && !frameIsOver())
            {
                doScanline(curPos.V);
                checkIrqOnLine(curPos.V, 0, targetPos.H);
                advance(targetPos.H);
            }
        }
    }


    inline Ppu::Coord Ppu::getCoordFromTimestamp(timestamp_t t)
    {
        Coord out;
        out.H = out.V = 0;
        if(t > v0_time)     t -= v0_time;
        else                out.V = 241;
        
        t /= 4;     // TODO move the 4 somewhere

        out.V += (t / 341);
        out.H += (t % 341);

        return out;
    }

    timestamp_t Ppu::convertHVToTimestamp(int H, int V)
    {
        if(H < 0 || H >= 341 || V < 0)
            return Time::Never;

        if(V >= 241)
        {
            V -= 241;
            return ((V * 341) + H) * 4;     // TODO - move this '4'
        }
        else if(v0_time != Time::Never)
        {
            return v0_time + ((V * 341) + H) * 4;     // TODO - move this '4'
        }

        // if we got here, we need v0 time but don't have it, so the timestamp can't
        //   be computed yet
        return Time::Never;
    }

    // Advance returns any V counter adjustment that needs to be made.  Normally this is zero
    int Ppu::advance(timestamp_t cycs)
    {
        if(frameIsOver())
            return 0;

        int line_adj = 0;

        curTick += (cycs * 4);      // TODO move this '4' somewhere
        curPos.H += cycs;
        while(curPos.H >= 341)
        {
            curPos.H -= 341;
            ++curPos.V;
            //    b) when in interlace mode, even frames are 263 scanlines long
            if(curPos.V > 262 || (curPos.V == 262 && (!interlaceMode || oddFrame)))
            {
                line_adj = -curPos.V;
                v0_time = (curPos.V - 241) * 341 * 4;     // TODO move this '4' somewhere
                evtManager->setLineCutoff(0);
                curPos.V = 0;
            }
        }

        return line_adj;
    }

    inline bool Ppu::frameIsOver() const
    {
        return vEnd_time != Time::Never;
    }

    void Ppu::doScanline(int line)
    {
        // see if we trigger an NMI on this line
        if(!nmiHasHappened)
        {
            if(overscanMode && line == 240)         nmiHasHappened = true;
            if(!overscanMode && line == 224)        nmiHasHappened = true;

            if(nmiHasHappened)
            {
                if(nmiEnabled)                      cpu->signalNmi();
                vblReadFlag = true;
                evtManager->vblankStarted(v0_time + (line * 341 * 4));      // TODO put this '4' somewhere else
            }
        }

        int lastrenderline = (overscanMode ? 240 : 224);    // TODO this seems wrong

        // otherwise, see if we render it!
        if( (line >= 1) && (line <= lastrenderline) )
        {
            if(video.buffer)
                renderLine(line);
        }
        if(line == (lastrenderline+1))
            vblReadFlag = false;

        // if this is line 241, and we hit scanline 0 already, then we are done with the frame!
        if((line == 241) && (v0_time != Time::Never))
        {
            vEnd_time = 241 * 341 * 4;  // TODO move this 4
            vEnd_time += v0_time;
        }
    }

    void Ppu::renderLine(int line)
    {
        u32 bgclr = getRawColor(cgRam[0]);

        if(forceBlank)
        {
            for(int i = 0; i < 512; ++i)
                video.buffer[i] = bgclr;
        }
        else
        {
            auto bgcolor = cgRam[0];
            bgcolor.colorMath = (colorMathLayers & 0x20) != 0;

            // clear buffers
            for(int i = 0; i < 256+32; ++i)
            {
                renderBufMan[i] = bgcolor;
                renderBufSub[i] = bgcolor;
            }

            // render BGs
            switch(bgMode)
            {
            case 0:
                bgLine_normal(0, line, 2, cgRam + 0x00, 8, 11);
                bgLine_normal(1, line, 2, cgRam + 0x20, 7, 10);
                bgLine_normal(2, line, 2, cgRam + 0x40, 2,  5);
                bgLine_normal(3, line, 2, cgRam + 0x60, 1,  4);
                break;

            case 1:
                bgLine_normal(0, line, 4, cgRam, 8, 11);
                bgLine_normal(1, line, 4, cgRam, 7, 10);
                bgLine_normal(2, line, 2, cgRam, 2, mode1AltPriority ? 13 : 5);
                break;

            case 7:
                bgLine_mode7(0, line, 5, 5);
                // todo EXTBG
                break;

            default:
                break;

                // TODO do other BG modes
            }

            sprLine(line);

            // actually output the lines!
            outputLinePixels();
        }
        video.buffer += video.pitch;
        ++linesRendered;
    }

    void Ppu::outputLinePixels()
    {
        if(0)
        {
            //  TODO Hi-res
        }
        else
        {
            if(colorMathSubscr)
            {
                for(int i = 0; i < 256; ++i)
                {
                    bool half = colorHalfMath;
                    const Color* c = &renderBufSub[i+16];
                    if(!c->prio)
                    {
                        c = &fixedColor;
                        half = false;
                    }

                    video.buffer[i*2] = video.buffer[i*2+1] = getRawColor( renderBufMan[i+16].doMath(
                            *c,
                            colorMathSubtract,
                            half
                    ));
                }
            }
            else
            {
                for(int i = 0; i < 256; ++i)
                {
                    video.buffer[i*2] = video.buffer[i*2+1] = getRawColor( renderBufMan[i+16].doMath(
                            fixedColor,
                            colorMathSubtract,
                            colorHalfMath
                    ));
                }
            }
        }
    }

    u32 Ppu::getRawColor(const Color& clr)
    {
        u32 out;
        
        out  = ((clr.r * brightnessMultiplier) >> 8) << video.r_shift;
        out |= ((clr.g * brightnessMultiplier) >> 8) << video.g_shift;
        out |= ((clr.b * brightnessMultiplier) >> 8) << video.b_shift;
        out |= video.alpha_or;

        return out;
    }

    VideoResult Ppu::getVideoResult() const
    {
        VideoResult out;
        out.lines = linesRendered;
        out.mode = VideoResult::RenderMode::Progressive;
        if(interlaceMode)
            out.mode = oddFrame ? VideoResult::RenderMode::InterlaceOdd : VideoResult::RenderMode::InterlaceEven;

        return out;
    }

    void Ppu::debug_dumpVram(const char* filename)
    {
        FILE* file = fopen(filename, "wb");
        fwrite(vram, 2, 0x8000, file);
        fclose(file);
    }
    
    void Ppu::performEvent(int eventId, timestamp_t clk)
    {
        switch(eventId)
        {
        case EventCode::CatchUp:
            runTo(clk);
            break;

        case EventCode::HdmaStart:
            if(!forceBlank)
                dmaUnit->doFrameStart();            // TODO  do this even if force blank?
            break;

        case EventCode::HdmaLineOverscan:
            if(nmiHasHappened)          break;      // don't do this if rendering has already ended
            if(!overscanMode)           break;      // don't do this if not in overscan mode
            // otherwise..... fall through to normal HdmaLine (no break)
        case EventCode::HdmaLine:
            if(!forceBlank)
                dmaUnit->doLine();                  // TODO even if force blank?
            break;
        }
    }

    void Ppu::signalIrq()
    {
        irqPending = true;
        cpu->signalIrq(1);
    }
    void Ppu::acknowledgeIrq()
    {
        irqPending = false;
        cpu->acknowledgeIrq(1);
    }

    void Ppu::checkIrqOnLine(int line, int minh, int maxh)
    {
        if(irqPending)          return;

        switch(irqMode)
        {
        case IrqMode::Disabled:
            break;

        case IrqMode::H:
            if(irqPos.H >= minh && irqPos.H < maxh)
                signalIrq();
            break;

        case IrqMode::V:
            if(irqPos.V == line && minh == 0)
                signalIrq();
            break;

        case IrqMode::HV:
            if(irqPos.V == line && irqPos.H >= minh && irqPos.H < maxh)
                signalIrq();
            break;
        }
    }

    void Ppu::addIrqEvent()
    {
        if(irqPending)          return;

        Coord coord = irqPos;
        switch(irqMode)
        {
        case IrqMode::Disabled:
            return;
        case IrqMode::H:
            coord.V = curPos.V;
            if(irqPos.H <= curPos.H)    ++coord.V;
            break;
        case IrqMode::V:
            coord.H = 0;
            break;
        case IrqMode::HV:
            break;
        }

        evtManager->addEvent(coord.H, coord.V, this, EventCode::CatchUp);
    }

}

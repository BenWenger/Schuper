
#include <algorithm>
#include "ppu.h"
#include "cpu/cpu.h"

namespace sch
{
    void Ppu::reset(bool hard, EventManager* evt)
    {
        evtManager = evt;
        if(hard)
        {
            for(auto& i : bgLayers)
            {
                i.chrAddr = 0;
                i.priorityBit = false;
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
            for(auto& i : cgRam)
            {
                i.r = i.g = i.b = i.prio = 0;
            }
        }
    }

    void Ppu::frameStart(Cpu* c, const VideoSettings& vid)
    {
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

        // set events for when NMI might trigger  (TODO this is wrong, touch this up)
        evtManager->addEvent( 246*341*4, this, EventCode::CatchUp );
        evtManager->addEvent( 247*341*4, this, EventCode::CatchUp );
        evtManager->addEvent( 248*341*4, this, EventCode::CatchUp );
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
            
        case 0x2101:
        case 0x2102:
        case 0x2103:
        case 0x2104:
            // TODO
            break;

        case 0x2105:
            bgMode =                    v & 0x07;
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
            tmp = a - 0x210B;
            bgLayers[tmp+0].chrAddr =           (v & 0x0F) << 13;
            bgLayers[tmp+2].chrAddr =           (v & 0xF0) << 9;
            break;

        case 0x210D:
            // TODO Mode 7 scroll stuff
            // NO BREAK
        case 0x210F: case 0x2111: case 0x2113:
            tmp = (a - 0x210D) >> 1;
            bgLayers[tmp].scrollX = ((bgLayers[tmp].scrollX >> 8) & 7)
                                        |   (scrollRegPrev & ~7)
                                        |   (v << 8);
            scrollRegPrev = v;
            break;
            
        case 0x210E:
            // TODO Mode 7 scroll stuff
            // NO BREAK
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
        case 0x211B:
        case 0x211C:
        case 0x211D:
        case 0x211E:
        case 0x211F:
        case 0x2120:
            // TODO - mode 7 garbage
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
        case 0x2131:
            // TODO color math bullshit
            break;

        case 0x2132:
            // TODO color intensity??????
            break;

        case 0x2133:
            // TODO interlace and overscan and UGH
            break;

        case 0x4200:
            nmiEnabled =        (v & 0x80) != 0;
            // TODO other shit here
            break;
        }
    }

    void Ppu::performEvent(int eventId, timestamp_t clk)
    {
        if(eventId == EventCode::CatchUp)
            runTo(clk);
    }

    void Ppu::regRead(u16 a, u8& v)
    {
        // TODO
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
            advance(targetPos.H - curPos.H);
        }
        // Scenario 2!  We're either at the exact start of a line, or the target is on a different line
        else
        {
            // if we're not at the start, fill out the rest of the line
            if(curPos.H > 0)            targetPos.V += advance(341 - curPos.H);

            // Now start doing full scanlines!
            while((curPos.V < targetPos.V) && !frameIsOver())
            {
                doScanline(curPos.V);
                targetPos.V += advance(341);
            }
            // Now, curPos.V == targetPos.V .. run to proper H time
            //    curPos.H should be 0 at this time
            if(targetPos.H > 0 && !frameIsOver())
            {
                doScanline(curPos.V);
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

            if(nmiEnabled && nmiHasHappened)        cpu->signalNmi();
        }

        // otherwise, see if we render it!
        if( (line >= 1) && (line <= (overscanMode ? 240 : 224)) )
        {
            if(video.buffer)
            {
                ++linesRendered;
                renderLine(line);
            }
        }

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
            // clear buffers
            for(int i = 0; i < 256+32; ++i)
            {
                renderBufMan[i] = cgRam[0];
                renderBufSub[i] = cgRam[0];
            }

            // render BGs
            switch(bgMode)
            {
            case 1:
                bgLine_normal(0, line, 4, cgRam, 8, 11);
                bgLine_normal(1, line, 4, cgRam, 7, 10);
                bgLine_normal(2, line, 4, cgRam, 2, mode1AltPriority ? 5 : 13);
                break;

                // TODO do other BG modes
            }

            // TODO render sprites

            // actually output the lines!
            outputLinePixels();
        }
    }

    void Ppu::outputLinePixels()
    {
        // TODO - color math and hi res stuff goes here eventually
        for(int i = 0; i < 256; ++i)
        {
            video.buffer[i*2] = video.buffer[i*2+1] = getRawColor(renderBufMan[i+16]);
        }
        video.buffer += video.pitch;
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

}

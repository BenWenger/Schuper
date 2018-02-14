
#include "ppu.h"

namespace sch
{
    void Ppu::reset(bool hard)
    {
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
            brightness = 0;

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
            brightness = v & 0x0F;
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
            // TODO interlace and overscand and UGH
            break;
        }
    }

    void Ppu::performEvent(int eventId, timestamp_t clk)
    {
    }

    void Ppu::regRead(u16 a, u8& v)
    {
        // TODO
    }

    void Ppu::runTo(timestamp_t runto)
    {
        // TODO
    }
    
    void Ppu::adjustTimestamp(timestamp_t adj)
    {
        // TODO
    }


}

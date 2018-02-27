
#include "ppu.h"

namespace sch
{
    void Ppu::bgLine_normal(int bg, int line, int planes, u8 loprio, u8 hiprio)
    {
        Color*      manscr = (manScrLayers & (1<<bg)) ? renderBufMan : nullptr;
        Color*      subscr = (subScrLayers & (1<<bg)) ? renderBufSub : nullptr;

        if(!manscr && !subscr)  return;     // nothing to draw if both screens are disabled
        auto& layer = bgLayers[bg];

        if(layer.use16Tiles)
        {
            // TODO this is garbage
        }
        else
        {
            int fineY = line + layer.scrollY;
            int coarseY = (fineY >> 3);
            fineY &= 7;
            
            int fineX = layer.scrollX;
            int coarseX = (fineX >> 3);
            fineX = 16 - (fineX & 7);

            u16 basetileaddr =      ((coarseY & 0x1F) << 5) | layer.tileMapAddr;
            if(coarseY & 0x20)      basetileaddr |= layer.tileMapYOverflow;

            for(; fineX < (256+16); fineX += 8, ++coarseX)
            {
                u16 tileaddr =          basetileaddr | (coarseX & 0x1F);
                if(coarseX & 0x20)      tileaddr |= layer.tileMapXOverflow;

                u16 tile = vram[tileaddr];
            }
        }
    }

    
    void Ppu::renderPixelsToBuf(Color* mainbuf, Color* subbuf, int planes, u16 addr,
                                const Color* palette, bool hflip, u8 prio)
    {
        u8 buf[8] = {0};

        for(int i = 0; i < planes; i += 2)
        {
            u16 dat = vram[(addr + i*4) & 0x7FFF];
            buf[0] |= (dat & 0x0080) >> 7 << i;
            buf[1] |= (dat & 0x0040) >> 6 << i;
            buf[2] |= (dat & 0x0020) >> 5 << i;
            buf[3] |= (dat & 0x0010) >> 4 << i;
            buf[4] |= (dat & 0x0008) >> 3 << i;
            buf[5] |= (dat & 0x0004) >> 2 << i;
            buf[6] |= (dat & 0x0002) >> 1 << i;
            buf[7] |= (dat & 0x0001)      << i;

            buf[0] |= (dat & 0x8000) >> 14 << i;
            buf[1] |= (dat & 0x4000) >> 13 << i;
            buf[2] |= (dat & 0x2000) >> 12 << i;
            buf[3] |= (dat & 0x1000) >> 11 << i;
            buf[4] |= (dat & 0x0800) >> 10 << i;
            buf[5] |= (dat & 0x0400) >> 9  << i;
            buf[6] |= (dat & 0x0200) >> 8  << i;
            buf[7] |= (dat & 0x0100) >> 7  << i;
        }

        int xx = hflip ? 7 : 0;

        if(mainbuf)
        {
            for(int i = 0; i < 8; ++i)      mainbuf[i].multiplex(buf[i^xx], palette, prio);
        }
        if(subbuf)
        {
            for(int i = 0; i < 8; ++i)      subbuf[i].multiplex(buf[i^xx], palette, prio);
        }
    }
}

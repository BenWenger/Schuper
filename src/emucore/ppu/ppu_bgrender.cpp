
#include "ppu.h"
#include <cstdio>       // TODO REMOVE THIS

namespace sch
{
    void Ppu::bgLine_normal(int bg, int line, int planes, const Color* palette, u8 loprio, u8 hiprio)
    {
        Color*      manscr = (manScrLayers & (1<<bg)) ? renderBufMan : nullptr;
        Color*      subscr = (subScrLayers & (1<<bg)) ? renderBufSub : nullptr;

        if(!manscr && !subscr)  return;     // nothing to draw if both screens are disabled
        auto& layer = bgLayers[bg];
        bool mathParticipate = (colorMathLayers & (1<<bg)) != 0;

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

            u16 basetileaddr =      ((coarseY & 0x1F) << 5) + layer.tileMapAddr;
            if(coarseY & 0x20)      basetileaddr += layer.tileMapYOverflow;

            for(; fineX < (256+16); fineX += 8, ++coarseX)
            {
                u16 tileaddr =          basetileaddr | (coarseX & 0x1F);
                if(coarseX & 0x20)      tileaddr += layer.tileMapXOverflow;

                u16 tile = vram[tileaddr & 0x7FFF];
                int y = (tile & 0x8000) ? (fineY ^ 7) : fineY;

                u16 chraddr = (tile & 0x03FF) * (8/2) * planes;
                chraddr |= y;
                chraddr += layer.chrAddr;

                if(planes == 2)
                {
                    bool foobar = false;
                    if(foobar)
                    {
                        FILE* dump = fopen("vramdump.bin", "wb");
                        fwrite(vram, 2, 0x8000, dump);
                        fclose(dump);
                    }
                }

                auto* pal = palette;
                if(planes != 8)     pal += (1 << planes) * ((tile & 0x1C00) >> 10);

                renderPixelsToBuf(
                    manscr ? (manscr+fineX) : nullptr,
                    subscr ? (subscr+fineX) : nullptr,
                    planes,
                    chraddr,
                    pal,
                    (tile & 0x4000) != 0,
                    (tile & 0x2000) ? hiprio : loprio,
                    mathParticipate,
                    false
                );
            }
        }
    }

    namespace
    {
        inline int m7clip(int a)
        {
            if(a & 0x2000)      return a | ~0x03FF;
            else                return a &  0x03FF;
        }
    }

    void Ppu::bgLine_mode7(int bg, int line, u8 loprio, u8 hiprio)
    {
        Color*      manscr = (manScrLayers & (1<<bg)) ? (renderBufMan + 16) : nullptr;
        Color*      subscr = (subScrLayers & (1<<bg)) ? (renderBufSub + 16) : nullptr;
        if(!manscr && !subscr)      return;
        
        u8 prio = loprio;
        bool math = (colorMathLayers & (1<<bg)) != 0;

        int cx = m7clip(m7_ScrollX - m7_CenterX);
        int cy = m7clip(m7_ScrollY - m7_CenterY);

        int srcx = ((m7_Matrix[0]*cx  ) & ~0x3F)
                 + ((m7_Matrix[1]*line) & ~0x3F)
                 + ((m7_Matrix[1]*cy  ) & ~0x3F)
                 + (m7_CenterX << 8);
        
        int srcy = ((m7_Matrix[2]*cx  ) & ~0x3F)
                 + ((m7_Matrix[3]*line) & ~0x3F)
                 + ((m7_Matrix[3]*cy  ) & ~0x3F)
                 + (m7_CenterY << 8);

        for(int dstx = 0; dstx < 256; ++dstx)
        {
            // if m7_FlipX || m7_FlipY  <-  figure these out
            int x = srcx >> 8;
            int y = srcy >> 8;
            srcx += m7_Matrix[0];
            srcy += m7_Matrix[2];
            if(m7_WrapMap)
            {
                x &= 0x03FF;
                y &= 0x03FF;
            }

            int tile = -1;
            if(x >= 0 && y >= 0 && x < 0x400 && y < 0x400)
                tile = vram[((y & ~7) << 4) | (x >> 3)] & 0xFF;
            else if(m7_FillWithTile0)
                tile = 0;

            u8 px = 0;
            if(tile >= 0)
            {
                px = vram[(tile << 6) | ((y&7)<<3) | (x&7)] >> 8;
                if(bg)          // EXTBG
                {
                    if(px & 0x80)       prio = hiprio;
                    else                prio = loprio;
                    px &= 0x7F;
                }
            }
            
            if(manscr)      manscr[dstx].multiplex(px, cgRam, prio, math, false);
            if(subscr)      subscr[dstx].multiplex(px, cgRam, prio, math, false);
        }
    }

    
    void Ppu::renderPixelsToBuf(Color* mainbuf, Color* subbuf, int planes, u16 addr,
                                const Color* palette, bool hflip, u8 prio, bool math, bool spr)
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
            for(int i = 0; i < 8; ++i)      mainbuf[i].multiplex(buf[i^xx], palette, prio, math, spr);
        }
        if(subbuf)
        {
            for(int i = 0; i < 8; ++i)      subbuf[i].multiplex(buf[i^xx], palette, prio, math, spr);
        }
    }
}

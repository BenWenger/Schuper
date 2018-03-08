
#include "ppu.h"

namespace sch
{
    namespace
    {
        struct SpriteSizeDetails
        {
            int         wd;
            int         ht;
            int         hxor;
            int         vxor;
        };
        const SpriteSizeDetails spriteSizeDetails[8][2] = {
            {   { 8,  8,  7,  7},   {16, 16, 15, 15}  },        // 000 =  8x8  and 16x16 sprites
            {   { 8,  8,  7,  7},   {32, 32, 31, 31}  },        // 001 =  8x8  and 32x32 sprites
            {   { 8,  8,  7,  7},   {64, 64, 63, 63}  },        // 010 =  8x8  and 64x64 sprites
            {   {16, 16, 15, 15},   {32, 32, 31, 31}  },        // 011 = 16x16 and 32x32 sprites
            {   {16, 16, 15, 15},   {64, 64, 63, 63}  },        // 100 = 16x16 and 64x64 sprites
            {   {32, 32, 31, 31},   {64, 64, 63, 63}  },        // 101 = 32x32 and 64x64 sprites
            {   {16, 32, 15, 15},   {32, 64, 31, 31}  },        // 110 = 16x32 and 32x64 sprites ('undocumented')
            {   {16, 32, 15, 15},   {32, 32, 31, 31}  }         // 111 = 16x32 and 32x32 sprites ('undocumented')
        };
        const u8    spritePriorities[4] = {3, 6, 9, 12};
    }

    void Sprite::reset()
    {
        xpos = ypos = 0;
        tile = 0;
        plt = 0;
        prio = spritePriorities[0];
        hflip = vflip = false;
        size = name = false;
    }

    void Ppu::w_2101(u8 v)
    {
        objSizeMode = (v>>5) & 7;
        objChrAddr[0] = (v & 0x07) << 13;
        objChrAddr[1] = ((v & 0x18) + 8) << 9;        // ?? is this right?
        objChrAddr[1] += objChrAddr[0];
    }

    void Ppu::w_2102(u8 v)
    {
        oamAddrLoad = (oamAddrLoad & 0x0200) | (v << 1);
        oamAddr = oamAddrLoad;
    }

    void Ppu::w_2103(u8 v)
    {
        oamAddrLoad = (oamAddrLoad & 0x01FE) | ((v & 0x01) << 9);
        oamAddr = oamAddrLoad;
        useAltSpr0 = (v & 0x80) != 0;
    }

    void Ppu::w_2104(u8 v)
    {
        if(oamAddr & 0x0200)        // high table!
        {
            int i = oamAddr & 0x1F;
            oamHigh[i] = v;
            i *= 4;
            int end = i + 4;
            for(; i < end; ++i, v >>= 2)
            {
                sprites[i].xpos = oamLow[i*4];
                if(v & 1)       sprites[i].xpos -= 0x100;
                sprites[i].size = (v & 2) != 0;
            }
        }
        else if(oamAddr & 0x0001)   // low table, but writing high byte so writes take effect
        {
            auto a = oamAddr & 0x1FE;
            oamLow[a+0] = oamBuffer;
            oamLow[a+1] = v;
            
            auto& s = sprites[a >> 2];
            if(a & 2)
            {
                s.tile = oamBuffer;
                s.name = (v & 0x01) != 0;
                s.plt = (v >> 1) & 7;
                s.prio = spritePriorities[(v & 0x30) >> 4];
                s.vflip = (v & 0x80) != 0;
                s.hflip = (v & 0x40) != 0;
            }
            else
            {
                s.xpos &= ~0xFF;
                s.xpos |= oamBuffer;
                s.ypos = v;
            }
        }
        else                        // low table, low byte
            oamBuffer = v;

        ++oamAddr;
    }

    /////////////////////////////////////////////////////////

    void Ppu::sprLine(int line)
    {
        Color* manscr = (manScrLayers & 0x10) ? (renderBufMan+16) : nullptr;
        Color* subscr = (subScrLayers & 0x10) ? (renderBufSub+16) : nullptr;
        if(!manscr && !subscr)      return;

        const auto* sizeTable = spriteSizeDetails[objSizeMode];
        bool allmath = (colorMathLayers & 0x10) != 0;

        --line;         // technically this stuff was supposed to be loaded on the previous line

        // TODO there are lots of weird quirks I'm not doing here

        u16 addr;
        u16 xaddr;
        int xadj;
        int sp_index = 0;
        if(useAltSpr0)      sp_index = (oamAddr >> 2);
        for(int i = 0; i < 0x80; ++i)
        {
            const auto& s = sprites[++sp_index & 0x7F];
            const auto& size = sizeTable[s.size];

            if(s.xpos >= 256)           continue;
            if(s.xpos + size.wd < 0)    continue;

            int y = (line - s.ypos) & 0xFF;
            if(y >= size.ht)            continue;

            if(s.vflip)     y ^= size.vxor;
            addr  = (((s.tile >> 4) + (y >> 3)) & 0x0F) << 8;
            addr |= y & 7;
            addr += objChrAddr[s.name];
            
            xaddr = (s.tile & 0x0F) << 4;
            if(s.hflip)
            {
                xaddr += (size.wd << 1) - (1<<4);
                xadj = -(1<<4);
            }
            else
                xadj = (1<<4);

            int x = s.xpos;
            int ctr = 0;
            // skip all tiles that are off-screen to the left
            while(x < -7)       { x += 8; ctr += 8; xaddr += xadj;    }

            bool math = allmath && (s.plt & 4);
            const Color* palette = &cgRam[0x80 + (s.plt * 16)];

            // start rendering tiles that are on screen!
            while(x < 256 && ctr < size.wd)
            {
                renderPixelsToBuf(
                    manscr ? (manscr+x) : nullptr,
                    subscr ? (subscr+x) : nullptr,
                    4,
                    addr + (xaddr & 0xF0),
                    palette,
                    s.hflip,
                    s.prio,
                    math,
                    true
                );

                x += 8;
                ctr += 8;
                xaddr += xadj;
            }
        }
    }
}


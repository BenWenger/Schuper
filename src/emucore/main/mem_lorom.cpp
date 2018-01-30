
#include "snes.h"

namespace sch
{

    int Snes::rd_LoRom(u32 a, u8& v, timestamp_t clk)
    {
        // There must be a better way to do this... but whatever

        if((a & 0xFE0000) == 0x7E0000)              // Banks 7E, 7F
        {
            v = ram[a & 0x1FFFF];
            return spdSlow;
        }
        else if((a & 0x400000) || (a & 0x008000))   // Banks 40-7D, C0-FF --- also the "ROM" parts of all banks
        {
            v = currentFile.memory[romMask & ( ((a & 0x7F0000) >> 1) | (a & 0x7FFF) ) ];

            if((a & 0x800000) /* && inFastRomMode() */ )        // TODO
                return spdFast;
            else
                return spdSlow;
        }
        else                                        // Everything else (reg space)
        {
            u16 trima = a & 0xFFFF;
            if(trima < 0x2000)
            {
                v = ram[trima];
                return spdSlow;
            }
            else
            {
                v = rd_Reg(trima, clk);
                if(trima < 0x4000)      return spdFast;
                if(trima < 0x4200)      return spdXSlow;
                if(trima < 0x6000)      return spdFast;
                return spdSlow;
            }
        }

        // should never reach here
        return spdSlow;
    }

    
    int Snes::wr_LoRom(u32 a, u8 v, timestamp_t clk)
    {
        // There must be a better way to do this... but whatever

        if((a & 0xFE0000) == 0x7E0000)              // Banks 7E, 7F
        {
            ram[a & 0x1FFFF] = v;
            return spdSlow;
        }
        else if((a & 0x400000) || (a & 0x008000))   // Banks 40-7D, C0-FF --- also the "ROM" parts of all banks
        {
            if((a & 0x800000) /* && inFastRomMode() */ )        // TODO
                return spdFast;
            else
                return spdSlow;
        }
        else                                        // Everything else (reg space)
        {
            u16 trima = a & 0xFFFF;
            if(trima < 0x2000)
            {
                ram[trima] = v;
                return spdSlow;
            }
            else
            {
                wr_Reg(trima, v, clk);
                if(trima < 0x4000)      return spdFast;
                if(trima < 0x4200)      return spdXSlow;
                if(trima < 0x6000)      return spdFast;
                return spdSlow;
            }
        }

        // should never reach here
        return spdSlow;
    }

    u8 Snes::pk_LoRom(u32 a) const
    {
        if((a & 0xFE0000) == 0x7E0000)              // Banks 7E, 7F
        {
            return ram[a & 0x1FFFF];
        }
        else if((a & 0x400000) || (a & 0x008000))   // Banks 40-7D, C0-FF --- also the "ROM" parts of all banks
        {
            return currentFile.memory[romMask & ( ((a & 0x7F0000) >> 1) | (a & 0x7FFF) ) ];
        }
        else                                        // Everything else (reg space)
        {
            u16 trima = a & 0xFFFF;
            if(trima < 0x2000)
            {
                return ram[trima];
            }
        }
        return 0;
    }
}


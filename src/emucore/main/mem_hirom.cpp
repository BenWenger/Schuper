
#include "snes.h"

namespace sch
{

    int Snes::rd_HiRom(u32 a, u8& v, timestamp_t clk)
    {
        // There must be a better way to do this... but whatever

        if((a & 0xFE0000) == 0x7E0000)              // Banks 7E, 7F
        {
            v = ram[a & 0x1FFFF];
            return spdSlow;
        }
        else if(a & 0x400000)       // banks $40-7D, C0-FF
        {
            v = currentFile.memory[romMask & (a & 0x3FFFFF)];
            if((a & 0x800000) /* && inFastRomMode() */ )        // TODO
                return spdFast;
            else
                return spdSlow;
        }
        else if(a & 0x008000)       // the "ROM" parts of other banks????
        {
            v = currentFile.memory[romMask & (a & 0x3FFFFF)];       ///  ????  TODO is this right?
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

    
    int Snes::wr_HiRom(u32 a, u8 v, timestamp_t clk)
    {
        // There must be a better way to do this... but whatever

        if((a & 0xFE0000) == 0x7E0000)              // Banks 7E, 7F
        {
            ram[a & 0x1FFFF] = v;
            return spdSlow;
        }
        else if(a & 0x400000)       // banks $40-7D, C0-FF
        {
            if((a & 0x800000) /* && inFastRomMode() */ )        // TODO
                return spdFast;
            else
                return spdSlow;
        }
        else if(a & 0x008000)       // the "ROM" parts of other banks????
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

    u8 Snes::pk_HiRom(u32 a) const
    {
        // There must be a better way to do this... but whatever

        if((a & 0xFE0000) == 0x7E0000)              // Banks 7E, 7F
        {
            return ram[a & 0x1FFFF];
        }
        else if(a & 0x400000)       // banks $40-7D, C0-FF
        {
            return currentFile.memory[romMask & (a & 0x3FFFFF)];
        }
        else if(a & 0x008000)       // the "ROM" parts of other banks????
        {
            return currentFile.memory[romMask & (a & 0x3FFFFF)];       ///  ????  TODO is this right?
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


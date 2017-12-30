
#ifndef SCHUPER_CPU_CPUSTATE_H_INCLUDED
#define SCHUPER_CPU_CPUSTATE_H_INCLUDED

#include "snestypes.h"

namespace sch
{
    class CpuState
    {
    public:
        u16         X;
        u16         Y;
        u16         A16;        // 16-bit A register
        u8          A8;         // 8-bit A register
        u8          B8;         // 'B' register when A is 8 bits
        
        u16         DP;
        u16         PC;
        u16         SP;

        u32         PBR;
        u32         DBR;

        int         fN;
        int         fZ;
        int         fV;
        int         fC;

        bool        fX;
        bool        fM;
        bool        fI;
        bool        fD;
        bool        fE;

        u8          getStatusByte(bool sw)
        {
            return 0;       // TODO
        }

        void        setStatusByte(u8 v)
        {
            // TODO
        }
    };
}

#endif

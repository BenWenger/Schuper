
#ifndef SCHUPER_CPU_CPUSTATE_H_INCLUDED
#define SCHUPER_CPU_CPUSTATE_H_INCLUDED

#include "snestypes.h"

namespace sch
{
    class CpuState
    {
    public:
        union SplitReg
        {
            u16         w;
            struct
            {
                u8      l;              // TODO - this only works with little endian systems
                u8      h;
            };
        };

        SplitReg    A;
        SplitReg    X;
        SplitReg    Y;
        
        u16         DP;
        u16         PC;
        SplitReg    SP;

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
            u8 out = 0;

            if(fC)                  out |= C_FLAG;
            if(!fZ)                 out |= Z_FLAG;
            if(fI)                  out |= I_FLAG;
            if(fD)                  out |= D_FLAG;
            if(fE)
            {
                if(sw)              out |= B_FLAG;
                                    out |= R_FLAG;
            }
            else
            {
                if(fX)              out |= X_FLAG;
                if(fM)              out |= M_FLAG;
            }
            if(fV)                  out |= V_FLAG;
            if(fN)                  out |= N_FLAG;

            return out;
        }

        void        setStatusByte(u8 v)
        {
            fC =                (v & C_FLAG);
            fZ =               !(v & Z_FLAG);
            fI =                (v & I_FLAG) != 0;      // TODO - repredict IRQs
            fD =                (v & D_FLAG) != 0;
            if(!fE)
            {
                fX =            (v & X_FLAG) != 0;
                if(fX)          X.h = Y.h = 0;
                fM =            (v & M_FLAG) != 0;
            }
            fV =                (v & V_FLAG);
            fN =                (v & N_FLAG);
        }

        enum
        {
            C_FLAG =        0x01,
            Z_FLAG =        0x02,
            I_FLAG =        0x04,
            D_FLAG =        0x08,
            X_FLAG =        0x10,
            M_FLAG =        0x20,
            V_FLAG =        0x40,
            N_FLAG =        0x80,

            B_FLAG =        0x10,
            R_FLAG =        0x20
        };
    };
}

#endif

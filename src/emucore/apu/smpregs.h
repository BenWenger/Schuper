
#ifndef SCHUPER_APU_SMPREGS_H_INCLUDED
#define SCHUPER_APU_SMPREGS_H_INCLUDED

#include "snestypes.h"

namespace sch
{

    class SmpRegs
    {
    public:
        u8              A;
        u8              X;
        u8              Y;
        u16             DP;
        u16             PC;
        u8              SP;

        int             fNZ;
        int             fV;
        int             fB;
        int             fH;
        int             fI;
        int             fC;

        void            clear()
        {
            A = 0;
            X = 0;
            Y = 0;
            DP = 0;
            PC = 0;
            SP = 0;
            fNZ = 0;
            fV = 0;
            fB = 0;
            fH = 0;
            fI = 0;
            fC = 0;
        }

        u8              getStatusByte() const
        {
            u8 out = 0;
            if(fC)              out |= C_FLAG;
            if(!(fNZ & 0xFF))   out |= Z_FLAG;
            if(fI)              out |= I_FLAG;
            if(fH)              out |= H_FLAG;
            if(fB)              out |= B_FLAG;
            if(DP)              out |= P_FLAG;
            if(fV)              out |= V_FLAG;
            if(fNZ & 0x180)     out |= N_FLAG;

            return out;
        }

        void            setStatusByte(u8 v)
        {
            fC =            v & C_FLAG;
            fNZ =           !(v & Z_FLAG);
            fI =            v & I_FLAG;
            fH =            v & H_FLAG;
            fB =            v & B_FLAG;
            DP =            (v & P_FLAG) ? 0x0100 : 0;
            fV =            v & V_FLAG;
            if(v & N_FLAG)  fNZ |= 0x100;
        }

    private:
        enum
        {
            C_FLAG =            0x01,
            Z_FLAG =            0x02,
            I_FLAG =            0x04,
            H_FLAG =            0x08,
            B_FLAG =            0x10,
            P_FLAG =            0x20,
            V_FLAG =            0x40,
            N_FLAG =            0x80
        };
    };

}

#endif

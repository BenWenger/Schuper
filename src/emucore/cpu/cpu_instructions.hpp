
#include "cpu.h"

namespace sch
{
    //////////////////////////////////////////////////

    void Cpu::ADC(u16 v)
    {
        if(regs.fD)
            ADC_decimal(v);
        else if(regs.fM)
        {
            int sum = regs.A.l + v + (regs.fC != 0);
            regs.fN = (sum & 0x80);
            regs.fV = (sum ^ v) & (sum ^ regs.A.l) & 0x80;
            regs.fC = (sum > 0xFF);
            regs.fZ = regs.A.l = static_cast<u8>(sum & 0xFF);
        }
        else
        {
            int sum = regs.A.w + v + (regs.fC != 0);
            regs.fN = (sum & 0x8000);
            regs.fV = (sum ^ v) & (sum ^ regs.A.l) & 0x8000;
            regs.fC = (sum > 0xFFFF);
            regs.fZ = regs.A.w = static_cast<u16>(sum & 0xFFFF);
        }
    }
    
    void Cpu::ADC_decimal(u16 v)
    {
        // TODO
    }

    void Cpu::AND(u16 v)
    {
        if(regs.fM)
        {
            regs.fZ = regs.A.l &= v;
            regs.fN = regs.A.l & 0x80;
        }
        else
        {
            regs.fZ = regs.A.w &= v;
            regs.fN = regs.A.w & 0x8000;
        }
    }
    
    u16 Cpu::ASL(u16 v, bool flg)
    {
        if(flg)
        {
            v <<= 1;
            regs.fC = (v & 0x100);
            regs.fN = (v & 0x80);
            regs.fZ = (v &= 0xFF);
        }
        else
        {
            regs.fC = (v & 0x8000);
            regs.fN = (v & 0x4000);
            regs.fZ = (v <<= 1);
        }
        return v;
    }

    void Cpu::BIT(u16 v, bool set_flags)
    {
        if(regs.fM)
        {
            if(set_flags)
            {
                regs.fN = (v & 0x80);
                regs.fV = (v & 0x40);
            }
            regs.fZ = (regs.A.l & v);
        }
        else
        {
            if(set_flags)
            {
                regs.fN = (v & 0x8000);
                regs.fV = (v & 0x4000);
            }
            regs.fZ = (regs.A.w & v);
        }
    }

    void Cpu::CPR(u16 v, u16 r, bool flg)
    {
        int dif = (r - v);
        if(flg)     regs.fN =   (dif & 0x80);
        else        regs.fN =   (dif & 0x8000);

        regs.fZ =   (dif != 0);
        regs.fC =   (dif >= 0);
    }
    
    
    u16 Cpu::DEC(u16 v, bool flg)
    {
        --v;
        if(flg)
        {
            regs.fN = (v & 0x80);
            regs.fZ = (v &= 0xFF);
        }
        else
        {
            regs.fN = (v & 0x8000);
            regs.fZ = v;
        }
        return v;
    }
    
    void Cpu::EOR(u16 v)
    {
        if(regs.fM)
        {
            regs.fZ = regs.A.l ^= v;
            regs.fN = regs.A.l & 0x80;
        }
        else
        {
            regs.fZ = regs.A.w ^= v;
            regs.fN = regs.A.w & 0x8000;
        }
    }
    
    u16 Cpu::INC(u16 v, bool flg)
    {
        ++v;
        if(flg)
        {
            regs.fN = (v & 0x80);
            regs.fZ = (v &= 0xFF);
        }
        else
        {
            regs.fN = (v & 0x8000);
            regs.fZ = v;
        }
        return v;
    }
    
    void Cpu::LDR(CpuState::SplitReg& r, u16 v, bool flg)
    {
        if(flg)
        {
            regs.fN = (v & 0x80);
            regs.fZ = r.l = static_cast<u8>(v);
        }
        else
        {
            regs.fN = (v & 0x8000);
            regs.fZ = r.w = v;
        }
    }
    
    u16 Cpu::LSR(u16 v, bool flg)
    {
        regs.fC = (v & 0x0001);
        regs.fN = 0;
        regs.fZ = (v >>= 1);
        return v;
    }
    
    void Cpu::ORA(u16 v)
    {
        if(regs.fM)
        {
            regs.fZ = regs.A.l |= v;
            regs.fN = regs.A.l & 0x80;
        }
        else
        {
            regs.fZ = regs.A.w |= v;
            regs.fN = regs.A.w & 0x8000;
        }
    }

    u16 Cpu::ROL(u16 v, bool flg)
    {
        auto oldC = regs.fC;
        if(flg)
        {
            regs.fC = (v & 0x80);
            regs.fN = (v & 0x40);
            v = (v << 1) & 0xFF;
        }
        else
        {
            regs.fC = (v & 0x8000);
            regs.fN = (v & 0x4000);
            v <<= 1;
        }
        if(oldC)        v |= 0x0001;
        regs.fZ = v;

        return v;
    }

    u16 Cpu::ROR(u16 v, bool flg)
    {
        regs.fN = regs.fC;
        regs.fC = (v & 0x0001);
        v >>= 1;
        if(regs.fN)
        {
            v |= (flg ? 0x80 : 0x8000);
        }

        regs.fZ = v;
        return v;
    }
    
    void Cpu::SBC(u16 v)
    {
        if(regs.fD)
            SBC_decimal(v);
        else if(regs.fM)
        {
            int dif = regs.A.l - v - !regs.fC;
            regs.fC = (dif >= 0);
            regs.fN = (dif & 0x80);
            regs.fV = (regs.A.l ^ v) & (regs.A.l ^ dif) & 0x80;
            regs.fZ = regs.A.l = static_cast<u8>(dif & 0xFF);
        }
        else
        {
            int dif = regs.A.w - v - !regs.fC;
            regs.fC = (dif >= 0);
            regs.fN = (dif & 0x8000);
            regs.fV = (regs.A.w ^ v) & (regs.A.w ^ dif) & 0x8000;
            regs.fZ = regs.A.w = static_cast<u16>(dif & 0xFFFF);
        }
    }

    void Cpu::SBC_decimal(u16 v)
    {
        // TODO
    }
    
    u16 Cpu::TRB(u16 v, bool flg)
    {
        if(flg)
        {
            regs.fZ = (regs.A.l & v);
            v &= ~regs.A.l;
        }
        else
        {
            regs.fZ = (regs.A.w & v);
            v &= ~regs.A.w;
        }
        return v;
    }

    u16 Cpu::TSB(u16 v, bool flg)
    {
        if(flg)
        {
            regs.fZ = (regs.A.l & v);
            v |= regs.A.l;
        }
        else
        {
            regs.fZ = (regs.A.w & v);
            v |= regs.A.w;
        }
        return v;
    }

    inline u16 Cpu::STA()
    {
        return (regs.fM ? regs.A.l : regs.A.w);
    }
    
    void Cpu::TAX()
    {
        if(regs.fX)
        {
            regs.fZ = regs.X.w = regs.A.l;
            regs.fN = (regs.X.w & 0x80);
        }
        else
        {
            regs.fZ = regs.X.w = regs.A.w;
            regs.fN = (regs.X.w & 0x8000);
        }
    }
    void Cpu::TAY()
    {
        if(regs.fX)
        {
            regs.fZ = regs.Y.w = regs.A.l;
            regs.fN = (regs.Y.w & 0x80);
        }
        else
        {
            regs.fZ = regs.Y.w = regs.A.w;
            regs.fN = (regs.Y.w & 0x8000);
        }
    }

    void Cpu::TCD()
    {
        regs.fZ = regs.DP = regs.A.w;
        regs.fN = (regs.A.w & 0x8000);
    }
    void Cpu::TDC()
    {
        regs.fZ = regs.A.w = regs.DP;
        regs.fN = (regs.A.w & 0x8000);
    }
    
    void Cpu::TCS()
    {
        regs.SP.w = regs.A.w;
        if(regs.fE)         regs.SP.h = 0x01;
    }

    void Cpu::TSC()
    {
        regs.A.w = regs.SP.w;
        if(regs.fE)
        {
            regs.fZ = regs.A.l;
            regs.fN = (regs.A.l & 0x80);
        }
        else
        {
            regs.fZ = regs.A.w;
            regs.fN = (regs.A.w & 0x8000);
        }
    }

    void Cpu::TSX()
    {
        if(regs.fX)
        {
            regs.X.w = regs.SP.l;
            regs.fN = (regs.X.w & 0x80);
        }
        else
        {
            regs.X.w = regs.SP.w;
            regs.fN = (regs.X.w & 0x8000);
        }
        regs.fZ = regs.X.w;
    }

    void Cpu::TXS()
    {
        regs.SP.w = regs.X.w;
        if(regs.fE)
            regs.SP.h = 0x01;
    }
    
    void Cpu::TXA()
    {
        if(regs.fM)
        {
            regs.fZ = regs.A.l = regs.X.l;
            regs.fN = (regs.A.l & 0x80);
        }
        else
        {
            regs.fZ = regs.A.w = regs.X.w;
            regs.fN = (regs.A.w & 0x8000);
        }
    }

    void Cpu::TYA()
    {
        if(regs.fM)
        {
            regs.fZ = regs.A.l = regs.Y.l;
            regs.fN = (regs.A.l & 0x80);
        }
        else
        {
            regs.fZ = regs.A.w = regs.Y.w;
            regs.fN = (regs.A.w & 0x8000);
        }
    }
    
    void Cpu::TXY()
    {
        regs.fZ = regs.Y.w = regs.X.w;
        regs.fN = (regs.Y.w & (regs.fX ? 0x80 : 0x8000));
    }

    void Cpu::TYX()
    {
        regs.fZ = regs.X.w = regs.Y.w;
        regs.fN = (regs.X.w & (regs.fX ? 0x80 : 0x8000));
    }
    
    void Cpu::u_BRL()
    {
        u16 a =     read_pc();
        a |=        read_pc() << 8;
        ioCyc();    regs.PC += a;
    }

    void Cpu::u_JMP_Absolute()
    {
        u16 a =     read_pc();
        a |=        read_pc() << 8;
        regs.PC = a;
    }
    
    void Cpu::u_JMP_AbsoluteLong()
    {
        u16 a =     read_pc();
        a |=        read_pc() << 8;
        regs.PBR =  read_pc() << 16;
        regs.PC = a;
    }

    void Cpu::u_JMP_Indirect()
    {
        u16 a =     read_pc();
        a |=        read_pc() << 8;
        regs.PC =   read(a++);
        regs.PC |=  read(a) << 8;
    }

    void Cpu::u_JMP_IndirectX()
    {
        u16 a =             read_pc();
        a |=                read_pc() << 8;
        a += regs.X.w;      ioCyc();
        regs.PC =           read(regs.PBR | a++);
        regs.PC |=          read(regs.PBR | a) << 8;
    }

    void Cpu::u_JMP_IndirectLong()
    {
        u16 a =     read_pc();
        a |=        read_pc() << 8;
        regs.PC =   read(a++);
        regs.PC |=  read(a++) << 8;
        regs.PBR =  read(a) << 16;
    }

    void Cpu::u_JSR_Absolute()
    {
        u16 a =         read_pc();
        a |=            read_pc() << 8;
        --regs.PC;      ioCyc();            // push next addr minus one?
        push( static_cast<u8>(regs.PC >> 8) );
        push( static_cast<u8>(regs.PC & 0xFF) );
        regs.PC = a;
    }

    void Cpu::u_JSR_AbsoluteLong()
    {
        u16 a =         read_pc();
        a |=            read_pc() << 8;
        push( static_cast<u8>(regs.PBR >> 16) );
        ioCyc();
        regs.PBR =      read( regs.PBR | regs.PC ) << 16;    // pushes PC minus one
        push( static_cast<u8>(regs.PC >> 8) );
        push( static_cast<u8>(regs.PC & 0xFF) );
        regs.PC = a;
    }

    void Cpu::u_JSR_IndirectX()
    {
        u16 a =         read_pc();
        push( static_cast<u8>(regs.PC >> 8) );
        push( static_cast<u8>(regs.PC & 0xFF) );    // does push minus one?
        a |=            read_pc() << 8;
        a += regs.X.w;  ioCyc();
        regs.PC =       read( regs.PBR | a++ );
        regs.PC |=      read( regs.PBR | a ) << 8;
    }

    void Cpu::u_MVNP(int adj)
    {
        regs.DBR =          read_pc() << 16;
        u32 sb =            read_pc() << 16;
        u8 v =              read( sb | regs.X.w );
                            write( regs.DBR | regs.Y.w, v );
        ioCyc();
        ioCyc();
        if(regs.A.w--)      regs.PC -= 3;

        if(regs.fX)
        {
            regs.X.l += adj;
            regs.Y.l += adj;
        }
        else
        {
            regs.X.w += adj;
            regs.Y.w += adj;
        }
    }

    void Cpu::u_PEA()
    {
        u8 lo =         read_pc();
        u8 hi =         read_pc();
        push( hi );
        push( lo );
    }

    void Cpu::u_PEI()
    {
        u16 d =         read_pc();
        d += regs.DBR;  dpCyc();
        u8 lo =         read(d++);
        u8 hi =         read(d);
        push(hi);
        push(lo);
    }

    void Cpu::u_PER()
    {
        u16 a =         read_pc();
        a |=            read_pc() << 8;
        a += regs.PC;   ioCyc();
        push( static_cast<u8>( a >> 8 ) );
        push( static_cast<u8>( a & 0xFF ) );
    }

    void Cpu::u_PLA()
    {
        ioCyc(2);
        regs.A.l = pull();
        if(regs.fM)
        {
            regs.fN = (regs.A.l & 0x80);
            regs.fZ = regs.A.l;
        }
        else
        {
            regs.A.h = pull();
            regs.fN = (regs.A.w & 0x8000);
            regs.fZ = regs.A.w;
        }
    }

    void Cpu::u_REP()
    {
        u8 v =          read_pc();
        ioCyc();
        regs.setStatusByte( regs.getStatusByte(true) & ~v );
        checkInterruptPending();
    }

    void Cpu::u_RTI()
    {
        ioCyc(2);
        regs.setStatusByte( pull() );               checkInterruptPending();
        regs.PC =           pull();
        regs.PC |=          pull() << 8;
        if(!regs.fE)        regs.PBR =      pull() << 16;
    }

    void Cpu::u_RTL()
    {
        ioCyc(2);
        regs.PC =           pull();
        regs.PC |=          pull() << 8;
        regs.PBR =          pull() << 16;       regs.PC++;
    }

    void Cpu::u_RTS()
    {
        ioCyc(2);
        regs.PC =           pull();
        regs.PC |=          pull() << 8;
        ++regs.PC;          ioCyc();
    }

    void Cpu::u_SEP()
    {
        u8 v =          read_pc();
        ioCyc();
        regs.setStatusByte( regs.getStatusByte(true) | v );
        checkInterruptPending();
    }

    void Cpu::u_XBA()
    {
        ioCyc();
        ioCyc();
        std::swap( regs.A.l, regs.A.h );
        regs.fN = (regs.A.l & 0x80);
        regs.fZ = regs.A.l;
    }

    void Cpu::u_XCE()
    {
        ioCyc();
        if(!regs.fE == !regs.fC)
            return;     // nothing to do

        if(regs.fC)     // Enter emulation mode
        {
            regs.fC = 0;
            regs.fM = regs.fX = regs.fE = true;
            regs.X.h = regs.Y.h = 0;
            regs.SP.h = 0x01;
            regs.DBR = regs.PBR = 0;                    // DBR and PBR to zero????
            //  no change to DP ????
        }
        else            // Exit emulation mode
        {
            regs.fC = 1;
            regs.fE = false;
            // nothing else changes
        }
    }


}

#include "cpu.h"

#define CALLOP(v,flg)       (this->*op)(v,flg)

namespace sch
{
    //////////////////////////////////////////////////

    u16 Cpu::ad_final_rd(u32 a, bool flg)
    {
        u16 v =             read( a );
        if(!flg)    v |=    read( a + 1 ) << 8;
        return v;
    }

    void Cpu::ad_final_wr(u32 a, u16 v, bool flg)
    {
                    write( a, static_cast<u8>(v & 0xFF) );
        if(!flg)    write( a + 1, static_cast<u8>(v >> 8) );
    }

    void Cpu::ad_final_rw(u32 a, rwop_t op, bool flg)
    {
        u16 v =             read( a );
        if(flg) {
            v = CALLOP(v,flg);  ioCyc();
        } else {
            v |=            read( a + 1 ) << 8;
            v = CALLOP(v,flg);  ioCyc();
                            write( a + 1, static_cast<u8>( v >> 8 ) );
        }
        write( a, static_cast<u8>(v) );
    }
        
    //////////////////////////////////////////////////

    // Immediate
    u16 Cpu::ad_rd_im(bool flg)
    {
        u16 v =             read_pc();
        if(!flg)    v |=    read_pc() << 8;
        return v;
    }

    // Accumulator
    void Cpu::ad_rw_ac(rwop_t op)
    {
        ioCyc();
        if(regs.fM)
            regs.A.l = static_cast<u8>( CALLOP(regs.A.l, true) );
        else
            regs.A.w = CALLOP(regs.A.w, false);
    }
    
    // Absolute
    u32 Cpu::ad_addr_ab()
    {
        u32 a =             regs.DBR | read_pc();
        a |=                read_pc() << 8;
        return a;
    }

    
    // Absolute Long
    u32 Cpu::ad_addr_al()
    {
        u32 a =             read_pc();
        a |=                read_pc() << 8;
        a |=                read_pc() << 16;
        return a;
    }
    
    // Direct Page
    u32 Cpu::ad_addr_dp()
    {
        u32 a =             read_pc() + regs.DP;
                            dpCyc();
        return a;
    }

    // Indirect, Y          LDA ($nn),Y
    u32 Cpu::ad_addr_iy(bool wr)
    {
        u32 dp =            regs.DP + read_pc();
                            dpCyc();
        u32 tmp =           read(dp);
        tmp |=              read(dp + 1) << 8;
        
        u32 a = tmp + regs.Y.w;

        if(wr || ((a ^ tmp) & 0xFF00))      ioCyc();

        return a + regs.DBR;
    }

    
    u32 Cpu::ad_addr_iyl()
    {
        u32 dp =            regs.DP + read_pc();
                            dpCyc();
        u32 addr =          read(dp);
        addr |=             read(dp+1) << 8;
        addr |=             read(dp+2) << 16;
        return addr + regs.Y.w;
    }
    
    u32 Cpu::ad_addr_ix()
    {
        u32 dp =            ad_addr_dr(regs.X.w);
        u32 a =             read(dp);
        a |=                read(dp+1) << 8;
        return a + regs.DBR;
    }
    
    u32 Cpu::ad_addr_dr(u16 r)
    {
        u16 dp =            regs.DP + read_pc();
                            dpCyc();
        dp += r;            ioCyc();
        return dp;
    }

    u32 Cpu::ad_addr_ar(u16 r, bool wr)
    {
        u32 tmp =           read_pc() | regs.DBR;
        tmp |=              read_pc() << 8;
        
        u32 a = tmp + r;
        if(wr || ((a ^ tmp) & 0xFF00))      ioCyc();

        return a;
    }

    u32 Cpu::ad_addr_axl()
    {
        u32 a =             read_pc();
        a |=                read_pc() << 8;
        a |=                read_pc() << 16;
        return a + regs.X.w;
    }

    void Cpu::ad_branch(bool condition)
    {
        int adj =           (read_pc() ^ 0x80) - 0x80;
        if(condition)
        {
            ioCyc();
            u16 newpc =     regs.PC + adj;
            if(regs.fE && ((newpc ^ regs.PC) & 0xFF00))
                ioCyc();

            regs.PC = newpc;
        }
    }
    
    u32 Cpu::ad_addr_di()
    {
        u16 dp =        regs.DP + read_pc();
                        dpCyc();
        u32 a =         read(dp++);
        a |=            read(dp) << 8;
        return a | regs.DBR;
    }
    
    u32 Cpu::ad_addr_dil()
    {
        u16 dp =        regs.DP + read_pc();
                        dpCyc();
        u32 a =         read(dp++);
        a |=            read(dp++) << 8;
        a |=            read(dp)   << 16;
        return a;
    }
    
    void Cpu::ad_push(u16 v, bool flg)
    {
        ioCyc();
        if(!flg)
            push(static_cast<u8>(v >> 8));
        push(static_cast<u8>(v & 0xFF));
    }

    u16 Cpu::ad_pull(bool flg)
    {
        ioCyc();
        ioCyc();
        u16 v = pull();
        if(!flg)
            v |= pull() << 8;
        return v;
    }
    
    u32 Cpu::ad_addr_sr()
    {
        u16 a =         regs.SP.w + read_pc();
                        ioCyc();
        return a;
    }

    u32 Cpu::ad_addr_siy()
    {
        u16 tmp =       ad_addr_sr();
        u16 a =         read(tmp);
        a |=            read(tmp) << 8;
                        ioCyc();
        return a + regs.DBR + regs.Y.w;
    }
    

}
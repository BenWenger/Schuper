
#include "cpu.h"

#define CALLOP(v,flg)       (this->*op)(v,flg)

namespace sch
{
    //////////////////////////////////////////////////

    inline u16 Cpu::ad_final_rd(u32 a, bool flg)
    {
        u16 v =             read( a );
        if(!flg)    v |=    read( a + 1 ) << 8;
        return v;
    }

    inline void Cpu::ad_final_wr(u32 a, u16 v, bool flg)
    {
                    write( a, static_cast<u8>(v & 0xFF) );
        if(!flg)    write( a, static_cast<u8>(v >> 8) );
    }

    inline void Cpu::ad_final_rw(u32 a, rwop_t op, bool flg)
    {
        u16 v =             read( a );
        if(flg) {
            CALLOP(v,flg);  ioCyc();
        } else {
            v |=            read( a + 1 ) << 8;
            CALLOP(v,flg);  ioCyc();
                            write( a + 1, static_cast<u8>( v >> 8 ) );
        }
        write( a, static_cast<u8>(v) );
    }
        
    //////////////////////////////////////////////////

    // Immediate
    inline u16 Cpu::ad_rd_im(bool flg)
    {
        u16 v =             read_pc();
        if(!flg)    v |=    read_pc() << 8;
        return v;
    }
    
    // Absolute
    inline u32 Cpu::ad_addr_ab()
    {
        u32 a =             regs.DBR | read_pc();
        a |=                read_pc() << 8;
        return a;
    }

    
    // Absolute Long
    inline u32 Cpu::ad_addr_al()
    {
        u32 a =             read_pc();
        a |=                read_pc() << 8;
        a |=                read_pc() << 16;
        return a;
    }
    
    // Direct Page
    inline u32 Cpu::ad_addr_dp()
    {
        u32 a =             read_pc() + regs.DP;
                            dpCyc();
        return a;
    }

    // Indirect, Y          LDA ($nn),Y
    inline u32 Cpu::ad_addr_iy(bool wr)
    {
        u32 dp =            regs.DP + read_pc();
                            dpCyc();
        u32 tmp =           read(dp);
        tmp |=              read(dp + 1) << 8;
        
        u32 a = tmp + regs.Y.w;

        if(wr || ((a ^ tmp) & 0xFF00))      ioCyc();

        return a;
    }

}

#ifndef SCHUPER_CPU_CPU_H_INCLUDED
#define SCHUPER_CPU_CPU_H_INCLUDED

#include <algorithm>
#include "snestypes.h"
#include "cpustate.h"

namespace sch
{
    class CpuBus;
    class CpuTracer;
    class MainClock;

    class Cpu
    {
    public:
        void            runTo(timestamp_t runto);
        void            setTracer(CpuTracer* trc)       { tracer = trc;             }

        void            reset(CpuBus* thebus, MainClock* theclock);

        void            signalNmi();
        void            signalIrq(u32 irqmask);
        void            acknowledgeIrq(u32 irqmask);

    private:
        enum class IntType
        {
            Reset,
            Abort,
            Nmi,
            Irq,
            Cop,
            Brk
        };
        MainClock*      clock;

        
        CpuTracer*      tracer;
        CpuBus*         bus;
        CpuState        regs;
        
        bool            stopped;
        bool            waiting;
        bool            interruptReady;
        bool            interruptPending;
        bool            resetPending;
        bool            nmiPending;
        u32             irqFlags;
        
        void            innerRun(timestamp_t runto);

        //////////////////////////////////////////////////////
        //////////////////////////////////////////////////////
        //  Reading/writing
        u8              read_pc();
        u8              read(u32 a);
        void            write(u32 a, u8 v);

        void            ioCyc();
        void            dpCyc()
        {
            if(regs.DP & 0xFF)      ioCyc();
        }
        
        //////////////////////////////////////////////////////
        //////////////////////////////////////////////////////
        //  Addressing modes
        typedef         void (Cpu::*rwop_t)(u16& v, bool flg);

        // Support
        u16     ad_final_rd(u32 a, bool flg);
        void    ad_final_wr(u32 a, u16 v, bool flg);
        void    ad_final_rw(u32 a, rwop_t op, bool flg);

        //  Immediate
        u16     ad_rd_im(bool flg);
        //  Absolute
        u32     ad_addr_ab();
        u16     ad_rd_ab(bool flg)              { return ad_final_rd( ad_addr_ab(),     flg );  }
        void    ad_wr_ab(u16 v, bool flg)       {        ad_final_wr( ad_addr_ab(),  v, flg );  }
        void    ad_rw_ab(rwop_t op, bool flg)   {        ad_final_rw( ad_addr_ab(), op, flg );  }
        //  Absolute Long
        u32     ad_addr_al();
        u16     ad_rd_al(bool flg)              { return ad_final_rd( ad_addr_al(),     flg );  }
        void    ad_wr_al(u16 v, bool flg)       {        ad_final_wr( ad_addr_al(),  v, flg );  }
        //  Direct Page
        u32     ad_addr_dp();
        u16     ad_rd_dp(bool flg)              { return ad_final_rd( ad_addr_dp(),     flg );  }
        void    ad_wr_dp(u16 v, bool flg)       {        ad_final_wr( ad_addr_dp(),  v, flg );  }
        void    ad_rw_dp(rwop_t op, bool flg)   {        ad_final_rw( ad_addr_dp(), op, flg );  }
        //  Indirect, Y         LDA ($nn), Y
        u32     ad_addr_iy(bool wr);
        u16     ad_rd_iy(bool flg)              { return ad_final_rd( ad_addr_iy(false),     flg );  }
        void    ad_wr_iy(u16 v, bool flg)       {        ad_final_wr( ad_addr_iy( true),  v, flg );  }
        //  Indirect, Y Long    LDA [$nn], Y
        u32     ad_addr_iyl();
        u16     ad_rd_iyl(bool flg);
        void    ad_wr_iyl(u16 v, bool flg);
        //  Indirect, X         LDA ($nn, X)
        u32     ad_addr_ix();
        u16     ad_rd_ix(bool flg);
        void    ad_wr_ix(u16 v, bool flg);
        //  Direct Page Indexed     LDA $nn,X / LDA $nn,Y
        u32     ad_addr_dr(u16 r);
        u16     ad_rd_dr(u16 r, bool flg);
        void    ad_wr_dr(u16 v, u16 r, bool flg);
        void    ad_rw_dx(rwop_t op, bool flg);
        u16     ad_rd_dx(bool flg)          { return ad_rd_dr(regs.X.w, flg);   }
        u16     ad_rd_dy(bool flg)          { return ad_rd_dr(regs.Y.w, flg);   }
        u16     ad_wr_dx(u16 v, bool flg)   { ad_wr_dr(v, regs.X.w, flg);       }
        u16     ad_wr_dy(u16 v, bool flg)   { ad_wr_dr(v, regs.Y.w, flg);       }
        //  Absolute Indexed     LDA $nnnn,X / LDA $nnnn,Y
        u32     ad_addr_ar(u16 r);
        u16     ad_rd_ar(u16 r, bool flg);
        void    ad_wr_ar(u16 v, u16 r, bool flg);
        void    ad_rw_ax(rwop_t op, bool flg);
        u16     ad_rd_ax(bool flg)          { return ad_rd_ar(regs.X.w, flg);   }
        u16     ad_rd_ay(bool flg)          { return ad_rd_ar(regs.Y.w, flg);   }
        u16     ad_wr_ax(u16 v, bool flg)   { ad_wr_ar(v, regs.X.w, flg);       }
        u16     ad_wr_ay(u16 v, bool flg)   { ad_wr_ar(v, regs.Y.w, flg);       }
        //  Absolute Long, X     LDA $nnnnnn,X
        u32     ad_addr_axl();
        u16     ad_rd_axl(bool flg);
        void    ad_wr_axl(u16 v, bool flg);
        //  Relative (branching)
        void    ad_branch(bool condition);
        //  Direct Page Indirect        LDA ($nn)
        u32     ad_addr_di();
        u16     ad_rd_di(bool flg);
        void    ad_wr_di(u16 v, bool flg);
        //  Direct Page Indirect Long   LDA [$nn]
        u32     ad_addr_dil();
        u16     ad_rd_dil(bool flg);
        void    ad_wr_dil(u16 v, bool flg);
        //  Stack push/pull modes       PHA / PHP / PLA / PLP
        void    ad_push(u16 v, bool flg);
        u16     ad_pull(bool flg);
        //  Stack Relative              LDA $nn, S
        u32     ad_addr_sr();
        u16     ad_rd_sr(bool flg);
        void    ad_wr_sr(u16 v, bool flg);
        //  Stack Relative Ind Y        LDA ($nn, S), Y
        u32     ad_addr_siy();
        u16     ad_rd_siy(bool flg);
        void    ad_wr_siy(u16 v, bool flg);
    };
}

#endif

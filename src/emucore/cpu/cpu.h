
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
        u8              read_pc()       { return read(regs.PBR | regs.PC++);    }
        u8              read(u32 a);
        void            write(u32 a, u8 v);

        void            push(u8 v);
        u8              pull();
        
        void            ioCyc();
        void            ioCyc(int cycs);
        void            dpCyc();
        void            tallyCycle(int cyc);
        void            checkInterruptPending();
        
        //////////////////////////////////////////////////////
        //////////////////////////////////////////////////////
        //  Instructions
        void            ADC(u16 v);
        void            ADC_decimal(u16 v);
        void            AND(u16 v);
        u16             ASL(u16 v, bool flg);
        void            BIT(u16 v, bool set_flags);
        void            CPR(u16 v, u16 r, bool flg);
        void            CMP(u16 v)                      { CPR(v, regs.fM ? regs.A.l : regs.A.w, regs.fM);    }
        void            CPX(u16 v)                      { CPR(v, regs.X.w, regs.fX);    }
        void            CPY(u16 v)                      { CPR(v, regs.Y.w, regs.fX);    }
        u16             DEC(u16 v, bool flg);
        void            EOR(u16 v);
        u16             INC(u16 v, bool flg);
        void            LDR(CpuState::SplitReg& r, u16 v, bool flg);
        void            LDA(u16 v)      { LDR(regs.A, v, regs.fM);  }
        void            LDX(u16 v)      { LDR(regs.X, v, regs.fX);  }
        void            LDY(u16 v)      { LDR(regs.Y, v, regs.fX);  }
        u16             LSR(u16 v, bool flg);
        void            ORA(u16 v);
        u16             ROL(u16 v, bool flg);
        u16             ROR(u16 v, bool flg);
        void            SBC(u16 v);
        void            SBC_decimal(u16 v);
        u16             TRB(u16 v, bool flg);
        u16             TSB(u16 v, bool flg);
        
        u16             STA();
        void            TAX();
        void            TAY();
        void            TCD();
        void            TCS();
        void            TDC();
        void            TSC();
        void            TSX();
        void            TXA();
        void            TXS();
        void            TXY();
        void            TYA();
        void            TYX();
        
        void            u_BRL();
        void            u_JMP_Absolute();
        void            u_JMP_AbsoluteLong();
        void            u_JMP_Indirect();
        void            u_JMP_IndirectX();
        void            u_JMP_IndirectLong();
        void            u_JSR_Absolute();
        void            u_JSR_AbsoluteLong();
        void            u_JSR_IndirectX();
        void            u_MVNP(int adj);
        inline void     u_MVN()                     { u_MVNP( 1);   }
        inline void     u_MVP()                     { u_MVNP(-1);   }
        void            u_PEA();
        void            u_PEI();
        void            u_PER();
        void            u_PLA();
        void            u_REP();
        void            u_RTI();
        void            u_RTL();
        void            u_RTS();
        void            u_SEP();
        void            u_XBA();
        void            u_XCE();

        void            doInterrupt(IntType type);
        
        //////////////////////////////////////////////////////
        //////////////////////////////////////////////////////
        //  Addressing modes
        typedef         u16 (Cpu::*rwop_t)(u16 v, bool flg);

        // Support
        u16     ad_final_rd(u32 a, bool flg);
        void    ad_final_wr(u32 a, u16 v, bool flg);
        void    ad_final_rw(u32 a, rwop_t op, bool flg);

        //  Accumulator mode
        void    ad_rw_ac(rwop_t op);
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
        u16     ad_rd_iyl(bool flg)             { return ad_final_rd( ad_addr_iyl(),     flg ); }
        void    ad_wr_iyl(u16 v, bool flg)      {        ad_final_wr( ad_addr_iyl(),  v, flg ); }
        //  Indirect, X         LDA ($nn, X)
        u32     ad_addr_ix();
        u16     ad_rd_ix(bool flg)              { return ad_final_rd( ad_addr_ix(),     flg );  }
        void    ad_wr_ix(u16 v, bool flg)       {        ad_final_wr( ad_addr_ix(),  v, flg );  }
        //  Direct Page Indexed     LDA $nn,X / LDA $nn,Y
        u32     ad_addr_dr(u16 r);
        u16     ad_rd_dx(bool flg)              { return ad_final_rd( ad_addr_dr(regs.X.w),     flg );  }
        void    ad_wr_dx(u16 v, bool flg)       {        ad_final_wr( ad_addr_dr(regs.X.w),  v, flg );  }
        void    ad_rw_dx(rwop_t op, bool flg)   {        ad_final_rw( ad_addr_dr(regs.X.w), op, flg );  }
        u16     ad_rd_dy(bool flg)              { return ad_final_rd( ad_addr_dr(regs.Y.w),     flg );  }
        void    ad_wr_dy(u16 v, bool flg)       {        ad_final_wr( ad_addr_dr(regs.Y.w),  v, flg );  }
        //  Absolute Indexed     LDA $nnnn,X / LDA $nnnn,Y
        u32     ad_addr_ar(u16 r, bool wr);
        u16     ad_rd_ax(bool flg)              { return ad_final_rd( ad_addr_ar(regs.X.w, false),     flg );  }
        void    ad_wr_ax(u16 v, bool flg)       {        ad_final_wr( ad_addr_ar(regs.X.w,  true),  v, flg );  }
        void    ad_rw_ax(rwop_t op, bool flg)   {        ad_final_rw( ad_addr_ar(regs.X.w,  true), op, flg );  }
        u16     ad_rd_ay(bool flg)              { return ad_final_rd( ad_addr_ar(regs.Y.w, false),     flg );  }
        void    ad_wr_ay(u16 v, bool flg)       {        ad_final_wr( ad_addr_ar(regs.Y.w,  true),  v, flg );  }
        //  Absolute Long, X     LDA $nnnnnn,X
        u32     ad_addr_axl();
        u16     ad_rd_axl(bool flg)             { return ad_final_rd( ad_addr_axl(),    flg );  }
        void    ad_wr_axl(u16 v, bool flg)      {        ad_final_wr( ad_addr_axl(), v, flg );  }
        //  Relative (branching)
        void    ad_branch(bool condition);
        //  Direct Page Indirect        LDA ($nn)
        u32     ad_addr_di();
        u16     ad_rd_di(bool flg)              { return ad_final_rd( ad_addr_di(),    flg );   }
        void    ad_wr_di(u16 v, bool flg)       {        ad_final_wr( ad_addr_di(), v, flg );   }
        //  Direct Page Indirect Long   LDA [$nn]
        u32     ad_addr_dil();
        u16     ad_rd_dil(bool flg)             { return ad_final_rd( ad_addr_dil(),    flg );  }
        void    ad_wr_dil(u16 v, bool flg)      {        ad_final_wr( ad_addr_dil(), v, flg );  }
        //  Stack push/pull modes       PHA / PHP / PLA / PLP
        void    ad_push(u16 v, bool flg);
        u16     ad_pull(bool flg);
        //  Stack Relative              LDA $nn, S
        u32     ad_addr_sr();
        u16     ad_rd_sr(bool flg)              { return ad_final_rd( ad_addr_sr(),    flg );   }
        void    ad_wr_sr(u16 v, bool flg)       {        ad_final_wr( ad_addr_sr(), v, flg );   }
        //  Stack Relative Ind Y        LDA ($nn, S), Y
        u32     ad_addr_siy();
        u16     ad_rd_siy(bool flg)             { return ad_final_rd( ad_addr_siy(),    flg );  }
        void    ad_wr_siy(u16 v, bool flg)      {        ad_final_wr( ad_addr_siy(), v, flg );  }
    };
}

#endif

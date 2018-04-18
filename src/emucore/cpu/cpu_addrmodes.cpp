
#include "cpu.h"

namespace sch
{
    u16 Cpu::ad_rd_im(bool flg)
    {
        u16 v =             read_pc();
        if(!flg)    v |=    read_pc() << 8;
    }
    /*
        //  Absolute
        u16     ad_rd_ab(bool flg);
        void    ad_wr_ab(u16 v, bool flg);
        void    ad_rw_ab(rwop_t op, bool flg);
        //  Absolute Long
        u16     ad_rd_al(bool flg);
        void    ad_wr_al(u16 v, bool flg);
        //  Direct Page
        u16     ad_rd_dp(bool flg);
        void    ad_wr_dp(u16 v, bool flg);
        void    ad_rw_dp(rwop_t op, bool flg);
        //  Implied
        void    ad_impl();
        //  Indirect, Y         LDA ($nn), Y
        u16     ad_rd_iy(bool flg);
        void    ad_wr_iy(u16 v, bool flg);
        //  Indirect, Y Long    LDA [$nn], Y
        u16     ad_rd_iyl(bool flg);
        void    ad_wr_iyl(u16 v, bool flg);
        //  Indirect, X         LDA ($nn, X)
        u16     ad_rd_ix(bool flg);
        void    ad_wr_ix(u16 v, bool flg);
        //  Direct Page Indexed     LDA $nn,X / LDA $nn,Y
        u16     ad_rd_dr(u16 r, bool flg);
        void    ad_wr_dr(u16 v, u16 r, bool flg);
        void    ad_rw_dx(rwop_t op, bool flg);
        u16     ad_rd_dx(bool flg)          { return ad_rd_dr(regs.X.w, flg);   }
        u16     ad_rd_dy(bool flg)          { return ad_rd_dr(regs.Y.w, flg);   }
        u16     ad_wr_dx(u16 v, bool flg)   { ad_wr_dr(v, regs.X.w, flg);       }
        u16     ad_wr_dy(u16 v, bool flg)   { ad_wr_dr(v, regs.Y.w, flg);       }
        //  Absolute Indexed     LDA $nnnn,X / LDA $nnnn,Y
        u16     ad_rd_ar(u16 r, bool flg);
        void    ad_wr_ar(u16 v, u16 r, bool flg);
        void    ad_rw_ax(rwop_t op, bool flg);
        u16     ad_rd_ax(bool flg)          { return ad_rd_ar(regs.X.w, flg);   }
        u16     ad_rd_ay(bool flg)          { return ad_rd_ar(regs.Y.w, flg);   }
        u16     ad_wr_ax(u16 v, bool flg)   { ad_wr_ar(v, regs.X.w, flg);       }
        u16     ad_wr_ay(u16 v, bool flg)   { ad_wr_ar(v, regs.Y.w, flg);       }
        //  Absolute Long, X     LDA $nnnnnn,X
        u16     ad_rd_axl(u16 r, bool flg);
        void    ad_wr_axl(u16 v, u16 r, bool flg);
        //  Relative (branching)
        void    ad_branch(bool condition);
        //  Direct Page Indirect        LDA ($nn)
        u16     ad_rd_di(bool flg);
        void    ad_wr_di(u16 v, bool flg);
        //  Direct Page Indirect Long   LDA [$nn]
        u16     ad_rd_dil(bool flg);
        void    ad_wr_dil(u16 v, bool flg);
        //  Stack push/pull modes       PHA / PHP / PLA / PLP
        void    ad_push(u16 v, bool flg);
        u16     ad_pull(bool flg);
        //  Stack Relative              LDA $nn, S
        u16     ad_rd_sr(bool flg);
        void    ad_wr_sr(u16 v, bool flg);
        //  Stack Relative Ind Y        LDA ($nn, S), Y
        u16     ad_rd_siy(bool flg);
        void    ad_wr_siy(u16 v, bool flg);
        */
}
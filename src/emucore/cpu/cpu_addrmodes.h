
//#define HACK_TO_GET_INTELLISENSE_TO_WORK

#ifdef HACK_TO_GET_INTELLISENSE_TO_WORK
    #error Disable this hack!

    #include "cpustate.h"

    namespace sch {
    CpuState     regs;
    
        void            dpCyc();                    // Do an IO cycle if low byte if DP isn't zero
        void            ioCyc();
        void            doIndex(u16& a, u16 idx);   // Add an index to an address, and add an extra IO cyc when appropriate

        u8              read_l(u32 a);              // long read (24-bit)
        u8              read_a(u16 a);              // absolute read (16-bit, using DBR)
        u8              read_p();                   // instruction read (using PC and PBR)
        u8              pull();
        
        void            write_l(u32 a, u8 v);       // long write (24-bit)
        void            write_a(u16 a, u8 v);       // absolute write (16-bit, using DBR)
        void            push(u8 v);

        typedef int     op_t;
        #define CALLOP(v)   v
#endif

//////////////////////////////////////////////////
// Read addressing modes

u16 ad_rd_im(bool flg)                      // Immediate:       LDA #$ii / LDA #$iiii
{
    u16 v =                 read_p();
    if(!flg)    v |=        read_p() << 8;
    return v;
}

u16 ad_rd_dp(bool flg)                      // Direct Page:     LDA $dd
{
    u16 a =                 read_p();
    a += regs.DP;           dpCyc();
    u16 v =                 read_l(a);
    if(!flg)    v |=        read_l(++a) << 8;
    return v;
}

u16 ad_rd_dr(u16 index, bool flg)           // Direct, X/Y:     LDA $dd, X   /  LDA $dd, Y
{
    u16 a =                 read_p();
    a += regs.DP;           dpCyc();
    a += index;             ioCyc();
    u16 v =                 read_l(a);
    if(!flg)    v |=        read_l(++a) << 8;
    return v;
}
u16 ad_rd_dx(bool flg)  { return ad_rd_dr(regs.X.w, flg);   }
u16 ad_rd_dy(bool flg)  { return ad_rd_dr(regs.Y.w, flg);   }

u16 ad_rd_ab(bool flg)                      // Absolute:        LDA $aaaa
{
    u16 a =                 read_p();
    a |=                    read_p() << 8;
    u16 v =                 read_a(a);
    if(!flg)    v |=        read_a(a+1) << 8;
    return v;
}

u16 ad_rd_ar(u16 index, bool flg)           // Absolute, X/Y:   LDA $aaaa,X  /  LDA $aaaa,Y
{
    u16 a =                 read_p();
    a |=                    read_p() << 8;
                            doIndex(a, index);
    u16 v =                 read_a(a);
    if(!flg)    v |=        read_a(a+1) << 8;
    return v;
}
u16 ad_rd_ax(bool flg)  { return ad_rd_ar(regs.X.w, flg);   }
u16 ad_rd_ay(bool flg)  { return ad_rd_ar(regs.Y.w, flg);   }

u16 ad_rd_al(bool flg)                      // Absolute Long:   LDA $llllll
{
    u32 a =                 read_p();
    a |=                    read_p() << 8;
    a |=                    read_p() << 16;
    u16 v =                 read_l(a);
    if(!flg)    v |=        read_l(a+1) << 8;
    return v;
}

u16 ad_rd_axl(bool flg)                     // Absolute Long X: LDA $llllll,X
{
    u16 a =                 read_p();
    a |=                    read_p() << 8;
    u32 bnk =               read_p() << 16;     a += regs.X.w;
    u16 v =                 read_l(bnk | a);
    if(!flg)    v |=        read_l(bnk | ++a) << 8;
    return v;
}

u16 ad_rd_iy(bool flg)                      // Indirect, Y:     LDA ($dd),Y
{
    u16 tmp =               read_p();
    tmp += regs.DP;         dpCyc();
    u16 a =                 read_l(tmp++);
    a |=                    read_l(tmp) << 8;
                            doIndex(a, regs.Y.w);
    u16 v =                 read_a(a);
    if(!flg)    v |=        read_a(a+1) << 8;
    return v;
}

u16 ad_rd_iyl(bool flg)                      // Indirect, Y Lng: LDA [$dd],Y
{
    u16 tmp =               read_p();
    tmp += regs.DP;         dpCyc();
    u16 a =                 read_l(tmp++);
    a |=                    read_l(tmp++) << 8;
    u32 bnk =               read_l(tmp) << 16;      a += regs.Y.w;
    u16 v =                 read_l(bnk | a);
    if(!flg)    v |=        read_l(bnk | ++a) << 8;
    return v;
}

u16 ad_rd_ix(bool flg)                      // Indirect, X:     LDA ($dd,X)
{
    u16 tmp =               read_p();
    tmp += regs.DP;         dpCyc();
    tmp += regs.X.w;        ioCyc();
    u16 a =                 read_l(tmp++);
    a |=                    read_l(tmp) << 8;
    u16 v =                 read_a(a);
    if(!flg)    v |=        read_a(a+1) << 8;
    return v;
}

u16 ad_rd_di(bool flg)                      // DP Indirect:     LDA ($dd)
{
    u16 tmp =               read_p();
    tmp += regs.DP;         dpCyc();
    u16 a =                 read_l(tmp++);
    a |=                    read_l(tmp) << 8;
    u16 v =                 read_a(a);
    if(!flg)    v |=        read_a(a+1) << 8;
    return v;
}

u16 ad_rd_dil(bool flg)                     // DP Indirect Lng: LDA [$dd]
{
    u16 tmp =               read_p();
    tmp += regs.DP;         dpCyc();
    u32 a =                 read_l(tmp++);
    a |=                    read_l(tmp++) << 8;
    a |=                    read_l(tmp) << 16;
    u16 v =                 read_l(a);
    if(!flg)    v |=        read_l(a+1) << 8;
    return v;
}

u16 ad_rd_sr(bool flg)                      // Stack Relative:  LDA $dd,S
{
    u16 a =                 read_p();
    a += regs.SP;           ioCyc();
    u16 v =                 read_l(a);
    if(!flg)    v |=        read_l(++a) << 8;
    return v;
}

u16 ad_rd_siy(bool flg)                     // Stack Rel Ind,Y: LDA ($dd,S),Y
{
    u16 tmp =               read_p();
    tmp += regs.SP;         ioCyc();
    u16 a =                 read_l(tmp++);
    a |=                    read_l(tmp) << 8;
    a += regs.Y.w;          ioCyc();
    u16 v =                 read_a(a);
    if(!flg)    v |=        read_a(a+1) << 8;
    return v;
}


//////////////////////////////////////////////////
// Read / Modify / Write modes

void ad_rw_ac(op_t op)
{
    ioCyc();
    if(regs.fM)     regs.A.l = CALLOP(regs.A.l, regs.fM) & 0xFF;
    else            regs.A.w = CALLOP(regs.A.w, regs.fM);
}

void ad_rw_dp(op_t op, bool flg)
{
    u16 a =                 read_p();
    a += regs.DP;           dpCyc();
    u16 v =                 read_l(a);
    if(flg)
    {
        v = CALLOP(v,flg);  ioCyc();
    }
    else
    {
        u16 a1 = a+1;
        v |=                read_l(a1) << 8;
        v = CALLOP(v,flg);  ioCyc();
                            write_l(a1, v >> 8);

    }
                            write_l(a, v & 0xFF);
}

void ad_rw_dx(op_t op, bool flg)
{
    u16 a =                 read_p();
    a += regs.DP;           dpCyc();
    a += regs.X.w;          ioCyc();
    u16 v =                 read_l(a);
    if(flg)
    {
        v = CALLOP(v,flg);  ioCyc();
    }
    else
    {
        u16 a1 = a+1;
        v |=                read_l(a1) << 8;
        v = CALLOP(v,flg);  ioCyc();
                            write_l(a1, v >> 8);

    }
                            write_l(a, v & 0xFF);
}

void ad_rw_ab(op_t op, bool flg)
{
    u16 a =                 read_p();
    a |=                    read_p() << 8;
    u16 v =                 read_a(a);
    if(flg)
    {
        v = CALLOP(v,flg);  ioCyc();
    }
    else
    {
        v |=                read_a(a+1) << 8;
        v = CALLOP(v,flg);  ioCyc();
                            write_a(a+1, v >> 8);

    }
                            write_a(a, v & 0xFF);
}

void ad_rw_ax(op_t op, bool flg)
{
    u16 a =                 read_p();
    a |=                    read_p() << 8;
    a += regs.X.w;          ioCyc();
    u16 v =                 read_a(a);
    if(flg)
    {
        v = CALLOP(v,flg);  ioCyc();
    }
    else
    {
        v |=                read_a(a+1) << 8;
        v = CALLOP(v,flg);  ioCyc();
                            write_a(a+1, v >> 8);

    }
                            write_a(a, v & 0xFF);
}


//////////////////////////////////////////////////
// Write addressing modes

void ad_wr_dp(u16 v, bool flg)              // Direct Page:     STA $dd
{
    u16 a =                 read_p();
    a += regs.DP;           dpCyc();
                            write_l(a, v & 0xFF);
    if(!flg)                write_l(++a, v >> 8);
}

void ad_wr_dr(u16 v, u16 index, bool flg)   // Direct, X/Y:     STA $dd, X   /  LDA $dd, Y
{
    u16 a =                 read_p();
    a += regs.DP;           dpCyc();
    a += index;             ioCyc();
                            write_l(a, v & 0xFF);
    if(!flg)                write_l(++a, v >> 8);
}
void ad_wr_dx(u16 v, bool flg)  { ad_wr_dr(v, regs.X.w, flg);   }
void ad_wr_dy(u16 v, bool flg)  { ad_wr_dr(v, regs.Y.w, flg);   }

void ad_wr_ab(u16 v, bool flg)              // Absolute:        STA $aaaa
{
    u16 a =                 read_p();
    a |=                    read_p() << 8;
                            write_a(a, v & 0xFF);
    if(!flg)                write_a(a+1, v >> 8);
}

void ad_wr_ar(u16 v, u16 index, bool flg)   // Absolute, X/Y:   STA $aaaa,X  /  LDA $aaaa,Y
{
    u16 a =                 read_p();
    a |=                    read_p() << 8;
    a += index;             ioCyc();
                            write_a(a, v & 0xFF);
    if(!flg)                write_a(a+1, v >> 8);
}
void ad_wr_ax(u16 v, bool flg)  { ad_wr_ar(v, regs.X.w, flg);   }
void ad_wr_ay(u16 v, bool flg)  { ad_wr_ar(v, regs.Y.w, flg);   }

void ad_wr_al(u16 v, bool flg)              // Absolute Long:   STA $llllll
{
    u32 a =                 read_p();
    a |=                    read_p() << 8;
    a |=                    read_p() << 16;
                            write_l(a, v & 0xFF);
    if(!flg)                write_l(a+1, v >> 8);
}

void ad_wr_axl(u16 v, bool flg)             // Absolute Long X: STA $llllll,X
{
    u16 a =                 read_p();
    a |=                    read_p() << 8;
    u32 bnk =               read_p() << 16;     a += regs.X.w;
                            write_l(bnk | a, v & 0xFF);
    if(!flg)                write_l(bnk | ++a, v >> 8);
}

void ad_wr_iy(u16 v, bool flg)              // Indirect, Y:     STA ($dd),Y
{
    u16 tmp =               read_p();
    tmp += regs.DP;         dpCyc();
    u16 a =                 read_l(tmp++);
    a |=                    read_l(tmp) << 8;
    a += regs.Y.w;          ioCyc();
                            write_a(a, v & 0xFF);
    if(!flg)                write_a(a+1, v >> 8);
}

void ad_wr_iyl(u16 v, bool flg)             // Indirect, Y Lng: STA [$dd],Y
{
    u16 tmp =               read_p();
    tmp += regs.DP;         dpCyc();
    u16 a =                 read_l(tmp++);
    a |=                    read_l(tmp++) << 8;
    u32 bnk =               read_l(tmp) << 16;      a += regs.Y.w;
                            write_l(bnk | a, v & 0xFF);
    if(!flg)                write_l(bnk | ++a, v >> 8);
}

void ad_wr_ix(u16 v, bool flg)              // Indirect, X:     STA ($dd,X)
{
    u16 tmp =               read_p();
    tmp += regs.DP;         dpCyc();
    tmp += regs.X.w;        ioCyc();
    u16 a =                 read_l(tmp++);
    a |=                    read_l(tmp) << 8;
                            write_a(a, v & 0xFF);
    if(!flg)                write_a(a+1, v >> 8);
}

void ad_wr_di(u16 v, bool flg)              // DP Indirect:     STA ($dd)
{
    u16 tmp =               read_p();
    tmp += regs.DP;         dpCyc();
    u16 a =                 read_l(tmp++);
    a |=                    read_l(tmp) << 8;
                            write_a(a, v & 0xFF);
    if(!flg)                write_a(a+1, v >> 8);
}

void ad_wr_dil(u16 v, bool flg)             // DP Indirect Lng: STA [$dd]
{
    u16 tmp =               read_p();
    tmp += regs.DP;         dpCyc();
    u32 a =                 read_l(tmp++);
    a |=                    read_l(tmp++) << 8;
    a |=                    read_l(tmp) << 16;
                            write_l(a, v & 0xFF);
    if(!flg)                write_l(a+1, v >> 8);
}

void ad_wr_sr(u16 v, bool flg)              // Stack Relative:  STA $dd,S
{
    u16 a =                 read_p();
    a += regs.SP;           ioCyc();
                            write_l(a, v & 0xFF);
    if(!flg)                write_l(++a, v >> 8);
}

void ad_wr_siy(u16 v, bool flg)             // Stack Rel Ind,Y: STA ($dd,S),Y
{
    u16 tmp =               read_p();
    tmp += regs.SP;         ioCyc();
    u16 a =                 read_l(tmp++);
    a |=                    read_l(tmp) << 8;
    a += regs.Y.w;          ioCyc();
                            write_a(a, v & 0xFF);
    if(!flg)                write_a(a+1, v >> 8);
}



//////////////////////////////////////////////////
// Stack modes

void ad_push(u16 v, bool flg)
{
                        ioCyc();
    if(!flg)            push(v >> 8);
                        push(v & 0xFF);
}

u16 ad_pull(bool flg)       // This updates NZ, which is probably not appropriate for an addressing mode
{                           //    function, but it makes this waaaaay easier
                        ioCyc();
                        ioCyc();
    u16 v =             pull();
    if(flg)
    {
        regs.fN = v & 0x80;
    }
    else
    {
        v |=            pull() << 8;
        regs.fN = v & 0x8000;
    }
    regs.fZ = v;
    return v;
}
        
#ifdef HACK_TO_GET_INTELLISENSE_TO_WORK
}
#endif
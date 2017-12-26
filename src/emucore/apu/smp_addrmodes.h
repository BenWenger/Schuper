
//#define HACK_TO_GET_INTELLISENSE_TO_WORK

#define CALLOP(dst,src) (this->*op)(dst,src)
#define CALLOP1(v)      (this->*op)(v)

#ifdef HACK_TO_GET_INTELLISENSE_TO_WORK
    #error Disable this hack!

    #include "smpregs.h"
    namespace sch {
    SmpRegs     regs;
    #undef CALLOP
    #define CALLOP(dst,src)

    #undef CALLOP1
    #define CALLOP1(v)      true

    #define BRANCH(condition, rl)

        u8              read(u16 a);            // standard read
        u8              dpRd(u8 a);             // direct page read
        u8              pull();                 // simple pull (single read off stack)
        void            write(u16 a, u8 v);     // standard write
        void            dpWr(u8 a, u8 v);       // direct page write
        void            push(u8 v);             // simple push (single write to stack)
        void            ioCyc();                // I/O cycle (for internal operations that take a cycle)    
        void            ioCyc(int cycs);        // I/O cycle (multiple)
#endif
    
///////////////////////////////////////////////
        
typedef         void (Smp::*op_t)(u8& dst, u8 src);
typedef         void (Smp::*op8_t)(u8& v);
typedef         void (Smp::*op16_t)(u16& v);
typedef         bool (Smp::*opTest_t)(u8 v);

///////////////////////////////////////////////
//  Most SPC instructions have 2 parameters... 'dst' and 'src'
//  Ex:  "MOV A, #0"  <-  A is the destination and #0 is the source
//
//  Each addressing mode is named 'ad_<dst>_<src>', where dst and src can be one
//  of the following:
//
//  ac = Accumulator
//  xx = X reg
//  yy = Y reg
//  ya = YA (Y and A combined for form a 16-bit value)
//  sp = SP reg
//  rg = A, X, or Y, passed in as an additional parameter
//  xi = (X)
//  yi = (Y)
//  xp = (X)+
//  im = Immediate          #0
//  dp = Direct Page        0
//  dx = Direct Page, X     0,X
//  dy = Direct Page, Y     0,Y
//  dr = Direct Page, X or Y (passed in as parameter)
//  ab = Absolute
//  ax = Absolute, X
//  ay = Absolute, Y
//  ar = Absolute, X or Y (passed in)
//  ix = Indirect, X        [0+X]
//  iy = Indirect, Y        [0]+Y
//  cc = Carry flag
//  mb = 13 bit memory address, with 3 bit bit number


//  Each mode takes the indicated number of cycles (remember that each read/write is one cycle)
//    The number mentioned is 1 higher than the cycles consumed in these functions, as the opcode
//    fetch prior to this function being called was the first cycle.

/////////////////////////////////////////////
//  MOV A, #0           (2 cycles)
void ad_rg_im(op_t op, u8& reg)
{
    u8 v = read(regs.PC++);
    CALLOP(reg, v);
}
inline void ad_ac_im(op_t op)       { ad_rg_im(op, regs.A);     }
inline void ad_xx_im(op_t op)       { ad_rg_im(op, regs.X);     }
inline void ad_yy_im(op_t op)       { ad_rg_im(op, regs.Y);     }

/////////////////////////////////////////////
//  MOV reg, reg
void ad_ac_xx(op_t op)              { ioCyc();      CALLOP(regs.A, regs.X);     }
void ad_ac_yy(op_t op)              { ioCyc();      CALLOP(regs.A, regs.Y);     }
void ad_sp_xx(op_t op)              { ioCyc();      CALLOP(regs.SP,regs.X);     }
void ad_xx_ac(op_t op)              { ioCyc();      CALLOP(regs.X, regs.A);     }
void ad_xx_sp(op_t op)              { ioCyc();      CALLOP(regs.X, regs.SP);    }
void ad_yy_ac(op_t op)              { ioCyc();      CALLOP(regs.Y, regs.A);     }

/////////////////////////////////////////////
//  MOV A, 0           (3 cycles)
void ad_rg_dp(op_t op, u8& dst)
{
    u8 addr = read(regs.PC++);
    u8 v    = dpRd(addr);
    CALLOP(dst, v);
}
inline void ad_ac_dp(op_t op)       { ad_rg_dp(op, regs.A);     }
inline void ad_xx_dp(op_t op)       { ad_rg_dp(op, regs.X);     }
inline void ad_yy_dp(op_t op)       { ad_rg_dp(op, regs.Y);     }

/////////////////////////////////////////////
//  MOV A, 0+X         (4 cycles)
void ad_rg_dr(op_t op, u8& dst, u8 index)
{
    u8 addr = read(regs.PC++);
    addr += index;          ioCyc();
    u8 v    = dpRd(addr);
    CALLOP(dst, v);
}
inline void ad_ac_dx(op_t op)       { ad_rg_dr(op, regs.A, regs.X);     }
inline void ad_xx_dy(op_t op)       { ad_rg_dr(op, regs.X, regs.Y);     }
inline void ad_yy_dx(op_t op)       { ad_rg_dr(op, regs.Y, regs.X);     }

/////////////////////////////////////////////
//  MOV A, (X)         (3 cycles)
void ad_ac_xi(op_t op)
{
    ioCyc();
    u8 v =  dpRd(regs.X);
    CALLOP(regs.A, v);
}

/////////////////////////////////////////////
//  MOV A, (X)+        (4 cycles)
void ad_ac_xp(op_t op)
{
    ioCyc();
    u8 v =  dpRd(regs.X++);
    ioCyc();
    CALLOP(regs.A, v);
}

/////////////////////////////////////////////
//  MOV A, [0+X]       (6 cycles)
void ad_ac_ix(op_t op)
{
    u8 dpaddr = read(regs.PC++);
    ioCyc();        dpaddr += regs.X;
    u16 addr = dpRd(dpaddr);
    addr |= dpRd(dpaddr+1) << 8;
    u8 v = read(addr);
    CALLOP(regs.A, v);
}

/////////////////////////////////////////////
//  MOV A, [0]+Y       (6 cycles)
void ad_ac_iy(op_t op)
{
    u8 dpaddr = read(regs.PC++);
    u16 addr = dpRd(dpaddr);
    addr |= dpRd(dpaddr+1) << 8;
    ioCyc();            addr += regs.Y;
    u8 v = read(addr);
    CALLOP(regs.A, v);
}

/////////////////////////////////////////////
//  MOV A, !0          (4 cycles)
void ad_rg_ab(op_t op, u8& dst)
{
    u16 addr = read(regs.PC++);
    addr |= read(regs.PC++) << 8;
    u8 v = read(addr);
    CALLOP(dst, v);
}
inline void ad_ac_ab(op_t op)       { ad_rg_ab(op, regs.A);     }
inline void ad_xx_ab(op_t op)       { ad_rg_ab(op, regs.X);     }
inline void ad_yy_ab(op_t op)       { ad_rg_ab(op, regs.Y);     }

/////////////////////////////////////////////
//  MOV A, !0+X        (5 cycles)
void ad_ac_ar(op_t op, u8 index)
{
    u16 addr = read(regs.PC++);
    addr |= read(regs.PC++) << 8;
    addr += index;          ioCyc();
    u8 v = read(addr);
    CALLOP(regs.A, v);
}
inline void ad_ac_ax(op_t op)       { ad_ac_ar(op, regs.X);     }
inline void ad_ac_ay(op_t op)       { ad_ac_ar(op, regs.Y);     }

/////////////////////////////////////////////
//  MOV 0, #0          (5 cycles)
void ad_dp_im(op_t op, bool performwrite = true)            // CMP doesn't write, it does an ioCyc instead
{
    u8 src = read(regs.PC++);
    u8 dstaddr = read(regs.PC++);
    u8 dst = dpRd(dstaddr);
    CALLOP(dst, src);
    if(performwrite) dpWr(dstaddr, dst);    else    ioCyc();
}

/////////////////////////////////////////////
//  MOV 0, A           (4 cycles)
void ad_dp_rg(op_t op, u8 src)
{
    u8 dstaddr = read(regs.PC++);
    u8 dst = dpRd(dstaddr);
    CALLOP(dst, src);
    dpWr(dstaddr, dst);
}
inline void ad_dp_ac(op_t op)       { ad_dp_rg(op, regs.A);     }
inline void ad_dp_xx(op_t op)       { ad_dp_rg(op, regs.X);     }
inline void ad_dp_yy(op_t op)       { ad_dp_rg(op, regs.Y);     }

/////////////////////////////////////////////
//  MOV 0+X, A         (5 cycles)
void ad_dr_rg(op_t op, u8 index, u8 src)
{
    u8 dstaddr = read(regs.PC++);
    dstaddr += index;           ioCyc();
    u8 dst = dpRd(dstaddr);
    CALLOP(dst, src);
    dpWr(dstaddr, dst);
}
inline void ad_dx_ac(op_t op)       { ad_dr_rg(op, regs.X, regs.A);     }
inline void ad_dy_xx(op_t op)       { ad_dr_rg(op, regs.Y, regs.X);     }
inline void ad_dx_yy(op_t op)       { ad_dr_rg(op, regs.X, regs.Y);     }

/////////////////////////////////////////////
//  MOV (X), A         (4 cycles)
void ad_xi_ac(op_t op)
{
    ioCyc();
    u8 dst = dpRd(regs.X);
    CALLOP(dst, regs.A);
    dpWr(regs.X, dst);
}

/////////////////////////////////////////////
//  MOV (X)+, A        (4 cycles)
void ad_xp_ac(op_t op)
{
    ioCyc();
    u8 dst = 0;     ioCyc();
    CALLOP(dst, regs.A);
    dpWr(regs.X++, dst);
}

/////////////////////////////////////////////
//  MOV [0+X], A       (7 cycles)
void ad_ix_ac(op_t op)
{
    u8 dstaddr = read(regs.PC++);
    dstaddr += regs.X;          ioCyc();
    u16 addr = dpRd(dstaddr);
    addr |= dpRd(dstaddr+1) << 8;
    u8 dst = read(addr);
    CALLOP(dst, regs.A);
    write(addr, dst);
}

/////////////////////////////////////////////
//  MOV [0]+Y, A       (7 cycles)
void ad_iy_ac(op_t op)
{
    u8 dstaddr = read(regs.PC++);
    u16 addr = dpRd(dstaddr);
    addr |= dpRd(dstaddr+1) << 8;
    addr += regs.Y;     ioCyc();
    u8 dst = read(addr);
    CALLOP(dst, regs.A);
    write(addr, dst);
}

/////////////////////////////////////////////
//  MOV !0, A       (5 cycles)
void ad_ab_rg(op_t op, u8 src)
{
    u16 addr = read(regs.PC++);
    addr |= read(regs.PC++) << 8;
    u8 dst = read(addr);
    CALLOP(dst, src);
    write(addr, dst);
}
inline void ad_ab_ac(op_t op)       { ad_ab_rg(op, regs.A);     }
inline void ad_ab_xx(op_t op)       { ad_ab_rg(op, regs.X);     }
inline void ad_ab_yy(op_t op)       { ad_ab_rg(op, regs.Y);     }

/////////////////////////////////////////////
//  MOV !0+X, A     (6 cycles)
void ad_ar_ac(op_t op, u8 index)
{
    u16 addr = read(regs.PC++);
    addr |= read(regs.PC++) << 8;
    addr += index;      ioCyc();
    u8 dst = read(addr);
    CALLOP(dst, regs.A);
    write(addr, dst);
}
inline void ad_ax_ac(op_t op)       { ad_ar_ac(op, regs.X);     }
inline void ad_ay_ac(op_t op)       { ad_ar_ac(op, regs.Y);     }

/////////////////////////////////////////////
//  MOV 0, 0        (6 cycles - or 5 for MOV)
void ad_dp_dp(op_t op, bool has_read = true, bool haswrite = true)     // MOV has no Read (only 5 cycles), CMP replaces write with iocyc
{
    u8 srcaddr = read(regs.PC++);
    u8 src = dpRd(srcaddr);
    u8 dstaddr = read(regs.PC++);
    u8 dst = (has_read ? dpRd(dstaddr) : 0);
    CALLOP(dst, src);
    if(haswrite) dpWr(dstaddr, dst); else ioCyc();
}

/////////////////////////////////////////////
//  MOV (X), (Y)    (5 cycles)
void ad_xi_yi(op_t op, bool performwrite = true)    // CMP doesn't write, it does an IO cyc instead
{
    ioCyc();
    u8 src = dpRd(regs.Y);
    u8 dst = dpRd(regs.X);
    CALLOP(dst, src);
    if(performwrite)    dpWr(regs.X, dst);      else ioCyc();
}

/////////////////////////////////////////////
//  ASL 0           (4 cycles)
void ad_dp(op8_t op)
{
    u8 addr = read(regs.PC++);
    u8 val = dpRd(addr);
    CALLOP1(val);
    dpWr(addr, val);
}

/////////////////////////////////////////////
//  NOT1 m.b        (5 cycles)
void ad_mb(op_t op)
{
    u16 addr = read(regs.PC++);
    addr |= read(regs.PC++) << 8;       u8 bitno = addr>>13;        addr &= 0x1FFF;
    u8 v = read(addr);
    CALLOP(v, 1<<bitno);
    write(addr, v);
}

/////////////////////////////////////////////
//  ASL 0+X         (5 cycles)
void ad_dx(op8_t op)
{
    u8 addr = read(regs.PC++);
    addr += regs.X;             ioCyc();
    u8 val = dpRd(addr);
    CALLOP1(val);
    dpWr(addr, val);
}

/////////////////////////////////////////////
//  ASL !0          (5 cycles, or 6 for TSET1, TCLR1)
void ad_ab(op8_t op, bool extracyc = false)      // TSET1, TCLR1 have an extra wasted cycle
{
    u16 addr = read(regs.PC++);
    addr |= read(regs.PC++) << 8;
    if(extracyc)                read(addr);
    u8 v = read(addr);
    CALLOP1(v);
    write(addr, v);
}

/////////////////////////////////////////////
//  AND1 m.b        (4 cycles, or 5 for EOR1 and OR1)
void ad_cc_mb(op_t op, bool extracyc = false)   // EOR1 and OR1 have an extra cycle
{
    u16 addr = read(regs.PC++);
    addr |= read(regs.PC++) << 8;       u8 bitno = addr>>13;        addr &= 0x1FFF;
    u8 v = read(addr);
    CALLOP(v, 1<<bitno);
    if(extracyc)            ioCyc();
}

/////////////////////////////////////////////
//  MOV1 m.b, cc    (6 cycles)
void ad_mb_cc(op_t op)
{
    u16 addr = read(regs.PC++);
    addr |= read(regs.PC++) << 8;       u8 bitno = addr>>13;        addr &= 0x1FFF;
    u8 v = read(addr);
    CALLOP(v, 1<<bitno);
    ioCyc();
    write(addr, v);
}

/////////////////////////////////////////////
//  ADDW YA, 0      (5 cycles, 4 for CMPW)
void ad_ya_dp(op16_t op, bool extracyc = true)      // CMPW has no extra cyc
{
    u8 addr = read(regs.PC++);
    u16 v = dpRd(addr);
    if(extracyc)        ioCyc();
    v |= dpRd(addr+1)<<8;
    CALLOP1(v);
}

/////////////////////////////////////////////
//  PUSH A          (4 cycles)
void ad_push(u8 v)
{
    ioCyc();
    push(v);
    ioCyc();
}

/////////////////////////////////////////////
//  POP A           (4 cycles)
u8 ad_pop()
{
    ioCyc();
    u8 v = pull();
    ioCyc();
    return v;
}

/////////////////////////////////////////////
//  BCC 0           (branching -- 2 cycles, +2 branch taken)
void ad_branch(bool condition)
{
    u8 rl = read(regs.PC++);
    BRANCH(condition, rl);
}

/////////////////////////////////////////////
//  CBNE dp, rl     (5 cycles, +2 if branch taken)
void ad_dp_rl(opTest_t op)
{
    u8 addr = read(regs.PC++);
    u8 v = dpRd(addr);
    u8 rl = read(regs.PC++);
    ioCyc();
    BRANCH( CALLOP1(v), rl );
}

/////////////////////////////////////////////
//  CBNE dp+X, rl     (6 cycles, +2 if branch taken)
void ad_dx_rl(opTest_t op)
{
    u8 addr = read(regs.PC++);
    addr += regs.X;         ioCyc();
    u8 v = dpRd(addr);
    u8 rl = read(regs.PC++);
    ioCyc();
    BRANCH( CALLOP1(v), rl );
}


#ifdef HACK_TO_GET_INTELLISENSE_TO_WORK
    }
#endif
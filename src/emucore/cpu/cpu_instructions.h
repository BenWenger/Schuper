
//#define HACK_TO_GET_INTELLISENSE_TO_WORK

#ifdef HACK_TO_GET_INTELLISENSE_TO_WORK
    #error Disable this hack!

    #include "cpustate.h"

    namespace sch {
    CpuState     regs;
    
        void            dpCyc();                    // Do an IO cycle if low byte if DP isn't zero
        void            ioCyc();
        void            ioCyc(int cycs);
        void            doIndex(u16& a, u16 idx);   // Add an index to an address, and add an extra IO cyc when appropriate

        u8              read_l(u32 a);              // long read (24-bit)
        u8              read_a(u16 a);              // absolute read (16-bit, using DBR)
        u8              read_p();                   // instruction read (using PC and PBR)
        u8              pull();
        
        void            write_l(u32 a, u8 v);       // long write (24-bit)
        void            write_a(u16 a, u8 v);       // absolute write (16-bit, using DBR)
        void            push(u8 v);
#endif

/////////////////////////////////////////////////////////
//  Common

void ADC_decimal(u16 v)
{
    // This is so freaking gross
    if(regs.fM)
    {
        int lo = (regs.A8 & 0x0F) + (v & 0x0F) + !!regs.fC;         if(lo > 0x09)   lo += 0x06;
        int hi = (regs.A8 & 0xF0) + (v & 0xF0) + (lo & 0xF0);

        regs.fZ = (regs.A8 + v + !!regs.fC) & 0xFF;
        regs.fN = (hi & 0x80);
        regs.fV = (hi ^ regs.A8) & (hi ^ v) & 0x80;

        if(hi > 0x90)       hi += 0x60;     // yes, apparently this happens after setting above flags

        regs.fC = (hi > 0xFF);
        regs.A8 = (hi & 0xF0) | (lo & 0x0F);
    }
    else
    {
        int a = (regs.A16 & 0x000F) + (v & 0x000F) + !!regs.fC;     if(a > 0x0009)  a += 0x0006;
        int b = (regs.A16 & 0x00F0) + (v & 0x00F0) + (a & 0x00F0);  if(b > 0x0090)  b += 0x0060;
        int c = (regs.A16 & 0x0F00) + (v & 0x0F00) + (b & 0x0F00);  if(c > 0x0900)  c += 0x0600;
        int d = (regs.A16 & 0xF000) + (v & 0xF000) + (c & 0xF000);

        regs.fZ = (regs.A16 + v + !!regs.fC) & 0xFFFF;
        regs.fN = (d & 0x8000);
        regs.fV = (d ^ regs.A16) & (d ^ v) & 0x8000;

        if(d > 0x9000)      d += 0x6000;

        regs.fC = (d > 0xFFFF);
        regs.A16 = (d & 0xF000) | (c & 0x0F00) | (b & 0x00F0) | (a & 0x000F);
    }
}

void ADC(u16 v)
{
    if( regs.fD )
        ADC_decimal(v);
    else
    {
        if(regs.fM)
        {
            int sum = regs.A8 + v + !!regs.fC;
            regs.fV = (sum ^ regs.A8) & (sum ^ v) & 0x80;
            regs.fC = (sum > 0xFF);
            regs.fN = (sum & 0x80);
            regs.fZ = regs.A8 = (sum & 0xFF);
        }
        else
        {
            int sum = regs.A16 + v + !!regs.fC;
            regs.fV = (sum ^ regs.A16) & (sum ^ v) & 0x8000;
            regs.fC = (sum > 0xFFFF);
            regs.fN = (sum & 0x8000);
            regs.fZ = regs.A16 = (sum & 0xFFFF);
        }
    }
}

void AND(u16 v)
{
    if(regs.fM)
    {
        regs.fZ = regs.A8 &= v;
        regs.fN = (regs.A8 & 0x80);
    }
    else
    {
        regs.fZ = regs.A16 &= v;
        regs.fN = (regs.A16 & 0x8000);
    }
}

u16 ASL(u16 v, bool flg)
{
    if(flg)
    {
        regs.fC = (v & 0x80);
        regs.fN = (v & 0x40);
        regs.fZ = v = (v << 1) & 0xFF;
    }
    else
    {
        regs.fC = (v & 0x8000);
        regs.fN = (v & 0x4000);
        regs.fZ = v = (v << 1) & 0xFFFF;
    }
    return v;
}

void BIT(u16 v, bool update_nv)
{
    if(regs.fM)
    {
        if(update_nv)
        {
            regs.fN = v & 0x80;
            regs.fV = v & 0x40;
        }
        regs.fZ = regs.A8 & v;
    }
    else
    {
        if(update_nv)
        {
            regs.fN = v & 0x8000;
            regs.fV = v & 0x4000;
        }
        regs.fZ = regs.A16 & v;
    }
}

void CMP(u16 v)
{
    if(regs.fM)
    {
        regs.fC = (regs.A8 >= v);
        int dif = regs.A8 - v;
        regs.fZ = dif;
        regs.fN = dif & 0x80;
    }
    else
    {
        regs.fC = (regs.A16 >= v);
        int dif = regs.A16 - v;
        regs.fZ = dif;
        regs.fN = dif & 0x8000;
    }
}

void CMP_XY(u16 reg, u16 v)
{
    regs.fC = (reg >= v);
    int dif = reg - v;
    regs.fZ = dif;

    if(regs.fX)     regs.fN = (dif & 0x80);
    else            regs.fN = (dif & 0x8000);
}
void CPX(u16 v) { CMP_XY(regs.X, v);        }
void CPY(u16 v) { CMP_XY(regs.Y, v);        }

u16 DEC(u16 v, bool flg)
{
    if(flg)
    {
        regs.fZ = v = (v - 1) & 0xFF;
        regs.fN = v & 0x80;
    }
    else
    {
        regs.fZ = --v;
        regs.fN = v & 0x8000;
    }
    return v;
}

void EOR(u16 v)
{
    if(regs.fM)
    {
        regs.fZ = regs.A8 ^= v;
        regs.fN = (regs.A8 & 0x80);
    }
    else
    {
        regs.fZ = regs.A16 ^= v;
        regs.fN = (regs.A16 & 0x8000);
    }
}

u16 INC(u16 v, bool flg)
{
    if(flg)
    {
        regs.fZ = v = (v + 1) & 0xFF;
        regs.fN = v & 0x80;
    }
    else
    {
        regs.fZ = ++v;
        regs.fN = v & 0x8000;
    }
    return v;
}

void LDA(u16 v)
{
    if(regs.fM)
    {
        regs.fZ = regs.A8 = v & 0xFF;
        regs.fN = (regs.A8 & 0x80);
    }
    else
    {
        regs.fZ = regs.A16 = v;
        regs.fN = (regs.A16 & 0x8000);
    }
}

void LD_XY(u16& reg, u16 v)
{
    regs.fZ = reg = v;
    if(regs.fX)     regs.fN = v & 0x80;
    else            regs.fN = v & 0x8000;
}
void LDX(u16 v) { LD_XY(regs.X, v); }
void LDY(u16 v) { LD_XY(regs.Y, v); }

u16 LSR(u16 v, bool flg)
{
    regs.fC = (v & 0x01);
    regs.fN = 0;
    if(flg)         regs.fZ = v = (v >> 1) & 0x7F;
    else            regs.fZ = v = (v >> 1) & 0x7FFF;

    return v;
}

void ORA(u16 v)
{
    if(regs.fM)
    {
        regs.fZ = regs.A8 |= v;
        regs.fN = (regs.A8 & 0x80);
    }
    else
    {
        regs.fZ = regs.A16 |= v;
        regs.fN = (regs.A16 & 0x8000);
    }
}

u16 ROL(u16 v, bool flg)
{
    if(flg)
    {
        int newC = v & 0x80;
        regs.fZ = v = ((v << 1) | (regs.fC ? 0x01 : 0x00)) & 0xFF;
        regs.fN = v & 0x80;
        regs.fC = newC;
    }
    else
    {
        int newC = v & 0x8000;
        regs.fZ = v = ((v << 1) | (regs.fC ? 0x0001 : 0x0000)) & 0xFFFF;
        regs.fN = v & 0x8000;
        regs.fC = newC;
    }
    return v;
}

u16 ROR(u16 v, bool flg)
{
    regs.fN = regs.fC;
    regs.fC = v & 0x0001;
    if(flg)     v = (v >> 1) | (regs.fN ? 0x80 : 0x00);
    else        v = (v >> 1) | (regs.fN ? 0x8000 : 0x0000);
    regs.fZ = v;
    return v;
}

void SBC_decimal(u16 v)
{
    if(regs.fM)
    {
        int lo = (regs.A8 & 0x0F) - (v & 0x0F) - !regs.fC;
        int hi = (regs.A8 & 0xF0) - (v & 0xF0);

        if(lo < 0)      {   lo -= 0x06;     hi -= 0x10;     }
        if(hi < 0)      {   hi -= 0x06;                     }

        // Flags don't care about decimal mode?
        int dif = regs.A8 - v - !regs.fC;
        regs.fC = (dif >= 0);
        regs.fZ = (dif & 0xFF);
        regs.fN = (dif & 0x80);
        regs.fV = (regs.A8 ^ dif) & (regs.A8 ^ v) & 0x80;

        regs.A8 = (hi & 0xF0) | (lo & 0x0F);
    }
    else
    {
        int a = (regs.A16 & 0x000F) - (v & 0x000F) - !regs.fC;
        int b = (regs.A16 & 0x00F0) - (v & 0x00F0);
        int c = (regs.A16 & 0x0F00) - (v & 0x0F00);
        int d = (regs.A16 & 0xF000) - (v & 0xF000);
        
        if(a < 0)       {   a -= 0x0006;    b -= 0x0010;    }
        if(b < 0)       {   b -= 0x0060;    c -= 0x0100;    }
        if(c < 0)       {   c -= 0x0600;    d -= 0x1000;    }
        if(d < 0)       {   d -= 0x6000;                    }

        // Flags don't care about decimal mode?
        int dif = regs.A16 - v - !regs.fC;
        regs.fC = (dif >= 0);
        regs.fZ = (dif & 0xFFFF);
        regs.fN = (dif & 0x8000);
        regs.fV = (regs.A16 ^ dif) & (regs.A16 ^ v) & 0x8000;

        regs.A16 = (d & 0xF000) | (c & 0x0F00) | (b & 0x00F0) | (a & 0x000F);
    }
}

void SBC(u16 v)
{
    if( regs.fD )
        SBC_decimal(v);
    else
    {
        if(regs.fM)
        {
            int dif = regs.A8 - v - !regs.fC;
            regs.fV = (regs.A8 ^ dif) & (regs.A8 ^ v) & 0x80;
            regs.fC = (dif >= 0);
            regs.fN = (dif & 0x80);
            regs.fZ = regs.A8 = (dif & 0xFF);
        }
        else
        {
            int dif = regs.A16 - v - !regs.fC;
            regs.fV = (regs.A16 ^ dif) & (regs.A16 ^ v) & 0x8000;
            regs.fC = (dif >= 0);
            regs.fN = (dif & 0x8000);
            regs.fZ = regs.A16 = (dif & 0xFFFF);
        }
    }
}

u16 STA()
{
    return regs.fM ? regs.A8 : regs.A16;
}

u16 TA_XY()
{
    u16 out;
    if(regs.fX)
    {
        if(regs.fM)     out = regs.B8 | regs.A8;
        else            out = regs.A16;
        regs.fN = out & 0x8000;
    }
    else
    {
        if(regs.fM)     out = regs.A8;
        else            out = regs.A16 & 0x00FF;
        regs.fN = out & 0x80;
    }
    regs.fZ = out;
    return out;
}
void    TAX()   { regs.X = TA_XY();     }
void    TAY()   { regs.Y = TA_XY();     }

u16 T_XY(u16 src)
{
    if(regs.fX)     regs.fN = src & 0x80;
    else            regs.fN = src & 0x8000;
    regs.fZ = src;
    return src;
}
void    TXY()   { regs.Y = T_XY(regs.X);        }
void    TYX()   { regs.X = T_XY(regs.Y);        }

void T_XY_A(u16 src)
{
    if(regs.fM)
    {
        regs.fZ = regs.A8 = (src & 0xFF);
        regs.fN = (src & 0x80);
    }
    else
    {
        regs.fZ = regs.A16 = src;
        regs.fN = src & 0x8000;
    }
}
void    TXA()   { T_XY_A( regs.X );     }
void    TYA()   { T_XY_A( regs.Y );     }

void    TXS()   { regs.SP = regs.X;     }
void    TSX()
{
    if(regs.fX)
    {
        regs.X = regs.SP & 0x00FF;
        regs.fN = regs.X & 0x80;
    }
    else
    {
        regs.X = regs.SP;
        regs.fN = regs.X & 0x8000;
    }
    regs.fZ = regs.X;
}

void TCD()
{
    if(regs.fM)     regs.DP = regs.B8 | regs.A8;
    else            regs.DP = regs.A16;
    regs.fN = regs.DP & 0x8000;
    regs.fZ = regs.DP;
}

void TDC()
{
    if(regs.fM)   { regs.A8 = regs.DP & 0xFF;       regs.B8 = regs.DP & 0xFF00; }
    else            regs.A16 = regs.DP;
    regs.fN = regs.DP & 0x8000;
    regs.fZ = regs.DP;
}

void TCS()
{
    if(regs.fM)     regs.SP = regs.B8 | regs.A8;
    else            regs.SP = regs.A16;
}

void TSC()
{
    if(regs.fM)   { regs.A8 = regs.SP & 0xFF;       regs.B8 = regs.SP & 0xFF00; }
    else            regs.A16 = regs.SP;
    regs.fN = regs.SP & 0x8000;
    regs.fZ = regs.SP;
}

u16 TRB(u16 v, bool flg)
{
    if(flg)
    {
        regs.fZ = regs.A16 & v;
        v &= ~regs.A16;
    }
    else
    {
        regs.fZ = regs.A8 & v;
        v &= ~regs.A8;
    }
    return v;
}

u16 TSB(u16 v, bool flg)
{
    if(flg)
    {
        regs.fZ = regs.A16 & v;
        v |= regs.A16;
    }
    else
    {
        regs.fZ = regs.A8 & v;
        v |= regs.A8;
    }
    return v;
}

/////////////////////////////////////////////////////////
//  "Full"

void u_Branch(bool condition)
{
    s8 v =          static_cast<s8>( read_p() );
    if(condition)
    {
        u16 newpc =     regs.PC + v;        ioCyc();
        if(regs.fE && ( (newpc & 0xFF00) != (regs.PC & 0xFF00) ) )
            ioCyc();
        regs.PC = newpc;
    }
}

void u_BRK()
{
    //TODO
}

void u_BRL()
{
    u16 v =         read_p();
    v |=            read_p() << 8;
    regs.PC += v;   ioCyc();
}

void u_COP()
{
    //TODO
}

void u_JMP_Absolute()       /* JMP $aaaa    */
{
    u8 a =          read_p();
    regs.PC =       (read_p() << 8) | a;
}

void u_JMP_Long()           /* JMP $llllll  */
{
    u16 a =         read_p();
    a |=            read_p() << 8;
    regs.PBR =      read_p() << 16;
    regs.PC = a;
}

void u_JMP_Indirect()       /* JMP ($aaaa)  */
{
    u16 a =         read_p();
    a |=            read_p() << 8;
    regs.PC =       read_l(a++);
    regs.PC |=      read_l(a) << 8;
}

void u_JMP_IndirectX()      /* JMP ($aaaa,X)*/
{
    u16 a =         read_p();
    a |=            read_p() << 8;
    a += regs.X;    ioCyc();
    regs.PC =       read_l(a++);
    regs.PC |=      read_l(a) << 8;
}

void u_JMP_IndirectLong()   /* JMP [$aaaa]  */
{
    u16 a =         read_p();
    a |=            read_p() << 8;
    regs.PC =       read_l(a++);
    regs.PC |=      read_l(a++) << 8;
    regs.PBR =      read_l(a) << 16;
}

void u_JSR_Absolute()       /* JSR $aaaa    */
{
    u16 a =         read_p();
    a |=            read_p() << 8;
    --regs.PC;      ioCyc();
                    push( regs.PC >> 8 );
                    push( regs.PC & 0xFF );     regs.PC = a;
}

void u_JSR_Long()          /* JSR $llllll  */
{
    u16 a =         read_p();
    a |=            read_p() << 8;
                    push(regs.PBR >> 16);
                    ioCyc();
    regs.PBR =      read_p() << 16;             --regs.PC;
                    push( regs.PC >> 8 );
                    push( regs.PC & 0xFF );     regs.PC = a;
}

void u_JSR_IndirectX()      /* JSR ($aaaa,X)*/
{
    u16 a =         read_p();
                    push( regs.PC >> 8 );
                    push( regs.PC & 0xFF );     regs.PC = a;
    a |=            read_p() << 8;
    a += regs.X;    ioCyc();
    regs.PC =       read_l(a++);
    regs.PC |=      read_l(a) << 8;
}

void u_MV_PN(int adj)
{
    // MVP and MVN change DBR to match the destination bank
    //  dest byte is first in binary        OP DS SR
    //  dest byte is 2nd in disassembly     MVN src, dst
    //  MVP subtracts from X,Y, MVN adds
    //  both MVP/MVN subtract from A
    //  A is 16 bits REGARDLESS of M flag!!!!
}

void u_MVP()    { u_MV_PN(-1);      }
void u_MVN()    { u_MV_PN( 1);      }

void u_PEA()
{
    u8 a =          read_p();
    u8 b =          read_p();
                    push(b);
                    push(a);
}

void u_PEI()
{
    u16 a =         read_p();
    a += regs.DP;   dpCyc();
    u8 v1 =         read_l(a++);
    u8 v2 =         read_l(a);
                    push(v2);
                    push(v1);
}

void u_PER()
{
    u16 a =         read_p();
    a |=            read_p() << 8;
    a += regs.PC;   ioCyc();
                    push(a >> 8);
                    push(a & 0xFF);
}

void u_PHA()
{
    if(regs.fM)     ad_push(regs.A8 , regs.fM);
    else            ad_push(regs.A16, regs.fM);
}

void u_PLA()
{
    if(regs.fM)     regs.A8  = ad_pull(regs.fM) & 0xFF;
    else            regs.A16 = ad_pull(regs.fM);
}

void u_REP()
{
    u8 v =          read_p();
    ioCyc();
    
    regs.setStatusByte( regs.getStatusByte(true) & ~v );
}

void u_RTI()
{
                            ioCyc(2);
    regs.setStatusByte( pull() );       // TODO - repredict IRQ
    regs.PC =               pull();
    regs.PC |=              pull() << 8;
    if(!regs.fE) regs.PBR = pull() << 16;
}

void u_RTS()
{
                            ioCyc(2);
    regs.PC =               pull();
    regs.PC |=              pull() << 8;
    ++regs.PC;              ioCyc();
}

void u_RTL()
{
                            ioCyc(2);
    regs.PC =               pull();
    regs.PC |=              pull() << 8;            ++regs.PC;
    regs.PBR =              pull() << 16;
}

void u_SEP()
{
    u8 v =          read_p();
    ioCyc();
    
    regs.setStatusByte( regs.getStatusByte(true) | v );
}

void u_STP()
{
    // TODO
}

void u_WAI()
{
    // TODO
}

void u_XBA()
{
    ioCyc(2);
    if(regs.fM)
    {
        u8 tmp = (regs.B8 >> 8);
        regs.B8 = regs.A8 << 8;
        regs.fZ = regs.A8 = tmp;
        regs.fN = regs.A8 & 0x80;
    }
    else
    {
        regs.A16 = (regs.A16 << 8) | (regs.A16 >> 8);
        regs.fZ = regs.A16 & 0xFF;
        regs.fN = regs.A16 & 0x80;
    }
}

void u_XCE()
{
    ioCyc();
    if(!regs.fC != !regs.fE)        // they are changing!
    {
        if(!regs.fM)                // going to an 8-bit accumulator
        {
            regs.A8 = regs.A16 & 0x00FF;
            regs.B8 = regs.A16 & 0xFF00;
        }
        regs.X &= 0x00FF;
        regs.Y &= 0x00FF;
        regs.fE = !regs.fE;
        regs.fC = !regs.fC;
        regs.fX = true;
        regs.fM = true;
    }
}

        
#ifdef HACK_TO_GET_INTELLISENSE_TO_WORK
}
#endif
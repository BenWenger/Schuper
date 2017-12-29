
//#define HACK_TO_GET_INTELLISENSE_TO_WORK

#ifdef HACK_TO_GET_INTELLISENSE_TO_WORK
    #error Disable this hack!
    #include "smpregs.h"
    namespace sch {
    SmpRegs     regs;
    
        u8              read(u16 a);            // standard read
        u8              dpRd(u8 a);             // direct page read
        u8              pull();                 // simple pull (single read off stack)
        void            write(u16 a, u8 v);     // standard write
        void            dpWr(u8 a, u8 v);       // direct page write
        void            push(u8 v);             // simple push (single write to stack)
        void            ioCyc();                // I/O cycle (for internal operations that take a cycle)    
        void            ioCyc(int cycs);        // I/O cycle (multiple)

        #define BRANCH(condition, rl)
#endif
    
///////////////////////////////////////////////
//  Instructions that use common addressing modes

void ADC(u8& dst, u8 src)
{
    regs.fH = ((dst & 0x0F) + (src & 0x0F) + !!regs.fC) & 0x10;     // Half-carry flag is stupid

    int sum = dst + src + !!regs.fC;
    regs.fC = (sum > 0xFF);
    regs.fV = (sum ^ dst) & (sum ^ src) & 0x80;
    regs.fNZ = dst = static_cast<u8>(sum);
}

void ADDW(u16& src)
{
    u16 ya = (regs.Y << 8) | regs.A;

    regs.fH = ((ya & 0x0FFF) + (src & 0x0FFF)) & 0x1000; // Half-carry flag is still stupid

    int sum = ya + src;
    regs.fC = (sum > 0xFFFF);
    regs.fV = (sum ^ ya) & (sum ^ src) & 0x8000;
    regs.fNZ = regs.Y = static_cast<u8>(sum >> 8);
    regs.A = static_cast<u8>(sum);

    if(regs.A)      regs.fNZ |= 1;
}

void AND(u8& dst, u8 src)
{
    regs.fNZ = dst &= src;
}

void AND1(u8& val, u8 bit)
{
    if(regs.fC && (val & bit))      regs.fC = 1;
    else                            regs.fC = 0;
}

void AND1_inv(u8& val, u8 bit)
{
    if(regs.fC && (~val & bit))     regs.fC = 1;
    else                            regs.fC = 0;
}

void ASL(u8& val)
{
    regs.fC = (val & 0x80);
    regs.fNZ = (val <<= 1);
}

template <u8 bit> bool BBC(u8 v)
{
    return (v & bit) == 0;
}

template <u8 bit> bool BBS(u8 v)
{
    return (v & bit) != 0;
}

bool CBNE(u8 v)
{
    return regs.A != v;
}

template <u8 bit> void CLR1(u8& v)
{
    v &= ~bit;
}

void CMP(u8& dst, u8 src)
{
    int diff = dst - src;
    regs.fC = (diff >= 0);
    regs.fNZ = static_cast<u8>( diff );
}

void CMPW(u16& v)
{
    int ya = (regs.Y << 8) | regs.A;

    int diff = ya - v;
    regs.fC = (diff >= 0);
    regs.fNZ = static_cast<u8>( diff >> 8 );

    if(diff & 0xFF)
        regs.fNZ |= 1;
}

void DEC(u8& val)
{
    regs.fNZ = (--val);
}

void EOR(u8& dst, u8 src)
{
    regs.fNZ = (dst ^= src);
}

void EOR1(u8& val, u8 bit)
{
    if(val & bit)
        regs.fC = !regs.fC;
}

void INC(u8& val)
{
    regs.fNZ = (++val);
}

void LSR(u8& val)
{
    regs.fC = (val & 0x01);
    regs.fNZ = (val >>= 1);
}

void MOV(u8& dst, u8 src)
{
    regs.fNZ = dst = src;
}

void MOVx(u8& dst, u8 src)      // MOV, but doesn't set any flags
{
    dst = src;
}

void MOV1(u8& val, u8 bit)      // modifies mem
{
    if(regs.fC)     val |= bit;
    else            val &= ~bit;
}

void MOV1_c(u8& val, u8 bit)    // modifies C
{
    regs.fC = (val & bit);
}

void MOVW(u16& src)             // modifies YA
{
    regs.fNZ = regs.Y = static_cast<u8>(src >> 8);
    regs.A = static_cast<u8>(src);

    if(regs.A)      regs.fNZ |= 1;
}

void NOT1(u8& val, u8 bit)
{
    val ^= bit;
}

void OR(u8& dst, u8 src)
{
    regs.fNZ = (dst |= src);
}

void OR1(u8& val, u8 bit)
{
    regs.fC |= (val & bit);
}

void OR1_inv(u8& val, u8 bit)
{
    regs.fC |= !(val & bit);
}

void ROL(u8& val)
{
    u8 oldC = regs.fC ? 0x01 : 0;
    regs.fC = val & 0x80;
    regs.fNZ = val = (val << 1) | oldC;
}

void ROR(u8& val)
{
    u8 oldC = regs.fC ? 0x80 : 0;
    regs.fC = val & 0x01;
    regs.fNZ = val = (val >> 1) | oldC;
}

void SBC(u8& dst, u8 src)
{
    ADC(dst, ~src);
}

template <u8 bit> void SET1(u8& v)
{
    v |= bit;
}

void SUBW(u16& src)
{
    u16 ya = (regs.Y << 8) | regs.A;

    regs.fH = ((ya & 0x0FFF) - (src & 0x0FFF)) >= 0; // Half-carry flag is still stupid

    int sum = ya - src;
    regs.fC = (sum >= 0);
    regs.fV = (sum ^ ya) & (ya ^ src) & 0x8000;
    regs.fNZ = regs.Y = static_cast<u8>(sum >> 8);
    regs.A = static_cast<u8>(sum);

    if(regs.A)      regs.fNZ |= 1;
}

void TCLR1(u8& val)
{
    regs.fNZ = (regs.A - val) & 0xFF;
    val &= ~regs.A;
}

void TSET1(u8& val)
{
    regs.fNZ = (regs.A - val) & 0xFF;
    val |= regs.A;
}
    
///////////////////////////////////////////////
//  'Full' Instructions that have their own addressing mode (not bundled with an ad_xx_xx call)

void op_BRK()           // 8 cycles
{
    push( static_cast<u8>( regs.PC >> 8 ) );
    push( static_cast<u8>( regs.PC      ) );
    push( regs.getStatusByte()            );
    regs.PC = read( 0xFFDE );
    regs.PC |= read( 0xFFDF ) << 8;
    ioCyc(2);
    regs.fB = 1;
    regs.fI = 0;
}

void op_CALL()          // 8 cycles
{
    u16 addr = read(regs.PC++);
    addr |= read(regs.PC++) << 8;
    push( static_cast<u8>( regs.PC >> 8 ) );
    push( static_cast<u8>( regs.PC      ) );
    ioCyc(3);           regs.PC = addr;
}

void op_DAA()           // 3 cycles
{
    if(regs.fC || (regs.A > 0x99)) {
        regs.A += 0x60;
        regs.fC = 1;
    }
    if(regs.fH || ((regs.A & 0x0F) > 0x09)) {
        regs.A += 0x06;
    }
    regs.fNZ = regs.A;
    ioCyc(2);
}

void op_DAS()           // 3 cycles
{
    if(!regs.fC || (regs.A > 0x99)) {
        regs.A -= 0x60;
        regs.fC = 0;
    }
    if(!regs.fH || ((regs.A & 0x0F) > 0x09)) {
        regs.A -= 0x06;
    }
    regs.fNZ = regs.A;
    ioCyc(2);
}

void op_DBNZ()          // 4 cycles (+2 if branch)
{
    u8 rl = read(regs.PC++);
    ioCyc(2);
    --regs.Y;
    BRANCH(regs.Y != 0, rl);
}

void op_DBNZ_mem()      // 5 cycles (+2 if branch)
{
    u8 addr = read(regs.PC++);
    u8 val = dpRd(addr);
    --val;
    dpWr(addr, val);
    u8 rl = read(regs.PC++);
    BRANCH(val != 0, rl);
}

void op_DECW()          // 6 cycles
{
    u8 addr = read(regs.PC++);
    u8 lo = dpRd(addr);         --lo;
    dpWr(addr++,lo);
    u8 hi = dpRd(addr);         if(lo == 0xFF)  --hi;
    dpWr(addr, hi);

    regs.fNZ = hi;
    if(lo)      regs.fNZ |= 1;
}

void op_DIV()           // 12 cycles
{
    int ya = (regs.Y << 8) | regs.A;
    regs.fV = (regs.Y >= regs.X);
    regs.fH = ( (regs.Y & 0x0F) >= (regs.X & 0x0F) );

    if(regs.Y < (regs.X << 1))      // quotient is <= 511 (will fit into 9-bit result)
    {
        regs.A = ya / regs.X;
        regs.Y = ya % regs.X;
    }
    else                            // weird "too big" behavior"
    {
        regs.A = 255    - (  (ya - (regs.X << 9)) / (256 - regs.X)  );
        regs.A = regs.X - (  (ya - (regs.X << 9)) % (256 - regs.X)  );
    }

    regs.fNZ = regs.A;
    ioCyc(11);
}

void op_INCW()          // 6 cycles
{
    u8 addr = read(regs.PC++);
    u8 lo = dpRd(addr);         ++lo;
    dpWr(addr++,lo);
    u8 hi = dpRd(addr);         if(lo == 0x00)  ++hi;
    dpWr(addr, hi);

    regs.fNZ = hi;
    if(lo)      regs.fNZ |= 1;
}

void op_JMP()           // 3 cycles
{
    u8 addr = read(regs.PC++);
    regs.PC = addr | (read(regs.PC) << 8);
}

void op_JMP_X()         // 6 cycles
{
    u16 addr = read(regs.PC++);
    addr |= read(regs.PC) << 8;
    addr += regs.X;         ioCyc();
    regs.PC = read(addr++);             // TODO:  can this cross a page???
    regs.PC |= read(addr) << 8;
}

void op_MOVW_write()         // 5 cycles
{
    u8 addr = read(regs.PC++);
    dpRd(addr);
    dpWr(addr, regs.A);
    dpWr(addr+1, regs.Y);
}

void op_MUL()           // 9 cycles
{
    u16 result = regs.Y * regs.A;
    regs.fNZ = regs.Y = static_cast<u8>( result >> 8 );
    regs.A = static_cast<u8>( result );
}

void op_PCALL()         // 6 cycles
{
    u8 addr = read(regs.PC++);
    push( static_cast<u8>( regs.PC >> 8 ) );
    push( static_cast<u8>( regs.PC      ) );
    ioCyc(2);
    regs.PC = 0xFF00 | addr;
}

void op_RET()           // 5 cycles
{
    regs.PC = pull();
    regs.PC |= pull() << 8;
    ioCyc(2);
}

void op_RET1()          // 6 cycles
{
    regs.setStatusByte( pull() );
    regs.PC = pull();
    regs.PC |= pull() << 8;
    ioCyc(2);
}

void op_TCALL(u8 lo)   // 8 cycles
{
    push( static_cast<u8>( regs.PC >> 8 ) );
    push( static_cast<u8>( regs.PC      ) );
    regs.PC = read(0xFF00 | lo);        ++lo;
    regs.PC |= read(0xFF00 | lo) << 8;
    ioCyc(3);
}

void op_XCN()           // 5 cycles
{
    regs.fNZ = regs.A = (regs.A >> 4) | (regs.A << 4);
    ioCyc(4);
}
 

#ifdef HACK_TO_GET_INTELLISENSE_TO_WORK
    }
#endif
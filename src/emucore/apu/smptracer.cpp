
#include <algorithm>
#include <cstring>
#include "smptracer.h"
#include "smpregs.h"
#include "spcbus.h"

namespace sch
{
    SmpTracer::SmpTracer()
    {
        traceFile = nullptr;
    }

    SmpTracer::~SmpTracer()
    {
        closeTraceFile();
    }

    void SmpTracer::closeTraceFile()
    {
        if(traceFile)
        {
            fclose(traceFile);
            traceFile = nullptr;
        }
    }

    void SmpTracer::openTraceFile(const char* filename)
    {
        closeTraceFile();

        traceFile = fopen( filename, "wt" );
    }

    namespace
    {
        ////////////////////////////////////////////////////////////////////////////////////
        //  Instruction names
const char* const op_names[0x100] = {
/*         x0       x1      x2      x3        x4      x5      x6      x7          x8      x9      xA      xB        xC      xD      xE      xF           */
/* 0x */ "NOP  ","TCALL","SET1 ","BBS  ",  "OR   ","OR   ","OR   ","OR   ",    "OR   ","OR   ","OR1  ","ASL  ",  "ASL  ","PUSH ","TSET1","BRK  ",  /* 0x */
/* 1x */ "BPL  ","TCALL","CLR1 ","BBC  ",  "OR   ","OR   ","OR   ","OR   ",    "OR   ","OR   ","DECW ","ASL  ",  "ASL  ","DEC  ","CMP  ","JMP  ",  /* 1x */
/* 2x */ "CLRP ","TCALL","SET1 ","BBS  ",  "AND  ","AND  ","AND  ","AND  ",    "AND  ","AND  ","OR1  ","ROL  ",  "ROL  ","PUSH ","CBNE ","BRA  ",  /* 2x */
/* 3x */ "BMI  ","TCALL","CLR1 ","BBC  ",  "AND  ","AND  ","AND  ","AND  ",    "AND  ","AND  ","INCW ","ROL  ",  "ROL  ","INC  ","CMP  ","CALL ",  /* 3x */
/* 4x */ "SETP ","TCALL","SET1 ","BBS  ",  "EOR  ","EOR  ","EOR  ","EOR  ",    "EOR  ","EOR  ","AND1 ","LSR  ",  "LSR  ","PUSH ","TCLR1","PCALL",  /* 4x */
/* 5x */ "BVC  ","TCALL","CLR1 ","BBC  ",  "EOR  ","EOR  ","EOR  ","EOR  ",    "EOR  ","EOR  ","CMPW ","LSR  ",  "LSR  ","MOV  ","CMP  ","JMP  ",  /* 5x */
/* 6x */ "CLRC ","TCALL","SET1 ","BBS  ",  "CMP  ","CMP  ","CMP  ","CMP  ",    "CMP  ","CMP  ","AND1 ","ROR  ",  "ROR  ","PUSH ","DBNZ ","RET  ",  /* 6x */
/* 7x */ "BVS  ","TCALL","CLR1 ","BBC  ",  "CMP  ","CMP  ","CMP  ","CMP  ",    "CMP  ","CMP  ","ADDW ","ROR  ",  "ROR  ","MOV  ","CMP  ","RET1 ",  /* 7x */
/*         x0       x1      x2      x3        x4      x5      x6      x7          x8      x9      xA      xB        xC      xD      xE      xF           */
/* 8x */ "SETC ","TCALL","SET1 ","BBS  ",  "ADC  ","ADC  ","ADC  ","ADC  ",    "ADC  ","ADC  ","EOR1 ","DEC  ",  "DEC  ","MOV  ","POP  ","MOV  ",  /* 8x */
/* 9x */ "BCC  ","TCALL","CLR1 ","BBC  ",  "ADC  ","ADC  ","ADC  ","ADC  ",    "ADC  ","ADC  ","SUBW ","DEC  ",  "DEC  ","MOV  ","DIV  ","XCN  ",  /* 9x */
/* Ax */ "EI   ","TCALL","SET1 ","BBS  ",  "SBC  ","SBC  ","SBC  ","SBC  ",    "SBC  ","SBC  ","MOV1 ","INC  ",  "INC  ","CMP  ","POP  ","MOV  ",  /* Ax */
/* Bx */ "BCS  ","TCALL","CLR1 ","BBC  ",  "SBC  ","SBC  ","SBC  ","SBC  ",    "SBC  ","SBC  ","MOVW ","INC  ",  "INC  ","MOV  ","DAS  ","MOV  ",  /* Bx */
/* Cx */ "DI   ","TCALL","SET1 ","BBS  ",  "MOV  ","MOV  ","MOV  ","MOV  ",    "CMP  ","MOV  ","MOV1 ","MOV  ",  "MOV  ","MOV  ","POP  ","MUL  ",  /* Cx */
/* Dx */ "BNE  ","TCALL","CLR1 ","BBC  ",  "MOV  ","MOV  ","MOV  ","MOV  ",    "MOV  ","MOV  ","MOVW ","MOV  ",  "DEC  ","MOV  ","CBNE ","DAA  ",  /* Dx */
/* Ex */ "CLRV ","TCALL","SET1 ","BBS  ",  "MOV  ","MOV  ","MOV  ","MOV  ",    "MOV  ","MOV  ","NOT1 ","MOV  ",  "MOV  ","NOTC ","POP  ","SLEEP",  /* Ex */
/* Fx */ "BEQ  ","TCALL","CLR1 ","BBC  ",  "MOV  ","MOV  ","MOV  ","MOV  ",    "MOV  ","MOV  ","MOV  ","MOV  ",  "INC  ","MOV  ","DBNZ ","STOP "   /* Fx */
/*         x0       x1      x2      x3        x4      x5      x6      x7          x8      x9      xA      xB        xC      xD      xE      xF           */
};
        ////////////////////////////////////////////////////////////////////////////////////
        // Addressing modes (not the full mode, just one argument)
        enum AddrArg
        {
            // 0 byte args
            a_ip=0, // implied / no argument
            a_ac,   // A
            a_xx,   // X
            a_yy,   // Y
            a_sp,   // SP
            a_sw,   // PSW
            a_cc,   // C
            a_ya,   // YA
            a_xi,   // (X)
            a_xp,   // (X)+
            a_yi,   // (Y)
            a_tc,   // tcall

            // 1 byte args
            a_im,   // #i
            a_dp,   // d
            a_dw,   // d                (direct page, but 2 bytes at given address.. ie, ADDW)
            a_bt,   // d.#
            a_dx,   // d+X
            a_dy,   // d+Y
            a_ix,   // [d+X]
            a_iy,   // [d]+Y
            a_rl,   // relative
            a_pc,   // pcall

            // 2 byte args
            a_ab,   // !a
            a_ax,   // !a+X
            a_ay,   // !a+Y
            a_mb,   // m.b
            a_nb,   // /m.b
            a_ja,   // !a               (for JMP/CALL)
            a_jx,   // [!a+X]           (for JMP)

            mode_count
        };

        const int md_bytes[mode_count] = {
            0,0,0,0,0,0,0,0,0,0,0,0,
            1,1,1,1,1,1,1,1,1,1,
            2,2,2,2,2,2,2
        };

//  The left argument
const AddrArg md_left[0x100] = {
/*         x0   x1   x2   x3     x4   x5   x6   x7        x8   x9   xA   xB     xC   xD   xE   xF          */
/* 0x */  a_ip,a_tc,a_bt,a_bt,  a_ac,a_ac,a_ac,a_ac,     a_ac,a_dp,a_cc,a_dp,  a_ac,a_sw,a_ab,a_ip,  /* 0x */
/* 1x */  a_rl,a_tc,a_bt,a_bt,  a_ac,a_ac,a_ac,a_ac,     a_dp,a_xi,a_dw,a_dx,  a_ac,a_xx,a_xx,a_jx,  /* 1x */
/* 2x */  a_ip,a_tc,a_bt,a_bt,  a_ac,a_ac,a_ac,a_ac,     a_ac,a_dp,a_cc,a_dp,  a_ab,a_ac,a_dp,a_rl,  /* 2x */
/* 3x */  a_rl,a_tc,a_bt,a_bt,  a_ac,a_ac,a_ac,a_ac,     a_dp,a_xi,a_dw,a_dx,  a_ac,a_xx,a_xx,a_ja,  /* 3x */
/* 4x */  a_ip,a_tc,a_bt,a_bt,  a_ac,a_ac,a_ac,a_ac,     a_ac,a_dp,a_cc,a_dp,  a_ab,a_xx,a_ab,a_pc,  /* 4x */
/* 5x */  a_rl,a_tc,a_bt,a_bt,  a_ac,a_ac,a_ac,a_ac,     a_dp,a_xi,a_ya,a_dx,  a_ac,a_xx,a_yy,a_ja,  /* 5x */
/* 6x */  a_ip,a_tc,a_bt,a_bt,  a_ac,a_ac,a_ac,a_ac,     a_ac,a_dp,a_cc,a_dp,  a_ab,a_yy,a_dp,a_ip,  /* 6x */
/* 7x */  a_rl,a_tc,a_bt,a_bt,  a_ac,a_ac,a_ac,a_ac,     a_dp,a_xi,a_ya,a_dx,  a_ac,a_ac,a_yy,a_ip,  /* 7x */
/*         x0   x1   x2   x3     x4   x5   x6   x7        x8   x9   xA   xB     xC   xD   xE   xF          */
/* 8x */  a_ip,a_tc,a_bt,a_bt,  a_ac,a_ac,a_ac,a_ac,     a_ac,a_dp,a_cc,a_dp,  a_ab,a_yy,a_sw,a_dp,  /* 8x */
/* 9x */  a_rl,a_tc,a_bt,a_bt,  a_ac,a_ac,a_ac,a_ac,     a_dp,a_xi,a_ya,a_dx,  a_ac,a_xx,a_ya,a_ac,  /* 9x */
/* Ax */  a_ip,a_tc,a_bt,a_bt,  a_ac,a_ac,a_ac,a_ac,     a_ac,a_dp,a_cc,a_dp,  a_ab,a_yy,a_ac,a_xp,  /* Ax */
/* Bx */  a_rl,a_tc,a_bt,a_bt,  a_ac,a_ac,a_ac,a_ac,     a_dp,a_xi,a_ya,a_dx,  a_ac,a_sp,a_ac,a_ac,  /* Bx */
/* Cx */  a_ip,a_tc,a_bt,a_bt,  a_dp,a_ab,a_xi,a_ix,     a_xx,a_ab,a_mb,a_dp,  a_ab,a_xx,a_xx,a_ya,  /* Cx */
/* Dx */  a_rl,a_tc,a_bt,a_bt,  a_dx,a_ax,a_ay,a_iy,     a_dp,a_dy,a_dw,a_dx,  a_yy,a_ac,a_dx,a_ac,  /* Dx */
/* Ex */  a_ip,a_tc,a_bt,a_bt,  a_ac,a_ac,a_ac,a_ac,     a_ac,a_xx,a_mb,a_yy,  a_yy,a_ip,a_yy,a_ip,  /* Ex */
/* Fx */  a_rl,a_tc,a_bt,a_bt,  a_ac,a_ac,a_ac,a_ac,     a_xx,a_xx,a_dp,a_yy,  a_yy,a_yy,a_yy,a_ip,  /* Fx */
/*         x0   x1   x2   x3     x4   x5   x6   x7        x8   x9   xA   xB     xC   xD   xE   xF          */
};

//  The right argument
const AddrArg md_right[0x100] = {
/*         x0   x1   x2   x3     x4   x5   x6   x7        x8   x9   xA   xB     xC   xD   xE   xF          */
/* 0x */  a_ip,a_ip,a_ip,a_rl,  a_dp,a_ab,a_xi,a_ix,     a_im,a_dp,a_mb,a_ip,  a_ip,a_ip,a_ip,a_ip,  /* 0x */
/* 1x */  a_ip,a_ip,a_ip,a_rl,  a_dx,a_ax,a_ay,a_iy,     a_im,a_yi,a_ip,a_ip,  a_ip,a_ip,a_ab,a_ip,  /* 1x */
/* 2x */  a_ip,a_ip,a_ip,a_rl,  a_dp,a_ab,a_xi,a_ix,     a_im,a_dp,a_nb,a_ip,  a_ip,a_ip,a_rl,a_ip,  /* 2x */
/* 3x */  a_ip,a_ip,a_ip,a_rl,  a_dx,a_ax,a_ay,a_iy,     a_im,a_yi,a_ip,a_ip,  a_ip,a_ip,a_dp,a_ip,  /* 3x */
/* 4x */  a_ip,a_ip,a_ip,a_rl,  a_dp,a_ab,a_xi,a_ix,     a_im,a_dp,a_mb,a_ip,  a_ip,a_ip,a_ip,a_ip,  /* 4x */
/* 5x */  a_ip,a_ip,a_ip,a_rl,  a_dx,a_ax,a_ay,a_iy,     a_im,a_yi,a_dw,a_ip,  a_ip,a_ac,a_ab,a_ip,  /* 5x */
/* 6x */  a_ip,a_ip,a_ip,a_rl,  a_dp,a_ab,a_xi,a_ix,     a_im,a_dp,a_nb,a_ip,  a_ip,a_ip,a_rl,a_ip,  /* 6x */
/* 7x */  a_ip,a_ip,a_ip,a_rl,  a_dx,a_ax,a_ay,a_iy,     a_im,a_yi,a_dw,a_ip,  a_ip,a_xx,a_dp,a_ip,  /* 7x */
/*         x0   x1   x2   x3     x4   x5   x6   x7        x8   x9   xA   xB     xC   xD   xE   xF          */
/* 8x */  a_ip,a_ip,a_ip,a_rl,  a_dp,a_ab,a_xi,a_ix,     a_im,a_dp,a_mb,a_ip,  a_ip,a_im,a_ip,a_im,  /* 8x */
/* 9x */  a_ip,a_ip,a_ip,a_rl,  a_dx,a_ax,a_ay,a_iy,     a_im,a_yi,a_dw,a_ip,  a_ip,a_sp,a_xx,a_ip,  /* 9x */
/* Ax */  a_ip,a_ip,a_ip,a_rl,  a_dp,a_ab,a_xi,a_ix,     a_im,a_dp,a_mb,a_ip,  a_ip,a_im,a_ip,a_ac,  /* Ax */
/* Bx */  a_ip,a_ip,a_ip,a_rl,  a_dx,a_ax,a_ay,a_iy,     a_im,a_yi,a_dw,a_ip,  a_ip,a_xx,a_ip,a_xp,  /* Bx */
/* Cx */  a_ip,a_ip,a_ip,a_rl,  a_ac,a_ac,a_ac,a_ac,     a_im,a_xx,a_cc,a_yy,  a_yy,a_im,a_ip,a_ip,  /* Cx */
/* Dx */  a_ip,a_ip,a_ip,a_rl,  a_ac,a_ac,a_ac,a_ac,     a_xx,a_xx,a_ya,a_yy,  a_ip,a_yy,a_rl,a_ip,  /* Dx */
/* Ex */  a_ip,a_ip,a_ip,a_rl,  a_dp,a_ab,a_xi,a_ix,     a_im,a_ab,a_ip,a_dp,  a_ab,a_ip,a_ip,a_ip,  /* Ex */
/* Fx */  a_ip,a_ip,a_ip,a_rl,  a_dx,a_ax,a_ay,a_iy,     a_dp,a_dy,a_dp,a_dx,  a_ip,a_ac,a_rl,a_ip,  /* Fx */
/*         x0   x1   x2   x3     x4   x5   x6   x7        x8   x9   xA   xB     xC   xD   xE   xF          */
};

        inline u16 getValFromOpBytes( AddrArg mode, const u8* bytes )
        {
            switch(md_bytes[mode])
            {
            case 1:     return bytes[0];
            case 2:     return bytes[0] | (bytes[1] << 8);
            }
            return 0;
        }

        void doParam(char* buffer, u16 newpc, const SmpRegs& regs, const SpcBus& bus, u8 op, u16 val, AddrArg mode, u16& targetaddr, int& targetsize, bool is_right)
        {
            if(is_right && mode != a_ip)
            {
                strcpy(buffer, ", ");
                buffer += 2;
            }

            switch(mode)
            {
                // 0 byte arguments
            case a_ip:                                  break;
            case a_ac:  strcpy(buffer, "A");            break;
            case a_xx:  strcpy(buffer, "X");            break;
            case a_yy:  strcpy(buffer, "Y");            break;
            case a_sp:  strcpy(buffer, "SP");           break;
            case a_sw:  strcpy(buffer, "PSW");          break;
            case a_cc:  strcpy(buffer, "C");            break;
            case a_ya:  strcpy(buffer, "YA");           break;
            case a_xi:  strcpy(buffer, "(X)");
                        targetaddr = regs.DP | regs.X;
                        targetsize = 1;
                        break;
            case a_xp:  strcpy(buffer, "(X)+");
                        targetaddr = regs.DP | regs.X;
                        targetsize = 1;
                        break;
            case a_yi:  strcpy(buffer, "(Y)");
                        targetaddr = regs.DP | regs.Y;
                        targetsize = 1;
                        break;
            case a_tc:  sprintf(buffer, "%d", (op>>4));
                        targetaddr = 0xFFDE - 2*(op>>4);
                        targetsize = 2;
                        break;

                // 1 byte arguments
            case a_im:  sprintf(buffer, "#$%02X", val );
                        break;
            case a_dp:  sprintf(buffer, "$%02X", val );
                        targetaddr = regs.DP | val;
                        targetsize = 1;
                        break;
            case a_dw:  sprintf(buffer, "$%02X", val );
                        targetaddr = regs.DP | val;
                        targetsize = 2;
                        break;
            case a_bt:  sprintf(buffer, "$%02X.%d", val, (op>>5) );
                        targetaddr = regs.DP | val;
                        targetsize = 1;
                        break;
            case a_dx:  sprintf(buffer, "$%02X+X", val );
                        targetaddr = regs.DP | static_cast<u8>(val + regs.X);
                        targetsize = 1;
                        break;
            case a_dy:  sprintf(buffer, "$%02X+Y", val );
                        targetaddr = regs.DP | static_cast<u8>(val + regs.Y);
                        targetsize = 1;
                        break;
            case a_ix:  sprintf(buffer, "($%02X+X)", val );
                        targetaddr = regs.DP | static_cast<u8>(val + regs.X);
                        targetaddr = bus.peek(targetaddr) | (bus.peek(targetaddr+1) << 8);
                        targetsize = 1;
                        break;
            case a_iy:  sprintf(buffer, "($%02X)+Y", val );
                        targetaddr = regs.DP | static_cast<u8>(val);
                        targetaddr = bus.peek(targetaddr) | (bus.peek(targetaddr+1) << 8);
                        targetaddr += regs.Y;
                        targetsize = 1;
                        break;
            case a_rl:  sprintf(buffer, "$%04X", static_cast<u16>( newpc + (val ^ 0x80) - 0x80 ) );
                        break;
            case a_pc:  sprintf(buffer, "$FF%02X", val);
                        break;

                // 2 byte arguments
            case a_ab:  sprintf(buffer, "$%04X", val);
                        targetaddr = val;
                        targetsize = 1;
                        break;
            case a_ax:  sprintf(buffer, "$%04X+X", val);
                        targetaddr = val + regs.X;
                        targetsize = 1;
                        break;
            case a_ay:  sprintf(buffer, "$%04X+Y", val);
                        targetaddr = val + regs.Y;
                        targetsize = 1;
                        break;
            case a_mb:  sprintf(buffer, "$%04X.%d", (val & 0x1FFF), (val >> 13));
                        targetaddr = val;
                        targetsize = 1;
                        break;
            case a_nb:  sprintf(buffer, "/$%04X.%d", (val & 0x1FFF), (val >> 13));
                        targetaddr = val;
                        targetsize = 1;
                        break;
            case a_ja:  sprintf(buffer, "$%04X", val);
                        targetaddr = val;
                        targetsize = 0;
                        break;
            case a_jx:  sprintf(buffer, "($%04X+X)", val);
                        targetaddr = val + regs.X;
                        targetsize = 2;
                        break;
            }
        }

    }

    
    //////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////

    void SmpTracer::cpuTrace(const SmpRegs& regs, const SpcBus& bus)
    {
        if(!traceFile)          return;

        ///////////////////////////
        //  Some basic prepwork

        u8  opbytes[5] = {};            // should only be 3 max, but use 5 just to be safe
        
        opbytes[0] = bus.peek(regs.PC + 0);
        opbytes[1] = bus.peek(regs.PC + 1);
        opbytes[2] = bus.peek(regs.PC + 2);

        auto lmode = md_left[opbytes[0]];
        auto rmode = md_right[opbytes[0]];

        // r value is first
        u16 rval = getValFromOpBytes( rmode, opbytes + 1 );
        u16 lval = getValFromOpBytes( lmode, opbytes + 1 + md_bytes[rmode] );

        // unless the r value is relative, in which case it's second
        if(rmode == a_rl && md_bytes[lmode])
            std::swap(lval, rval);

        int bytes_to_print = 1 + md_bytes[lmode] + md_bytes[rmode];

        //////////////////////////
        //  Print the PC
        fprintf( traceFile, "%04X:  ", regs.PC );

        //  Print the op bytes
        switch(bytes_to_print)
        {
        case 3:     fprintf( traceFile, "%02X %02X %02X", opbytes[0], opbytes[1], opbytes[2] );     break;
        case 2:     fprintf( traceFile, "%02X %02X   ",   opbytes[0], opbytes[1]             );     break;
        default:    fprintf( traceFile, "%02X      ",     opbytes[0]                         );     break;
        }

        // instruction mnemonic
        fprintf( traceFile, "     %s  ", op_names[opbytes[0]] );

        // the left/right arguments
        char buffer[80] = "";
        u16 targetaddr = 0;
        int targetbytes = -1;
        u16 newpc = regs.PC + bytes_to_print;
        
        doParam( buffer,                  newpc, regs, bus, opbytes[0], lval, lmode, targetaddr, targetbytes, false );
        doParam( buffer + strlen(buffer), newpc, regs, bus, opbytes[0], rval, rmode, targetaddr, targetbytes, true );

        // pad out the buffer to a fixed width  (there's probably a better way to do this -- who cares)
        int i = (int)strlen(buffer);
        for(; i < 14; ++i)  buffer[i] = ' ';
        buffer[i] = 0;

        fprintf( traceFile, "%s", buffer );

        // print the target address
        switch(targetbytes)
        {
        case 2:     fprintf( traceFile, "[%04X=%02X%02X]", targetaddr, bus.peek(targetaddr+1), bus.peek(targetaddr) );  break;
        case 1:     fprintf( traceFile, "  [%04X=%02X]", targetaddr, bus.peek(targetaddr) );                            break;
        case 0:     fprintf( traceFile, "     [%04X]", targetaddr );                                                    break;
        default:    fprintf( traceFile, "           " );                                                                break;
        }

        // Misc regs!
        fprintf( traceFile, "  %02X %02X %02X  [%c%c%c%c%c%c%c%c]  %02X\n",
                regs.A,
                regs.X,
                regs.Y,
                ( (regs.fNZ & 0x180)    ? 'N' : '.' ),
                (  regs.fV              ? 'V' : '.' ),
                (  regs.DP              ? 'P' : '.' ),
                (  regs.fB              ? 'B' : '.' ),
                (  regs.fH              ? 'H' : '.' ),
                (  regs.fI              ? 'I' : '.' ),
                ( (regs.fNZ & 0x0FF)    ? '.' : 'Z' ),
                (  regs.fC              ? 'C' : '.' ),
                regs.SP
        );
    }

}

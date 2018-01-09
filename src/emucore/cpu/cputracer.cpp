
#include "cputracer.h"
#include "cpustate.h"
#include "bus/cpubus.h"

namespace sch
{

    CpuTracer::CpuTracer()
    {
        traceFile = nullptr;
    }

    CpuTracer::~CpuTracer()
    {
        closeTraceFile();
    }

    void CpuTracer::openTraceFile(const char* filename)
    {
        closeTraceFile();
        traceFile = fopen(filename, "wt");
    }

    void CpuTracer::closeTraceFile()
    {
        if(traceFile)
        {
            fclose(traceFile);
            traceFile = nullptr;
        }
    }

    namespace
    {

        enum AddrMode
        {
            impld, accum,                       // Implied / Accumulator

            im__m, im__x, im__8,                // immediate modes ('M' flag, 'X' flag, and 'always 8 bit regardless of flag')
            dp__m, dp__x,                       // Direct page
            dx__m, dx__x,                       // Direct, X
                   dy__x,                       // Direct, Y
            sr__m,                              // Stack Relative
            ab__m, ab__x,                       // Absolute
            ax__m, ax__x,                       // Absolute, X
            ay__m, ay__x,                       // Absolute, Y
            abl_m,                              // Absolute Long
            axl_m,                              // Absolute Long, X
            di__m,                              // DP Indirect
            dil_m,                              // DP Indirect Long
            ix__m,                              // Indirect, X
            iy__m,                              // Indirect, Y
            iyl_m,                              // Indirect, Y Long
            siy_m,                              // Stack Relative Indirect, Y
            
            brnch, jp_rl,                       // Branch, and relative long jump
            jp_ab, jp_al, jp_ix, jp_in, jp_il,  // A bunch of jumps

            mv_np, __pei, __per                 // One-off modes
        };

const AddrMode opmodes[0x100] = {       // LDA is where I left off
/*         x0    x1    x2    x3       x4    x5    x6    x7         x8    x9    xA    xB       xC    xD    xE    xF         */
/* 0x */ im__8,ix__m,im__8,sr__m,   dp__m,dp__m,dp__m,dil_m,     impld,im__m,accum,impld,   ab__m,ab__m,ab__m,abl_m, /* 0x */
/* 1x */ brnch,iy__m,di__m,siy_m,   dp__m,dx__m,dx__m,iyl_m,     impld,ay__m,accum,impld,   ab__m,ax__m,ax__m,axl_m, /* 1x */
/* 2x */ jp_ab,ix__m,jp_al,sr__m,   dp__m,dp__m,dp__m,dil_m,     impld,im__m,accum,impld,   ab__m,ab__m,ab__m,abl_m, /* 2x */
/* 3x */ brnch,iy__m,di__m,siy_m,   dx__m,dx__m,dx__m,iyl_m,     impld,ay__m,accum,impld,   ax__m,ax__m,ax__m,axl_m, /* 3x */
/* 4x */ impld,ix__m,impld,sr__m,   mv_np,dp__m,dp__m,dil_m,     impld,im__m,accum,impld,   jp_ab,ab__m,ab__m,abl_m, /* 4x */
/* 5x */ brnch,iy__m,di__m,siy_m,   mv_np,dx__m,dx__m,iyl_m,     impld,ay__m,impld,impld,   jp_al,ax__m,ax__m,axl_m, /* 5x */
/* 6x */ impld,ix__m,__per,sr__m,   dp__m,dp__m,dp__m,dil_m,     impld,im__m,accum,impld,   jp_in,ab__m,ab__m,abl_m, /* 6x */
/* 7x */ brnch,iy__m,di__m,siy_m,   dx__m,dx__m,dx__m,iyl_m,     impld,ay__m,impld,impld,   jp_ix,ax__m,ax__m,axl_m, /* 7x */
/*         x0    x1    x2    x3       x4    x5    x6    x7         x8    x9    xA    xB       xC    xD    xE    xF         */
/* 8x */ brnch,ix__m,jp_rl,sr__m,   dp__x,dp__m,dp__x,dil_m,     impld,im__m,impld,impld,   ab__x,ab__m,ab__x,abl_m, /* 8x */
/* 9x */ brnch,iy__m,di__m,siy_m,   dx__x,dx__m,dy__x,iyl_m,     impld,ay__m,impld,impld,   ab__m,ax__m,ax__m,axl_m, /* 9x */
/* Ax */ im__x,ix__m,im__x,sr__m,   dp__x,dp__m,dp__x,dil_m,     impld,im__m,impld,impld,   ab__x,ab__m,ab__x,abl_m, /* Ax */
/* Bx */ brnch,iy__m,di__m,siy_m,   dx__x,dx__m,dy__x,iyl_m,     impld,ay__m,impld,impld,   ax__x,ax__m,ay__x,axl_m, /* Bx */
/* Cx */ im__x,ix__m,im__8,sr__m,   dp__x,dp__m,dp__m,dil_m,     impld,im__m,impld,impld,   ab__x,ab__m,ab__m,abl_m, /* Cx */
/* Dx */ brnch,iy__m,di__m,siy_m,   __pei,dx__m,dx__m,iyl_m,     impld,ay__m,impld,impld,   jp_il,ax__m,ax__m,axl_m, /* Dx */
/* Ex */ im__x,ix__m,im__8,sr__m,   dp__x,dp__m,dp__m,dil_m,     impld,im__m,impld,impld,   ab__x,ab__m,ab__m,abl_m, /* Ex */
/* Fx */ brnch,iy__m,di__m,siy_m,   jp_ab,dx__m,dx__m,iyl_m,     impld,ay__m,impld,impld,   jp_ix,ax__m,ax__m,axl_m  /* Fx */
/*         x0    x1    x2    x3       x4    x5    x6    x7         x8    x9    xA    xB       xC    xD    xE    xF         */
};

const char* const opnames[0x100] = {
/*         x0    x1    x2    x3      x4    x5    x6    x7        x8    x9    xA    xB      xC    xD    xE    xF         */
/* 0x */ "BRK","ORA","COP","ORA",  "TSB","ORA","ASL","ORA",    "PHP","ORA","ASL","PHD",  "TSB","ORA","ASL","ORA", /* 0x */
/* 1x */ "BPL","ORA","ORA","ORA",  "TRB","ORA","ASL","ORA",    "CLC","ORA","INC","TCS",  "TRB","ORA","ASL","ORA", /* 1x */
/* 2x */ "JSR","AND","JSR","AND",  "BIT","AND","ROL","AND",    "PLP","AND","ROL","PLD",  "BIT","AND","ROL","AND", /* 2x */
/* 3x */ "BMI","AND","AND","AND",  "BIT","AND","ROL","AND",    "SEC","AND","DEC","TSC",  "BIT","AND","ROL","AND", /* 3x */
/* 4x */ "RTI","EOR","WDM","EOR",  "MVP","EOR","LSR","EOR",    "PHA","EOR","LSR","PHK",  "JMP","EOR","LSR","EOR", /* 4x */
/* 5x */ "BVC","EOR","EOR","EOR",  "MVN","EOR","LSR","EOR",    "CLI","EOR","PHY","TCD",  "JMP","EOR","LSR","EOR", /* 5x */
/* 6x */ "RTS","ADC","PER","ADC",  "STZ","ADC","ROR","ADC",    "PLA","ADC","ROR","RTL",  "JMP","ADC","ROR","ADC", /* 6x */
/* 7x */ "BVS","ADC","ADC","ADC",  "STZ","ADC","ROR","ADC",    "SEI","ADC","PLY","TDC",  "JMP","ADC","ROR","ADC", /* 7x */
/*         x0    x1    x2    x3      x4    x5    x6    x7        x8    x9    xA    xB      xC    xD    xE    xF         */
/* 8x */ "BRA","STA","BRL","STA",  "STY","STA","STX","STA",    "DEY","BIT","TXA","PHB",  "STY","STA","STX","STA", /* 8x */
/* 9x */ "BCC","STA","STA","STA",  "STY","STA","STX","STA",    "TYA","STA","TXS","TXY",  "STZ","STA","STZ","STA", /* 9x */
/* Ax */ "LDY","LDA","LDX","LDA",  "LDY","LDA","LDX","LDA",    "TAY","LDA","TAX","PLB",  "LDY","LDA","LDX","LDA", /* Ax */
/* Bx */ "BCS","LDA","LDA","LDA",  "LDY","LDA","LDX","LDA",    "CLV","LDA","TSX","TYX",  "LDY","LDA","LDX","LDA", /* Bx */
/* Cx */ "CPY","CMP","REP","CMP",  "CPY","CMP","DEC","CMP",    "INY","CMP","DEX","WAI",  "CPY","CMP","DEC","CMP", /* Cx */
/* Dx */ "BNE","CMP","CMP","CMP",  "PEI","CMP","DEC","CMP",    "CLD","CMP","PHX","STP",  "JMP","CMP","DEC","CMP", /* Dx */
/* Ex */ "CPX","SBC","SEP","SBC",  "CPX","SBC","INC","SBC",    "INX","SBC","NOP","XBA",  "CPX","SBC","INC","SBC", /* Ex */
/* Fx */ "BEQ","SBC","SBC","SBC",  "PEA","SBC","INC","SBC",    "SED","SBC","PLX","XCE",  "JSR","SBC","INC","SBC"  /* Fx */
/*         x0    x1    x2    x3      x4    x5    x6    x7        x8    x9    xA    xB      xC    xD    xE    xF         */
};

    int getOpBytes(const CpuState& regs, AddrMode mode)
    {
        switch(mode)
        {
            // Always 1 byte
        case impld: case accum:
            return 1;

            // Always 2 bytes
        case im__8:
        case dp__m: case dp__x:
        case dx__m: case dx__x:
                    case dy__x:
        case sr__m:
        case di__m:
        case dil_m:
        case ix__m:
        case iy__m:
        case iyl_m:
        case siy_m:
        case brnch:
        case __pei:
            return 2;
            
            // Always 3 bytes
        case ab__m: case ab__x:
        case ax__m: case ax__x:
        case ay__m: case ay__x:
        case jp_rl: case jp_ab: case jp_ix: case jp_in: case jp_il:
        case mv_np:
        case __per:
            return 3;

            // Always 4 bytes
        case abl_m:
        case axl_m:
        case jp_al:
            return 4;

            // Depends on flags
        case im__m:
            return (regs.fM ? 2 : 3);

        case im__x:
            return (regs.fX ? 2 : 3);
        }

        return 1;
    }


    }

    void CpuTracer::cpuTrace(const CpuState& regs, const CpuBus& bus)
    {
        if(!traceFile)          return;

        /////////////////////////
        //  Print PC
        fprintf(traceFile, "%02X:%04X:   ", (regs.PBR >> 16), regs.PC);

        //  Get the op bytes / argument
        u16 pc = regs.PC;
        u8 op = bus.peek(regs.PBR | pc++);
        auto mode = opmodes[op];
        fprintf(traceFile, "%02X ", op);
        
        int len = getOpBytes(regs, mode);
        u8 argbytes[3] = {0,0,0};
        for(int i = 1; i < len; ++i)
            argbytes[i-1] = bus.peek(regs.PBR | pc++);

        u32 param = 0;
        switch(len)
        {
        case 2:
            param = argbytes[0];
            fprintf(traceFile, "%02X      ", argbytes[0]);
            break;
        case 3:
            param = argbytes[0] | (argbytes[1] << 8);
            fprintf(traceFile, "%02X %02X   ", argbytes[0], argbytes[1]);
            break;
        case 4:
            param = argbytes[0] | (argbytes[1] << 8) | (argbytes[2] << 16);
            fprintf(traceFile, "%02X %02X %02X", argbytes[0], argbytes[1], argbytes[2]);
            break;
        default:
            fprintf(traceFile, "        ");
            break;
        }

        //////////////////////////////
        //  Print the mnemonic
        fprintf(traceFile, "  %s ", opnames[op]);

        //////////////////////////////
        //  Print the parameter
    }

}

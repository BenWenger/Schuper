
#include "cputracer.h"
#include "cpustate.h"
#include "bus/cpubus.h"
#include "internaldebug/internaldebug.h"

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
/* 2x */ "JSR","AND","JSL","AND",  "BIT","AND","ROL","AND",    "PLP","AND","ROL","PLD",  "BIT","AND","ROL","AND", /* 2x */
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

    void doParam(FILE* traceFile, const CpuState& regs, const CpuBus& bus, AddrMode mode, u32 arg)
    {
        char buf[80] = "";
        u16 tmp;
        u32 previewaddr;
        int previewbytes = -1;

        bool mdflg = regs.fM;
        switch(mode)
        {
        case im__x: mdflg = regs.fX;    mode = im__m;   break;
        case im__8: mdflg = true;       mode = im__m;   break;
        case dp__x: mdflg = regs.fX;    mode = dp__m;   break;
        case dx__x: mdflg = regs.fX;    mode = dx__m;   break;
        case ab__x: mdflg = regs.fX;    mode = ab__m;   break;
        case ax__x: mdflg = regs.fX;    mode = ax__m;   break;
        case ay__x: mdflg = regs.fX;    mode = ay__m;   break;
        }

        int mdbytes = mdflg ? 1 : 2;

        switch(mode)
        {
        case impld:                                                     break;
        case accum: sprintf(buf, "A");                                  break;
        case im__m: sprintf(buf, mdflg ? "#$%02X" : "#$%04X", arg);     break;

        case dp__m: sprintf(buf, "$%02X", arg);
                    previewaddr = (arg + regs.DP) & 0xFFFF;
                    previewbytes = mdbytes;                             break;

        case dx__m: sprintf(buf, "$%02X,X", arg);
                    previewaddr = (arg + regs.X.w + regs.DP) & 0xFFFF;
                    previewbytes = mdbytes;                             break;

        case dy__x: sprintf(buf, "$%02X,Y", arg);
                    previewaddr = (arg + regs.X.w + regs.DP) & 0xFFFF;
                    previewbytes = mdbytes;                             break;

        case sr__m: sprintf(buf, "$%02X,S", arg);
                    previewaddr = (arg + regs.SP) & 0xFFFF;
                    previewbytes = mdbytes;                             break;

        case ab__m: sprintf(buf, "$%04X", arg);
                    previewaddr = arg | regs.DBR;
                    previewbytes = mdbytes;                             break;

        case ax__m: sprintf(buf, "$%04X,X", arg);
                    previewaddr = ((arg + regs.X.w) & 0xFFFF) | regs.DBR;
                    previewbytes = mdbytes;                             break;

        case ay__m: sprintf(buf, "$%04X,Y", arg);
                    previewaddr = ((arg + regs.Y.w) & 0xFFFF) | regs.DBR;
                    previewbytes = mdbytes;                             break;

        case abl_m: sprintf(buf, "$%02X:%04X", (arg >> 16), (arg & 0xFFFF));
                    previewaddr = arg;
                    previewbytes = mdbytes;                             break;

        case axl_m: sprintf(buf, "$%02X:%04X,X", (arg >> 16), (arg & 0xFFFF));
                    previewaddr = ((arg + regs.X.w) & 0xFFFF) | (arg & 0xFF0000);
                    previewbytes = mdbytes;                             break;

        case di__m: sprintf(buf, "($%02X)", arg);
                    tmp = (arg + regs.DP) & 0xFFFF;
                    previewaddr  = bus.peek(tmp++);
                    previewaddr |= bus.peek(tmp) << 8;
                    previewaddr |= regs.DBR;
                    previewbytes = mdbytes;                             break;
                    
        case dil_m: sprintf(buf, "[$%02X]", arg);
                    tmp = (arg + regs.DP) & 0xFFFF;
                    previewaddr  = bus.peek(tmp++);
                    previewaddr |= bus.peek(tmp++) << 8;
                    previewaddr |= bus.peek(tmp) << 16;
                    previewbytes = mdbytes;                             break;

        case ix__m: sprintf(buf, "($%02X,X)", arg);
                    tmp = (arg + regs.DP + regs.X.w) & 0xFFFF;
                    previewaddr  = bus.peek(tmp++);
                    previewaddr |= bus.peek(tmp) << 8;
                    previewaddr |= regs.DBR;
                    previewbytes = mdbytes;                             break;
                    
        case iy__m: sprintf(buf, "($%02X),Y", arg);
                    tmp = (arg + regs.DP) & 0xFFFF;
                    previewaddr  = bus.peek(tmp++);
                    previewaddr |= bus.peek(tmp) << 8;
                    previewaddr  = ((previewaddr + regs.Y.w) & 0xFFFF) | regs.DBR;
                    previewbytes = mdbytes;                             break;
                    
        case iyl_m: sprintf(buf, "[$%02X],Y", arg);
                    tmp = (arg + regs.DP) & 0xFFFF;
                    previewaddr  = bus.peek(tmp++);
                    previewaddr |= bus.peek(tmp++) << 8;
                    previewaddr  = ((previewaddr + regs.Y.w) & 0xFFFF);
                    previewaddr |= bus.peek(tmp) << 16;
                    previewbytes = mdbytes;                             break;
                    
        case siy_m: sprintf(buf, "($%02X,S),Y", arg);
                    tmp = (arg + regs.SP) & 0xFFFF;
                    previewaddr  = bus.peek(tmp++);
                    previewaddr |= bus.peek(tmp) << 8;
                    previewaddr  = ((previewaddr + regs.Y.w) & 0xFFFF) | regs.DBR;
                    previewbytes = mdbytes;                             break;

        case brnch: previewaddr  = (regs.PC + 2 + (arg ^ 0x80) - 0x80) & 0xFFFF;
                    sprintf(buf, "$%04X", previewaddr);
                    previewaddr |= regs.PBR;
                    previewbytes = 0;                                   break;
                    
        case jp_rl: previewaddr  = (regs.PC + 3 + (arg ^ 0x8000) - 0x8000) & 0xFFFF;
                    sprintf(buf, "$%04X", previewaddr);
                    previewaddr |= regs.PBR;
                    previewbytes = 0;                                   break;
                    
        case jp_ab: previewaddr  = arg;
                    sprintf(buf, "$%04X", previewaddr);
                    previewaddr |= regs.PBR;
                    previewbytes = 0;                                   break;
                    
        case jp_al: previewaddr  = arg;
                    sprintf(buf, "$%02X:%04X", (previewaddr >> 16), (previewaddr & 0xFFFF));
                    previewbytes = 0;                                   break;
                    
        case jp_ix: sprintf(buf, "($%04X,X)", arg);
                    tmp = (arg + regs.X.w) & 0xFFFF;
                    previewaddr  = bus.peek(regs.PBR | tmp++);
                    previewaddr |= bus.peek(regs.PBR | tmp) << 8;
                    previewaddr |= regs.PBR;
                    previewbytes = 0;                                   break;
                    
        case jp_in: sprintf(buf, "($%04X)", arg);
                    tmp = arg + regs.DP;
                    previewaddr  = bus.peek(regs.PBR | tmp++);
                    previewaddr |= bus.peek(regs.PBR | tmp) << 8;
                    previewaddr |= regs.PBR;
                    previewbytes = 0;                                   break;
                    
        case jp_il: sprintf(buf, "[$%04X]", arg);
                    tmp = arg + regs.DP;
                    previewaddr  = bus.peek(regs.PBR | tmp++);
                    previewaddr |= bus.peek(regs.PBR | tmp++) << 8;
                    previewaddr |= bus.peek(regs.PBR | tmp) << 16;
                    previewbytes = 0;                                   break;

        case mv_np: sprintf(buf, "$%02X, $%02X", (arg >> 8), (arg & 0xFF));
                    previewbytes = -1;                                  break;

        case __pei: sprintf(buf, "($%02X)", arg);
                    previewaddr = (arg + regs.DP) & 0xFFFF;
                    previewbytes = 2;                                   break;

        case __per: previewaddr = (regs.PC + 3 + (arg ^ 0x8000) - 0x8000) & 0xFFFF;
                    sprintf(buf, "$%04X", arg);
                    previewbytes = 0;                                   break;
        }

        static const char* const padding = "            \0~~~~~~~~~~~~~~~~~~~";
        fprintf(traceFile, "%s%s", buf, padding + strlen(buf));

        switch(previewbytes)
        {
        case 2:  fprintf(traceFile, "[%02X:%04X=%02X%02X]", previewaddr >> 16, previewaddr & 0xFFFF, bus.peek(previewaddr+1), bus.peek(previewaddr));   break;
        case 1:  fprintf(traceFile, "  [%02X:%04X=%02X]",   previewaddr >> 16, previewaddr & 0xFFFF, bus.peek(previewaddr));                            break;
        case 0:  fprintf(traceFile, "     [%02X:%04X]",     previewaddr >> 16, previewaddr & 0xFFFF);                                                   break;
        default: fprintf(traceFile, "              ");                                                                                                  break;
        }
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
        doParam(traceFile, regs, bus, mode, param);


        //////////////////////////////
        //  A register
        if(regs.fM) fprintf(traceFile, "  (%02X) %02X ", regs.A.h, regs.A.l);
        else        fprintf(traceFile, "     %04X ", regs.A.w);

        //////////////////////////////
        //  X,Y regs
        if(regs.fX) fprintf(traceFile, "  %02X   %02X", regs.X.w, regs.Y.w);
        else        fprintf(traceFile, "%04X %04X", regs.X.w, regs.Y.w);

        //////////////////////////////
        //  Status flags & DP, SP
        fprintf(traceFile, "  [%c %c%c%c%c%c%c%c%c]  D=%04X  S=%04X  B=%02X",
                (regs.fE ? 'E' : '.'),
                (regs.fN ? 'N' : '.'),
                (regs.fV ? 'V' : '.'),
                (regs.fM ? 'M' : '.'),
                (regs.fX ? 'X' : '.'),
                (regs.fD ? 'D' : '.'),
                (regs.fI ? 'I' : '.'),
                (regs.fZ ? '.' : 'Z'),
                (regs.fC ? 'C' : '.'),
                regs.DP,
                regs.SP,
                (regs.DBR >> 16)
        );

#ifdef IDBG_ENABLED
        int h, v;
        InternalDebug.getPpuPos(h,v);

        fprintf(traceFile, "  H=%03d  V=%03d", h, v);
#endif

        fprintf(traceFile, "\n");
    }
    
    void CpuTracer::traceLine(const char* line)
    {
        if(!traceFile)          return;

        /////////////////////////
        //  Print PC
        fprintf(traceFile, "%s\n", line);
    }

}

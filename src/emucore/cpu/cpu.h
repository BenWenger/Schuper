
#ifndef SCHUPER_CPU_CPU_H_INCLUDED
#define SCHUPER_CPU_CPU_H_INCLUDED

#include <algorithm>
#include "snestypes.h"
#include "cpustate.h"

namespace sch
{
    class CpuBus;
    class CpuTracer;

    class Cpu
    {
    public:
        void            runTo(timestamp_t runto);

        void            setTracer(CpuTracer* trc)       { tracer = trc;             }

    private:


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
        timestamp_t     curTick;        // current timestamp
        timestamp_t     ioTickBase;     // tick base of 1 "io cycle"

        
        CpuTracer*      tracer;
        CpuBus*         bus;
        CpuState        regs;
        bool            interruptPending;
        bool            resetPending;
        bool            stopped;
        bool            waiting;

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

        typedef         u16 (Cpu::*op_t)(u16, bool);
        #define         CALLOP(v,flg)       (this->*op)(v,flg)
        
#include "cpu_addrmodes.h"
#include "cpu_instructions.h"

    };
}

#endif

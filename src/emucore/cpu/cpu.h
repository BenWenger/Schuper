
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

        bool            interruptReady;     // An interrupt can happen now
        bool            interruptPending;   // An interrupt is pending, but can't happen for 1 cycle (simulate 1 cycle delay)
        bool            resetPending;       // The pending interrupt is a reset
        bool            nmiPending;         // The pending interrupt is an NMI
        u32             irqFlags;           // Bitmask of any IRQs pending (0 = no IRQs)

        bool            stopped;            // The CPU is stopped / deadlocked  (happens on STP instruction)
        bool            waiting;            // The CPU is sleeping / waiting for interrupt (WAI instruction)

        void            dpCyc();                    // Do an IO cycle if low byte if DP isn't zero
        void            ioCyc();
        void            ioCyc(int cycs);
        void            tallyCycle(int cycs);
        void            doIndex(u16& a, u16 idx);   // Add an index to an address, and add an extra IO cyc when appropriate

        u8              read_l(u32 a);              // long read (24-bit)
        u8              read_a(u16 a);              // absolute read (16-bit, using DBR)
        u8              read_p();                   // instruction read (using PC and PBR)
        u8              pull();
        
        void            write_l(u32 a, u8 v);       // long write (24-bit)
        void            write_a(u16 a, u8 v);       // absolute write (16-bit, using DBR)
        void            push(u8 v);
        
        void            doInterrupt(IntType type);
        void            checkInterruptPending();

        void            innerRun(timestamp_t runto);

        typedef         u16 (Cpu::*op_t)(u16, bool);
        #define         CALLOP(v,flg)       (this->*op)(v,flg)
        
#include "cpu_addrmodes.h"
#include "cpu_instructions.h"

    };
}

#endif


#ifndef SCHUPER_APU_SMP_H_INCLUDED
#define SCHUPER_APU_SMP_H_INCLUDED

#include "smpregs.h"
#include "util/clockedsubsystem.h"

namespace sch
{
    class SpcBus;
    class SmpTracer;
    class SnesFile;

    class Smp : public ClockedSubsystem
    {
    public:
                        Smp();
        virtual void    runTo(timestamp_t runto) override;
        void            setTracer(SmpTracer* trcr)          { tracer = trcr;            }

        void            resetWithFile(SpcBus* bs, const SnesFile& file);

    private:
        SmpRegs         regs;
        SpcBus*         bus;
        bool            stopped;
        SmpTracer*      tracer;

        u8              read(u16 a);            // standard read
        u8              dpRd(u8 a);             // direct page read
        u8              pull();                 // simple pull (single read off stack)
        void            write(u16 a, u8 v);     // standard write
        void            dpWr(u8 a, u8 v);       // direct page write
        void            push(u8 v);             // simple push (single write to stack)
        void            ioCyc();                // I/O cycle (for internal operations that take a cycle)
        void            ioCyc(int cycs);        // I/O cycle (multiple)


        
#define BRANCH(condition, rl)           \
    if(condition)                       \
    {                                   \
        ioCyc(2);                       \
        regs.PC += (rl ^ 0x80) - 0x80;  \
    }

#include "smp_addrmodes.h"
#include "smp_instructions.h"

#undef BRANCH
    };

}

#endif

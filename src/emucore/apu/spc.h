
#ifndef SCHUPER_APU_SPC_H_INCLUDED
#define SCHUPER_APU_SPC_H_INCLUDED

#include "spcbus.h"
#include "smp.h"
#include "smptracer.h"

namespace sch
{
    class SnesFile;

    class Spc
    {
    public:
                    Spc();
        void        loadSpcFile(const SnesFile& file);
        void        setTrace(const char* filename);
        void        runForCycs(timestamp_t cycs);           // TODO this is temporary


    private:
        Smp         cpu;
        SmpTracer   tracer;
        SpcBus      bus;
        u8          ram[0x10000];
        bool        useIplBootRom;
        bool        traceEnabled;
        
        u8          rdFunc(u16 a, timestamp_t tick);
        u8          rdFunc_Page0(u16 a, timestamp_t tick);
        u8          rdFunc_PageF(u16 a, timestamp_t tick);

        u8          pkFunc(u16 a) const;
        
        void        wrFunc(u16 a, u8 v, timestamp_t tick);
        void        wrFunc_Page0(u16 a, u8 v, timestamp_t tick);
    };

}

#endif

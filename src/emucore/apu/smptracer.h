
#ifndef SCHUPER_APU_SMPTRACER_H_INCLUDED
#define SCHUPER_APU_SMPTRACER_H_INCLUDED

#include <cstdio>           // fprintf      --  because iostream is freaking garbage

namespace sch
{
    class SpcBus;
    class SmpRegs;

    class SmpTracer
    {
    public:
                    SmpTracer();
                    ~SmpTracer();

        void        openTraceFile(const char* filename);
        void        closeTraceFile();
        bool        isOpen() const      { return traceFile != nullptr;       }
        void        cpuTrace(const SmpRegs& regs, const SpcBus& bus);

    private:
        SmpTracer(const SmpTracer&) = delete;
        SmpTracer& operator = (const SmpTracer&) = delete;
        FILE*       traceFile;
    };

}

#endif

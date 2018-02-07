
#ifndef SCHUPER_CPU_CPUTRACER_H_INCLUDED
#define SCHUPER_CPU_CPUTRACER_H_INCLUDED

#include <cstdio>

namespace sch
{
    class CpuState;
    class CpuBus;

    class CpuTracer
    {
    public:
                    CpuTracer();
                    ~CpuTracer();

        void        openTraceFile(const char* filename);
        void        closeTraceFile();
        void        cpuTrace(const CpuState& regs, const CpuBus& bus);
        bool        isOpen() const          { return traceFile != nullptr;      }

        void        traceLine(const char* line);

    private:
        CpuTracer(const CpuTracer&) = delete;
        CpuTracer& operator = (const CpuTracer&) = delete;
        FILE*       traceFile;

    };
}

#endif

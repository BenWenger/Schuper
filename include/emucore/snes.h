
#ifndef SCHUPER_SNES_H_INCLUDED
#define SCHUPER_SNES_H_INCLUDED

#include <memory>
#include <vector>
#include "snestypes.h"
#include "snesfile.h"

namespace sch
{
    class Spc;
    class Cpu;
    class CpuBus;
    class CpuTracer;

    class Snes
    {
    public:
        Snes();
        ~Snes();
        Snes(const Snes&) = delete;
        Snes& operator = (const Snes&) = delete;


        SnesFile::Type  loadFile(SnesFile&& file);
        void            hardReset();

        void            startCpuTrace(const char* filename);
        void            stopCpuTrace();

        void            doFrame();

    private:
        
        int             rd_LoRom(u32 a, u8& v, timestamp_t clk);
        int             wr_LoRom(u32 a, u8  v, timestamp_t clk);
        u8              pk_LoRom(u32 a) const;
        
        void            wr_Reg(u16 a, u8 v, timestamp_t clk);
        u8              rd_Reg(u16 a, timestamp_t clk);

        SnesFile                    currentFile;

        u32                         romMask;

        bool                        nmiEnabled;   // TODO remove this shit!

        std::unique_ptr<u8[]>       ram;
            
        int                         spdSlow;
        int                         spdFast;
        int                         spdXSlow;

        std::unique_ptr<Spc>        spc;
        std::unique_ptr<Cpu>        cpu;
        std::unique_ptr<CpuBus>     cpuBus;
        std::unique_ptr<CpuTracer>  cpuTracer;
    };

}

#endif


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
    class SmpTracer;
    class AudioBuffer;

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
        bool            isCpuTracing();
        
        void            startSpcTrace(const char* filename);
        void            stopSpcTrace();
        bool            isSpcTracing();

        void            setAudioBuffer(s16* bufA, int sizInBytesA, s16* bufB = nullptr, int sizInBytesB = 0);
        int             getBytesOfAudioWritten();
        int             getBytesOfAudioForAFrame();

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
        u32                         altRamAddr;
            
        int                         spdSlow;
        int                         spdFast;
        int                         spdXSlow;

        std::unique_ptr<Spc>            spc;
        std::unique_ptr<Cpu>            cpu;
        std::unique_ptr<CpuBus>         cpuBus;
        std::unique_ptr<CpuTracer>      cpuTracer;
        std::unique_ptr<SmpTracer>      smpTracer;
        std::unique_ptr<AudioBuffer>    audioBuffer;
    };

}

#endif

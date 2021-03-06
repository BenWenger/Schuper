
#ifndef SCHUPER_APU_SPC_H_INCLUDED
#define SCHUPER_APU_SPC_H_INCLUDED

#include "spcbus.h"
#include "smp.h"
#include "spctimer.h"
#include "dsp.h"
#include <memory>

namespace sch
{
    class SnesFile;
    class SmpTracer;

    class Spc
    {
    public:
                    Spc();
        void        loadSpcFile(const SnesFile& file);

        void        setTracer(SmpTracer* tracer)            { cpu.setTracer(tracer);        }

        void        reset();

        void        runTo(timestamp_t runto);
        void        runToFillAudio();
        
        void        setClockBase(timestamp_t base);
        timestamp_t getClockBase() const;
        void        adjustTimestamp(timestamp_t adj);
        void        forciblySetTimestamp(timestamp_t ts);
        timestamp_t getTick() const;

        void        setAudioBuffer(AudioBuffer* buf)        { dsp->setAudioBuffer(buf);         }
        
        u8          readIoReg(u8 a) const                   { return spcIO_Output[a&3];         }
        void        writeIoReg(u8 a, u8 v)                  { spcIO_Input[a&3] = v;             }


    private:
        typedef     std::unique_ptr<Dsp>    DspPtr;

        Smp         cpu;
        SpcTimer    timers[3];
        SpcBus      bus;
        DspPtr      dsp;
        u8          ram[0x10000];
        u8          fauxRam[2];
        u8          dspAddr;
        
        u8          spcIO_Input[4];
        u8          spcIO_Output[4];

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

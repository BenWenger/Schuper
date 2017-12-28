
#ifndef SCHUPER_APU_DSP_H_INCLUDED
#define SCHUPER_APU_DSP_H_INCLUDED

#include "snestypes.h"
#include "util/clockedsubsystem.h"
#include "util/audiobuffer.h"

namespace sch
{
    class SpcTimer;

    class Dsp : public ClockedSubsystem
    {
    public:
        virtual ~Dsp()      {}

        virtual u8          read(u8 a) = 0;
        virtual void        write(u8 a, u8 v) = 0;
        virtual void        runFor(timestamp_t ticks) = 0;
        virtual timestamp_t findTimestampToFillAudio() = 0;

        void                setSharedObjects(u8* ram, SpcTimer* timers)
        {
            this->ram = ram;
            this->timers = timers;
        }

        void                setAudioBuffer(AudioBuffer* buf)
        {
            if(buf)     audBuf = (buf->canOutput() ? buf : nullptr);
            else        audBuf = nullptr;
        }

    protected:
        u8*             ram;
        SpcTimer*       timers;
        AudioBuffer*    audBuf;
        
        enum class Adsr
        {
            Attack,
            Decay,
            Sustain,
            Release,
            Off
        };


        static inline s16      clamp16(int x)
        {
            if     (x >  0x7FFF)    return 0x7FFF;
            else if(x < -0x8000)    return -0x8000;
            else                    return x;
        }
        static inline s16      wrap16(int x)
        {
            return static_cast<s16>(x & 0xFFFF);
        }
        static inline s16      clamp15(int x)
        {
            if     (x >  0x3FFF)    return 0x3FFF;
            else if(x < -0x4000)    return -0x4000;
            else                    return x;
        }
        static inline s16      wrap15(int x)
        {
            x &= 0x7FFF;
            return static_cast<s16>((x ^ 0x4000) - 0x4000);
        }

        int     processAdsr(int envelope, Adsr& adsr, u8 VxADSR1, u8 VxADSR2, u8 VxGAIN);
        void    processBrr(u8 hdr, s16 block, s16* prevsamples, s16* nextsamples);
        s16     gaussInterpolation(int phase, int a, int b, int c, int d);


        bool    rateCheck(int r);
        void    internalReset()         { rateCounter = 0x77FF;     noiseSample = 0x4000;                   }
        void    updateRateCounter()     { if(rateCounter)   --rateCounter;  else rateCounter = 0x77FF;      }
        s16     getNoiseSample() const  { return static_cast<s16>((noiseSample ^ 4000) - 0x4000);           }
        void    updateNoise()
        {
            noiseSample = (noiseSample >> 1) |
                          (   ( (noiseSample<<14)^(noiseSample<<13) )   & 0x4000   );
        }
        

        inline bool canOutputAudio() const
        {
            return (audBuf != nullptr);
        }

        inline void outputSample(s16 left, s16 right)
        {
            if(audBuf->outputSample(left, right))
                audBuf = nullptr;
        }


    private:
        int     doAdsrAdjustment(int envelope, int mode, int r);

        int     rateCounter = 0x77FF;
        int     noiseSample = 0x4000;
    };

}

#endif

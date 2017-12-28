
#ifndef SCHUPER_APU_SIMPLEDSP_H_INCLUDED
#define SCHUPER_APU_SIMPLEDSP_H_INCLUDED

#include "snestypes.h"
#include "dsp.h"

namespace sch
{
    /*
        This is a simple (faster, but not super accurate) DSP implementation
    */

    class SimpleDsp : public Dsp
    {
    public:
        virtual u8          read(u8 a) override;
        virtual void        write(u8 a, u8 v) override;
        virtual void        runFor(timestamp_t ticks) override;
        virtual timestamp_t findTimestampToFillAudio() override;

    private:
        void            doSamples(int count);

        void            updateVoice(int index);
        void            processEcho(s16 samps[2]);

        void            loadOneBrrBlock(int index);


        struct Voice
        {
            // regs
            int         vol[2];     // [0] = L, [1] = R
            int         pitch;
            u8          srcn;
            u8          adsr1Reg;
            u8          adsr2Reg;
            u8          gainReg;

            // runtime junk
            int         konDelay;
            Adsr        adsr;
            u16         envelope;
            int         outSample;  // after envelope applied, before L/R volume
            int         out[2];     // after vol applied.   [0] = L, [1] = R

            // BRR ring buffer
            int         phase;
            int         brrWritePos;        // 0, 4, 8, or 12
            int         brrReadPos;         // 0, 4, 8, or 12
            s16         brr[16];
            u16         brrSrcPointer;
            int         brrSrcPos;          // 1, 3, 5, or 7
        };

        ////////////////////////////////////
        Voice           voices[8];
        int             mvol[2];    // master volume    [0] = L, [1] = R
        int             evol[2];    // echo volume
        u8              flg;        // FLG register
        u8              endx;       // ENDX register
        int             echoFeedback;
        u8              pmon;       // PMON register (pitch modulation enable)
        u8              non;        // NON register (noise)
        u8              eon;        // EON register (echo)
        u16             brrTableAddr;
        u16             echoBufferAddr;
        int             echoBufferSize; // size in bytes
        int             echoBufferPos;

        u8              kon;        // actual KON (what is used) -- AKA "internal KON"
        u8              koff;

        int             firCoeff[8];
        int             firRing[2][8];  // [0] = L, [1] = R .... with 8 entries each
        int             firPos;

        u8              rawRegs[0x80];

        ////////////////////
        //  Timestamp and syncing stuff
        u32             dspTick;
    };

}

#endif

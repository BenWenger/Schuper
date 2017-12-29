
#include "simpledsp.h"
#include "spctimer.h"

namespace sch
{
    
    void SimpleDsp::doSamples(int count)
    {
        for(int i = 0; (i < count) && canOutputAudio(); ++i)
        {
            // Update and mix all voices
            s16 mainSamp[2] = {0,0};
            s16 echoSamp[2] = {0,0};
            for(int vc = 0; vc < 8; ++vc)
            {
                updateVoice(vc);
                
                mainSamp[0] = clamp16(mainSamp[0] + voices[vc].out[0]);
                mainSamp[1] = clamp16(mainSamp[1] + voices[vc].out[1]);

                if(eon & (1<<vc))
                {
                    echoSamp[0] = clamp16(echoSamp[0] + voices[vc].out[0]);
                    echoSamp[1] = clamp16(echoSamp[1] + voices[vc].out[1]);
                }
            }

            // apply main volume
            mainSamp[0] = (mainSamp[0] * mvol[0]) >> 7;
            mainSamp[1] = (mainSamp[1] * mvol[1]) >> 7;

            // do FIR/echo processing
            processEcho(echoSamp);

            // final output!
            if(flg & 0x40)
                outputSample( 0, 0 );
            else
            {
                outputSample( clamp16(mainSamp[0] + echoSamp[0]),
                              clamp16(mainSamp[1] + echoSamp[1])
                            );
            }

            // update the global rate counter
            //      and noise (if appropriate)
            updateRateCounter();
            if(rateCheck(flg & 0x1F))
                updateNoise();
        }
    }

    void SimpleDsp::processEcho(s16 samps[2])
    {
        // Echo ring buffer              ->  FIR in
        // FIR out * EFB   +   samps     ->  Echo Ring Buffer  (if FLG permits writes)
        // FIR out * EVOL                ->  samps

        u8* echobuf = &ram[(echoBufferAddr + echoBufferPos) & 0xFFFC];
        int firout;

        for(int i = 0; i < 2; ++i)
        {
            firRing[i][firPos] = static_cast<s16>(echobuf[0] | (echobuf[1] << 8)) >> 1;
            if(silenceEcho)     firRing[i][firPos] = 0;

            firout =    (firRing[i][(firPos+7)&7] * firCoeff[6]) >> 6;
            firout +=   (firRing[i][(firPos+6)&7] * firCoeff[5]) >> 6;
            firout +=   (firRing[i][(firPos+5)&7] * firCoeff[4]) >> 6;
            firout +=   (firRing[i][(firPos+4)&7] * firCoeff[3]) >> 6;
            firout +=   (firRing[i][(firPos+3)&7] * firCoeff[2]) >> 6;
            firout +=   (firRing[i][(firPos+2)&7] * firCoeff[1]) >> 6;
            firout +=   (firRing[i][(firPos+1)&7] * firCoeff[0]) >> 6;

            firout =    wrap16(firout);
            firout +=   (firRing[i][firPos] * firCoeff[7]) >> 6;
            firout =    clamp16(firout);
            firout &=   ~1;

            if(!(flg & 0x20))       // is echo writing permitted?
            {
                int towrite = (firout * echoFeedback) >> 7;
                towrite = clamp16( towrite + samps[i] ) & ~1;
                echobuf[0] = static_cast<u8>( towrite );
                echobuf[1] = static_cast<u8>( towrite >> 8 );
            }

            samps[i] = static_cast<s16>( (firout * evol[i]) >> 7 );
            echobuf += 2;
        }

        // Update echo ring buffer position & FIR filter position
        echoBufferPos += 4;
        if(echoBufferPos >= echoBufferSize)
        {
            echoBufferPos  = 0;
            silenceEcho = false;
        }

        firPos = (firPos+1) & 7;
    }

    void SimpleDsp::updateVoice(int index)
    {
        Voice& vc = voices[index];
        int bitflg = (1<<index);

        // Key on / Key Off
        if((koff & bitflg) && (vc.konDelay < 3))        // konDelay here is a bit of a hack
        {
            vc.adsr = Adsr::Release;
        }
        if(kon & bitflg)
        {
            vc.adsr = Adsr::Attack;
            vc.envelope = 0;
            vc.konDelay = 5;
            kon &= ~bitflg;
            endx &= ~bitflg;
        }
        if(flg & 0x80)
        {
            vc.adsr = Adsr::Off;
            vc.envelope = 0;
        }

        // KON delay stuff
        if(vc.konDelay)
        {
            --vc.konDelay;
            vc.outSample = 0;
            vc.out[0] = vc.out[1] = 0;

            // load up the first 12 BRR samples (3 blocks)
            if(!vc.konDelay)
            {
                u16 a = brrTableAddr + (vc.srcn << 2);
                vc.brrSrcPointer = ram[a] | (ram[a+1] << 8);
                vc.brrSrcPos = 1;
                vc.brrWritePos = 0;
                vc.phase = 0;
                loadOneBrrBlock(index);
                loadOneBrrBlock(index);
                loadOneBrrBlock(index);
                vc.brrReadPos = 0;          // do this after the loadOneBrrBlock calls, since those move it
            }

            return;                         // do nothing else when in KON delay
        }


        // Envelope!
        vc.envelope = processAdsr(vc.envelope, vc.adsr, vc.adsr1Reg, vc.adsr2Reg, vc.gainReg);

        // Update phase, load more BRR if needed
        if( pmon & bitflg & ~non & ~1 )
        {
            vc.phase += vc.pitch + (((voices[index-1].outSample >> 5) * vc.pitch) >> 10);
            if(vc.phase > 0x7FFF)
                vc.phase = 0x7FFF;
        }
        else
            vc.phase += vc.pitch;

        if(vc.phase >= 0x4000)
        {
            loadOneBrrBlock(index);
            vc.phase -= 0x4000;
        }

        // get output sample!
        if(non & bitflg)
            vc.outSample = getNoiseSample();
        else
        {
            int i = (vc.phase >> 12) + vc.brrReadPos;
            vc.outSample = gaussInterpolation(
                            vc.phase,
                            vc.brr[(i+0) & 0x0F],
                            vc.brr[(i+1) & 0x0F],
                            vc.brr[(i+2) & 0x0F],
                            vc.brr[(i+3) & 0x0F]
                );
        }

        // currently we have a 15-bit sample ... apply envelope
        vc.outSample = (vc.outSample * vc.envelope) >> 11;

        // still 15-bit ... apply volume, but make it 16-bit in the process
        vc.out[0] = (vc.outSample * vc.vol[0]) >> 6;
        vc.out[1] = (vc.outSample * vc.vol[1]) >> 6;

        // vc.out is a 16-bit signed sample!
    }

    void SimpleDsp::loadOneBrrBlock(int index)
    {
        Voice&  vc = voices[index];

        u8      hdr = ram[vc.brrSrcPointer];
        if((hdr & 3) == 1)      // 'e' set and 'l' clear -- immediate end
        {
            vc.adsr = Adsr::Off;
            vc.envelope = 0;
        }


        u8      a   = ram[ (vc.brrSrcPointer + vc.brrSrcPos    ) & 0xFFFF ];
        u8      b   = ram[ (vc.brrSrcPointer + vc.brrSrcPos + 1) & 0xFFFF ];

        s16*    prev = &vc.brr[ (vc.brrWritePos + 12) & 0x0F ];
        s16*    next = &vc.brr[ vc.brrWritePos ];

        processBrr( hdr, (a << 8) | b, prev, next );
        
        vc.brrWritePos = (vc.brrWritePos + 4) & 0x0F;
        vc.brrReadPos  = (vc.brrReadPos  + 4) & 0x0F;

        vc.brrSrcPos += 2;
        if(vc.brrSrcPos >= 9)
        {
            vc.brrSrcPos = 1;

            if(hdr & 1)     // 'e' is set, go to loop position
            {
                endx |= (1<<index);             // set ENDX flag
                u16 addr = brrTableAddr + (vc.srcn << 2) + 2;       // get loop position
                vc.brrSrcPointer = ram[addr] | (ram[addr+1] << 8);
            }
            else
                vc.brrSrcPointer += 9;
        }
    }

    u8 SimpleDsp::read(u8 a)
    {
        u8 out = rawRegs[a];

        switch(a)
        {
        case 0x08: case 0x18: case 0x28: case 0x38:     // 'VxENVX'
        case 0x48: case 0x58: case 0x68: case 0x78:
            out = voices[a>>4].envelope >> 4;
            break;
            
        case 0x09: case 0x19: case 0x29: case 0x39:     // 'VxOUTX'
        case 0x49: case 0x59: case 0x69: case 0x79:
            out = voices[a>>4].outSample >> 8;
            break;

        case 0x7C:          // ENDX
            out = endx;
            break;
        }

        return out;
    }
    
    void SimpleDsp::write(u8 a, u8 v)
    {
        rawRegs[a] = v;
        if((a & 0x0F) < 0x08)           // voice specific regs
        {
            Voice& vc = voices[a>>4];

            switch(a & 0x0F)
            {
            case 0: vc.vol[0] = (v ^ 0x80) - 0x80;                              break;  // VxVOLL
            case 1: vc.vol[1] = (v ^ 0x80) - 0x80;                              break;  // VxVOLR
            case 2: vc.pitch = (vc.pitch & 0xFF00) | v;                         break;  // VxPITCHL
            case 3: vc.pitch = (vc.pitch & 0x00FF) | ((v & 0x3F) << 8);         break;  // VxPITCHH
            case 4: vc.srcn = v;                                                break;  // VxSRCN
            case 5: vc.adsr1Reg = v;                                            break;  // VxADSR1
            case 6: vc.adsr2Reg = v;                                            break;  // VxADSR2
            case 7: vc.gainReg = v;                                             break;  // VxGAIN
            }
        }
        else                            // general-purpose regs
        {
            switch(a)
            {
            case 0x0C:  mvol[0] = (v ^ 0x80) - 0x80;                            break;  // MVOLL
            case 0x1C:  mvol[1] = (v ^ 0x80) - 0x80;                            break;  // MVOLR
            case 0x2C:  evol[0] = (v ^ 0x80) - 0x80;                            break;  // MVOLL
            case 0x3C:  evol[1] = (v ^ 0x80) - 0x80;                            break;  // MVOLR
            case 0x4C:  kon = v;                                                break;  // KON
            case 0x5C:  koff = v;                                               break;  // KOFF
            case 0x6C:  flg = v;                                                break;  // FLG
            case 0x7C:  endx = 0;                                               break;  // ENDX (any write here clears all bits)
            case 0x0D:  echoFeedback = (v ^ 0x80) - 0x80;                       break;  // EFB
            case 0x2D:  pmon = v;                                               break;  // PMON
            case 0x3D:  non = v;                                                break;  // NONN
            case 0x4D:  eon = v;                                                break;  // EON
            case 0x5D:  brrTableAddr = (v << 8);                                break;  // DIR
            case 0x6D:  echoBufferAddr = (v << 8);                              break;  // ESA
            case 0x7D:  echoBufferSize = ((v & 0x0F) << 11);                    break;  // EDL

            case 0x0F: case 0x1F: case 0x2F: case 0x3F:     // FIRx
            case 0x4F: case 0x5F: case 0x6F: case 0x7F:
                firCoeff[a >> 4] = (v ^ 0x80) - 0x80;
                break;
            }
        }
    }

    void SimpleDsp::runFor(timestamp_t ticks)
    {
        ClockedSubsystem::runFor(ticks);

        u32 newtick = dspTick + ticks;
        
        // every 16 cycles:     clock timer 2
        // every 32 cycles:     generate and output one sample
        // every 128 cycles:    clock timers 0, 1

        u32 t2 = ((newtick & ~0x0F) - (dspTick & ~0x0F)) >> 4;
        u32 samps = ((newtick & ~0x1F) - (dspTick & ~0x1F)) >> 5;
        u32 t01 = ((newtick & ~0x7F) - (dspTick & ~0x7F)) >> 7;

        for(u32 i = 0; i < t2; ++i)
            timers[2].clock();
        for(u32 i = 0; i < t01; ++i)
        {
            timers[0].clock();
            timers[1].clock();
        }
        if(samps > 0)
            doSamples( static_cast<int>(samps) );

        dspTick = newtick;
    }

    
    timestamp_t SimpleDsp::findTimestampToFillAudio()
    {
        if(canOutputAudio())
        {
            auto samps = audBuf->samplesLeftToWrite();

            u32 targettick = (dspTick & ~0x1F) + (samps << 5) + 1;

            return getTick() + ((targettick - dspTick) * getClockBase());
        }
        else
            return getTick();
    }

    void SimpleDsp::reset()
    {
        internalReset();
        
        mvol[0] = mvol[1] = 0;
        evol[0] = evol[1] = 0;
        flg = 0;
        endx = 0;
        echoFeedback = 0;
        pmon = 0;
        non = 0;
        eon = 0;
        brrTableAddr = 0;
        echoBufferAddr = 0;
        echoBufferSize = 0;
        echoBufferPos = 0;

        kon = 0;
        koff = 0;
        for(int i = 0; i < 8; ++i)
        {
            firCoeff[i] = 0;
            firRing[0][i] = 0;
            firRing[1][i] = 0;
        }
        firPos = 0;

        for(auto& x : rawRegs)
            x = 0;

        dspTick = 0;

        for(auto& vc : voices)
        {
            vc.vol[0] = vc.vol[1] = 0;
            vc.pitch = 0;
            vc.srcn = 0;
            vc.adsr1Reg = vc.adsr2Reg = vc.gainReg = 0;

            vc.konDelay = 0;
            vc.adsr = Adsr::Off;
            vc.envelope = 0;
            vc.outSample = 0;
            vc.out[0] = vc.out[1] = 0;

            vc.phase = 0;
            vc.brrWritePos = 0;
            vc.brrReadPos = 0;
            for(auto& x : vc.brr)
                x = 0;
            vc.brrSrcPointer = 0;
            vc.brrSrcPos = 1;
        }
    }
    
    void SimpleDsp::loadFromSpcFile(const u8* regs)
    {
        reset();

        for(u8 i = 0; i < 0x80; ++i)
        {
            write(i, regs[i]);
        }

        silenceEcho = true;
    }
}

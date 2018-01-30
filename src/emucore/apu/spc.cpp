
#include <cstring>              // memcpy
#include "spc.h"
#include "snesfile.h"
#include "simpledsp.h"

namespace sch
{
    namespace
    {
        const u8 iplBootRom[0x40] = {
        0xCD, 0xEF, 0xBD, 0xE8, 0x00, 0xC6, 0x1D, 0xD0, 0xFC, 0x8F, 0xAA, 0xF4, 0x8F, 0xBB, 0xF5, 0x78,
        0xCC, 0xF4, 0xD0, 0xFB, 0x2F, 0x19, 0xEB, 0xF4, 0xD0, 0xFC, 0x7E, 0xF4, 0xD0, 0x0B, 0xE4, 0xF5,
        0xCB, 0xF4, 0xD7, 0x00, 0xFC, 0xD0, 0xF3, 0xAB, 0x01, 0x10, 0xEF, 0x7E, 0xF4, 0x10, 0xEB, 0xBA,
        0xF6, 0xDA, 0x00, 0xBA, 0xF4, 0xC4, 0xF4, 0xDD, 0x5D, 0xD0, 0xDB, 0x1F, 0x00, 0x00, 0xC0, 0xFF
        };

        enum 
        {
            r_TEST      = 0xF0,
            r_CONTROL   = 0xF1,
            r_DSPADDR   = 0xF2,
            r_DSPDATA   = 0xF3,
            r_CPUIO_0   = 0xF4,
            r_CPUIO_1   = 0xF5,
            r_CPUIO_2   = 0xF6,
            r_CPUIO_3   = 0xF7,
            r_RAM_0     = 0xF8,
            r_RAM_1     = 0xF9,
            r_T0TARGET  = 0xFA,
            r_T1TARGET  = 0xFB,
            r_T2TARGET  = 0xFC,
            r_T0OUT     = 0xFD,
            r_T1OUT     = 0xFE,
            r_T2OUT     = 0xFF
        };
    }

    Spc::Spc()
    {
        dsp = std::make_unique<SimpleDsp>();
        dsp->setSharedObjects(ram, timers);

        bus.setPkProc( &Spc::pkFunc, this );
        bus.setRdProc(   0,   0, &Spc::rdFunc_Page0, this );
        bus.setRdProc(   1, 0xE, &Spc::rdFunc      , this );
        bus.setRdProc( 0xF, 0xF, &Spc::rdFunc_PageF, this );
        bus.setWrProc(   0,   0, &Spc::wrFunc_Page0, this );
        bus.setWrProc(   1, 0xF, &Spc::wrFunc      , this );

        setClockBase(1);
        forciblySetTimestamp(0);
    }

    u8 Spc::rdFunc(u16 a, timestamp_t tick)
    {
        return ram[a];
    }

    u8 Spc::rdFunc_Page0(u16 a, timestamp_t tick)
    {
        if((a & 0xFFF0) == 0x00F0)
        {
            switch(a)
            {
            case r_DSPADDR:                             return dspAddr;
            case r_DSPDATA:         dsp->runTo(tick);   return dsp->read(dspAddr & 0x7F);

            case r_CPUIO_0:
            case r_CPUIO_1:
            case r_CPUIO_2:
            case r_CPUIO_3:                             return spcIO_Input[a&3];
                
            case r_RAM_0:
            case r_RAM_1:                               return fauxRam[a&1];
                
            case r_T0OUT:           dsp->runTo(tick);   return timers[0].readOutput();
            case r_T1OUT:           dsp->runTo(tick);   return timers[1].readOutput();
            case r_T2OUT:           dsp->runTo(tick);   return timers[2].readOutput();

            default:                                    return 0;           // write-only regs read as zero
            }
        }
        
        return ram[a];
    }

    u8 Spc::rdFunc_PageF(u16 a, timestamp_t tick)
    {
        if(useIplBootRom && ((a & 0xFFC0) == 0xFFC0))
            return iplBootRom[a & 0x3F];
        return ram[a];
    }

    u8 Spc::pkFunc(u16 a) const
    {
        if(useIplBootRom && ((a & 0xFFC0) == 0xFFC0))
            return iplBootRom[a & 0x3F];
        return ram[a];
    }

    void Spc::wrFunc(u16 a, u8 v, timestamp_t tick)
    {
        ram[a] = v;
    }
    
    void Spc::wrFunc_Page0(u16 a, u8 v, timestamp_t tick)
    {
        if((a & 0xFFF0) == 0x00F0)
        {
            switch(a)
            {
            case r_TEST:
                // TODO - emulate this reg.. maybe?  It's kind of pointless
                break;

            case r_CONTROL:
                dsp->runTo(tick);
                timers[0].writeEnableBit( (v & 0x01) != 0 );
                timers[1].writeEnableBit( (v & 0x02) != 0 );
                timers[2].writeEnableBit( (v & 0x04) != 0 );
                
                if(v & 0x10)    spcIO_Input[0] = spcIO_Input[1] = 0;
                if(v & 0x20)    spcIO_Input[2] = spcIO_Input[3] = 0;

                useIplBootRom = (v & 0x80) != 0;
                break;

            case r_DSPADDR:
                dspAddr = v;
                break;

            case r_DSPDATA:
                if(dspAddr < 0x80)
                {
                    dsp->runTo(tick);
                    dsp->write(dspAddr, v);
                }
                break;
                
            case r_CPUIO_0:
            case r_CPUIO_1:
            case r_CPUIO_2:
            case r_CPUIO_3:
                spcIO_Output[a&3] = v;
                break;

            case r_RAM_0:
            case r_RAM_1:
                fauxRam[a&1] = v;
                break;
                
            case r_T0TARGET:    dsp->runTo(tick);   timers[0].writeTarget(v);       break;
            case r_T1TARGET:    dsp->runTo(tick);   timers[1].writeTarget(v);       break;
            case r_T2TARGET:    dsp->runTo(tick);   timers[2].writeTarget(v);       break;
            }
        }
        
        ram[a] = v;
    }

    
    //////////////////////////////////////////////////////////////

    void Spc::reset()
    {
        cpu.reset(&bus);
        for(auto& t : timers)       t.hardReset();
        dsp->reset();

        for(auto& i : ram)          i = 0;
        for(auto& i : fauxRam)      i = 0;
        dspAddr = 0;
        
        for(auto& i : spcIO_Input)  i = 0;
        for(auto& i : spcIO_Output) i = 0;

        useIplBootRom = true;
    }

    //////////////////////////////////////////////////////////////

    void Spc::loadSpcFile(const SnesFile& file)
    {
        if(file.type != SnesFile::Type::Spc)            return;
        if(file.memory.size() != 0x10000)               return;

        memcpy( ram, file.memory.data(), 0x10000 );
        cpu.resetWithFile( &bus, file );
        dsp->loadFromSpcFile( file.dspRegs );

        
        wrFunc_Page0(r_CONTROL, ram[r_CONTROL], 0);
        wrFunc_Page0(r_DSPADDR, ram[r_DSPADDR], 0);
        
        wrFunc_Page0(r_T0TARGET, ram[r_T0TARGET], 0);
        wrFunc_Page0(r_T1TARGET, ram[r_T1TARGET], 0);
        wrFunc_Page0(r_T2TARGET, ram[r_T2TARGET], 0);

        for(int i = 0; i < 4; ++i)
        {
            spcIO_Output[i] = 0;
        }
        spcIO_Input[0] = ram[r_CPUIO_0];
        spcIO_Input[1] = ram[r_CPUIO_1];
        spcIO_Input[2] = ram[r_CPUIO_2];
        spcIO_Input[3] = ram[r_CPUIO_3];
    }

    void Spc::setTrace(const char* filename)
    {
        if(filename)
        {
            tracer.openTraceFile(filename);
            cpu.setTracer(&tracer);
        }
        else
        {
            tracer.closeTraceFile();
            cpu.setTracer(nullptr);
        }
    }

    ////////////////////////////////////////////////////////////

    void Spc::setClockBase(timestamp_t base)
    {
        dsp->setClockBase(base);
        cpu.setClockBase(base);
    }

    timestamp_t Spc::getClockBase() const
    {
        return dsp->getClockBase();
    }

    void Spc::adjustTimestamp(timestamp_t adj)
    {
        dsp->adjustTimestamp(adj);
        cpu.adjustTimestamp(adj);
    }

    void Spc::forciblySetTimestamp(timestamp_t ts)
    {
        dsp->forciblySetTimestamp(ts);
        cpu.forciblySetTimestamp(ts);
    }

    timestamp_t Spc::getTick() const
    {
        return cpu.getTick();
    }

    /////////////////////////////////////////////////////////
    
    void Spc::runTo(timestamp_t runto)
    {
        cpu.runTo(runto);
        dsp->runTo(runto);
    }

    void Spc::runToFillAudio()
    {
        timestamp_t orig = cpu.getTick();
        timestamp_t runto = dsp->findTimestampToFillAudio();

        runTo(runto);

        timestamp_t adj = orig - cpu.getTick();
        cpu.adjustTimestamp(adj);
        dsp->adjustTimestamp(adj);
    }
}
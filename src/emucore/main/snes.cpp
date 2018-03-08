
#include "snes.h"
#include "snesfile.h"
#include "apu/spc.h"
#include "cpu/cpu.h"
#include "bus/cpubus.h"
#include "cpu/cputracer.h"
#include "apu/smptracer.h"
#include "dma/dmaunit.h"
#include "ppu/ppu.h"
#include "mainclock.h"

namespace sch
{
    Snes::Snes()
        : ram(new u8[0x20000])
    {
        spc = std::make_unique<Spc>();
        cpu = std::make_unique<Cpu>();
        cpuBus = std::make_unique<CpuBus>();
        audioBuffer = std::make_unique<AudioBuffer>();
        dmaUnit = std::make_unique<DmaUnit>();
        ppu = std::make_unique<Ppu>();
        mainClock = std::make_unique<MainClock>();
        eventManager = std::make_unique<EventManager>();

        spc->setAudioBuffer(audioBuffer.get());
        
        spdFast = 6;
        spdSlow = 8;
        spdXSlow = 12;
        spc->setClockBase(21);                  // roughly 21 master cycles per SPC cycle
    }

    Snes::~Snes()
    {
    }

    SnesFile::Type Snes::loadFile(SnesFile&& file)
    {
        // File size must be 64K minimum
        if(file.memory.size() < 0x10000)
            return SnesFile::Type::Invalid;

        switch(file.type)
        {
        case SnesFile::Type::Spc:
            // nothing needs to be done here -- the load happens during reset
            break;

        case SnesFile::Type::Rom:
            {
                switch(file.memmap)
                {
                case SnesFile::MemMap::LoRom:
                    cpuBus->setReader(this, &Snes::rd_LoRom);
                    cpuBus->setWriter(this, &Snes::wr_LoRom);
                    cpuBus->setPeeker(this, &Snes::pk_LoRom);
                    break;

                default:
                    return SnesFile::Type::Invalid;
                }
            }
            break;

        default:
            return SnesFile::Type::Invalid;
        }


        // If we made it here, the file is good.  Take it!
        romMask = file.memory.size()-1;
        currentFile = std::move(file);

        hardReset();

        return currentFile.type;
    }


    void Snes::hardReset()
    {
        switch(currentFile.type)
        {
        case SnesFile::Type::Rom:
            cpuBus->reset();
            eventManager->reset();
            mainClock->reset(eventManager.get());
            cpu->reset(cpuBus.get(), mainClock.get());
            spc->reset();
            dmaUnit->reset(true, cpuBus.get(), mainClock.get(), 12, 8, 8);
            ppu->reset(true, eventManager.get());
            altRamAddr = 0;
            for(int i = 0; i < 0x20000; ++i)        ram[i] = 0;

            mulReg_A = 0;
            mulReg_B = 0;
            mulReg_Dividend = 0;
            mulReg_Divisor = 0;
            mulReg_Quotient = 0;
            mulReg_Product = 0;
            break;

        case SnesFile::Type::Spc:
            spc->loadSpcFile(currentFile);
            break;
        }
    }

    u8 Snes::rd_Reg(u16 a, timestamp_t clk)
    {
        u8 out = 0;
        switch(a)
        {
        case 0x2180:
            out = ram[altRamAddr];
            altRamAddr++;
            altRamAddr &= 0x1FFFF;
            break;

        case 0x2140:  case 0x2141:  case 0x2142:  case 0x2143:
            spc->runTo(clk);
            out = spc->readIoReg(a&3);
            break;
            
        case 0x4214:    out = static_cast<u8>(mulReg_Quotient & 0xFF);      break;
        case 0x4215:    out = static_cast<u8>(mulReg_Quotient >> 8  );      break;
        case 0x4216:    out = static_cast<u8>(mulReg_Product  & 0xFF);      break;
        case 0x4217:    out = static_cast<u8>(mulReg_Product  >> 8  );      break;

        case 0x4218: case 0x4219: case 0x421A: case 0x421B:
        case 0x4211: case 0x4016: case 0x4017:
            // TODO
            break;

        case 0x4210:
        case 0x4212:    ppu->runTo(clk);        ppu->regRead(a, out);       break;

        default:
            if(a >= 0x2100 && a < 0x2140)
            {
                ppu->runTo(clk);
                ppu->regRead(a, out);
            }
            else if(a >= 0x4300 && a < 0x4400)
            {
                dmaUnit->read(a, out, clk);
            }
            break;
        }
        return out;
    }
    
    void Snes::wr_Reg(u16 a, u8 v, timestamp_t clk)
    {
        switch(a)
        {
        case 0x2180:
            ram[altRamAddr] = v;
            altRamAddr++;
            altRamAddr &= 0x1FFFF;
            break;
            
        case 0x2181:        altRamAddr = (altRamAddr & 0x1FF00) | v;                break;
        case 0x2182:        altRamAddr = (altRamAddr & 0x100FF) | (v << 8);         break;
        case 0x2183:        altRamAddr = (altRamAddr & 0x0FFFF) | ((v & 1) << 16);  break;

        case 0x2140:  case 0x2141:  case 0x2142:  case 0x2143:
            spc->runTo(clk);
            spc->writeIoReg(a&3, v);
            break;

        case 0x4200:
            ppu->runTo(clk);
            ppu->regWrite(a, v);
            break;

        case 0x4202:
            mulReg_A = v;
            break;

        case 0x4203:
            mulReg_B = v;
            mulReg_Product = mulReg_A * mulReg_B;
            break;

        case 0x4204:
            mulReg_Dividend = (mulReg_Dividend & 0xFF00) | v;
            break;
            
        case 0x4205:
            mulReg_Dividend = (mulReg_Dividend & 0x00FF) | (v << 8);
            break;

        case 0x4206:
            mulReg_Divisor = v;
            if(!mulReg_Divisor)     {   mulReg_Product = mulReg_Dividend;                   mulReg_Quotient = 0xFFFF;                             }
            else                    {   mulReg_Product = mulReg_Dividend % mulReg_Divisor;  mulReg_Quotient = mulReg_Dividend / mulReg_Divisor;   }
            break;

        case 0x420B: case 0x420C:
            ppu->runTo(clk);
            dmaUnit->write(a, v, clk);
            break;

        default:
            if(a >= 0x2100 && a < 0x2140)
            {
                ppu->runTo(clk);
                ppu->regWrite(a, v);
            }
            else if(a >= 0x4300 && a < 0x4400)
            {
                dmaUnit->write(a, v, clk);
            }
            break;
        }
    }


    void Snes::startCpuTrace(const char* filename)
    {
        if(!cpuTracer)
            cpuTracer = std::make_unique<CpuTracer>();
        cpuTracer->openTraceFile(filename);
        cpu->setTracer(cpuTracer.get());
    }

    void Snes::stopCpuTrace()
    {
        if(cpuTracer)
            cpuTracer->closeTraceFile();
        cpu->setTracer(nullptr);
    }

    bool Snes::isCpuTracing()
    {
        return cpuTracer && cpuTracer->isOpen();
    }
    
    void Snes::startSpcTrace(const char* filename)
    {
        if(!smpTracer)
            smpTracer = std::make_unique<SmpTracer>();
        smpTracer->openTraceFile(filename);
        spc->setTracer(smpTracer.get());
    }

    void Snes::stopSpcTrace()
    {
        if(smpTracer)
            smpTracer->closeTraceFile();
        spc->setTracer(nullptr);
    }

    bool Snes::isSpcTracing()
    {
        return smpTracer && smpTracer->isOpen();
    }

    VideoResult Snes::doFrame(const VideoSettings& vid)
    {
        VideoResult out;

        spc->setAudioBuffer(audioBuffer.get());

        switch(currentFile.type)
        {
        case SnesFile::Type::Rom:
            {
                timestamp_t target = ppu->getMaxTicksPerFrame();
                ppu->frameStart(cpu.get(), vid);

                if(isCpuTracing())
                    cpuTracer->traceLine(">>  Beginning Frame  >>");

                cpu->runTo(target);
                spc->runTo(target);
                ppu->runTo(target);

                auto adj = -ppu->getPrevFrameLength();
                mainClock->adjustTimestamp(adj);
                spc->adjustTimestamp(adj);
                eventManager->adjustTimestamps(adj);

                out = ppu->getVideoResult();
            }
            break;

        case SnesFile::Type::Spc:
            spc->runToFillAudio();
            break;

        }

        spc->setAudioBuffer(nullptr);
        return out;
    }

    int Snes::getBytesOfAudioForAFrame()
    {
        // TODO do this better
        return (32000 * 4 / 60) + 50;
    }

    int Snes::getBytesOfAudioWritten()
    {
        return audioBuffer->getBytesWritten();
    }

    void Snes::setAudioBuffer(s16* bufA, int sizInBytesA, s16* bufB, int sizInBytesB)
    {
        audioBuffer->setBuffer(bufA, sizInBytesA, bufB, sizInBytesB);
    }

    
    void Snes::debug_dumpVram(const char* filename)
    {
        ppu->debug_dumpVram(filename);
    }

    void Snes::debug_dumpWram(const char* filename)
    {
        FILE* file = fopen(filename, "wb");
        fwrite(ram.get(), 1, 0x20000, file);
        fclose(file);
    }
}

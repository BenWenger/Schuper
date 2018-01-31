
#include "snes.h"
#include "snesfile.h"
#include "apu/spc.h"
#include "cpu/cpu.h"
#include "bus/cpubus.h"
#include "cpu/cputracer.h"

namespace sch
{
    Snes::Snes()
        : ram(new u8[0x20000])
    {
        spc = std::make_unique<Spc>();
        cpu = std::make_unique<Cpu>();
        cpuBus = std::make_unique<CpuBus>();
        
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
            return SnesFile::Type::Invalid;
            // TODO
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
        if(currentFile)
        {
            cpuBus->reset();
            cpu->reset(cpuBus.get(), spdSlow);
            spc->reset();

            nmiEnabled = false;
        }
    }

    u8 Snes::rd_Reg(u16 a, timestamp_t clk)
    {
        u8 out = 0;
        switch(a)
        {
        case 0x2140:  case 0x2141:  case 0x2142:  case 0x2143:
            spc->runTo(clk);
            out = spc->readIoReg(a&3);
            break;
        }
        return out;
    }
    
    void Snes::wr_Reg(u16 a, u8 v, timestamp_t clk)
    {
        switch(a)
        {
        case 0x2140:  case 0x2141:  case 0x2142:  case 0x2143:
            spc->runTo(clk);
            spc->writeIoReg(a&3, v);
            break;

        case 0x4200:
            nmiEnabled = (v & 0x80) != 0;
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

    void Snes::doFrame()
    {
        if(!currentFile)
            return;

        //  ~21477272.7272 master cycles per second
        //    ~357954.5454 master cycles per frame
        timestamp_t frm = 357955;

        if(nmiEnabled)
            cpu->triggerNmi();

        cpu->runTo(frm);
        spc->runTo(frm);

        cpu->adjustTimestamp(-frm);
        spc->adjustTimestamp(-frm);
    }
}

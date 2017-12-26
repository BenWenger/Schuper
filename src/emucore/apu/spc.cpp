
#include <cstring>              // memcpy
#include "spc.h"
#include "snesfile.h"

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
    }

    Spc::Spc()
    {
        bus.setPkProc( &Spc::pkFunc, this );
        bus.setRdProc(   0,   0, &Spc::rdFunc_Page0, this );
        bus.setRdProc(   1, 0xE, &Spc::rdFunc      , this );
        bus.setRdProc( 0xF, 0xF, &Spc::rdFunc_PageF, this );
        bus.setWrProc(   0,   0, &Spc::wrFunc_Page0, this );
        bus.setWrProc(   1, 0xF, &Spc::wrFunc      , this );
    }

    u8 Spc::rdFunc(u16 a, timestamp_t tick)
    {
        return ram[a];
    }

    u8 Spc::rdFunc_Page0(u16 a, timestamp_t tick)
    {
        if((a & 0xFFF0) == 0x00F0)
        {
            if(a == 0xFD)       return 1;
            return 0;       // TODO reg read
        }
        else
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
        ram[a] = v;
        if((a & 0xFFF0) == 0x00F0)
        {
            // TODO reg write
        }
    }



    //////////////////////////////////////////////////////////////

    void Spc::loadSpcFile(const SnesFile& file)
    {
        if(file.type != SnesFile::Type::Spc)            return;
        if(file.memory.size() != 0x10000)               return;



        memcpy( ram, file.memory.data(), 0x10000 );
        cpu.resetWithFile( &bus, file );
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

    void Spc::runForCycs(timestamp_t cycs)
    {
        cpu.run(cycs);
    }
}
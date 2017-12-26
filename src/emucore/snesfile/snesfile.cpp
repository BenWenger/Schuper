
#include "snesfile.h"
#include <cstdio>

namespace sch
{
    namespace
    {
        bool load_Spc(SnesFile& snes, FILE* file, s32 filesize)
        {
            // SPC files must be at least 0x10180 bytes
            if(filesize < 0x10180)
                return false;

            snes.type = SnesFile::Type::Invalid;
            snes.memory.resize(0x10000);            // always 64K of memory

            // SMP regs
            fseek(file, 0x25, SEEK_SET);
            u8 buf[2];
            fread(buf, 1, 2, file);     snes.smpRegs.PC = (buf[1] << 8) | buf[0];
            fread(&snes.smpRegs.A, 1, 1, file);
            fread(&snes.smpRegs.X, 1, 1, file);
            fread(&snes.smpRegs.Y, 1, 1, file);
            fread(&snes.smpRegs.PSW, 1, 1, file);
            fread(&snes.smpRegs.SP, 1, 1, file);

            // Main RAM
            fseek(file, 0x100, SEEK_SET);
            fread(&snes.memory[0], 1, 0x10000, file);

            // DSP regs
            fread(snes.dspRegs, 1, 0x80, file);

            // done!
            snes.type = SnesFile::Type::Spc;

            return true;
        }
    }

    SnesFile::SnesFile()
    {
        type = Type::Invalid;
    }

    bool SnesFile::load(const char* path)
    {
        FILE* file = fopen(path, "rb");
        if(!file)
            return false;
        try
        {
            fseek(file, 0, SEEK_END);
            s32 filesize = ftell(file);
            fseek(file, 0, SEEK_SET);
        
            // check the header, see if this is an SPC file
            u8 hdr[11] = {};
            fread(hdr, 1, 11, file);

            bool load_ok = false;

            if(!memcmp(hdr, "SNES-SPC700", 11))
                load_ok = load_Spc(*this, file, filesize);
            // else try to load as ROM   TODO

            fclose(file);
            return load_ok;
        }
        catch(...)
        {
            fclose(file);
            throw;
        }
    }
}
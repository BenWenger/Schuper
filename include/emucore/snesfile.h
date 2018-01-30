
#ifndef SCHUPER_EMUCORE_SNESFILE_H_INCLUDED
#define SCHUPER_EMUCORE_SNESFILE_H_INCLUDED

#include "snestypes.h"
#include <vector>

namespace sch
{

    class SnesFile
    {
    public:
        SnesFile();
        SnesFile(const SnesFile&) = default;
        SnesFile(SnesFile&&);
        
        SnesFile& operator = (const SnesFile&) = default;
        SnesFile& operator = (SnesFile&&);

        ~SnesFile() = default;

        bool        load(const char* path);

        enum class Type
        {
            Invalid,
            Spc,
            Rom
        };
        enum class MemMap
        {
            Unknown,
            LoRom
        };

        Type                        type;
        MemMap                      memmap;
        std::vector<u8>             memory;
        u8                          dspRegs[0x80];
        struct SmpRegs
        {
            u16     PC;
            u8      A;
            u8      X;
            u8      Y;
            u8      PSW;
            u8      SP;
        }                           smpRegs;

        inline operator bool ()
        {
            return type != Type::Invalid;
        }

    };
}

#endif


#ifndef SCHUPER_APU_SPCBUS_H_INCLUDED
#define SCHUPER_APU_SPCBUS_H_INCLUDED

#include <functional>
#include "snestypes.h"

namespace sch
{
    class SpcBus
    {
    public:
        typedef     std::function<u8(u16,timestamp_t)>          rdproc_t;
        typedef     std::function<void(u16,u8,timestamp_t)>     wrproc_t;
        typedef     std::function<u8(u16)>                      pkproc_t;

        u8          peek(u16 a) const                           { return peeker(a);                 }
        u8          read(u16 a, timestamp_t clk)                { return readers[a>>12](a,clk);     }
        void        write(u16 a, u8 v, timestamp_t clk)         { writers[a>>12](a,v,clk);          }
        
        void        setRdProc(int pagestart, int pageend, const rdproc_t& proc);
        void        setWrProc(int pagestart, int pageend, const wrproc_t& proc);
        void        setPkProc(const pkproc_t& proc);

        template <typename T> inline void setRdProc(int pagestart, int pageend, u8 (T::*proc)(u16,timestamp_t), T* obj)
        {
            setRdProc(pagestart, pageend, std::bind(proc, obj, std::placeholders::_1, std::placeholders::_2) );
        }
        template <typename T> inline void setWrProc(int pagestart, int pageend, void (T::*proc)(u16,u8,timestamp_t), T* obj)
        {
            setWrProc(pagestart, pageend, std::bind(proc, obj, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3) );
        }
        template <typename T> inline void setPkProc(u8 (T::*proc)(u16) const, const T* obj)
        {
            setPkProc(std::bind(proc, obj, std::placeholders::_1) );
        }

        SpcBus() = default;
        SpcBus(const SpcBus&) = delete;                 // forbid copying because we own function pointers.
        SpcBus& operator = (const SpcBus&) = delete;

    private:
        rdproc_t        readers[0x10];
        wrproc_t        writers[0x10];
        pkproc_t        peeker;
    };
}

#endif

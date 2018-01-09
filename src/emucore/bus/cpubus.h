
#ifndef SCHUPER_BUS_CPUBUS_H_INCLUDED
#define SCHUPER_BUS_CPUBUS_H_INCLUDED

#include "snestypes.h"
#include <functional>

namespace sch
{
    class CpuBus
    {
    public:
        // read/write return the times consumed by the memory access  (it's added to the CPU's timestamp)
        int         read(u32 a, u8& v, timestamp_t ts);
        int         write(u32 a, u8 v, timestamp_t ts);
        u8          peek(u32 a) const;
        
        typedef     std::function<int(u32,u8&,timestamp_t)>     rd_proc;
        typedef     std::function<int(u32,u8 ,timestamp_t)>     wr_proc;
        typedef     std::function<void(u32,u8&)>                pk_proc;
        
        void        addReader(int pagestart, int pageend, const rd_proc& proc)          { reader = proc;                    }
        void        addWriter(int pagestart, int pageend, const wr_proc& proc)          { writer = proc;                    }
        void        addPeeker(int pagestart, int pageend, const pk_proc& proc)          { peeker = proc;                    }

        template <typename T> void addReader(int pagestart, int pageend, int (T::*proc)(u32,u8&,timestamp_t), T* obj)
        {   addReader(pagestart, pageend, std::bind(proc, obj, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));       }
        template <typename T> void addWriter(int pagestart, int pageend, int (T::*proc)(u32,u8 ,timestamp_t), T* obj)
        {   addWriter(pagestart, pageend, std::bind(proc, obj, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));       }
        template <typename T> void addPeeker(int pagestart, int pageend, void (T::*proc)(u32,u8&) const, const T* obj)
        {   addPeeker(pagestart, pageend, std::bind(proc, obj, std::placeholders::_1, std::placeholders::_2));                              }

    private:
        // TODO -- is it worth it to split by bank?  I kind of doubt it... 3 lookup tables of 256 entries each seems like a waste
        //   and would just lead to cache misses
        rd_proc         reader;
        wr_proc         writer;
        pk_proc         peeker;

        u8              data;
    };

    inline int CpuBus::read(u32 a, u8& v, timestamp_t ts)
    {
        int ret = reader(a, data, ts);
        v = data;
        return ret;
    }
    
    inline int CpuBus::write(u32 a, u8 v, timestamp_t ts)
    {
        data = v;
        return writer(a, v, ts);
    }

    inline u8 CpuBus::peek(u32 a) const
    {
        u8 v = 0;
        peeker(a, v);
        return v;
    }
}

#endif

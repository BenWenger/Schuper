
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
        typedef     std::function<u8(u32)>                      pk_proc;
        
        void        setReader(const rd_proc& proc)          { reader = proc;                    }
        void        setWriter(const wr_proc& proc)          { writer = proc;                    }
        void        setPeeker(const pk_proc& proc)          { peeker = proc;                    }

        template <typename T> void setReader(T* obj, int (T::*proc)(u32,u8&,timestamp_t))
        {   setReader(std::bind(proc, obj, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));       }
        template <typename T> void setWriter(T* obj, int (T::*proc)(u32,u8 ,timestamp_t))
        {   setWriter(std::bind(proc, obj, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));       }
        template <typename T> void setPeeker(const T* obj, u8 (T::*proc)(u32) const)
        {   setPeeker(std::bind(proc, obj, std::placeholders::_1));                                                     }

        void reset()
        {
            data = 0;
        }

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
        return peeker(a);
    }
}

#endif

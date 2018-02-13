
#ifndef SCHUPER_EMUCORE_TYPES_H_INCLUDED
#define SCHUPER_EMUCORE_TYPES_H_INCLUDED

#include <cstdint>
#include <limits>

namespace sch
{
    
    typedef     std::uint8_t            u8;
    typedef     std::uint16_t           u16;
    typedef     std::uint32_t           u32;
    typedef     std::uint64_t           u64;
    typedef     std::int8_t             s8;
    typedef     std::int16_t            s16;
    typedef     std::int32_t            s32;
    typedef     std::int64_t            s64;

    typedef     std::int_fast32_t       timestamp_t;


    class Time
    {
        Time() = delete;
        ~Time() = delete;

    public:
        static const timestamp_t    Never = std::numeric_limits<timestamp_t>::max();
    };

}

#endif

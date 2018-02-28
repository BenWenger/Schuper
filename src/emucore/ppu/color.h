
#ifndef SCHUPER_PPU_COLOR_H_INCLUDED
#define SCHUPER_PPU_COLOR_H_INCLUDED

namespace sch
{

    struct Color
    {
        u8      r = 0;
        u8      g = 0;
        u8      b = 0;
        u8      prio = 0;
        bool    colorMath = false;

        void    from15Bit(u16 v)
        {
            r = (v      ) & 0x1F;
            g = (v >>  5) & 0x1F;
            b = (v >> 10) & 0x1F;
            prio = 0;
            // don't change colorMath here
        }

        inline void multiplex(u8 bit, const Color* plt, u8 newprio, bool math)
        {
            if(!bit)            return;
            if(prio > newprio)  return;
            *this = plt[bit];
            prio = newprio;
            colorMath = math;
        }

        void    reset()
        {
            r = g = b = prio = 0;
            colorMath = false;
        }
    };


}


#endif

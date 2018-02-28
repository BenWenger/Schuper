
#ifndef SCHUPER_PPU_COLOR_H_INCLUDED
#define SCHUPER_PPU_COLOR_H_INCLUDED

namespace sch
{

    struct Color
    {
        s8      r = 0;              //  rgb should be signed to allow for clipping
        s8      g = 0;              //   during color math
        s8      b = 0;
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

        Color   doMath(const Color& rhs, bool subtract, bool half) const
        {
            if(!colorMath)      return *this;
            Color out;

            if(subtract)
            {
                out.r = r - rhs.r;
                out.g = g - rhs.g;
                out.b = b - rhs.b;
                if(half)
                {
                    out.r >>= 1;
                    out.g >>= 1;
                    out.b >>= 1;
                }
                if(out.r < 0)       out.r = 0;
                if(out.g < 0)       out.g = 0;
                if(out.b < 0)       out.b = 0;
            }
            else
            {
                out.r = r + rhs.r;
                out.g = g + rhs.g;
                out.b = b + rhs.b;
                if(half)
                {
                    out.r >>= 1;
                    out.g >>= 1;
                    out.b >>= 1;
                }
                if(out.r > 0x1F)    out.r = 0x1F;
                if(out.g > 0x1F)    out.g = 0x1F;
                if(out.b > 0x1F)    out.b = 0x1F;
            }

            return out;
        }
    };


}


#endif

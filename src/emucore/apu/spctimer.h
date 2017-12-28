
#ifndef SCHUPER_APU_SPCTIMER_H_INCLUDED
#define SCHUPER_APU_SPCTIMER_H_INCLUDED

#include "snestypes.h"

namespace sch
{
    class SpcTimer
    {
    public:
        void    writeEnableBit(bool en)
        {
            if(!enabled && en)
            {
                counter = output = 0;
            }
            enabled = en;
        }

        void    writeTarget(u8 v)
        {
            target = v;
        }

        u8      readOutput()
        {
            u8 out = (output & 0x0F);
            output = 0;
            return out;
        }

        void    clock()
        {
            if(enabled)
            {
                ++counter;
                if(counter == target)
                    ++output;
            }
        }

        void    hardReset()
        {
            enabled = false;
            counter = 0;
            target = 0;
            output = 0;
        }

        SpcTimer()
        {
            hardReset();
        }


    private:
        bool    enabled;
        u8      counter;
        u8      target;
        u8      output;
    };

}

#endif

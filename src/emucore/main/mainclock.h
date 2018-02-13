
#ifndef SCHPUER_MAIN_MAINCLOCK_H_INCLUDED
#define SCHPUER_MAIN_MAINCLOCK_H_INCLUDED

#include "snestypes.h"

namespace sch
{

    class MainClock
    {
    public:
        inline void     ioCyc()         {   tallyCycle(6);      }
        inline void     fastCyc()       {   tallyCycle(6);      }
        inline void     slowCyc()       {   tallyCycle(8);      }
        inline void     xslowCyc()      {   tallyCycle(12);     }

        inline void     tallyCycle(int cycs)
    };

}

#endif

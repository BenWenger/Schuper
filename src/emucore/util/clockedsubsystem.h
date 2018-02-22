
#ifndef SCHUPER_UTIL_CLOCKEDSUBSYSTEM_H_INCLUDED
#define SCHUPER_UTIL_CLOCKEDSUBSYSTEM_H_INCLUDED

// This class is kind of dumb.  Maybe get rid of it?

#include "snestypes.h"

namespace sch
{
    class ClockedSubsystem
    {
    public:
        virtual             ~ClockedSubsystem()       {}

        virtual void        runTo(timestamp_t runto)
        {
            if(runto > curTimestamp)
            {
                timestamp_t cycs = (runto - curTimestamp) / clockBase;

                runFor(cycs);
            }
        }

        inline void         setClockBase(timestamp_t base)                  { clockBase = base;         }
        inline timestamp_t  getClockBase() const                            { return clockBase;         }
        inline void         adjustTimestamp(timestamp_t adj)                { curTimestamp += adj;      }
        inline void         forciblySetTimestamp(timestamp_t ts)            { curTimestamp = ts;        }
        inline timestamp_t  getTick() const                                 { return curTimestamp;      }

    protected:
        virtual void        runFor(timestamp_t ticks)
        {
            cyc(ticks);
        }

        inline timestamp_t  cyc()           { return curTimestamp += clockBase;             }
        inline timestamp_t  cyc(int cycs)   { return curTimestamp += (clockBase * cycs);    }

    private:
        timestamp_t             curTimestamp;
        timestamp_t             clockBase;
    };

}

#endif
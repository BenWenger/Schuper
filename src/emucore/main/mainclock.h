
#ifndef SCHPUER_MAIN_MAINCLOCK_H_INCLUDED
#define SCHPUER_MAIN_MAINCLOCK_H_INCLUDED

#include <algorithm>  // for min/max
#include "snestypes.h"
#include "../event/eventmanager.h"
#include "internaldebug/internaldebug.h"

namespace sch
{
    class EventManager;

    class MainClock
    {
    public:
        MainClock()
        {
#ifdef IDBG_ENABLED
            InternalDebug.getMainClock = std::bind(&MainClock::getTick, this);
#endif
        }


        inline void     ioCyc()             { tallyCycle(6);        }
        inline void     fastCyc()           { tallyCycle(6);        }
        inline void     slowCyc()           { tallyCycle(8);        }
        inline void     xslowCyc()          { tallyCycle(12);       }

        inline void     tallyCycle(int cycs)
        {
            timestamp += cycs;
            events->processEvents(timestamp);
        }

        inline timestamp_t  getTick() const { return timestamp;     }
        inline void         adjustTimestamp(timestamp_t adj)
        {
            timestamp += adj;
        }

        void reset(EventManager* evt)
        {
            events = evt;
            timestamp = 0;
        }

        void runToNextEvent(timestamp_t max)
        {
            timestamp_t x = std::min(max, events->getNextEventTime());
            timestamp = std::max(timestamp, x);
            events->processEvents(timestamp);
        }

    private:
        EventManager*   events;
        timestamp_t     timestamp;
    };

}

#endif

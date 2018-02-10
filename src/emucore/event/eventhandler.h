
#ifndef SCHPUER_EVENT_EVENTHANDLER_H_INCLUDED
#define SCHPUER_EVENT_EVENTHANDLER_H_INCLUDED

#include "snestypes.h"

namespace sch
{

    class EventHandler
    {
    public:
        virtual ~EventHandler() {}

        virtual timestamp_t     performEvent(int eventId, timestamp_t clk) = 0;
    };

}


#endif
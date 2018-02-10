
#include "eventmanager.h"
#include "eventhandler.h"
#include "cpu/cpu.h"

namespace sch
{

    EventManager::EventManager(Cpu* c)
    {
        nextEvent = Time::Never;
        cpu = c;
    }
    
    void EventManager::addEvent(timestamp_t clk, EventHandler* evt, int id)
    {
        events.emplace(clk,evt, id);
        if(clk < nextEvent)
            nextEvent = clk;
    }

    void EventManager::doEvents(timestamp_t clk)
    {
        timestamp_t origclk = clk;

        while(!events.empty())
        {
            auto x = *events.begin();
            if(clk > x.time)
                break;

            events.erase(events.begin());

            clk = x.evt->performEvent(x.id, clk);
        }

        if(events.empty())
            nextEvent = Time::Never;
        else
            nextEvent = events.begin()->time;

        cpu->adjustTimestamp(clk - origclk);
    }
}

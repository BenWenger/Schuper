
#include "eventmanager.h"
#include "eventhandler.h"
#include "main/mainclock.h"

namespace sch
{

    EventManager::EventManager()
    {
        nextEvent = Time::Never;
    }

    void EventManager::reset()
    {
        events.clear();
        vblNotifiers.clear();
        nextEvent = Time::Never;
    }
    
    void EventManager::addEvent(timestamp_t clk, EventHandler* evt, int id)
    {
        events.emplace(clk,evt, id);
        if(clk < nextEvent)
            nextEvent = clk;
    }

    void EventManager::doEvents(timestamp_t clk)
    {
        // This routine has to be re-entrant, since an event may update the clock which 
        //   may call this function again!  Therefore I cannot hold onto iterators,
        //   and I have to remove events from the list BEFORE actually performing them.
        while(!events.empty())
        {
            auto x = *events.begin();
            if(clk < x.time)
                break;

            events.erase(events.begin());

            x.evt->performEvent(x.id, clk);
        }

        if(events.empty())
            nextEvent = Time::Never;
        else
            nextEvent = events.begin()->time;
    }
    
    void EventManager::adjustTimestamps(timestamp_t adj)
    {
        if(events.empty())      return;

        std::set<EventBlock>    newevents;
        for(auto& i : events)
        {
            newevents.insert( EventBlock(i.time + adj, i.evt, i.id) );
        }

        events = std::move(newevents);
        nextEvent = events.begin()->time;
    }
    
    void EventManager::vblankStarted(timestamp_t clk)
    {
        for(auto& i : vblNotifiers)
            i->vblankStart(clk);
    }

    void EventManager::addVBlankNotification(EventHandler* hndlr)
    {
        vblNotifiers.insert(hndlr);
    }
}

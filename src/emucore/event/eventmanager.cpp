
#include "eventmanager.h"
#include "eventhandler.h"
#include "main/mainclock.h"
#include "ppu/ppu.h"

namespace sch
{

    EventManager::EventManager()
    {
        nextEvent = Time::Never;
    }

    void EventManager::reset(Ppu* p)
    {
        ppu = p;
        events.clear();
        vblNotifiers.clear();
        nextEvent = Time::Never;
        lineCutoff = 0;
    }
    
    void EventManager::addEvent(timestamp_t clk, EventHandler* evt, int id)
    {
        if(clk != Time::Never)
        {
            events.emplace(clk,evt, id);
            if(clk < nextEvent)
                nextEvent = clk;
        }
    }

    void EventManager::addEvent(int H, int V, EventHandler* evt, int id)
    {
        if(V >= lineCutoff)
        {
            auto clk = ppu->convertHVToTimestamp(H,V);
            if(clk != Time::Never)
                addEvent( clk + 4, evt, id );       // +4 so we run THROUGH the dot and not just TO the dot
        }
        else
            eventsAfterV0.emplace_back(H, V, evt, id);
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
        vblNotifiers.push_back(hndlr);
    }

    void EventManager::setLineCutoff(int co)
    {
        lineCutoff = co;
        if(!eventsAfterV0.empty())
        {
            decltype(eventsAfterV0)     tmp;

            for(auto& i : eventsAfterV0)
            {
                if(i.V >= lineCutoff)       addEvent( ppu->convertHVToTimestamp(i.H,i.V), i.evt, i.id );
                else                        tmp.push_back(i);
            }

            eventsAfterV0.swap(tmp);
        }
    }
}

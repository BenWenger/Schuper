
#ifndef SCHPUER_EVENT_EVENTMANAGER_H_INCLUDED
#define SCHPUER_EVENT_EVENTMANAGER_H_INCLUDED

#include <set>
#include "snestypes.h"

namespace sch
{
    class EventHandler;

    class EventManager
    {
    public:
        EventManager();

        inline void processEvents(timestamp_t clk)
        {
            if(clk < nextEvent)         return;
            doEvents(clk);
        }

        void        addEvent(timestamp_t clk, EventHandler* evt, int id);
        void        adjustTimestamps(timestamp_t adj);

        void        reset();

        timestamp_t getNextEventTime() const    { return nextEvent;        }

        void        vblankStarted(timestamp_t clk);
        void        addVBlankNotification(EventHandler* hndlr);

    private:
        void        doEvents(timestamp_t clk);

        struct EventBlock
        {
            timestamp_t     time;
            EventHandler*   evt;
            int             id;

            inline bool operator < (const EventBlock& rhs) const
            {
                if(time < rhs.time)     return true;
                if(rhs.time < time)     return false;
                if(evt < rhs.evt)       return true;
                if(rhs.evt < evt)       return false;
                return id < rhs.id;
            }

            EventBlock(timestamp_t t, EventHandler* e, int i) : time(t), evt(e), id(i) {}
        };

        timestamp_t             nextEvent;
        std::set<EventBlock>    events;
        std::set<EventHandler*> vblNotifiers;
    };
}

/*
Timing notes:

Frame:      262 scanlines  (usually)
            263 scanlines  (even frames when in interlace mode)
            
            scanline 0 is "prerender"
            lines 1-224  are rendered lines
            1-239 are rendered if in overscan mode

            lines 225 (or 240) to end of frame are "VBlank"
            V=0, H=0 is end of vblank
            
            if overscan disabled between lines 225-239, rendering immediately disabled
            and NMI/Vblank stuff starts at H=0 of next scanline
            
            IRQs:
                just X:     every scanline (including vblank????) at H=x+3.5
                just Y:     V=y, H=x+2.5
                X&Y:        V=y, H=x+3.5
            
            
Lines:
            Each line is 340 dots
            Each dot is 4 master cycles.  Except for...
                323 and 327, which are 6 cycles.
                
            Dots 22-227 output pixels
            Dots 274-1 are HBlank
            
            
HDMA:
            All HDMA channels are deactivated at start of VBlank
            HDMA consumes 18 cycles every scanline if ANY channel active, even if there's no xfer
            Each active channel consumes 8 cycles each scanline (even if no xfer)
            +16 cycles if new indirect addr is required
            +8 cycles for each byte xfered
            
            HDMA starts at dot 278 of each rendered scanline
            
            at V=0, H=6, HDMA consumes 18 cycles + 8 for each channel for initialization
            +16 more for each channel with indirect addr
            
            
Auto Joy Read:
            Doesn't seem to steal any cycles.  Don't worry about it for now
            
WRAM refresh:
            This steals some cycles but it's not worth worrying about
            
WAI:
            ending WAI takes 12 cycles (2 IO cycs), after which the NMI/IRQ happens immediately.




*/

#endif

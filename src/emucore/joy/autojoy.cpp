
#include "autojoy.h"
#include "bus/cpubus.h"
#include "event/eventmanager.h"

namespace sch
{
    void AutoJoy::reset(EventManager* evt, CpuBus* cpubus)
    {
        bus = cpubus;
        evtManager = evt;
        enabled = false;
        active = false;
        evt->addVBlankNotification(this);
    }

    void AutoJoy::write_4200(u8 v)
    {
        enabled = (v & 0x01) != 0;
        if(!enabled)
            active = false;
    }

    void AutoJoy::read_4212(u8& v)
    {
        if(active)      v |= 1;
        else            v &= ~1;
    }

    u8 AutoJoy::read_joydata(int a)
    {
        u16 v = data[(a>>1) & 3];
        if(a & 1)       return static_cast<u8>(v >> 8);
        else            return static_cast<u8>(v & 0xFF);
    }

    void AutoJoy::performEvent(int eventId, timestamp_t clk)
    {
        if(!active)     return;

        u8 v;

        switch(eventId)
        {
            case EvtType::Clear4016:
                bus->write(0x4016, 0, clk);
                break;

            case EvtType::Read4016:
                data[0] <<= 1;
                data[2] <<= 1;

                bus->read(0x4016, v, clk);
                if(v & 0x01)    data[0] |= 1;
                if(v & 0x02)    data[2] |= 1;
                break;

            case Read4017_And_Finalize:
                active = false;
                //  no break!  flow into Read4017
            case EvtType::Read4017:
                data[1] <<= 1;
                data[3] <<= 1;

                bus->read(0x4017, v, clk);
                if(v & 0x01)    data[1] |= 1;
                if(v & 0x02)    data[3] |= 1;
                break;
        }
    }

    void AutoJoy::vblankStart(timestamp_t clk)
    {
        // Auto joy read apparently takes ~3 scanlines
        //    That is 341*3*4 master cycles.  In that time, we need to do the following:
        // - write 1 to 4016  (assume this happens now, at start of vbl)
        // - write 0 to 4016
        // - read $4016
        // - read $4017
        //      (repeat the last two 16 times)
        //
        // So this is 34 actions (minus the one we're doing now), spread across 3 scanlines

        constexpr timestamp_t timegap = (341*3*4) / 33;         // master cycles between each "action"

        //  Do the first action (1 -> 4016)
        if(enabled)
        {
            active = true;
            bus->write(0x4016, 1, clk);
        }
        
        int id;
        for(int i = 1; i < 34; ++i)
        {
            if(i == 1)          id = EvtType::Clear4016;
            else if(i == 33)    id = EvtType::Read4017_And_Finalize;
            else if(i & 1)      id = EvtType::Read4017;
            else                id = EvtType::Read4016;

            evtManager->addEvent(clk + (timegap * i), this, id);
        }
    }
}

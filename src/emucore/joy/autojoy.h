
#ifndef SCHPUER_JOY_AUTOJOY_H_INCLUDED
#define SCHPUER_JOY_AUTOJOY_H_INCLUDED

#include "event/eventhandler.h"

namespace sch
{
    class EventManager;
    class CpuBus;

    class AutoJoy : public EventHandler
    {
    public:
        void            reset(EventManager* evt, CpuBus* cpubus);
        void            write_4200(u8 v);
        void            read_4212(u8& v);
        u8              read_joydata(int a);

        virtual void    performEvent(int eventId, timestamp_t clk) override;
        virtual void    vblankStart(timestamp_t clk) override;

    private:
        CpuBus*         bus;
        EventManager*   evtManager;
        bool            enabled;
        bool            active;
        u16             data[4];

        enum EvtType
        {
            Clear4016,
            Read4016,
            Read4017,
            Read4017_And_Finalize
        };
    };

}

#endif

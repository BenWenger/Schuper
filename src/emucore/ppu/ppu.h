
#ifndef SCHUPER_PPU_PPU_H_INCLUDED
#define SCHUPER_PPU_PPU_H_INCLUDED

#include "snestypes.h"
#include "bglayer.h"
#include "color.h"
#include "event/eventhandler.h"

namespace sch
{

    class Ppu : public EventHandler
    {

    public:
        void        regWrite(u16 a, u8 v);
        void        regRead(u16 a, u8& v);

        void        runTo(timestamp_t runto);
        void        adjustTimestamp(timestamp_t adj);

        void        reset(bool hard);
        
        void        performEvent(int eventId, timestamp_t clk) override;

    private:
        BgLayer     bgLayers[4];

        // 2100
        bool        forceBlank;
        int         brightness;

        // 2101 - 2104
            // TODO OAM stuff

        // 2105
        int         bgMode;
        bool        mode1AltPriority;

        u8          scrollRegPrev;

        // 2115
        bool        addrIncOnHigh;
        unsigned    addrInc;
        int         vramRemapMode;
        u16         getEffectiveVramAddr() const;

        // 2116, 7
        u16         vramAddr;
        u16         vram[0x8000];

        // 2121
        u16         cgAddr;
        u16         cgRegPrev;
        bool        cgRegToggle;
        Color       cgRam[0x100];

        // 212C, D
        u8          manScrLayers;
        u8          subScrLayers;



    };

}

#endif
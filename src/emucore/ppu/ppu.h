
#ifndef SCHUPER_PPU_PPU_H_INCLUDED
#define SCHUPER_PPU_PPU_H_INCLUDED

#include "snestypes.h"
#include "bglayer.h"
#include "color.h"
#include "event/eventhandler.h"

/*
    Some notes on frame structure.

    One "frame" is 262 "scanlines".  Each scanline is 340 "dots"
    Each "dot" is 4 master cycles.

    With some oddities:
    - 2 dots are actually 6 cycles long instead of 4 (effectively making
        the scanline 341 dots long, even though there are only 340 dots)
    - when in interlace mode, even frames are 263 scanlines long

    This means that a full frame is [usually] 4*341*262 =   357,368 master cycles

    The "scanline" and "dot" position are represented as 'V' and 'H' respectively.
    So V=5, H=10 is dot 10 of scanline 5.  These V/H values can trigger mid-frame
    IRQs if enabled, and also can be read back by the CPU.  They also have other
    implications for coordinating timing.

    The PPU does different things for different scanlines:

    V=0:            "pre-render" - technically considered rendering, but no pixels are drawn
    V=1 to V=224:   "render" - pixels are actually being output
    V=225 to V=239: "overscan" - pixels are rendered only if overscan is enabled
    V=240 to V=261: VBlank (game can do all its drawing and PPU communication stuff here)
    V=262:          VBlank, but this line only exists on even frames in interlace mode

    Overscan lines are render-time, but only if overscan is enabled. Otherwise, those lines are
    part of VBlank.

    If enabled, NMI is triggered when VBlank starts. Normally this is V=225, but will be V=240
    if overscan is enabled.



    Now... you might think the most logical thing would be to have the frame start at V=0.  But
    this doesn't work well because to emulate on a frame-by-frame basis...


    TODO - explain why frame needs to start at V=241


*/

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
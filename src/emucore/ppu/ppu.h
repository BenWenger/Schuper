
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
    this doesn't work well because we want to emulate on a frame-by-frame basis, and there are
    some hiccups with that:

    - We can't stop the frame at any CPU cycle.  We can only stop between instructions
    - We also can't stop in the middle of a DMA.
    - For those reasons, there is going to be "spillover" and the CPU is going to run past the
        end of the frame.
    - This is accompanied with reads/writes after the end of the frame, which could have
        an impact on PPU behavior of the next frame!


    Basically we don't want anything of significance to the PPU to happen after the end of the frame.
    But that's unavoidable.  So instead, we want anything that happens we don't want to interfere
    with rendering the next frame.  Which means we want the frame to start with as much VBlank time
    as possible, so that any spillover won't change anything.

    This is complicated by the fact that VBlank starts at different times (depending on whether
    overscan is enabled), and that the frame itself is of variable length (depending on whether
    interlace mode is enabled).

    So I'm starting the frame at V=241!

    --frame start--
    V=241-261           VBlank
    V=262               More VBlank, but skipped unless interlace yadda yadda
    V=0                 Pre-render
    V=1-224             Render
    V=225-239           VBlank or Render, depending on overscan     \____  NMI happens somewhere in here
    V=240               VBlank                                      /

    It is still technically possible for DMA to spill into rendering of the next frame (DMAs can
    be VERY long), but this should be "good enough" for all actual games.
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
        BgLayer         bgLayers[4];
        timestamp_t     curTick;
        int             curScanline;

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

        
        // 2133
        bool        mode7ExtraBg;
        bool        hiResMode;
        bool        overscanMode;
        bool        interlaceObjects;
        bool        interlaceMode;

        // 4200
        bool        nmiEnabled;
        enum class IrqMode
        {
            Disabled,
            H,
            V,
            HV
        }           irqMode;
        bool        irqPending;

        // 4210
        bool        nmiReadFlag;

        // 4212
        u8          ppuStatus;


    };

}

#endif
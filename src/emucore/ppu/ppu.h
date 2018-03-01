
#ifndef SCHUPER_PPU_PPU_H_INCLUDED
#define SCHUPER_PPU_PPU_H_INCLUDED

#include "snestypes.h"
#include "bglayer.h"
#include "color.h"
#include "event/eventhandler.h"
#include "videosettings.h"
#include "event/eventmanager.h"

/*
    Some notes on frame structure.

    One "frame" is 262 "scanlines".  Each scanline is 340 "dots"
    Each "dot" is 4 master cycles.

    With some oddities:
    a) 2 dots are actually 6 cycles long instead of 4 (effectively making
        the scanline 341 dots long, even though there are only 340 dots)
        (dots 323 and 327 are the long ones)
    b) when in interlace mode, even frames are 263 scanlines long
    c) when NOT in interlace mode, on odd frames, scanline 240 is 4 cycles shorter.
        This is for a weird NTSC sync thing -- I'm completely ignoring this in
        my emulator. There's no way those 4 cycles are going to make a
        difference, and implementing this is a big complication.

    ****  technically, 'a' and 'c' above are related. the 4 cycles lost on that scanline
        are because the "long" dots run at normal speed for that scanline, but again I'm
        fudging both of these a bit  ****

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



    Note agian that there are only 340 dots per line and that two dots are 50% longer than the others.
    To simplify this, I'm just going to emulate 341 dots per line all at the same length.  This will
    throw off IRQ timing *VERY SLIGHTLY* but I doubt it will matter.
*/

namespace sch
{
    class Cpu;

    class Ppu : public EventHandler
    {

    public:
                    Ppu();
        void        regWrite(u16 a, u8 v);
        void        regRead(u16 a, u8& v);

        void        runTo(timestamp_t runto);
        void        reset(bool hard, EventManager* evt);        
        void        performEvent(int eventId, timestamp_t clk) override;
        void        frameStart(Cpu* c, const VideoSettings& vid);

        timestamp_t getMaxTicksPerFrame() const
        {
            return 341 * 263 * 4;       // TODO - move this '4' somewhere?
        }

        timestamp_t getPrevFrameLength() const
        {
            return vEnd_time;
        }

        VideoResult getVideoResult() const;

    private:
        struct Coord
        {
            int H;
            int V;

            int compare(const Coord& rhs) const {
                if(V < rhs.V)       return -1;
                if(V > rhs.V)       return 1;
                if(H < rhs.H)       return -1;
                if(H > rhs.H)       return 1;
                return 0;
            }
            bool operator <  (const Coord& rhs) const { return compare(rhs) <  0;    }
            bool operator <= (const Coord& rhs) const { return compare(rhs) <= 0;    }
            bool operator >  (const Coord& rhs) const { return compare(rhs) >  0;    }
            bool operator >= (const Coord& rhs) const { return compare(rhs) >= 0;    }
            bool operator == (const Coord& rhs) const { return (V == rhs.V) && (H == rhs.H);    }
            bool operator != (const Coord& rhs) const { return !(*this == rhs);                 }
        };


        Coord getCoordFromTimestamp(timestamp_t t);
        int   advance(timestamp_t cycs);
        bool  frameIsOver() const;
        void  doScanline(int line);
        void  renderLine(int line);
        u32   getRawColor(const Color& clr);
        void  outputLinePixels();

        Cpu*            cpu;        // need a pointer so we can signal interrupts
        EventManager*   evtManager;

        VideoSettings   video;
        int             linesRendered;

        BgLayer         bgLayers[4];
        timestamp_t     curTick;
        bool            oddFrame;               // toggles every frame
        timestamp_t     v0_time;                // the timestamp at which V=0, H=0 happened
                                                //     or Time::Never if that hasn't happened yet
                                                // this is useful for calculating V/H coords based on timestamps
        timestamp_t     vEnd_time;              // the timestamp at which the "frame" actually ended
        bool            nmiHasHappened;         // Whether or not NMI has already happened this frame
        
        Coord           curPos;                 // current position of rendering (should match 'curTick')
        Coord           irqPos;                 //  next IRQ position

        Color           renderBufMan[256+32];
        Color           renderBufSub[256+32];

        // 2100
        bool        forceBlank;
        int         brightness_4bit;
        int         brightnessMultiplier;

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

        enum WindowMode
        {
            Never = 0,
            Outside = 1,
            Inside = 2,
            Always = 3
        };

        // 2130
        WindowMode  colorWindowClip;
        WindowMode  colorWindowStopMath;
        bool        colorMathSubscr;
        bool        direct256;

        // 2131
        bool        colorMathSubtract;
        bool        colorHalfMath;
        u8          colorMathLayers;

        // 2132
        Color       fixedColor;
        
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


        enum EventCode
        {
            CatchUp = 0,
        };

        ////////////////////////////////////////
        void    bgLine_normal(int bg, int line, int planes, const Color* palette, u8 loprio, u8 hiprio);

        void    renderPixelsToBuf(Color* mainbuf, Color* subbuf, int planes, u16 addr, const Color* palette, bool hflip, u8 prio, bool math);
    };

}

#endif

#ifndef SCHUPER_EMUCORE_VIDEOSETTINGS_H_INCLUDED
#define SCHUPER_EMUCORE_VIDEOSETTINGS_H_INCLUDED

#include "snestypes.h"

namespace sch
{
    struct VideoSettings
    {
        u32*        buffer = nullptr;
        int         pitch;              // pitch in u32s
        int         r_shift;
        int         g_shift;
        int         b_shift;
        u32         alpha_or;
    };

    struct VideoResult
    {
        enum class RenderMode
        {
            Progressive,
            InterlaceEven,
            InterlaceOdd,
            None
        };

        RenderMode          mode = RenderMode::None;
        int                 lines = 0;
    };
}


#endif

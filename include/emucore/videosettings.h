
#ifndef SCHUPER_EMUCORE_VIDEOSETTINGS_H_INCLUDED
#define SCHUPER_EMUCORE_VIDEOSETTINGS_H_INCLUDED

#include "snestypes.h"

namespace sch
{
    struct VideoSettings
    {
        u32*        buffer;
        int         pitch;
        int         r_shift;
        int         g_shift;
        int         b_shift;
        u32         alpha_or;
    };
    
    enum class RenderMode
    {
        Progressive,
        InterlaceEven,
        InterlaceOdd
    };
}


#endif

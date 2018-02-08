
#ifndef SCHUPER_PPU_PPU_H_INCLUDED
#define SCHUPER_PPU_PPU_H_INCLUDED

#include "snestypes.h"

namespace sch
{

    class Ppu
    {

    private:
        // 2100
        bool        forceBlank;
        int         brightness;

        // 2101 - 2104
            // TODO OAM stuff

        // 2105
        int         bgMode;
        bool        mode1AltPriority;
        bool        bgPriority[4];

        // 2106 - TODO Mosaic

        // 2107 - 210C
        int         bgTileMapAddr[4];
        int         bgTileMapX


    };

}

#endif
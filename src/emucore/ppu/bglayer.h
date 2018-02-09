
#ifndef SCHUPER_PPU_BGLAYER_H_INCLUDED
#define SCHUPER_PPU_BGLAYER_H_INCLUDED

namespace sch
{

    class BgLayer
    {
    public:
        bool        use16Tiles;
        bool        priorityBit;
        unsigned    tileMapAddr;
        unsigned    tileMapXOverflow;
        unsigned    tileMapYOverflow;
        unsigned    chrAddr;
        
        unsigned    scrollX;
        unsigned    scrollY;

    };


}


#endif

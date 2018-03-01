
#ifndef SCHUPER_PPU_SPRITE_H_INCLUDED
#define SCHUPER_PPU_SPRITE_H_INCLUDED

namespace sch
{
    
    class Sprite
    {
    public:
        int     xpos;
        int     ypos;
        u8      tile;
        u8      plt;
        u8      prio;
        bool    hflip;
        bool    vflip;
        bool    size;
        bool    name;

        void    reset();
    };
}


#endif

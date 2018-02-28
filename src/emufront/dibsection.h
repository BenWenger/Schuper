
#ifndef DIBSECTION_H_INCLUDED
#define DIBSECTION_H_INCLUDED

#include "snes.h"

class DibSection
{
public:
    DibSection();
    ~DibSection();
    
    DibSection(const DibSection& rhs) = delete;
    DibSection& operator = (const DibSection& rhs) = delete;

    void            supplyPixels(const sch::u32* src, const sch::VideoResult& vidresult);
    operator HDC () { return mdc;       }

private:
    sch::u32*       pixels;
    HBITMAP         dib;
    HBITMAP         old;
    HDC             mdc;
};


#endif

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include "dibsection.h"

namespace
{
    void fillPixels_prog(sch::u32* dst, const sch::u32* src, int lines)
    {
        for(int i = 0; i < lines; ++i)
        {
            std::copy(src, src+512, dst);
            std::copy(src, src+512, dst+512);
            src += 512;
            dst += 1024;
        }
    }
    
    void fillPixels_intr(sch::u32* dst, const sch::u32* src, int lines)
    {
        for(int i = 0; i < lines; ++i)
        {
            std::copy(src, src+512, dst);
            src += 512;
            dst += 1024;
        }
    }
}


DibSection::DibSection()
{
    BITMAPINFO      bi = {};
    bi.bmiHeader.biSize =           sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth =          512;
    bi.bmiHeader.biHeight =         -480;
    bi.bmiHeader.biBitCount =       32;
    bi.bmiHeader.biPlanes =         1;
    bi.bmiHeader.biCompression =    BI_RGB;

    mdc = CreateCompatibleDC(nullptr);
    dib = CreateDIBSection(mdc, &bi, DIB_RGB_COLORS, (void**)(&pixels), nullptr, 0);

    old = (HBITMAP)SelectObject(mdc, dib);
}

DibSection::~DibSection()
{
}

void DibSection::supplyPixels(const sch::u32* src, const sch::VideoResult& vidresult)
{
    switch(vidresult.mode)
    {
    case sch::VideoResult::RenderMode::Progressive:     fillPixels_prog(pixels, src, vidresult.lines);          break;
    case sch::VideoResult::RenderMode::InterlaceEven:   fillPixels_intr(pixels, src, vidresult.lines);          break;
    case sch::VideoResult::RenderMode::InterlaceOdd:    fillPixels_intr(pixels + 512, src, vidresult.lines);    break;
    }
}
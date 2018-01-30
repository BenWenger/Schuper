
#include "snes.h"
#include <Windows.h>

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
    char path[MAX_PATH] = "";
    OPENFILENAMEA ofn = {};
    ofn.Flags =             OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
    ofn.lpstrFilter =       "Sfc Files\0*.sfc\0All Files\0*\0\0";
    ofn.lpstrFile =         path;
    ofn.lStructSize =       sizeof(ofn);
    ofn.nFilterIndex =      1;
    ofn.nMaxFile =          MAX_PATH;
    ofn.hInstance =         inst;

    if(!GetOpenFileNameA(&ofn))
        return 1;


    sch::SnesFile file;
    if(!file.load(path))
        return 2;

    sch::Snes snes;
    if(snes.loadFile(std::move(file)) != sch::SnesFile::Type::Rom)
        return 3;

    snes.startCpuTrace("schuper_65816_trace.txt");

    for(int i = 0; i < 10; ++i) {
        snes.doFrame();
    }

    return 0;
}

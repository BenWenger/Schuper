
#include "tempspc.h"
#include <Windows.h>

int WINAPI WinMain(HINSTANCE prev, HINSTANCE inst, LPSTR cmd, int show)
{
    char path[MAX_PATH] = "";
    OPENFILENAMEA ofn = {};
    ofn.Flags =             OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
    ofn.lpstrFilter =       "Spc Files\0*.spc\0All Files\0*\0\0";
    ofn.lpstrFile =         path;
    ofn.lStructSize =       sizeof(ofn);
    ofn.nFilterIndex =      1;
    ofn.nMaxFile =          MAX_PATH;
    ofn.hInstance =         inst;

    if(!GetOpenFileNameA(&ofn))
        return 1;

    if(!sch::doSpcTrace(path, "schuper_trace.txt", 600000))
        return 1;

    return 0;
}

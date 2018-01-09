
#include <Windows.h>
#include <soundout.h>
#include "tempspc.h"

namespace
{
    bool runApp = true;
    bool loaded = false;
    char path[MAX_PATH] = "";
    SoundOut*       snd = nullptr;
    sch::TempSpc    spc;

    void textOut(HDC dc, int x, int y, const char* txt)
    {
        TextOutA(dc,x,y,txt, strlen(txt));
    }

    const char* const wndClassName = "Temp Spc Player";
}

void fillAudio()
{
    auto lk = snd->lock();

    auto written = spc.generateAudio(
        lk.getBuffer<sch::s16>(0),
        lk.getSize(0),
        lk.getBuffer<sch::s16>(1),
        lk.getSize(1)
    );

    lk.setWritten(written);
}

void doLoad(HWND wnd)
{
    if(loaded)
        snd->stop(false);

    OPENFILENAMEA ofn = {};
    ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
    ofn.hwndOwner = wnd;
    ofn.lpstrFile = path;
    ofn.lpstrFilter = "Spc Files\0*.spc\0All Files\0*\0\0";
    ofn.lStructSize = sizeof(ofn);
    ofn.nFilterIndex = 1;
    ofn.nMaxFile = MAX_PATH;

    if(!GetOpenFileName(&ofn))
    {
        if(loaded)
            snd->play();
    }
    else
    {
        snd->stop(true);
        loaded = spc.loadFile(path);

        if(loaded)
        {
            fillAudio();
            snd->play();
        }
    }
}

void doPaint(HWND wnd)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(wnd,&ps);

    if(loaded)
        textOut(dc, 20, 20, path);
    else
        textOut(dc, 20, 20, "(No file loaded)");

    EndPaint(wnd,&ps);
}

LRESULT CALLBACK wndProc(HWND wnd, UINT msg, WPARAM w, LPARAM l)
{
    switch(msg)
    {
    case WM_PAINT:
        doPaint(wnd);
        break;

    case WM_SYSCOMMAND:
        if((w & 0xFFF0) == SC_CLOSE)
        {
            runApp = false;
        }
        break;

    case WM_KEYDOWN:
        if(w == VK_ESCAPE)
        {
            doLoad(wnd);
            InvalidateRect(wnd, nullptr, true);
        }
        break;
    }
    return DefWindowProc(wnd,msg,w,l);
}

int WINAPI WinMain(HINSTANCE prev, HINSTANCE inst, LPSTR cmd, int show)
{
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.hCursor = LoadCursor(inst, IDC_ARROW);
    wc.hInstance = inst;
    wc.lpfnWndProc = &wndProc;
    wc.lpszClassName = wndClassName;

    if(!RegisterClassExA(&wc))
        return 1;

    HWND wnd = CreateWindowExA( 0, wndClassName, "Temp SPC Player", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 50, 50, 640, 480, NULL, NULL, inst, nullptr );
    if(!wnd)
        return 2;

    SoundOut sndObj(32000, true, 500);
    sndObj.stop(true);
    snd = &sndObj;
    

    MSG msg;

    while(runApp)
    {
        while(PeekMessage(&msg, wnd, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if(loaded && (snd->canWrite() >= 4000))
        {
            fillAudio();
        }
        else
        {
            Sleep(10);
        }
    }

    return 0;
}

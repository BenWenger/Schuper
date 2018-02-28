
#ifdef UNICODE
#undef UNICODE
#endif

#define NOMINMAX
#include <Windows.h>
#include "soundout.h"
#include "snes.h"
#include "dibsection.h"

namespace
{
    const char* const       wndClassName = "Schuper_Window_Class";
    
    const char* const       cpuTraceFileName = "Schuper_cpu_trace.txt";
    const char* const       spcTraceFileName = "Schuper_spc_trace.txt";


    typedef sch::SnesFile::Type     FileType;

    SoundOut*               snd;
    sch::Snes*              snes;
    bool                    runapp;
    FileType                loadState;
    sch::VideoSettings      videoSettings;
    sch::u32                tmpVidBuffer[240 * 512];
    DibSection              dibBuffer;
    int                     displayedLines = 0;

    void toggleCpuTrace()
    {
        if(snes->isCpuTracing())    snes->stopCpuTrace();
        else                        snes->startCpuTrace(cpuTraceFileName);
    }
    
    void toggleSpcTrace()
    {
        if(snes->isSpcTracing())    snes->stopSpcTrace();
        else                        snes->startSpcTrace(spcTraceFileName);
    }

    bool isLoaded()
    {
        return loadState != FileType::Invalid;
    }

    void doDraw(HDC screendc)
    {
        if(loadState == FileType::Rom)
        {
            BitBlt(screendc, 0, 0, 512, displayedLines, dibBuffer, 0, 0, SRCCOPY);
        }
    }

    void doFrame(HWND wnd)
    {
        auto lk = snd->lock();
        snes->setAudioBuffer( lk.getBuffer<sch::s16>(0), lk.getSize(0), lk.getBuffer<sch::s16>(1), lk.getSize(1) );
        auto res = snes->doFrame(videoSettings);
        lk.setWritten( snes->getBytesOfAudioWritten() );
        dibBuffer.supplyPixels(tmpVidBuffer, res);
        displayedLines = res.lines * 2;

        if(loadState == FileType::Rom)
        {
            HDC dc = GetDC(wnd);
            doDraw(dc);
            ReleaseDC(wnd,dc);
        }
    }

    void unloadFile()
    {
        loadState = FileType::Invalid;
        snd->stop(true);
    }

    void doFileSelect(HWND wnd)
    {
        char filename[MAX_PATH+1] = "";
        OPENFILENAMEA       ofn = {};
        ofn.Flags =         OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
        ofn.hwndOwner =     wnd;
        ofn.lpstrFile =     filename;
        ofn.lpstrFilter =   "Supported Files\0*.sfc;*.spc\0All Files\0*\0\0";
        ofn.lStructSize =   sizeof(ofn);
        ofn.nFilterIndex =  1;
        ofn.nMaxFile =      MAX_PATH;

        snd->stop(false);

        if(GetOpenFileNameA(&ofn))
        {
            sch::SnesFile file;
            if(file.load(filename))
            {
                snd->stop(true);
                loadState = snes->loadFile(std::move(file));
            }
        }

        if(isLoaded())
            snd->play();
    }

    LRESULT CALLBACK mainWndProc(HWND wnd, UINT msg, WPARAM w, LPARAM l)
    {
        switch(msg)
        {
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC dc = BeginPaint(wnd,&ps);
                doDraw(dc);
                EndPaint(wnd,&ps);
            };
        case WM_KEYDOWN:
            switch(w)
            {
            case VK_ESCAPE:
                if(GetAsyncKeyState(VK_LSHIFT) & 0x8000)        unloadFile();
                else                                            doFileSelect(wnd);

                break;

            case VK_F7:         toggleCpuTrace();               break;
            case VK_F8:         toggleSpcTrace();               break;
            }
            break;
        case WM_SYSCOMMAND:
            if((w & 0xFFF0) == SC_CLOSE)
            {
                runapp = false;
            }
            break;
        }

        return DefWindowProcA(wnd, msg, w, l);
    }


}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
    videoSettings.buffer = tmpVidBuffer;
    videoSettings.alpha_or = 0;
    videoSettings.r_shift = 16;
    videoSettings.g_shift =  8;
    videoSettings.b_shift =  0;
    videoSettings.pitch =    512;

    sch::Snes snesobj;
    snes = &snesobj;
    runapp = true;
    loadState = FileType::Invalid;

    SoundOut sndobj(32000, true, 100);
    snd = &sndobj;

    WNDCLASSEXA         wc = {};
    wc.cbSize =         sizeof(wc);
    wc.hCursor =        LoadCursorA(inst, IDC_ARROW);
    wc.hInstance =      inst;
    wc.lpfnWndProc =    &mainWndProc;
    wc.lpszClassName =  wndClassName;

    if(!RegisterClassExA(&wc))
        return 1;

    HWND wnd = CreateWindowA(wndClassName, "Schpuer", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 400, 200, 640, 550, nullptr, nullptr, inst, nullptr);
    if(!wnd)
        return 2;

    ShowWindow(wnd, SW_SHOW);

    MSG msg;
    while(runapp)
    {
        while(PeekMessage(&msg, wnd, 0,0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if(isLoaded())
        {
            if(snd->canWrite() >= snes->getBytesOfAudioForAFrame())
                doFrame(wnd);
            else
                Sleep(1);
        }
        else
            Sleep(10);
    }

    return 0;
}

#include "tempspc.h"
#include "apu/spc.h"
#include "snesfile.h"
#include "util/audiobuffer.h"

namespace sch
{
    bool doSpcTrace(const char* spcpath, const char* logpath, timestamp_t cycs)
    {
        SnesFile            file;
        if(!file.load(spcpath))
            return false;
        if(file.type != SnesFile::Type::Spc)
            return false;

        Spc                 spc;
        spc.setClockBase(1);
        spc.setTrace(logpath);
        spc.loadSpcFile(file);
        spc.runTo(cycs);

        return true;
    }


    TempSpc::TempSpc()
    {
        loaded = false;
        spc = std::make_unique<Spc>();
        spc->setClockBase(1);
    }

    TempSpc::~TempSpc()
    {
        loaded = false;
    }

    bool TempSpc::loadFile(const char* path)
    {
        loaded = false;

        SnesFile            file;
        if(!file.load(path))
            return false;
        if(file.type != SnesFile::Type::Spc)
            return false;

        spc->setClockBase(1);
        spc->loadSpcFile(file);

        loaded = true;
        return true;
    }

    int TempSpc::generateAudio(s16* bufa, int siza, s16* bufb, int sizb)
    {
        if(!loaded)     return 0;

        AudioBuffer buf;
        buf.setBuffer(bufa, siza, bufb, sizb);

        spc->setAudioBuffer(&buf);
        spc->runToFillAudio();
        spc->setAudioBuffer(nullptr);

        return buf.getBytesWritten();
    }
}
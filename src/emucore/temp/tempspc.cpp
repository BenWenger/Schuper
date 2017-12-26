
#include "tempspc.h"
#include "apu/spc.h"
#include "snesfile.h"

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
        spc.setTrace(logpath);
        spc.loadSpcFile(file);
        spc.runForCycs(cycs);

        return true;
    }

}
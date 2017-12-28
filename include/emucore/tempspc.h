
#ifndef SCHUPER_TEMPSPC_H_INCLUDED
#define SCHUPER_TEMPSPC_H_INCLUDED

#include "snesfile.h"
#include "snestypes.h"
#include <memory>

namespace sch
{

    bool            doSpcTrace(const char* spcpath, const char* logpath, timestamp_t cycs);

    class Spc;

    class TempSpc
    {
    public:
                        TempSpc();
                        ~TempSpc();
        bool            loadFile(const char* path);
        int             generateAudio(s16* bufa, int siza, s16* bufb, int sizb);
        bool            isLoaded() const { return loaded;   }

    private:
        std::unique_ptr<Spc>            spc;
        bool                            loaded;
    };
}

#endif

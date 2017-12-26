
#ifndef SCHUPER_TEMPSPC_H_INCLUDED
#define SCHUPER_TEMPSPC_H_INCLUDED

#include "snesfile.h"
#include "snestypes.h"

namespace sch
{

    bool            doSpcTrace(const char* spcpath, const char* logpath, timestamp_t cycs);

}

#endif

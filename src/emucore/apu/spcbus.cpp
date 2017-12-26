
#include "spcbus.h"

namespace sch
{
    namespace
    {
        template <typename T>
        void setter(int pagestart, int pageend, T (&dst)[0x10], const T& proc)
        {
            if(pagestart < 0)       pagestart = 0;
            if(pageend > 0x0F)      pageend = 0x0F;

            for(int i = pagestart; i <= pageend; ++i)
                dst[i] = proc;
        }
    }
    
    void SpcBus::setRdProc(int pagestart, int pageend, const rdproc_t& proc )
    {
        setter(pagestart, pageend, readers, proc);
    }

    void SpcBus::setWrProc(int pagestart, int pageend, const wrproc_t& proc )
    {
        setter(pagestart, pageend, writers, proc);
    }

    void SpcBus::setPkProc(const pkproc_t& proc )
    {
        peeker = proc;
    }


}
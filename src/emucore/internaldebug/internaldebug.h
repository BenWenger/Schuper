
#ifndef SCHPUER_INTERNALDEBUG_INTERNALDEBUG_H_INCLUDED
#define SCHPUER_INTERNALDEBUG_INTERNALDEBUG_H_INCLUDED


//#define IDBG_ENABLED



#ifdef IDBG_ENABLED
    //#define IDBG_DUMP_DMA_UNIT_FILENAME        "dma_log.txt"



    #include <functional>

    namespace sch
    {
        struct InternalDebugStruct
        {
            std::function<u16()>    getPpuAddress;
            std::function<int()>    getPpuAddrInc;
        };

        extern InternalDebugStruct      InternalDebug;
    }

#endif




#endif
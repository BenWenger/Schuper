
#ifndef SCHPUER_INTERNALDEBUG_INTERNALDEBUG_H_INCLUDED
#define SCHPUER_INTERNALDEBUG_INTERNALDEBUG_H_INCLUDED


#define IDBG_ENABLED



#ifdef IDBG_ENABLED
    //#define IDBG_DUMP_DMA_UNIT_FILENAME        "dma_log.txt"



    #include <functional>

    namespace sch
    {
        struct InternalDebugStruct
        {
            std::function<timestamp_t()>    getMainClock;
            std::function<u16()>            getPpuAddress;
            std::function<int()>            getPpuAddrInc;
            std::function<void(int&,int&)>  getPpuPos;
        };

        extern InternalDebugStruct      InternalDebug;
    }

#endif




#endif
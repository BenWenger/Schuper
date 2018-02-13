
#ifndef SCHPUER_DMA_DMAUNIT_H_INCLUDED
#define SCHPUER_DMA_DMAUNIT_H_INCLUDED

#include "snestypes.h"

namespace sch
{
    class CpuBus;
    class Cpu;

    class DmaUnit
    {
    public:
        void            reset(bool hard, CpuBus* b, Cpu* c, timestamp_t dma_len, timestamp_t chan_len, timestamp_t xfer_len);
        void            write(u16 a, u8 v, timestamp_t clk);
        void            read(u16 a, u8& v, timestamp_t clk);

    private:
        struct Channel
        {
            bool        ppuRead;
            bool        indirect;
            int         addrAdj;
            int         xferMode;
            u8          modeByte;

            u8          dstAddr;
            u32         srcBank;
            u16         srcAddr;

            u16         size;
        };

        timestamp_t     dmaDelay;
        timestamp_t     chanDelay;
        timestamp_t     xferDelay;
        CpuBus*         bus = nullptr;
        Cpu*            cpu = nullptr;
        Channel         chans[8];


        void            doDma(u8 chanmask, timestamp_t clk);

    };
}


#endif

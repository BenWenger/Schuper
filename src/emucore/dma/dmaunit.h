
#ifndef SCHPUER_DMA_DMAUNIT_H_INCLUDED
#define SCHPUER_DMA_DMAUNIT_H_INCLUDED

#include "snestypes.h"

namespace sch
{
    class CpuBus;
    class MainClock;

    class DmaUnit
    {
    public:
        void            reset(bool hard, CpuBus* b, MainClock* c, timestamp_t dma_len, timestamp_t chan_len, timestamp_t xfer_len);
        void            write(u16 a, u8 v, timestamp_t clk);
        void            read(u16 a, u8& v, timestamp_t clk);

        void            doFrameStart();
        void            doLine();

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
            u32         indirectSrcBank;
            u16         hdmaAddr;

            u8          lineCountAndRepeat;
            bool        doTransfer;

            u16         size;
        };

        timestamp_t     dmaDelay;
        timestamp_t     chanDelay;
        timestamp_t     xferDelay;
        CpuBus*         bus = nullptr;
        MainClock*      clock = nullptr;
        Channel         chans[8];
        u8              hdmaEnable;
        u8              hdmaActive;     // bit becomes zero when a channel is "terminated"


        void            doDma(u8 chanmask);

        void            readIndirectAddr(Channel& chan);
        u8              readHdmaTableByte(Channel& chan);
    };
}


#endif


#include "dmaunit.h"
#include "bus/cpubus.h"
#include "main/mainclock.h"
#include "internaldebug/internaldebug.h"


#ifdef IDBG_DUMP_DMA_UNIT_FILENAME
#include <cstdio>
namespace
{
    std::FILE* dmaLogFile = nullptr;
}
#endif

namespace sch
{
    namespace
    {
        static const u8 modePatterns[8][4] = {
            {0,0,0,0},
            {0,1,0,1},
            {0,0,0,0},
            {0,0,1,1},
            {0,1,2,3},
            {0,1,0,1},
            {0,0,0,0},
            {0,0,1,1}
        };

        static const int modeSizes[8] = {
            1,2,2,4,4,4,2,4
        };
    }

    void DmaUnit::read(u16 a, u8& v, timestamp_t clk)
    {
        auto& chan = chans[(a & 0x0070) >> 4];

        switch(a & 0xFF8F)
        {
        case 0x4300:    v = chan.modeByte;                                  break;
        case 0x4301:    v = chan.dstAddr;                                   break;
        case 0x4302:    v = static_cast<u8>(chan.srcAddr & 0xFF);           break;
        case 0x4303:    v = static_cast<u8>(chan.srcAddr >> 8);             break;
        case 0x4304:    v = static_cast<u8>(chan.srcBank >> 16);            break;
        case 0x4305:    v = static_cast<u8>(chan.size & 0xFF);              break;
        case 0x4306:    v = static_cast<u8>(chan.size >> 8);                break;
            
        case 0x4307:    v = static_cast<u8>(chan.indirectSrcBank >> 16);    break;
        case 0x4308:    v = static_cast<u8>(chan.hdmaAddr & 0xFF);          break;
        case 0x4309:    v = static_cast<u8>(chan.hdmaAddr >> 8);            break;
        case 0x430A:    v = chan.lineCountAndRepeat;                        break;
        }
    }

    void DmaUnit::write(u16 a, u8 v, timestamp_t clk)
    {
        if(a == 0x420B)
        {
            if(!v)          return;
            doDma(v);                   //  TODO Move this to an event?  I guess it happens a few cycles after the write?
        }
        else if(a == 0x420C)
        {
            hdmaEnable = v;
        }
        else
        {
            auto& chan = chans[(a & 0x0070) >> 4];

            switch(a & 0xFF8F)
            {
            case 0x4300:
                chan.modeByte =     v;
                chan.ppuRead =      (v & 0x80) != 0;
                chan.indirect =     (v & 0x40) != 0;
                if(v & 0x08)        chan.addrAdj = 0;
                else                chan.addrAdj = (v & 0x10) ? -1 : 1;
                chan.xferMode =     v & 0x07;
                break;

            case 0x4301:    chan.dstAddr =  v;                                  break;
            case 0x4302:    chan.srcAddr = (chan.srcAddr & 0xFF00) | v;         break;
            case 0x4303:    chan.srcAddr = (chan.srcAddr & 0x00FF) | (v << 8);  break;
            case 0x4304:    chan.srcBank = (v << 16);                           break;
            
            case 0x4305:    chan.size = (chan.size & 0xFF00) | v;               break;
            case 0x4306:    chan.size = (chan.size & 0x00FF) | (v << 8);        break;
            case 0x4307:    chan.indirectSrcBank = (v << 16);                   break;
            case 0x4308:    chan.srcAddr = (chan.hdmaAddr & 0xFF00) | v;        break;
            case 0x4309:    chan.srcAddr = (chan.hdmaAddr & 0x00FF) | (v << 8); break;
            case 0x430A:    chan.lineCountAndRepeat = v;                        break;
            }
        }
    }


    ///////////////////////////////////////////////////////////////

    void DmaUnit::doDma(u8 chanmask)
    {

        u8 v;

        clock->tallyCycle(dmaDelay);
        for(int i = 0; i < 8; ++i)
        {
            if(chanmask & (1<<i))
            {
                clock->tallyCycle(chanDelay);

                auto& chan = chans[i];
                
#ifdef IDBG_DUMP_DMA_UNIT_FILENAME
                if(!dmaLogFile)
                    dmaLogFile = fopen(IDBG_DUMP_DMA_UNIT_FILENAME, "wt");

                fprintf(dmaLogFile, "DMA triggered on chan %d:\n"
                            "    Current PPU addr:  $%04X\n"
                            "    Reading:  %s\n"
                            "    Source:  $%06X\n"
                            "    Dest: $21%02X\n"
                            "    Size: $%04X\n"
                            "    Addr Adjust: %d\n"
                            "    Mode: %d\n\n\n"
                        ,i,
                        InternalDebug.getPpuAddress(),
                        chan.ppuRead ? "yes" : "no",
                        chan.srcAddr | chan.srcBank,
                        chan.dstAddr,
                        chan.size,
                        chan.addrAdj,
                        chan.xferMode
                );
#endif

                int phase = 0;
                do
                {
                    u8 dst = chan.dstAddr + modePatterns[chan.xferMode][phase];

                    if(chan.ppuRead)
                    {
                        bus->read(0x2100 | dst, v, clock->getTick());
                        bus->write(chan.srcAddr | chan.srcBank, v, clock->getTick());
                    }
                    else
                    {
                        bus->read(chan.srcAddr | chan.srcBank, v, clock->getTick());
                        bus->write(0x2100 | dst, v, clock->getTick());
                    }

                    phase = (phase + 1) & 3;
                    chan.srcAddr += chan.addrAdj;
                    --chan.size;
                    clock->tallyCycle(xferDelay);

                }while(chan.size);
            }
        }
    }


    
    void DmaUnit::reset(bool hard, CpuBus* b, MainClock* c, timestamp_t dma_len, timestamp_t chan_len, timestamp_t xfer_len)
    {
        bus = b;
        clock = c;
        dmaDelay = dma_len;
        chanDelay = chan_len;
        xferDelay = xfer_len;
        hdmaEnable = 0;

        if(hard)
        {
            for(u16 a = 0; a < 0x80; a += 0x0010)
            {
                write(0x4300 | a, 0xFF, 0);
                write(0x4301 | a, 0xFF, 0);
                write(0x4302 | a, 0xFF, 0);
                write(0x4303 | a, 0xFF, 0);
                write(0x4304 | a, 0xFF, 0);
                write(0x4305 | a, 0xFF, 0);
                write(0x4306 | a, 0xFF, 0);
                write(0x4307 | a, 0xFF, 0);
                write(0x4308 | a, 0xFF, 0);
                write(0x4309 | a, 0xFF, 0);
                write(0x430A | a, 0xFF, 0);
            }
            for(int i = 0; i < 8; ++i)
                chans[i].doTransfer = false;
        }
    }
    
    u8 DmaUnit::readHdmaTableByte(Channel& chan)
    {
        u8 v;
        bus->read(chan.hdmaAddr++ | chan.srcBank, v, clock->getTick());
        clock->tallyCycle(8);
        return v;
    }
    
    void DmaUnit::readIndirectAddr(Channel& chan)
    {
        chan.size  = readHdmaTableByte(chan);
        chan.size |= readHdmaTableByte(chan) << 8;
    }
    
    void DmaUnit::doFrameStart()
    {
        clock->tallyCycle(18);
        hdmaActive = 0xFF;

        for(int i = 0; i < 8; ++i)
        {
            auto& chan = chans[i];

            if(!(hdmaEnable & (1<<i)))
               continue;
            
            chan.hdmaAddr = chan.srcAddr;
            chan.lineCountAndRepeat = readHdmaTableByte(chan);
            if(!chan.lineCountAndRepeat)
                hdmaActive &= ~(1<<i);

            if(chan.indirect)
                readIndirectAddr(chan);

            chan.doTransfer = true;
        }
    }

    void DmaUnit::doLine()
    {
        u8 chanmask = hdmaEnable & hdmaActive;
        if(!chanmask)       // nothing to do if no channels active
            return;

        clock->tallyCycle(18);

        for(int i = 0; i < 8; ++i)
        {
            if(!(chanmask & (1<<i)))
                continue;

            auto& chan = chans[i];

            if(chan.doTransfer)
            {
                u8 v;
                u32 srcBank = (chan.indirect ? chan.indirectSrcBank : chan.srcBank);
                u16& src = (chan.indirect ? chan.size : chan.hdmaAddr);
                for(int x = 0; x < modeSizes[chan.xferMode]; ++x)
                {
                    u8 dst = chan.dstAddr + modePatterns[chan.xferMode][x];

                    if(chan.ppuRead)
                    {
                        bus->read(0x2100 | dst, v, clock->getTick());
                        bus->write(srcBank | src++, v, clock->getTick());
                    }
                    else
                    {
                        bus->read(srcBank | src++, v, clock->getTick());
                        bus->write(0x2100 | dst, v, clock->getTick());
                    }
                    clock->tallyCycle(8);
                }
            }

            --chan.lineCountAndRepeat;
            chan.doTransfer = !!(chan.lineCountAndRepeat & 0x80);
            if(chan.lineCountAndRepeat & 0x7F)
                clock->tallyCycle(8);
            else
            {
                chan.lineCountAndRepeat = readHdmaTableByte(chan);
                if(chan.indirect)
                    readIndirectAddr(chan);

                if(!chan.lineCountAndRepeat)
                    hdmaActive &= ~(1<<i);

                chan.doTransfer = true;
            }
        }
    }

}

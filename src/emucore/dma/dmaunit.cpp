
#include "dmaunit.h"
#include "bus/cpubus.h"
#include "cpu/cpu.h"

namespace sch
{
    void DmaUnit::read(u16 a, u8& v, timestamp_t clk)
    {
        auto& chan = chans[(a & 0x0070) >> 4];

        switch(a & 0xFF8F)
        {
        case 0x4300:    v = chan.modeByte;                          break;
        case 0x4301:    v = chan.dstAddr;                           break;
        case 0x4302:    v = static_cast<u8>(chan.srcAddr & 0xFF);   break;
        case 0x4303:    v = static_cast<u8>(chan.srcAddr >> 8);     break;
        case 0x4304:    v = static_cast<u8>(chan.srcBank >> 16);    break;
        case 0x4305:    v = static_cast<u8>(chan.size & 0xFF);      break;
        case 0x4306:    v = static_cast<u8>(chan.size >> 8);        break;
            
        case 0x4307:    /* TODO */                                  break;
        case 0x4308:    /* TODO */                                  break;
        case 0x4309:    /* TODO */                                  break;
        case 0x430A:    /* TODO */                                  break;
        }
    }

    void DmaUnit::write(u16 a, u8 v, timestamp_t clk)
    {
        if(a == 0x420B)
        {
            if(!v)          return;
            doDma(v, clk);
        }
        else if(a == 0x420C)
        {
            // TODO HDMA enable
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
            case 0x4307:    /* TODO */                                          break;
            case 0x4308:    /* TODO */                                          break;
            case 0x4309:    /* TODO */                                          break;
            case 0x430A:    /* TODO */                                          break;
            }
        }
    }


    ///////////////////////////////////////////////////////////////

    void DmaUnit::doDma(u8 chanmask, timestamp_t clk)
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

        timestamp_t origclk = clk;
        u8 v;

        clk += dmaDelay;
        for(int i = 0; i < 8; ++i)
        {
            if(chanmask & (1<<i))
            {
                clk += chanDelay;

                auto& chan = chans[i];
                int phase = 0;
                do
                {
                    u8 dst = chan.dstAddr + modePatterns[chan.xferMode][phase];

                    if(chan.ppuRead)
                    {
                        bus->read(0x2100 | dst, v, clk);
                        bus->write(chan.srcAddr | chan.srcBank, v, clk);
                    }
                    else
                    {
                        bus->read(chan.srcAddr | chan.srcBank, v, clk);
                        bus->write(0x2100 | dst, v, clk);
                    }

                    phase = (phase + 1) & 3;
                    chan.srcAddr += chan.addrAdj;
                    --chan.size;
                    clk += xferDelay;

                }while(chan.size);
            }
        }

        cpu->adjustTimestamp(clk - origclk);
    }


    
    void DmaUnit::reset(bool hard, CpuBus* b, Cpu* c, timestamp_t dma_len, timestamp_t chan_len, timestamp_t xfer_len)
    {
        bus = b;
        cpu = c;
        dmaDelay = dma_len;
        chanDelay = chan_len;
        xferDelay = xfer_len;

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
        }
    }

}

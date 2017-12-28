
#ifndef SCHUPER_UTIL_AUDIOBUFFER_H_INCLUDED
#define SCHUPER_UTIL_AUDIOBUFFER_H_INCLUDED

#include "snestypes.h"

namespace sch
{
    class AudioBuffer
    {
    public:
                AudioBuffer() = default;
                AudioBuffer(const AudioBuffer&) = delete;
                AudioBuffer& operator = (const AudioBuffer&) = delete;

        void    setBuffer(s16* bufa, int siza, s16* bufb, int sizb);    // size in bytes
        int     getBytesWritten() const { return bytesWritten;      }   // returns size in bytes

        bool    outputSample(s16 left, s16 right);                      // returns true if no more samples can be output after this
        bool    canOutput() const   { return (bufOutA != nullptr);  }

        int     samplesLeftToWrite() const { return sizeA + sizeB;  }   // returns size in 32-bit blocks (number of calls to outputSample)


    private:
        s16*    bufOutA = nullptr;
        s16*    bufOutB = nullptr;
        int     sizeA = 0;
        int     sizeB = 0;
        int     bytesWritten = 0;

        inline void moveBA()
        {
            bufOutA = bufOutB;      bufOutB = nullptr;
            sizeA = sizeB;          sizeB = 0;
        }
    };


    inline void AudioBuffer::setBuffer(s16* bufa, int siza, s16* bufb, int sizb)
    {
        // convert from bytes to stereo s16s
        sizeA = siza >> 2;
        sizeB = sizb >> 2;
        bufOutA = bufa;
        bufOutB = bufb;

        // validate
        if(!bufOutB || (sizeB <= 0))
        {
            bufOutB = nullptr;
            sizeB = 0;
        }
        if(!bufOutA || (sizeA <= 0))
        {
            moveBA();
        }
    }
    
    inline bool AudioBuffer::outputSample(s16 left, s16 right)  // returns true if no more samples can be output after this
    {
        bufOutA[0] = left;
        bufOutA[1] = right;
        bytesWritten += 4;
        --sizeA;

        if(sizeA)
        {
            bufOutA += 2;
            return false;
        }
        else
        {
            moveBA();
            return (bufOutA == nullptr);
        }
    }
}

#endif

#ifndef SCHUPER_EMUCORE_SNESINPUT_H_INCLUDED
#define SCHUPER_EMUCORE_SNESINPUT_H_INCLUDED

namespace sch
{
    class InputDevice
    {
    private:
        friend class Snes;
        virtual void        reset(bool hard) = 0;
        virtual u8          read() = 0;
        virtual void        write(u8 v) = 0;
    };

    class StdController : public InputDevice
    {
    public:
        enum Btn
        {
            B =         0x0001,
            Y =         0x0002,
            Select =    0x0004,
            Start =     0x0008,
            Up =        0x0010,
            Down =      0x0020,
            Left =      0x0040,
            Right =     0x0080,
            A =         0x0100,
            X =         0x0200,
            L =         0x0400,
            R =         0x0800
        };

        void        setState(int btns)      { state = (btns & 0x0FFF);  }

    private:
        u16         state = 0;
        u16         latch = 0;
        bool        latching = false;

        virtual void        reset(bool hard) override
        {
            latch = 0;
            latching = 0;
        }

        virtual u8          read() override
        {
            if(latching)        return (state & 1);
            
            u8 out = (latch & 1);
            latch >>= 1;
            latch |= 0x8000;
            return out;
        }

        virtual void        write(u8 v) override
        {
            if(v)
                latching = true;
            else if(latching)
            {
                latching = false;
                latch = state;
            }
        }
    };
}


#endif

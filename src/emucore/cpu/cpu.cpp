
#include "cpu.h"
#include "bus/cpubus.h"
#include "cputracer.h"
#include "main/mainclock.h"

#include "cpu_instructions.hpp"
#include "cpu_addrmodes.hpp"


namespace sch
{
    inline void Cpu::dpCyc()            {   if(regs.DP & 0x00FF)        ioCyc();            }
    inline void Cpu::ioCyc()            {   tallyCycle(8);                                  }       // TODO move this '8' somewhere
    inline void Cpu::ioCyc(int cycs)    {   tallyCycle(cycs*8);                             }       // TODO move this '8' somewhere

    inline void Cpu::tallyCycle(int cycs)
    {
        interruptReady = interruptPending;
        clock->tallyCycle(cycs);
    }

    inline u8 Cpu::read(u32 a)
    {
        u8 v;
        tallyCycle( bus->read(a, v, clock->getTick()) );
        return v;
    }
    inline void Cpu::write(u32 a, u8 v)
    {
        tallyCycle( bus->write(a, v, clock->getTick() ) );
    }

    inline void Cpu::push(u8 v)
    {
        if(regs.fE)             write(0x0100 | regs.SP.l--, v);
        else                    write(regs.SP.w--, v);
    }
    
    inline u8 Cpu::pull()
    {
        if(regs.fE)             return read(0x0100 | ++regs.SP.l);
        else                    return read(++regs.SP.w);
    }
    
    void Cpu::reset(CpuBus* thebus, MainClock* theclock)
    {
        clock = theclock;
        bus = thebus;
        
        regs.A.w = 0;
        regs.X.w = 0;
        regs.Y.w = 0;
        regs.DBR = 0;
        regs.PBR = 0;
        regs.DP = 0;
        regs.fC = 0;
        regs.fD = false;
        regs.fI = true;
        regs.fM = true;
        regs.fE = true;
        regs.fN = 0;
        regs.fV = 0;
        regs.fX = true;
        regs.fZ = 1;
        regs.PC = 0;
        regs.SP.w = 0x01FF;
        
        // Set interrupt stuff
            // immediately interrupt with a RESET
        interruptReady = interruptPending = resetPending = true;
        nmiPending = false;
        irqFlags = 0;

        stopped = false;
        waiting = false;
    }

    // Running is split into both 'runTo' and 'innerRun' because STP and WAI are awkward and
    //   disrupt the normal flow.  'innerRun' does pretty much all the work, but WAI/STP exit
    //   that function immediately so they can zoom ahead in time.
    void Cpu::runTo(timestamp_t runto)
    {
        while(clock->getTick() < runto)
        {
            if(stopped)
            {
                clock->runToNextEvent(runto);
            }
            else if(waiting)
            {
                if(interruptPending)        // note:  PENDING, not ready!   That's important, as ready won't
                {                           //    be set by runToNextEvent!
                    waiting = false;
                    ioCyc(2);
                }
                else
                    clock->runToNextEvent(runto);
            }
            else
            {
                // normal flow goes here
                innerRun(runto);
            }
        }
    }

    void Cpu::innerRun(timestamp_t runto)
    {
        while(clock->getTick() < runto)
        {
            if(interruptReady)
            {
                // Default to IRQ, even if irqFlags is zero (theoretically the IRQ could have been
                //   acknowledged after the 'point of no return' where interruptReady is true

                IntType t = IntType::Irq;

                if( resetPending )          t = IntType::Reset;     // reset overrides everything
                else if( nmiPending )       t = IntType::Nmi;       // NMI overrides IRQ

                doInterrupt( t );
                continue;
            }

            if(tracer)
                tracer->cpuTrace(regs, *bus);
            u8 op = read_pc();


            switch(op)
            {
                /* Branches */
            case 0x10:  ad_branch( regs.fN == 0 );                  break;  /* BPL  */
            case 0x30:  ad_branch( regs.fN != 0 );                  break;  /* BMI  */
            case 0x50:  ad_branch( regs.fV == 0 );                  break;  /* BVC  */
            case 0x70:  ad_branch( regs.fV != 0 );                  break;  /* BVS  */
            case 0x90:  ad_branch( regs.fC == 0 );                  break;  /* BCC  */
            case 0xB0:  ad_branch( regs.fC != 0 );                  break;  /* BCS  */
            case 0xD0:  ad_branch( regs.fZ != 0 );                  break;  /* BNE  */
            case 0xF0:  ad_branch( regs.fZ == 0 );                  break;  /* BEQ  */
            case 0x80:  ad_branch( true );                          break;  /* BRA  */

                /* Flag toggling    */
            case 0x18:  regs.fC = 0; ioCyc();                           break;  /* CLC  */
            case 0xD8:  regs.fD = 0; ioCyc();                           break;  /* CLD  */
            case 0x58:  regs.fI = 0; ioCyc(); checkInterruptPending();  break;  /* CLI  */
            case 0xB8:  regs.fV = 0; ioCyc();                           break;  /* CLV  */
            case 0x38:  regs.fC = 1; ioCyc();                           break;  /* SEC  */
            case 0xF8:  regs.fD = 1; ioCyc();                           break;  /* SED  */
            case 0x78:  regs.fI = 1; ioCyc(); checkInterruptPending();  break;  /* SEI  *

                /* Stack ops        */
            case 0x48:  ad_push(regs.A.w, regs.fM);                 break;  /* PHA  */
            case 0xDA:  ad_push(regs.X.w, regs.fX);                 break;  /* PHX  */
            case 0x5A:  ad_push(regs.Y.w, regs.fX);                 break;  /* PHY  */
            case 0x08:  ad_push(regs.getStatusByte(true), true);    break;  /* PHP  */
            case 0x0B:  ad_push(regs.DP, false);                    break;  /* PHD  */
            case 0x8B:  ad_push(regs.DBR >> 16, true);              break;  /* PHB  */
            case 0x4B:  ad_push(regs.PBR >> 16, true);              break;  /* PHK  */

            case 0x68:  u_PLA();                                    break;  /* PLA  */
            case 0xFA:  regs.X.w = ad_pull(regs.fX);                break;  /* PLX  */
            case 0x7A:  regs.Y.w = ad_pull(regs.fX);                break;  /* PLY  */
            case 0x2B:  regs.DP  = ad_pull(false);                  break;  /* PLD  */
            case 0xAB:  regs.DBR = ad_pull(true) << 16;             break;  /* PLB  */
            case 0x28:  regs.setStatusByte( ad_pull(true) & 0xFF );         /* PLP  */
                        checkInterruptPending();                    break;

                /* Reg Xfer     */
            case 0xAA:  TAX();              ioCyc();                break;  /* TAX  */
            case 0xA8:  TAY();              ioCyc();                break;  /* TAY  */
            case 0x5B:  TCD();              ioCyc();                break;  /* TCD  */
            case 0x1B:  TCS();              ioCyc();                break;  /* TCS  */
            case 0x7B:  TDC();              ioCyc();                break;  /* TDC  */
            case 0x3B:  TSC();              ioCyc();                break;  /* TSC  */
            case 0xBA:  TSX();              ioCyc();                break;  /* TSX  */
            case 0x8A:  TXA();              ioCyc();                break;  /* TXA  */
            case 0x9A:  TXS();              ioCyc();                break;  /* TXS  */
            case 0x9B:  TXY();              ioCyc();                break;  /* TXY  */
            case 0x98:  TYA();              ioCyc();                break;  /* TYA  */
            case 0xBB:  TYX();              ioCyc();                break;  /* TYX  */

                /* One-off garbage  */
            case 0x00:  doInterrupt(IntType::Brk);                  break;  /* BRK  */
            case 0x82:  u_BRL();                                    break;  /* BRL  */
            case 0x02:  doInterrupt(IntType::Cop);                  break;  /* COP  */
            case 0x4C:  u_JMP_Absolute();                           break;  /* JMP $aaaa    */
            case 0x5C:  u_JMP_AbsoluteLong();                       break;  /* JMP $llllll  */
            case 0x6C:  u_JMP_Indirect();                           break;  /* JMP ($aaaa)  */
            case 0x7C:  u_JMP_IndirectX();                          break;  /* JMP ($aaaa,X)*/
            case 0xDC:  u_JMP_IndirectLong();                       break;  /* JMP [$aaaa]  */
            case 0x20:  u_JSR_Absolute();                           break;  /* JSR $aaaa    */
            case 0x22:  u_JSR_AbsoluteLong();                       break;  /* JSR $llllll  */
            case 0xFC:  u_JSR_IndirectX();                          break;  /* JSR ($aaaa,X)*/
            case 0x54:  u_MVN();                                    break;  /* MVN  */
            case 0x44:  u_MVP();                                    break;  /* MVP  */
            case 0xEA:  ioCyc();                                    break;  /* NOP  */
            case 0xF4:  u_PEA();                                    break;  /* PEA  */
            case 0xD4:  u_PEI();                                    break;  /* PEI  */
            case 0x62:  u_PER();                                    break;  /* PER  */
            case 0xC2:  u_REP();                                    break;  /* REP  */
            case 0x40:  u_RTI();                                    break;  /* RTI  */
            case 0x6B:  u_RTL();                                    break;  /* RTL  */
            case 0x60:  u_RTS();                                    break;  /* RTS  */
            case 0xE2:  u_SEP();                                    break;  /* SEP  */
            case 0xDB:  stopped = true;     return;                         /* STP  -- note: it returns */
            case 0xCB:  waiting = true;     return;                         /* WAI  -- note: it returns */
            case 0x42:  ioCyc();                                    break;  /* WDP  */
            case 0xEB:  u_XBA();                                    break;  /* XBA  */
            case 0xFB:  u_XCE();                                    break;  /* XCE  */

                /* ADC  */
            case 0x61:  ADC( ad_rd_ix (regs.fM) );                  break;
            case 0x63:  ADC( ad_rd_sr (regs.fM) );                  break;
            case 0x65:  ADC( ad_rd_dp (regs.fM) );                  break;
            case 0x67:  ADC( ad_rd_dil(regs.fM) );                  break;
            case 0x69:  ADC( ad_rd_im (regs.fM) );                  break;
            case 0x6D:  ADC( ad_rd_ab (regs.fM) );                  break;
            case 0x6F:  ADC( ad_rd_al (regs.fM) );                  break;
            case 0x71:  ADC( ad_rd_iy (regs.fM) );                  break;
            case 0x72:  ADC( ad_rd_di (regs.fM) );                  break;
            case 0x73:  ADC( ad_rd_siy(regs.fM) );                  break;
            case 0x75:  ADC( ad_rd_dx (regs.fM) );                  break;
            case 0x77:  ADC( ad_rd_iyl(regs.fM) );                  break;
            case 0x79:  ADC( ad_rd_ay (regs.fM) );                  break;
            case 0x7D:  ADC( ad_rd_ax (regs.fM) );                  break;
            case 0x7F:  ADC( ad_rd_axl(regs.fM) );                  break;
                
                /* AND  */
            case 0x21:  AND( ad_rd_ix (regs.fM) );                  break;
            case 0x23:  AND( ad_rd_sr (regs.fM) );                  break;
            case 0x25:  AND( ad_rd_dp (regs.fM) );                  break;
            case 0x27:  AND( ad_rd_dil(regs.fM) );                  break;
            case 0x29:  AND( ad_rd_im (regs.fM) );                  break;
            case 0x2D:  AND( ad_rd_ab (regs.fM) );                  break;
            case 0x2F:  AND( ad_rd_al (regs.fM) );                  break;
            case 0x31:  AND( ad_rd_iy (regs.fM) );                  break;
            case 0x32:  AND( ad_rd_di (regs.fM) );                  break;
            case 0x33:  AND( ad_rd_siy(regs.fM) );                  break;
            case 0x35:  AND( ad_rd_dx (regs.fM) );                  break;
            case 0x37:  AND( ad_rd_iyl(regs.fM) );                  break;
            case 0x39:  AND( ad_rd_ay (regs.fM) );                  break;
            case 0x3D:  AND( ad_rd_ax (regs.fM) );                  break;
            case 0x3F:  AND( ad_rd_axl(regs.fM) );                  break;

                /* ASL  */
            case 0x0A:  ad_rw_ac( &Cpu::ASL );                      break;
            case 0x06:  ad_rw_dp( &Cpu::ASL, regs.fM );             break;
            case 0x0E:  ad_rw_ab( &Cpu::ASL, regs.fM );             break;
            case 0x16:  ad_rw_dx( &Cpu::ASL, regs.fM );             break;
            case 0x1E:  ad_rw_ax( &Cpu::ASL, regs.fM );             break;

                /* BIT  */
            case 0x89:  BIT( ad_rd_im (regs.fM), false );           break;
            case 0x24:  BIT( ad_rd_dp (regs.fM), true );            break;
            case 0x2C:  BIT( ad_rd_ab (regs.fM), true );            break;
            case 0x34:  BIT( ad_rd_dx (regs.fM), true );            break;
            case 0x3C:  BIT( ad_rd_ax (regs.fM), true );            break;
                
                /* CMP  */
            case 0xC1:  CMP( ad_rd_ix (regs.fM) );                  break;
            case 0xC3:  CMP( ad_rd_sr (regs.fM) );                  break;
            case 0xC5:  CMP( ad_rd_dp (regs.fM) );                  break;
            case 0xC7:  CMP( ad_rd_dil(regs.fM) );                  break;
            case 0xC9:  CMP( ad_rd_im (regs.fM) );                  break;
            case 0xCD:  CMP( ad_rd_ab (regs.fM) );                  break;
            case 0xCF:  CMP( ad_rd_al (regs.fM) );                  break;
            case 0xD1:  CMP( ad_rd_iy (regs.fM) );                  break;
            case 0xD2:  CMP( ad_rd_di (regs.fM) );                  break;
            case 0xD3:  CMP( ad_rd_siy(regs.fM) );                  break;
            case 0xD5:  CMP( ad_rd_dx (regs.fM) );                  break;
            case 0xD7:  CMP( ad_rd_iyl(regs.fM) );                  break;
            case 0xD9:  CMP( ad_rd_ay (regs.fM) );                  break;
            case 0xDD:  CMP( ad_rd_ax (regs.fM) );                  break;
            case 0xDF:  CMP( ad_rd_axl(regs.fM) );                  break;
                
                /* CPX  */
            case 0xE0:  CPX( ad_rd_im (regs.fX) );                  break;
            case 0xE4:  CPX( ad_rd_dp (regs.fX) );                  break;
            case 0xEC:  CPX( ad_rd_ab (regs.fX) );                  break;
                
                /* CPY  */
            case 0xC0:  CPY( ad_rd_im (regs.fX) );                  break;
            case 0xC4:  CPY( ad_rd_dp (regs.fX) );                  break;
            case 0xCC:  CPY( ad_rd_ab (regs.fX) );                  break;

                /* DEC  */
            case 0xCA:  regs.X.w = DEC( regs.X.w, regs.fX );    ioCyc();    break;  /* DEX  */
            case 0x88:  regs.Y.w = DEC( regs.Y.w, regs.fX );    ioCyc();    break;  /* DEY  */
            case 0x3A:  ad_rw_ac( &Cpu::DEC );                              break;
            case 0xC6:  ad_rw_dp( &Cpu::DEC, regs.fM );                     break;
            case 0xCE:  ad_rw_ab( &Cpu::DEC, regs.fM );                     break;
            case 0xD6:  ad_rw_dx( &Cpu::DEC, regs.fM );                     break;
            case 0xDE:  ad_rw_ax( &Cpu::DEC, regs.fM );                     break;
                
                /* EOR  */
            case 0x41:  EOR( ad_rd_ix (regs.fM) );                  break;
            case 0x43:  EOR( ad_rd_sr (regs.fM) );                  break;
            case 0x45:  EOR( ad_rd_dp (regs.fM) );                  break;
            case 0x47:  EOR( ad_rd_dil(regs.fM) );                  break;
            case 0x49:  EOR( ad_rd_im (regs.fM) );                  break;
            case 0x4D:  EOR( ad_rd_ab (regs.fM) );                  break;
            case 0x4F:  EOR( ad_rd_al (regs.fM) );                  break;
            case 0x51:  EOR( ad_rd_iy (regs.fM) );                  break;
            case 0x52:  EOR( ad_rd_di (regs.fM) );                  break;
            case 0x53:  EOR( ad_rd_siy(regs.fM) );                  break;
            case 0x55:  EOR( ad_rd_dx (regs.fM) );                  break;
            case 0x57:  EOR( ad_rd_iyl(regs.fM) );                  break;
            case 0x59:  EOR( ad_rd_ay (regs.fM) );                  break;
            case 0x5D:  EOR( ad_rd_ax (regs.fM) );                  break;
            case 0x5F:  EOR( ad_rd_axl(regs.fM) );                  break;
                
                /* INC  */
            case 0xE8:  regs.X.w = INC( regs.X.w, regs.fX );    ioCyc();    break;  /* INX  */
            case 0xC8:  regs.Y.w = INC( regs.Y.w, regs.fX );    ioCyc();    break;  /* INY  */
            case 0x1A:  ad_rw_ac( &Cpu::INC );                              break;
            case 0xE6:  ad_rw_dp( &Cpu::INC, regs.fM );                     break;
            case 0xEE:  ad_rw_ab( &Cpu::INC, regs.fM );                     break;
            case 0xF6:  ad_rw_dx( &Cpu::INC, regs.fM );                     break;
            case 0xFE:  ad_rw_ax( &Cpu::INC, regs.fM );                     break;
                
                /* LDA  */
            case 0xA1:  LDA( ad_rd_ix (regs.fM) );                  break;
            case 0xA3:  LDA( ad_rd_sr (regs.fM) );                  break;
            case 0xA5:  LDA( ad_rd_dp (regs.fM) );                  break;
            case 0xA7:  LDA( ad_rd_dil(regs.fM) );                  break;
            case 0xA9:  LDA( ad_rd_im (regs.fM) );                  break;
            case 0xAD:  LDA( ad_rd_ab (regs.fM) );                  break;
            case 0xAF:  LDA( ad_rd_al (regs.fM) );                  break;
            case 0xB1:  LDA( ad_rd_iy (regs.fM) );                  break;
            case 0xB2:  LDA( ad_rd_di (regs.fM) );                  break;
            case 0xB3:  LDA( ad_rd_siy(regs.fM) );                  break;
            case 0xB5:  LDA( ad_rd_dx (regs.fM) );                  break;
            case 0xB7:  LDA( ad_rd_iyl(regs.fM) );                  break;
            case 0xB9:  LDA( ad_rd_ay (regs.fM) );                  break;
            case 0xBD:  LDA( ad_rd_ax (regs.fM) );                  break;
            case 0xBF:  LDA( ad_rd_axl(regs.fM) );                  break;
                
                /* LDX  */
            case 0xA2:  LDX( ad_rd_im (regs.fX) );                  break;
            case 0xA6:  LDX( ad_rd_dp (regs.fX) );                  break;
            case 0xAE:  LDX( ad_rd_ab (regs.fX) );                  break;
            case 0xB6:  LDX( ad_rd_dy (regs.fX) );                  break;
            case 0xBE:  LDX( ad_rd_ay (regs.fX) );                  break;
                
                /* LDY  */
            case 0xA0:  LDY( ad_rd_im (regs.fX) );                  break;
            case 0xA4:  LDY( ad_rd_dp (regs.fX) );                  break;
            case 0xAC:  LDY( ad_rd_ab (regs.fX) );                  break;
            case 0xB4:  LDY( ad_rd_dx (regs.fX) );                  break;
            case 0xBC:  LDY( ad_rd_ax (regs.fX) );                  break;
                
                /* LSR  */
            case 0x4A:  ad_rw_ac( &Cpu::LSR );                      break;
            case 0x46:  ad_rw_dp( &Cpu::LSR, regs.fM );             break;
            case 0x4E:  ad_rw_ab( &Cpu::LSR, regs.fM );             break;
            case 0x56:  ad_rw_dx( &Cpu::LSR, regs.fM );             break;
            case 0x5E:  ad_rw_ax( &Cpu::LSR, regs.fM );             break;
                
                /* ORA  */
            case 0x01:  ORA( ad_rd_ix (regs.fM) );                  break;
            case 0x03:  ORA( ad_rd_sr (regs.fM) );                  break;
            case 0x05:  ORA( ad_rd_dp (regs.fM) );                  break;
            case 0x07:  ORA( ad_rd_dil(regs.fM) );                  break;
            case 0x09:  ORA( ad_rd_im (regs.fM) );                  break;
            case 0x0D:  ORA( ad_rd_ab (regs.fM) );                  break;
            case 0x0F:  ORA( ad_rd_al (regs.fM) );                  break;
            case 0x11:  ORA( ad_rd_iy (regs.fM) );                  break;
            case 0x12:  ORA( ad_rd_di (regs.fM) );                  break;
            case 0x13:  ORA( ad_rd_siy(regs.fM) );                  break;
            case 0x15:  ORA( ad_rd_dx (regs.fM) );                  break;
            case 0x17:  ORA( ad_rd_iyl(regs.fM) );                  break;
            case 0x19:  ORA( ad_rd_ay (regs.fM) );                  break;
            case 0x1D:  ORA( ad_rd_ax (regs.fM) );                  break;
            case 0x1F:  ORA( ad_rd_axl(regs.fM) );                  break;
                
                /* ROL  */
            case 0x2A:  ad_rw_ac( &Cpu::ROL );                      break;
            case 0x26:  ad_rw_dp( &Cpu::ROL, regs.fM );             break;
            case 0x2E:  ad_rw_ab( &Cpu::ROL, regs.fM );             break;
            case 0x36:  ad_rw_dx( &Cpu::ROL, regs.fM );             break;
            case 0x3E:  ad_rw_ax( &Cpu::ROL, regs.fM );             break;
                
                /* ROR  */
            case 0x6A:  ad_rw_ac( &Cpu::ROR );                      break;
            case 0x66:  ad_rw_dp( &Cpu::ROR, regs.fM );             break;
            case 0x6E:  ad_rw_ab( &Cpu::ROR, regs.fM );             break;
            case 0x76:  ad_rw_dx( &Cpu::ROR, regs.fM );             break;
            case 0x7E:  ad_rw_ax( &Cpu::ROR, regs.fM );             break;
                
                /* SBC  */
            case 0xE1:  SBC( ad_rd_ix (regs.fM) );                  break;
            case 0xE3:  SBC( ad_rd_sr (regs.fM) );                  break;
            case 0xE5:  SBC( ad_rd_dp (regs.fM) );                  break;
            case 0xE7:  SBC( ad_rd_dil(regs.fM) );                  break;
            case 0xE9:  SBC( ad_rd_im (regs.fM) );                  break;
            case 0xED:  SBC( ad_rd_ab (regs.fM) );                  break;
            case 0xEF:  SBC( ad_rd_al (regs.fM) );                  break;
            case 0xF1:  SBC( ad_rd_iy (regs.fM) );                  break;
            case 0xF2:  SBC( ad_rd_di (regs.fM) );                  break;
            case 0xF3:  SBC( ad_rd_siy(regs.fM) );                  break;
            case 0xF5:  SBC( ad_rd_dx (regs.fM) );                  break;
            case 0xF7:  SBC( ad_rd_iyl(regs.fM) );                  break;
            case 0xF9:  SBC( ad_rd_ay (regs.fM) );                  break;
            case 0xFD:  SBC( ad_rd_ax (regs.fM) );                  break;
            case 0xFF:  SBC( ad_rd_axl(regs.fM) );                  break;

                /* STA  */
            case 0x81:  ad_wr_ix ( STA(), regs.fM );                break;
            case 0x83:  ad_wr_sr ( STA(), regs.fM );                break;
            case 0x85:  ad_wr_dp ( STA(), regs.fM );                break;
            case 0x87:  ad_wr_dil( STA(), regs.fM );                break;
            case 0x8D:  ad_wr_ab ( STA(), regs.fM );                break;
            case 0x8F:  ad_wr_al ( STA(), regs.fM );                break;
            case 0x91:  ad_wr_iy ( STA(), regs.fM );                break;
            case 0x92:  ad_wr_di ( STA(), regs.fM );                break;
            case 0x93:  ad_wr_siy( STA(), regs.fM );                break;
            case 0x95:  ad_wr_dx ( STA(), regs.fM );                break;
            case 0x97:  ad_wr_iyl( STA(), regs.fM );                break;
            case 0x99:  ad_wr_ay ( STA(), regs.fM );                break;
            case 0x9D:  ad_wr_ax ( STA(), regs.fM );                break;
            case 0x9F:  ad_wr_axl( STA(), regs.fM );                break;
                
                /* STX  */
            case 0x86:  ad_wr_dp ( regs.X.w, regs.fX );             break;
            case 0x8E:  ad_wr_ab ( regs.X.w, regs.fX );             break;
            case 0x96:  ad_wr_dy ( regs.X.w, regs.fX );             break;
                
                /* STY  */
            case 0x84:  ad_wr_dp ( regs.Y.w, regs.fX );             break;
            case 0x8C:  ad_wr_ab ( regs.Y.w, regs.fX );             break;
            case 0x94:  ad_wr_dx ( regs.Y.w, regs.fX );             break;

                /* STZ  */
            case 0x64:  ad_wr_dp ( 0, regs.fM );                    break;
            case 0x74:  ad_wr_dx ( 0, regs.fM );                    break;
            case 0x9C:  ad_wr_ab ( 0, regs.fM );                    break;
            case 0x9E:  ad_wr_ax ( 0, regs.fM );                    break;

                /* TRB  */
            case 0x14:  ad_rw_dp ( &Cpu::TRB, regs.fM );            break;
            case 0x1C:  ad_rw_ab ( &Cpu::TRB, regs.fM );            break;
                
                /* TSB  */
            case 0x04:  ad_rw_dp ( &Cpu::TSB, regs.fM );            break;
            case 0x0C:  ad_rw_ab ( &Cpu::TSB, regs.fM );            break;
            }
        }
    }

    
    /////////////////////////////////////////////////////////
    //  Interrupts!

    void Cpu::doInterrupt(IntType type)
    {
        bool is_sw = (type == IntType::Cop) || (type == IntType::Brk);
        if(is_sw)
        {
          //read_pc();  <- This is the opcode fetch -- this cycle happened already
            read_pc();  // Signature byte
        }
        else
        {
            read( regs.PBR | regs.PC );         // dummy read and IO cyc
            ioCyc();                            //   instead of op and signature reads
        }

        if(type == IntType::Reset)
        {
            ioCyc(3);       // don't actually push, do IO cycs instead
            regs.fE = true;
            regs.fM = true;
            regs.fX = true;
            regs.X.h = regs.Y.h = 0;
        }
        else
        {
            if(!regs.fE)        push( regs.PBR >> 16 );     // push PBR if in native mode
            push( regs.PC >> 8 );
            push( regs.PC & 0xFF );
            push( regs.getStatusByte(is_sw) );
        }

        regs.fD = false;
        regs.fI = true;
        regs.PBR = 0;

        // TODO:  Can interrupts be interrupted like on the NES?
        //   if so, add that here at some point.  Maybe.. it's just a bizarre edge case

        u16 vec;
        const char* tracemsg = nullptr;
        switch(type)
        {
        case IntType::Reset:    vec = 0xFFFC;                       resetPending = false;   tracemsg = "*** RESET ***";     break;
        case IntType::Abort:    vec = regs.fE ? 0xFFF8 : 0xFFE8;                            break;
        case IntType::Nmi:      vec = regs.fE ? 0xFFFA : 0xFFEA;    nmiPending = false;     tracemsg = "*** NMI ***";       break;
        case IntType::Irq:      vec = regs.fE ? 0xFFFE : 0xFFEE;                            tracemsg = "*** IRQ ***";       break;
        case IntType::Cop:      vec = regs.fE ? 0xFFF4 : 0xFFE4;                            break;
        case IntType::Brk:      vec = regs.fE ? 0xFFFE : 0xFFE6;                            break;
        }
        
        if(tracer && tracemsg)
            tracer->traceLine(tracemsg);

        checkInterruptPending();

        regs.PC  = read( vec );
        regs.PC |= read( vec+1 ) << 8;
    }

    void Cpu::signalNmi()
    {
        interruptPending = nmiPending = true;
    }

    void Cpu::signalIrq(u32 irq)
    {
        irqFlags |= irq;
        if(!regs.fI && irqFlags)
            interruptPending = true;
    }

    void Cpu::acknowledgeIrq(u32 irq)
    {
        irqFlags &= ~irq;
        checkInterruptPending();
    }

    void Cpu::checkInterruptPending()
    {
        interruptPending = resetPending || nmiPending;
        if(!regs.fI && irqFlags)
            interruptPending = true;
    }
}
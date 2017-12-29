
#include "smp.h"
#include "spcbus.h"
#include "smptracer.h"
#include "snesfile.h"

namespace sch
{
    inline u8   Smp::read(u16 a)        { return bus->read( a, cyc() );             }
    inline u8   Smp::dpRd(u8 a)         { return read( regs.DP | a );               }
    inline u8   Smp::pull()             { return read( 0x0100 | ++regs.SP );        }

    inline void Smp::write(u16 a, u8 v) { bus->write( a, v, cyc() );                }
    inline void Smp::dpWr(u8 a, u8 v)   { write( regs.DP | a, v );                  }
    inline void Smp::push(u8 v)         { write( 0x0100 | regs.SP--, v );           }
    
    inline void Smp::ioCyc()            { cyc();                                    }
    inline void Smp::ioCyc(int cycs)    { cyc(cycs);                                }
    
    Smp::Smp()
    {
        regs.clear();
        bus = nullptr;
        stopped = true;
        tracer = nullptr;
    }

    void Smp::resetWithFile(SpcBus* bs, const SnesFile& file)
    {
        // TODO reset timestamp?
        regs.PC     = file.smpRegs.PC;
        regs.A      = file.smpRegs.A;
        regs.X      = file.smpRegs.X;
        regs.Y      = file.smpRegs.Y;
        regs.SP     = file.smpRegs.SP;
        regs.setStatusByte( file.smpRegs.PSW );

        bus =       bs;
        stopped =   false;
    }

    void Smp::runTo(timestamp_t runto)
    {
        if(stopped)
        {
            forciblySetTimestamp(runto);
            return;
        }

        u8 op;
        while(getTick() < runto)
        {
            if(tracer)
                tracer->cpuTrace(regs, *bus);

            op = read(regs.PC++);

            switch(op)
            {
                /* ADC      */
            case 0x99:  ad_xi_yi( &Smp::ADC );              break;
            case 0x88:  ad_ac_im( &Smp::ADC );              break;
            case 0x86:  ad_ac_xi( &Smp::ADC );              break;
            case 0x97:  ad_ac_iy( &Smp::ADC );              break;
            case 0x87:  ad_ac_ix( &Smp::ADC );              break;
            case 0x84:  ad_ac_dp( &Smp::ADC );              break;
            case 0x94:  ad_ac_dx( &Smp::ADC );              break;
            case 0x85:  ad_ac_ab( &Smp::ADC );              break;
            case 0x95:  ad_ac_ax( &Smp::ADC );              break;
            case 0x96:  ad_ac_ay( &Smp::ADC );              break;
            case 0x89:  ad_dp_dp( &Smp::ADC );              break;
            case 0x98:  ad_dp_im( &Smp::ADC );              break;

                /* ADDW     */
            case 0x7A:  ad_ya_dp( &Smp::ADDW );             break;
                
                /* AND      */
            case 0x39:  ad_xi_yi( &Smp::AND );              break;
            case 0x28:  ad_ac_im( &Smp::AND );              break;
            case 0x26:  ad_ac_xi( &Smp::AND );              break;
            case 0x37:  ad_ac_iy( &Smp::AND );              break;
            case 0x27:  ad_ac_ix( &Smp::AND );              break;
            case 0x24:  ad_ac_dp( &Smp::AND );              break;
            case 0x34:  ad_ac_dx( &Smp::AND );              break;
            case 0x25:  ad_ac_ab( &Smp::AND );              break;
            case 0x35:  ad_ac_ax( &Smp::AND );              break;
            case 0x36:  ad_ac_ay( &Smp::AND );              break;
            case 0x29:  ad_dp_dp( &Smp::AND );              break;
            case 0x38:  ad_dp_im( &Smp::AND );              break;

                /* AND1     */
            case 0x4A:  ad_cc_mb( &Smp::AND1 );             break;
            case 0x6A:  ad_cc_mb( &Smp::AND1_inv );         break;

                /* ASL      */
            case 0x1C:  ASL(regs.A);        ioCyc();        break;
            case 0x0B:  ad_dp( &Smp::ASL );                 break;
            case 0x1B:  ad_dx( &Smp::ASL );                 break;
            case 0x0C:  ad_ab( &Smp::ASL );                 break;

                /* BBC      */
            case 0x13:  ad_dp_rl( &Smp::BBC<0x01> );        break;
            case 0x33:  ad_dp_rl( &Smp::BBC<0x02> );        break;
            case 0x53:  ad_dp_rl( &Smp::BBC<0x04> );        break;
            case 0x73:  ad_dp_rl( &Smp::BBC<0x08> );        break;
            case 0x93:  ad_dp_rl( &Smp::BBC<0x10> );        break;
            case 0xB3:  ad_dp_rl( &Smp::BBC<0x20> );        break;
            case 0xD3:  ad_dp_rl( &Smp::BBC<0x40> );        break;
            case 0xF3:  ad_dp_rl( &Smp::BBC<0x80> );        break;
                
                /* BBS      */
            case 0x03:  ad_dp_rl( &Smp::BBS<0x01> );        break;
            case 0x23:  ad_dp_rl( &Smp::BBS<0x02> );        break;
            case 0x43:  ad_dp_rl( &Smp::BBS<0x04> );        break;
            case 0x63:  ad_dp_rl( &Smp::BBS<0x08> );        break;
            case 0x83:  ad_dp_rl( &Smp::BBS<0x10> );        break;
            case 0xA3:  ad_dp_rl( &Smp::BBS<0x20> );        break;
            case 0xC3:  ad_dp_rl( &Smp::BBS<0x40> );        break;
            case 0xE3:  ad_dp_rl( &Smp::BBS<0x80> );        break;

                /* Branches */
            case 0x90:  ad_branch( regs.fC == 0 );                  break;  // BCC
            case 0xB0:  ad_branch( regs.fC != 0 );                  break;  // BCS
            case 0xF0:  ad_branch( (regs.fNZ & 0xFF) == 0 );        break;  // BEQ
            case 0x30:  ad_branch( (regs.fNZ & 0x180) != 0 );       break;  // BMI
            case 0xD0:  ad_branch( (regs.fNZ & 0xFF) != 0 );        break;  // BNE
            case 0x10:  ad_branch( (regs.fNZ & 0x180) == 0 );       break;  // BPL
            case 0x50:  ad_branch( regs.fV == 0 );                  break;  // BVC
            case 0x70:  ad_branch( regs.fV != 0 );                  break;  // BVS
            case 0x2F:  ad_branch( true );                          break;  // BRA

                /* CBNE     */
            case 0x2E:  ad_dp_rl( &Smp::CBNE );             break;
            case 0xDE:  ad_dx_rl( &Smp::CBNE );             break;
                
                /* CLR1     */
            case 0x12:  ad_dp( &Smp::CLR1<0x01> );          break;
            case 0x32:  ad_dp( &Smp::CLR1<0x02> );          break;
            case 0x52:  ad_dp( &Smp::CLR1<0x04> );          break;
            case 0x72:  ad_dp( &Smp::CLR1<0x08> );          break;
            case 0x92:  ad_dp( &Smp::CLR1<0x10> );          break;
            case 0xB2:  ad_dp( &Smp::CLR1<0x20> );          break;
            case 0xD2:  ad_dp( &Smp::CLR1<0x40> );          break;
            case 0xF2:  ad_dp( &Smp::CLR1<0x80> );          break;

                /* CMP      */
            case 0x79:  ad_xi_yi( &Smp::CMP, false );       break;
            case 0x68:  ad_ac_im( &Smp::CMP );              break;
            case 0x66:  ad_ac_xi( &Smp::CMP );              break;
            case 0x77:  ad_ac_iy( &Smp::CMP );              break;
            case 0x67:  ad_ac_ix( &Smp::CMP );              break;
            case 0x64:  ad_ac_dp( &Smp::CMP );              break;
            case 0x74:  ad_ac_dx( &Smp::CMP );              break;
            case 0x65:  ad_ac_ab( &Smp::CMP );              break;
            case 0x75:  ad_ac_ax( &Smp::CMP );              break;
            case 0x76:  ad_ac_ay( &Smp::CMP );              break;
            case 0xC8:  ad_xx_im( &Smp::CMP );              break;
            case 0x3E:  ad_xx_dp( &Smp::CMP );              break;
            case 0x1E:  ad_xx_ab( &Smp::CMP );              break;
            case 0xAD:  ad_yy_im( &Smp::CMP );              break;
            case 0x7E:  ad_yy_dp( &Smp::CMP );              break;
            case 0x5E:  ad_yy_ab( &Smp::CMP );              break;
            case 0x69:  ad_dp_dp( &Smp::CMP, true, false ); break;
            case 0x78:  ad_dp_im( &Smp::CMP, false );       break;

                /* CMPW     */
            case 0x5A:  ad_ya_dp( &Smp::CMPW, false );      break;

                /* DEC      */
            case 0x9C:  DEC(regs.A);            ioCyc();    break;
            case 0x1D:  DEC(regs.X);            ioCyc();    break;
            case 0xDC:  DEC(regs.Y);            ioCyc();    break;
            case 0x8B:  ad_dp( &Smp::DEC );                 break;
            case 0x9B:  ad_dx( &Smp::DEC );                 break;
            case 0x8C:  ad_ab( &Smp::DEC );                 break;
                
                /* EOR      */
            case 0x59:  ad_xi_yi( &Smp::EOR );              break;
            case 0x48:  ad_ac_im( &Smp::EOR );              break;
            case 0x46:  ad_ac_xi( &Smp::EOR );              break;
            case 0x57:  ad_ac_iy( &Smp::EOR );              break;
            case 0x47:  ad_ac_ix( &Smp::EOR );              break;
            case 0x44:  ad_ac_dp( &Smp::EOR );              break;
            case 0x54:  ad_ac_dx( &Smp::EOR );              break;
            case 0x45:  ad_ac_ab( &Smp::EOR );              break;
            case 0x55:  ad_ac_ax( &Smp::EOR );              break;
            case 0x56:  ad_ac_ay( &Smp::EOR );              break;
            case 0x49:  ad_dp_dp( &Smp::EOR );              break;
            case 0x58:  ad_dp_im( &Smp::EOR );              break;
                
                /* EOR1     */
            case 0x8A:  ad_cc_mb( &Smp::EOR1, true );       break;
                
                /* INC      */
            case 0xBC:  INC(regs.A);            ioCyc();    break;
            case 0x3D:  INC(regs.X);            ioCyc();    break;
            case 0xFC:  INC(regs.Y);            ioCyc();    break;
            case 0xAB:  ad_dp( &Smp::INC );                 break;
            case 0xBB:  ad_dx( &Smp::INC );                 break;
            case 0xAC:  ad_ab( &Smp::INC );                 break;
                
                /* LSR      */
            case 0x5C:  LSR(regs.A);        ioCyc();        break;
            case 0x4B:  ad_dp( &Smp::LSR );                 break;
            case 0x5B:  ad_dx( &Smp::LSR );                 break;
            case 0x4C:  ad_ab( &Smp::LSR );                 break;

                /* ALL THE FREAKING MOVS!!!!!   */
            case 0xAF:  ad_xp_ac( &Smp::MOVx    );          break;
            case 0xC6:  ad_xi_ac( &Smp::MOVx    );          break;
            case 0xD7:  ad_iy_ac( &Smp::MOVx    );          break;
            case 0xC7:  ad_ix_ac( &Smp::MOVx    );          break;
            case 0xE8:  ad_ac_im( &Smp::MOV     );          break;
            case 0xE6:  ad_ac_xi( &Smp::MOV     );          break;
            case 0xBF:  ad_ac_xp( &Smp::MOV     );          break;
            case 0xF7:  ad_ac_iy( &Smp::MOV     );          break;
            case 0xE7:  ad_ac_ix( &Smp::MOV     );          break;
            case 0x7D:  ad_ac_xx( &Smp::MOV     );          break;
            case 0xDD:  ad_ac_yy( &Smp::MOV     );          break;
            case 0xE4:  ad_ac_dp( &Smp::MOV     );          break;
            case 0xF4:  ad_ac_dx( &Smp::MOV     );          break;
            case 0xE5:  ad_ac_ab( &Smp::MOV     );          break;
            case 0xF5:  ad_ac_ax( &Smp::MOV     );          break;
            case 0xF6:  ad_ac_ay( &Smp::MOV     );          break;
            case 0xBD:  ad_sp_xx( &Smp::MOVx    );          break;
            case 0xCD:  ad_xx_im( &Smp::MOV     );          break;
            case 0x5D:  ad_xx_ac( &Smp::MOV     );          break;
            case 0x9D:  ad_xx_sp( &Smp::MOV     );          break;
            case 0xF8:  ad_xx_dp( &Smp::MOV     );          break;
            case 0xF9:  ad_xx_dy( &Smp::MOV     );          break;
            case 0xE9:  ad_xx_ab( &Smp::MOV     );          break;
            case 0x8D:  ad_yy_im( &Smp::MOV     );          break;
            case 0xFD:  ad_yy_ac( &Smp::MOV     );          break;
            case 0xEB:  ad_yy_dp( &Smp::MOV     );          break;
            case 0xFB:  ad_yy_dx( &Smp::MOV     );          break;
            case 0xEC:  ad_yy_ab( &Smp::MOV     );          break;
            case 0xFA:  ad_dp_dp( &Smp::MOVx, false );      break;
            case 0xD4:  ad_dx_ac( &Smp::MOVx    );          break;
            case 0xDB:  ad_dx_yy( &Smp::MOVx    );          break;
            case 0xD9:  ad_dy_xx( &Smp::MOVx    );          break;
            case 0x8F:  ad_dp_im( &Smp::MOVx    );          break;
            case 0xC4:  ad_dp_ac( &Smp::MOVx    );          break;
            case 0xD8:  ad_dp_xx( &Smp::MOVx    );          break;
            case 0xCB:  ad_dp_yy( &Smp::MOVx    );          break;
            case 0xD5:  ad_ax_ac( &Smp::MOVx    );          break;
            case 0xD6:  ad_ay_ac( &Smp::MOVx    );          break;
            case 0xC5:  ad_ab_ac( &Smp::MOVx    );          break;
            case 0xC9:  ad_ab_xx( &Smp::MOVx    );          break;
            case 0xCC:  ad_ab_yy( &Smp::MOVx    );          break;
                
                /* MOV1     */
            case 0xAA:  ad_cc_mb( &Smp::MOV1_c );           break;
            case 0xCA:  ad_mb_cc( &Smp::MOV1   );           break;

                /* MOVW     */
            case 0xBA:  ad_ya_dp( &Smp::MOVW );             break;
            case 0xDA:  op_MOVW_write();                    break;

                /* NOT1     */
            case 0xEA:  ad_mb( &Smp::NOT1 );                break;
                
                /* OR       */
            case 0x19:  ad_xi_yi( &Smp::OR );               break;
            case 0x08:  ad_ac_im( &Smp::OR );               break;
            case 0x06:  ad_ac_xi( &Smp::OR );               break;
            case 0x17:  ad_ac_iy( &Smp::OR );               break;
            case 0x07:  ad_ac_ix( &Smp::OR );               break;
            case 0x04:  ad_ac_dp( &Smp::OR );               break;
            case 0x14:  ad_ac_dx( &Smp::OR );               break;
            case 0x05:  ad_ac_ab( &Smp::OR );               break;
            case 0x15:  ad_ac_ax( &Smp::OR );               break;
            case 0x16:  ad_ac_ay( &Smp::OR );               break;
            case 0x09:  ad_dp_dp( &Smp::OR );               break;
            case 0x18:  ad_dp_im( &Smp::OR );               break;
                
                /* OR1      */
            case 0x0A:  ad_cc_mb( &Smp::OR1, true     );    break;
            case 0x2A:  ad_cc_mb( &Smp::OR1_inv, true );    break;
                
                /* ROL      */
            case 0x3C:  ROL(regs.A);        ioCyc();        break;
            case 0x2B:  ad_dp( &Smp::ROL );                 break;
            case 0x3B:  ad_dx( &Smp::ROL );                 break;
            case 0x2C:  ad_ab( &Smp::ROL );                 break;
                
                /* ROR      */
            case 0x7C:  ROR(regs.A);        ioCyc();        break;
            case 0x6B:  ad_dp( &Smp::ROR );                 break;
            case 0x7B:  ad_dx( &Smp::ROR );                 break;
            case 0x6C:  ad_ab( &Smp::ROR );                 break;
                
                /* SBC      */
            case 0xB9:  ad_xi_yi( &Smp::SBC );              break;
            case 0xA8:  ad_ac_im( &Smp::SBC );              break;
            case 0xA6:  ad_ac_xi( &Smp::SBC );              break;
            case 0xB7:  ad_ac_iy( &Smp::SBC );              break;
            case 0xA7:  ad_ac_ix( &Smp::SBC );              break;
            case 0xA4:  ad_ac_dp( &Smp::SBC );              break;
            case 0xB4:  ad_ac_dx( &Smp::SBC );              break;
            case 0xA5:  ad_ac_ab( &Smp::SBC );              break;
            case 0xB5:  ad_ac_ax( &Smp::SBC );              break;
            case 0xB6:  ad_ac_ay( &Smp::SBC );              break;
            case 0xA9:  ad_dp_dp( &Smp::SBC );              break;
            case 0xB8:  ad_dp_im( &Smp::SBC );              break;
                
                /* SET1     */
            case 0x02:  ad_dp( &Smp::SET1<0x01> );          break;
            case 0x22:  ad_dp( &Smp::SET1<0x02> );          break;
            case 0x42:  ad_dp( &Smp::SET1<0x04> );          break;
            case 0x62:  ad_dp( &Smp::SET1<0x08> );          break;
            case 0x82:  ad_dp( &Smp::SET1<0x10> );          break;
            case 0xA2:  ad_dp( &Smp::SET1<0x20> );          break;
            case 0xC2:  ad_dp( &Smp::SET1<0x40> );          break;
            case 0xE2:  ad_dp( &Smp::SET1<0x80> );          break;
                
                /* SUBW     */
            case 0x9A:  ad_ya_dp( &Smp::SUBW );             break;
                
                /* TCALL    */
            case 0x01:  op_TCALL( 0xDE );                   break;
            case 0x11:  op_TCALL( 0xDC );                   break;
            case 0x21:  op_TCALL( 0xDA );                   break;
            case 0x31:  op_TCALL( 0xD8 );                   break;
            case 0x41:  op_TCALL( 0xD6 );                   break;
            case 0x51:  op_TCALL( 0xD4 );                   break;
            case 0x61:  op_TCALL( 0xD2 );                   break;
            case 0x71:  op_TCALL( 0xD0 );                   break;
            case 0x81:  op_TCALL( 0xCE );                   break;
            case 0x91:  op_TCALL( 0xCC );                   break;
            case 0xA1:  op_TCALL( 0xCA );                   break;
            case 0xB1:  op_TCALL( 0xC8 );                   break;
            case 0xC1:  op_TCALL( 0xC6 );                   break;
            case 0xD1:  op_TCALL( 0xC4 );                   break;
            case 0xE1:  op_TCALL( 0xC2 );                   break;
            case 0xF1:  op_TCALL( 0xC0 );                   break;
                
                /* TCLR1    */
            case 0x4E:  ad_ab( &Smp::TCLR1, true );         break;
                
                /* TSET1    */
            case 0x0E:  ad_ab( &Smp::TSET1, true );         break;

                /* flag toggling    */
            case 0x60:  regs.fC = 0;            ioCyc();    break;  // CLRC
            case 0x20:  regs.DP = 0;            ioCyc();    break;  // CLRP
            case 0xE0:  regs.fV = regs.fH = 0;  ioCyc();    break;  // CLRV
            case 0xC0:  regs.fI = 0;            ioCyc();    break;  // DI
            case 0xA0:  regs.fI = 1;            ioCyc();    break;  // EI
            case 0xED:  regs.fC = !regs.fC;     ioCyc(2);   break;  // NOTC
            case 0x80:  regs.fC = 1;            ioCyc();    break;  // SETC
            case 0x40:  regs.DP = 0x0100;       ioCyc();    break;  // SETP

                /* Stack ops    */
            case 0x8E:  regs.setStatusByte(ad_pop());       break;
            case 0xAE:  regs.A = ad_pop();                  break;
            case 0xCE:  regs.X = ad_pop();                  break;
            case 0xEE:  regs.Y = ad_pop();                  break;
            case 0x0D:  ad_push( regs.getStatusByte() );    break;
            case 0x2D:  ad_push( regs.A );                  break;
            case 0x4D:  ad_push( regs.X );                  break;
            case 0x6D:  ad_push( regs.Y );                  break;

                /* one-off ops  */
            case 0x0F:  op_BRK();                           break;
            case 0x3F:  op_CALL();                          break;
            case 0xDF:  op_DAA();                           break;
            case 0xBE:  op_DAS();                           break;
            case 0xFE:  op_DBNZ();                          break;
            case 0x6E:  op_DBNZ_mem();                      break;
            case 0x1A:  op_DECW();                          break;
            case 0x9E:  op_DIV();                           break;
            case 0x3A:  op_INCW();                          break;
            case 0x5F:  op_JMP();                           break;
            case 0x1F:  op_JMP_X();                         break;
            case 0xCF:  op_MUL();                           break;
            case 0x00:  ioCyc();                            break;  // NOP
            case 0x4F:  op_PCALL();                         break;
            case 0x6F:  op_RET();                           break;
            case 0x7F:  op_RET1();                          break;
            case 0x9F:  op_XCN();                           break;

            case 0xEF:
            case 0xFF:
                stopped = true;
                forciblySetTimestamp(runto);
                break;
            }
        }
    }


}
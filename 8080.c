// this file contains the 8080 emulator
// DAA and auxiliary flags are not handled for the sake of preserving my sanity

#include "invaders.h"

#define HL  ((uint16_t)(cpu->reg[4] << 8 | cpu->reg[5]))
#define BC  ((uint16_t)(cpu->reg[0] << 8 | cpu->reg[1]))
#define DE  ((uint16_t)(cpu->reg[2] << 8 | cpu->reg[3]))
#define ADDRESS (((uint16_t)*read_mem(cpu->pc+1)) << 8 | *read_mem(cpu->pc))

#define DAD(rp) {p1 = HL;\
                 p1 += rp ;\
                 cpu->reg[4] = p1 >> 8 ;\
                 cpu->reg[5] = p1 ;}


#define POP_PC    { cpu->pc = *read_mem(cpu->sp++) ;\
                    cpu->pc |= (uint16_t) *read_mem(cpu->sp++) << 8 ;}

#define PUSH_PC(pc)  { *read_mem(--(cpu->sp)) = (pc) >> 8 ; *read_mem(--(cpu->sp)) = (pc) ; }

#define JUMP_IF_FLAG(flag) cpu->pc = (flag) ? ( ADDRESS ) : ( cpu->pc+2 );

#define CALL_IF_FLAG(flag) { if ( flag )\
                             {\
                                 PUSH_PC(cpu->pc+2) ;\
                                 cpu->pc = ADDRESS ;\
                                 cycle_count += 6 ;\
                             }\
                             else\
                             {\
                                 cpu->pc+=2 ;\
                             }};
#define RETURN_IF_FLAG(flag) { if (flag)\
                               {\
                                   POP_PC ;\
                                   cycle_count += 6 ;\
                               }} ;

uint8_t mem [mem_size] ; // for space invaders only and should be changed according to implementation


static const uint8_t opcode_cycles[] = {
//  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    4,  10, 7,  5,  5,  5,  7,  4,  4,  10, 7,  5,  5,  5,  7,  4,  // 0
    4,  10, 7,  5,  5,  5,  7,  4,  4,  10, 7,  5,  5,  5,  7,  4,  // 1
    4,  10, 16, 5,  5,  5,  7,  4,  4,  10, 16, 5,  5,  5,  7,  4,  // 2
    4,  10, 13, 5,  10, 10, 10, 4,  4,  10, 13, 5,  5,  5,  7,  4,  // 3
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  // 4
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  // 5
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  // 6
    7,  7,  7,  7,  7,  7,  7,  7,  5,  5,  5,  5,  5,  5,  7,  5,  // 7
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // 8
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // 9
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // A
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // B
    5,  10, 10, 10, 11, 11, 7,  11, 5,  10, 10, 10, 11, 11, 7,  11, // C
    5,  10, 10, 10, 11, 11, 7,  11, 5,  10, 10, 10, 11, 11, 7,  11, // D
    5,  10, 10, 18, 11, 11, 7,  11, 5,  5,  10, 5,  11, 11, 7,  11, // E
    5,  10, 10, 4,  11, 11, 7,  11, 5,  5,  10, 4,  11, 11, 7,  11  // F
}; // for conditional returns and subroutines calls we add +6 to the cycle count if the condition is met




uint8_t execute_opcode ( cpu_8080 *cpu )
{
    uint16_t p = 0 ; // used as temp
    uint32_t p1 = 0 ; // also used as temp
    uint8_t opcode = *read_mem(cpu -> pc++) ;

    // extract some bit fields from the initial byte

    uint8_t low = opcode & 0x07 , cycle_count = 0 ; // extract lowest 3 bits
    uint8_t high = (opcode & 0x38) >> 3 ; // extract 4,5,6th bit
    bool    bit_3 = opcode & 0x10 ;
    //printf(" WARNING HL: %04x and next instruction: %02x at PC : %04x \n", HL , opcode , cpu->pc-1 );
    switch ( opcode )
    {
        case 0x00: // NOP : no operation
            break ;
        case 0x04: case 0x0C: case 0x14: case 0x1C: case 0x24: case 0x2C: case 0x34: case 0x3C: // INR : increment cpu->register or memory
            p = (high == 6) ? ++*read_mem(HL) : ++cpu->reg[high] ;
            set_ZPS ( p , cpu ) ;
            break ;
        case 0x05: case 0x0D: case 0x15: case 0x1D: case 0x25: case 0x2D: case 0x35: case 0x3D: // DCR : decrement cpu->register or memory
            p = (high == 6) ? --*read_mem(HL) : --cpu->reg[high] ;
            set_ZPS ( p , cpu ) ;
            break;
        case 0x27 : // DAA : decimal adjust accumulator
        /*
            if ( p = (0x0F & cpu->reg[7]) > 9 || cpu->flags.AC )
                cpu->reg[7] += 6 ;
                cpu->flags.AC = p >> 4 ? 1 : 0 ;
            if ( (p = (0xF0 & cpu->reg[7])) >> 4 || cpu->flags.C )
                cpu->reg[7] += (6 << 4) ; // since its the higher order bit
                cpu->flags.C = p >> 4 ? 1 : 0 ; */
            break;
        case 0x2F : // CMA : complement accumulator
            cpu->reg[7] = ~cpu->reg[7] ;
            break ;
        case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47: case 0x48:// MOV : move
        case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F: case 0x50: case 0x51:
        case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: case 0x58: case 0x59: case 0x5A:
        case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F: case 0x60: case 0x61: case 0x62: case 0x63:
        case 0x64: case 0x65: case 0x66: case 0x67: case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C:
        case 0x6D: case 0x6E: case 0x6F: case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75:
        case 0x77: case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:
            if ( high == 6 )   // memory reference as destination
                *read_mem(HL) = cpu->reg[low] ;
            else if ( low == 6 ) // memory reference as source
                cpu->reg[high] = *read_mem(HL) ;
            else  // can't be both at the same time
                cpu->reg[high] = cpu->reg[low] ;
            break ;
        case 0x02: case 0x12: // STAX : store accumulator in memory addresses by cpu->register pair
            *read_mem( bit_3 ? DE : BC ) = cpu->reg[7] ; // 0 rp BC else DE
            break ;
        case 0x0A: case 0x1A : // LDAX : load accumulator from memory addressed cpu->register pair
            cpu->reg[7] = *read_mem( bit_3 ? DE : BC ) ;
            break ;
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: // ADD add memory or cpu->register to accumulator
            p = cpu->reg[7] ; // we used a 16bit variable to be able to capture
            p += ( low == 6 ) ? *read_mem(HL) : cpu->reg[low] ;
            cpu->reg[7] = p ;
            cpu->flags.C = p >> 8 ;
            set_ZPS ( p , cpu ) ;
            break ;
        case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F: // ADC add but with carry flag
            p = cpu->reg[7];
            p += (( low == 6 ) ? *read_mem(HL) : cpu->reg[low]) + cpu->flags.C ;
            cpu->reg[7] = p ;
            cpu->flags.C = p >> 8 ;
            set_ZPS ( p , cpu ) ;
            break ;
        case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: // SUB subtract cpu->register or memory from accumulator
            p = cpu->reg[7] ;
            p -= ( low == 6 ) ? *read_mem(HL) : cpu->reg[low] ;
            cpu->reg[7] = p ;
            cpu->flags.C = !( p >> 8 ) ;
            set_ZPS ( p , cpu );
            break ;
        case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9E: case 0x9F: // SBB substruct cpu->register or memory with borrow
            p = cpu->reg[7] ;
            p -= ((( low == 6 ) ? *read_mem(HL) : cpu->reg[low]) + cpu->flags.C) ;
            cpu->reg[7] = p ;
            cpu->flags.C = !( p >> 8 ) ;
            set_ZPS ( p , cpu );
            break ;
        case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA6: case 0xA7: // ANA logical and cpu->register or memory with accumulator
            cpu->reg[7] &= ( low == 6 ) ? *read_mem(HL) : cpu->reg[low] ;
            cpu->flags.C = 0 ;
            set_ZPS ( cpu->reg[7] , cpu ) ;
            break ;
        case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF: // XRA logical exclusive OR cpu->register or memory with accumulator
            cpu->reg[7] ^= ( low == 6 ) ? *read_mem(HL) : cpu->reg[low] ;
            cpu->flags.C = 0 ;
            set_ZPS ( cpu->reg[7] , cpu ) ;
            break ;
        case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: case 0xB7: // ORA logical OR cpu->register or memory with accumulator
            cpu->reg[7] |= ( low == 6 ) ? *read_mem(HL) : cpu->reg[low] ;
            cpu->flags.C = 0 ;
            set_ZPS ( cpu->reg[7] , cpu ) ;
            break ;
        case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBE: case 0xBF: // CMP compare cpu->register or memory with accumulator
            p = cpu->reg[7];
            p -= ( low == 6 ) ? *read_mem(HL) : cpu->reg[low] ;
            cpu->flags.C = (p >> 8) ;
            set_ZPS ( p , cpu ) ;
            break ;
        case 0x07 : // RLC rotate accumulator left with carry
            cpu->reg[7] = cpu->reg[7] << 1 | (cpu->flags.C = (cpu->reg[7] & 0x80) >> 7  ) ;
            break ;
        case 0x0F : // RRC rotate accumulator right with carry
            cpu->reg[7] = cpu->reg[7] >> 1 | (cpu->flags.C = (cpu->reg[7] & 1) ) << 7  ;
            break ;
        case 0x17 : // RAL rotate accumulator left through carry
            p = cpu->reg[7] >> 7 ;
            cpu->reg[7] = cpu->reg[7] << 1 | cpu->flags.C ;
            cpu->flags.C = p ;
            break ;
        case 0x1F : // RAR rotate accumulator right through carry
            p = cpu->reg[7] & 0x01 ;
            cpu->reg[7] = cpu->reg[7] >> 1 | cpu->flags.C << 7 ;
            cpu->flags.C = p ;
            break ;
        case 0xC5: // PUSH BC onto stack
            *read_mem(--cpu->sp) = cpu->reg[0];
            *read_mem(--cpu->sp) = cpu->reg[1];
            break ;
        case 0xD5: // PUSH DE onto stack
            *read_mem(--cpu->sp) = cpu->reg[2] ;
            *read_mem(--cpu->sp) = cpu->reg[3] ;
            break ;
        case 0xE5: // PUSH HL onto stack
            *read_mem(--cpu->sp) = cpu->reg[4];
            *read_mem(--cpu->sp) = cpu->reg[5];
            break ;
        case 0xF5: // PUSH program status word (PSW) onto stack
            *read_mem(--cpu->sp) = cpu->reg[7];
            // construct flag byte // S,Z,0,AC,0,P,1,C
            p |= cpu->flags.S << 7 ;
            p |= cpu->flags.Z << 6 ;
            p |= cpu->flags.AC << 4;
            p |= cpu->flags.P << 2 ;
            p |= cpu->flags.C | 2 ;
            *read_mem(--cpu->sp) = p ;
            break;
        case 0xC1: // POP pop BC off stack
            cpu->reg[1] = *read_mem(cpu->sp++) ;
            cpu->reg[0] = *read_mem(cpu->sp++) ;
            break ;
        case 0xD1: // POP DE off stack
            cpu->reg[3] = *read_mem(cpu->sp++) ;
            cpu->reg[2] = *read_mem(cpu->sp++) ;
            break ;
        case 0xE1: // POP HL off stack
            cpu->reg[5] = *read_mem(cpu->sp++) ;
            cpu->reg[4] = *read_mem(cpu->sp++) ;
            break ;
        case 0xF1: // POP accumulator and flags byte from stack
            p = *read_mem(cpu->sp++) ;
            cpu->reg[7] = *read_mem(cpu->sp++) ;
            // deconstruct byte into flags
            cpu->flags.S = (p >> 7) & 1;
            cpu->flags.Z = (p >> 6) & 1;
            cpu->flags.AC= (p >> 4) & 1;
            cpu->flags.P = (p >> 2) & 1;
            cpu->flags.C = p & 1;
            break ;
        case 0x09: // DAD double add BC
            DAD(BC);
            cpu->flags.C = p1 >> 16 ;
            break;
        case 0x19: // DAD double add DE
            DAD(DE);
            cpu->flags.C = p1 >> 16 ;
            break ;
        case 0x29: // DAD double add HL
            DAD(HL);
            cpu->flags.C = p1 >> 16 ;
            break ;
        case 0x39: // DAD double add stack pointer
            DAD(cpu->sp);
            cpu->flags.C = p1 >> 16 ;
            break ;
        case 0x03: // INX increment register pair BC
            p1 = BC + 1 ;
            cpu->reg[0] = p1 >> 8 ;
            cpu->reg[1] = p1 ;
            break ;
        case 0x13: // INX increment register pair DE
            p1 = DE + 1 ;
            cpu->reg[2] = p1 >> 8 ;
            cpu->reg[3] = p1 ;
            break;
        case 0x23: // INX increment register pair HL
            p1 = HL + 1 ;
            cpu->reg[4] = p1 >> 8 ;
            cpu->reg[5] = p1 ;
            break ;
        case 0x33: // INX increment register pair SP
            ++cpu->sp;
            break ;
        case 0x0B: // DCX decrement register pair BC
            p1 = BC - 1 ;
            cpu->reg[0] = p1 >> 8 ;
            cpu->reg[1] = p1 ;
            break ;
        case 0x1B: // DCX decrement register pair DE
            p1 = DE - 1 ;
            cpu->reg[2] = p1 >> 8 ;
            cpu->reg[3] = p1 ;
            break;
        case 0x2B: // DCX decrement register pair HL
            p1 = HL - 1 ;
            cpu->reg[4] = p1 >> 8 ;
            cpu->reg[5] = p1 ;
            break ;
        case 0x3B: // DCX decrement register pair SP
            --cpu->sp;
            break ;
        case 0xEB: // XCHG exchange registers
            p = cpu->reg[2];
            cpu->reg[2] = cpu->reg[4];
            cpu->reg[4] = p;
            p = cpu->reg[3];
            cpu->reg[3] = cpu->reg[5];
            cpu->reg[5] = p;
            break ;
        case 0xE3: // XTHL exchange stack
            p = cpu->reg[5] ;
            cpu->reg[5] = *read_mem(cpu->sp);
            *read_mem(cpu->sp) = p;
            p = cpu->reg[4];
            cpu->reg[4] = *read_mem(cpu->sp+1);
            *read_mem(cpu->sp+1) = p ;
            break ;
        case 0xF9: // SPHL load sp from H and L
            cpu->sp = HL ;
            break ;
        case 0x01: // LXI load immediate into BC
            cpu->reg[1] = *read_mem(cpu->pc++);
            cpu->reg[0] = *read_mem(cpu->pc++);
            break;
        case 0x11: //LXI load immediate into DE
            cpu->reg[3] = *read_mem(cpu->pc++);
            cpu->reg[2] = *read_mem(cpu->pc++);
            break;
        case 0x21: // LXI load immediate into HL
            cpu->reg[5] = *read_mem(cpu->pc++);
            cpu->reg[4] = *read_mem(cpu->pc++);
            break;
        case 0x31: // LXI load immediate into SP
            cpu->sp = ADDRESS ;
            cpu->pc += 2 ;
            break ;
        case 0x06: case 0x0E: case 0x16: case 0x1E: case 0x26: case 0x2E: case 0x36: case 0x3E: // MVI move immediate data
            ;
            uint8_t *x = high == 6 ? read_mem(HL) : &(cpu->reg[high]) ;
            *x = *read_mem(cpu->pc++) ;
            break;
        case 0xC6: // ADI add immediate to accumulator
            p = cpu->reg[7];
            p += *read_mem(cpu->pc++);
            cpu->reg[7] = p ;
            cpu->flags.C = p >> 8 ;
            set_ZPS ( p , cpu ) ;
            break ;
        case 0xCE: // ACI add immediate with carry to accumulator
            p = cpu->reg[7] + cpu->flags.C ;
            p += *read_mem(cpu->pc++);
            cpu->reg[7] = p;
            cpu->flags.C = p >> 8 ;
            set_ZPS ( p , cpu ) ;
            break ;
        case 0xD6: // SUI substract immediate from accumulator
            p = cpu->reg[7];
            p -= *read_mem(cpu->pc++);
            cpu->reg[7] = p;
            cpu->flags.C = (p >> 8) ;
            set_ZPS ( p , cpu );
            break ;
        case 0xDE: // SBI substract immediate from accumulator
            p = cpu->reg[7];
            p -= ( *read_mem(cpu->pc++) + cpu->flags.C ) ;
            cpu->flags.C = (p >> 8) ;
            set_ZPS ( p , cpu ) ;
            cpu->reg[7] = p;
            break ;
        case 0xE6: // ANI and immediate with accumulator
            cpu->reg[7] &= *read_mem(cpu->pc++) ;
            cpu->flags.C = 0 ;
            set_ZPS( cpu->reg[7] , cpu ) ;
            break ;
        case 0xEE: // XRI exclusive or immediate with accumulator
            cpu->reg[7] ^= *read_mem(cpu->pc++);
            cpu->flags.C = 0 ;
            set_ZPS( cpu->reg[7] , cpu ) ;
            break ;
        case 0xF6: // ORI immediate OR with accumulator
            cpu->reg[7] |= *read_mem(cpu->pc++) ;
            cpu->flags.C = 0 ;
            set_ZPS( cpu->reg[7] , cpu ) ;
            break ;
        case 0xFE: // CPI compare immediate with accumulator
            p = cpu->reg[7];
            p -= *read_mem(cpu->pc++);
            set_ZPS( p , cpu ) ;
            cpu->flags.C = (p >> 8) ;
            break;
        case 0x32: // STA store accumulator direct
            *read_mem(ADDRESS) = cpu->reg[7];
            cpu->pc += 2 ;
            break ;
        case 0x3A: // LDA load accumulator direct
            cpu->reg[7] = *read_mem(ADDRESS) ;
            cpu->pc += 2 ;
            break ;
        case 0x22: // SHLD store H and L direct
            p = ADDRESS ;
            *read_mem(p) = cpu->reg[5];
            *read_mem(p+1) = cpu->reg[4];
            cpu->pc += 2 ;
            break ;
        case 0x2A: // LHLD load H and L direct
            p = ADDRESS ;
            cpu->reg[5] = *read_mem(p) ;
            cpu->reg[4] = *read_mem(p+1);
            cpu->pc += 2 ;
            break ;
        case 0xE9: // PCHL load program counter
            cpu->pc = HL ;
            break ;
        case 0xC3: // JMP jump unconditionnaly
            cpu->pc = ADDRESS ;
            break ;
        case 0xDA: // JC jump if carry set
            JUMP_IF_FLAG ( cpu->flags.C ) ;
            break ;
        case 0xD2: // JNC jump if no carry
            JUMP_IF_FLAG ( !cpu->flags.C ) ;
            break ;
        case 0xCA: // JZ jump if zero
            JUMP_IF_FLAG ( cpu->flags.Z ) ;
            break ;
        case 0xC2: // JNZ jump if not zero
            JUMP_IF_FLAG ( !cpu->flags.Z ) ;
            break ;
        case 0xFA: // JM jump if minus
            JUMP_IF_FLAG ( cpu->flags.S ) ;
            break ;
        case 0xF2: // JP jump if positive
            JUMP_IF_FLAG ( !cpu->flags.S ) ;
            break ;
        case 0xEA: // JPE jump if parity even
            JUMP_IF_FLAG ( cpu->flags.P ) ;
            break ;
        case 0xE2: // JPO jump if parity odd
            JUMP_IF_FLAG ( !cpu->flags.P ) ;
            break ;
        case 0xCD: // CALL jump to subroutine
            CALL_IF_FLAG ( true ) ;
            break ;
        case 0xDC: // CC call if carry set
            CALL_IF_FLAG( cpu->flags.C ) ;
            break ;
        case 0xD4: // CNC call if no carry
            CALL_IF_FLAG( !cpu->flags.C ) ;
            break ;
        case 0xCC: // CZ call if zero
            CALL_IF_FLAG( cpu->flags.Z ) ;
            break ;
        case 0xC4: // CNZ call if not zero
            CALL_IF_FLAG( !cpu->flags.Z ) ;
            break ;
        case 0xFC: // CM call if minus
            CALL_IF_FLAG( cpu->flags.S ) ;
            break ;
        case 0xF4: // CP call if plus
            CALL_IF_FLAG( !cpu->flags.S ) ;
            break ;
        case 0xEC: // CPE call if parity even
            CALL_IF_FLAG( cpu->flags.P ) ;
            break ;
        case 0xE4: // Call if parity odd
            CALL_IF_FLAG( !cpu->flags.P ) ;
            break ;
        case 0xC9: // RET return
            RETURN_IF_FLAG ( true ) ;
            break ;
        case 0xD8: // RC return if carry set
            RETURN_IF_FLAG( cpu->flags.C ) ;
            break ;
        case 0xD0: // RNC return if no carry
            RETURN_IF_FLAG( !cpu->flags.C ) ;
            break ;
        case 0xC8: // RZ return if zero
            RETURN_IF_FLAG( cpu->flags.Z ) ;
            break ;
        case 0xC0: // RNZ return if not zero
            RETURN_IF_FLAG( !cpu->flags.Z ) ;
            break ;
        case 0xF8: // RM return if minus
            RETURN_IF_FLAG( cpu->flags.S ) ;
            break ;
        case 0xF0: // RP return if plus
            RETURN_IF_FLAG( !cpu->flags.S ) ;
            break ;
        case 0xE8: // RPE return if parity even
            RETURN_IF_FLAG( cpu->flags.P ) ;
            break ;
        case 0xE0:
            RETURN_IF_FLAG( !cpu->flags.P ) ;
            break ;
        case 0xC7: case 0xCF: case 0xD7: case 0xDF: case 0xE7: case 0xEF: case 0xF7: case 0xFF: // RST restart
            PUSH_PC(cpu->pc) ;
            cpu->pc = high << 3 ;
            break ;
        case 0xFB: // EI enable interrupts
            cpu->interrupt_ff = true ;
            break ;
        case 0xF3: // DI disable interrrupts
            cpu->interrupt_ff = false ;
            break ;
        case 0xDB: // IN input , reads a byte from device #byte and replaces accumulator
            cpu->reg[7] = port_read ( *read_mem(cpu->pc++) , cpu ) ; // system specific
            break ;
        case 0xD3: // OUT output, write a byte from device #byte from accumulator
            port_write ( cpu->reg[7] , *read_mem(cpu->pc++) , cpu ) ; // system specific
            break ;
        case 0x3F: // CMC complement carry
             cpu->flags.C = ~cpu->flags.C ;
            break ;
        case 0x37: // STC set carry
            cpu->flags.C = 1 ;
            break ;
        case 0x76: // HLT HALT instruction , halts until interrupt occurs
            break ;

    }

    return cycle_count+=opcode_cycles[opcode] ;

}

void set_ZPS (uint16_t sum , cpu_8080 *cpu )
{
    cpu->flags.Z = sum ? 0 : 1 ;
    cpu->flags.S = sum & 0x80 ? 1 : 0 ;
    cpu -> flags.P = sum & 1 ? 0 : 1 ;
}

void generate_interrupt ( uint16_t address , cpu_8080 *cpu )
{
 // in a real system the interrupting device places a RST instruction that the CPU reads from the bus
 // but the way this program was designed ( bad ) doing the pushes manually saves time and overhead
    PUSH_PC(cpu->pc) ;
    cpu->pc = address ;
    cpu->interrupt_ff = 0 ; // disable interrupts
}

uint8_t * read_mem ( uint16_t address )
{ // since 0x4000-0x6000 is a RAM mirror we have to use this function
  // we dont't have to worry about illegal reads or writes since the code is known to work
  if ( address >= 0x6000 )
    printf("WARNING : ILLEGAL READ/WRITE at address 0x%04x\n", address  );

  return mem + (( address >= 0x4000 ) ? address - 0x2000 : address ) ;
}

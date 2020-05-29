#ifndef INVADERS_H_
#define INVADERS_H_

#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_image.h>
#include <sys/time.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL_mixer.h>


typedef struct
{
    uint8_t port_dir[2] ; //0 for data when read by processor and 1 for data written by processor
} port_8080 ;



typedef struct
{
    //
    uint8_t reg[8] ; // B,C,D,E,H,L,M ( not used ),A
    /* instead of defining each register alone we could take advantage of the structure of the 8080
    assembly opcodes which define which register it is using ( source or destination ) as a field of 3 bits  */


    uint16_t pc , sp ;
    bool interrupt_ff ;
    // port slots
    port_8080 *port[256] ; // since we use a byte to address the ports


    struct {

        uint8_t S : 1 ; // sign
        uint8_t Z : 1 ; // zero
        uint8_t AC : 1 ; // auxiliary carry
        uint8_t P : 1 ; // parity
        uint8_t C : 1 ; // carry

        // the A cpu->register and the status bits together are called the program status word or PSW

    } flags ;

} cpu_8080 ;

#define VIDEO_MEM_START 0x2400
#define H 256
#define W 224
#define CYCLE_PRE_FRAME 33334 // since the 8080 runs at 2Mhz and refresh rate = 60Hz
#define mem_size 0x4000
#define program_start 0


void init ( FILE *program , cpu_8080 *cpu , uint8_t flags ) ;
void run_cpu ( cpu_8080 *cpu, int cycle_count ) ;
uint8_t port_read ( uint8_t port_number , cpu_8080 *cpu ) ;
void port_write ( uint8_t value , uint8_t port_number , cpu_8080 *cpu ) ;
void extract_pixels ( uint32_t *pixels  ) ;
void set_clear_bit ( uint8_t *number , int bit_number , bool state ) ;
uint8_t * read_mem ( uint16_t address ) ;
void set_ZPS (uint16_t sum , cpu_8080 *cpu ) ;
void generate_interrupt ( uint16_t address , cpu_8080 *cpu ) ;



#endif

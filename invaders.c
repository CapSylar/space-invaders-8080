

#include "8080.c"
#include "audio.c"
#include "invaders.h"


uint16_t port4_mem ;
uint32_t pixels[H*W] ;


int main ( int argc , char **argv  )
{
    int c ;
    char *help = "\n-h : list all conmmands\n-c : disable coin info ( default ):on\
                  \n-l : number of lives (0:3,1:4,2:5,3:6)\n-f : alternative binary file name (default): invaders.bin\
                  \n-b : bonus life at 0:1500 1:1000 ( default ):1\nGAME CONTROLS:\nKEY_1 : player 1 start button\nKEY_2 : player 2 start button\
                  \nKEY_C : insert coin into machine\nKEY_Q : player 1 left button\nKEY_W : player 1 shoot button\
                  \nKEY_E : player 1 right button\nKEY_UP_ARROW : player 2 shoot button\
                  \nKEY_RIGHT_ARROW : player 2 right button\nKEY_LEFT_ARROW : player 2 left button\n" ;
    uint8_t option_flags = 0 , response ; // 0 and 1 for number of lives, 3 for extra life and bit 7 for coin info
    char *filename = "invaders.bin" ;

    while ( --argc > 0 && (*++argv)[0] == '-' )
        while ( c = *++argv[0] )
            switch(c)
            {
                case 'h': // summon help
                    printf("%s",help);
                    return 0 ;
                break ;
                case 'c': // coin info
                    option_flags |= 0x80 ;
                    break ;
                case 'l': // number of lives
                    response = (*++argv)[0] - '0' ;
                    option_flags |= response & 3 ;
                    --argc ;
                    break ;
                case 'f': // alternative filename
                    strcpy ( filename , *++argv ) ;
                    break ;
                case 'b': // bonus life
                    option_flags |= ((*++argv)[0] - '0') << 2  ;
                    break ;
                default :
                    printf("no such flag dummy\n");
                    return 0 ;
            }

    FILE *program_fp = fopen (filename , "rb" ) ;

    if ( program_fp == NULL )
    {
        fprintf(stderr, "Error opening binary file for reading\n");
        return 1 ;
    }

    // read program into memory location 0x0000
    cpu_8080 space_cpu ;
    init ( program_fp , &space_cpu , option_flags ) ;
    init_audio () ; // initliase audio and load wav files

    // create windows and initialise graphics
    SDL_Window *window = SDL_CreateWindow ( *argv , SDL_WINDOWPOS_UNDEFINED , SDL_WINDOWPOS_UNDEFINED , W*3 , H*3 ,
        SDL_WINDOW_RESIZABLE ) ;
    SDL_Renderer *renderer = SDL_CreateRenderer ( window , -1 , 0 ) ;
    SDL_Texture *texture = SDL_CreateTexture ( renderer , SDL_PIXELFORMAT_ARGB8888 , SDL_TEXTUREACCESS_STREAMING , W , H  ) ;

    SDL_Event event ;
    bool halt = false , state ;

    for(; !halt ;)
    {
        run_cpu ( &space_cpu , CYCLE_PRE_FRAME/2 ) ;
        if ( space_cpu.interrupt_ff )
            generate_interrupt ( 0x08 , &space_cpu ) ;
        run_cpu ( &space_cpu , CYCLE_PRE_FRAME/2 ) ;
        if ( space_cpu.interrupt_ff )
            generate_interrupt ( 0x10 , &space_cpu ) ;

        while ( SDL_PollEvent ( &event ) )
            switch ( event.type )
            {
                case SDL_QUIT: // handles x button on the upper right of the window
                    halt = true ;
                    break ;

                case SDL_KEYDOWN:
                case SDL_KEYUP:
                        state = ( event.type == SDL_KEYDOWN ) ;
                        switch ( event.key.keysym.scancode )
                        {
                            case SDL_SCANCODE_C: // player 1 coin button is pressed
                                set_clear_bit ( space_cpu.port[1]->port_dir , 0 , !state )  ;
                                break ;
                            case SDL_SCANCODE_1: // player 1 start button
                                set_clear_bit ( space_cpu.port[1]->port_dir , 2 , state )  ;
                                break ;
                            case SDL_SCANCODE_2: // player 2 start button
                                set_clear_bit ( space_cpu.port[1]->port_dir , 1 , state )  ;
                                break ;
                            case SDL_SCANCODE_W: // player 1 shoot button
                                set_clear_bit ( space_cpu.port[1]->port_dir , 4 , state )  ;
                                break ;
                            case SDL_SCANCODE_E: // player 1 right button
                                set_clear_bit ( space_cpu.port[1]->port_dir , 6 , state )  ;
                                break ;
                            case SDL_SCANCODE_Q: // player 1 left button
                                set_clear_bit ( space_cpu.port[1]->port_dir , 5 , state )  ;
                                break ;
                            case SDL_SCANCODE_LEFT: // player 2 left button
                                set_clear_bit ( space_cpu.port[1]->port_dir + 1 , 5 , state )  ;
                                break ;
                            case SDL_SCANCODE_UP: // player 2 shoot button
                                set_clear_bit ( space_cpu.port[1]->port_dir + 1 , 4 , state )  ;
                                break ;
                            case SDL_SCANCODE_RIGHT: // player 2 right button
                                set_clear_bit ( space_cpu.port[1]->port_dir + 1 , 6 , state )  ;
                                break ;
                            case SDL_SCANCODE_ESCAPE:
                                halt = true ;
                                break ;

                        }
                break ;
                    }

        // render graphics
        extract_pixels ( pixels ) ;
        SDL_UpdateTexture( texture , NULL , pixels , W*4 ) ; // each pixel is 4 bytes
        SDL_RenderCopy ( renderer , texture , NULL , NULL ) ;
        SDL_RenderPresent ( renderer ) ;


        SDL_Delay ( 1000/60 ) ; // 60Hz refresh rate
    }

    free_audio () ;
    SDL_Quit() ;
    // free the ports that were created

    for ( int i = 1 ;  i <= 6 ; i++ )
        free(space_cpu.port[i]) ;

    return 0 ;
}

void init ( FILE *program , cpu_8080 *cpu , uint8_t flags )
{
    // load program into memory

    int c ;
    for ( int i = 0  ; (c = getc ( program )) != EOF ; i++ )
        mem[i] = c ;

    fclose ( program ) ;

    // initliase cpu flags and cpu->registers

    cpu -> pc = program_start ;
    cpu-> interrupt_ff = 0 ;

    // create and initialise the ports
    // we have ports on slots 1 2 3 4 5 6
    for ( int i = 1 ; i <= 6 ; i++ )
    {
        cpu->port[i] = ( port_8080 * ) malloc ( sizeof ( port_8080 ));
        cpu->port[i]->port_dir[0] = 0 ; // clear reads
    }

    cpu->port[1]->port_dir[0] |= 1 ; // diactivate coin
    cpu->port[2]->port_dir[0] |= flags ;
}

uint8_t port_read ( uint8_t port_number , cpu_8080 *cpu )
{
    switch ( port_number )
    {
        case 3:
            return ( port4_mem << (cpu->port[2]->port_dir[1]) ) >> 8  ;
            break ;

        default : return cpu->port[port_number]->port_dir[0] ;
    }
}
void port_write ( uint8_t value , uint8_t port_number , cpu_8080 *cpu )
{
    // in the case of the space invaders arcade system , some clever peripherals were added
    // so the IO devices must be emulated
    switch ( port_number )
    {
        case 2:
            cpu->port[2]->port_dir[1] = value & 0x07 ;
            break ;
        case 3:
        case 5:
            cpu->port[port_number]->port_dir[1] = value ;
            handle_sound ( value |= 0x80 * (( port_number == 3 ) ? 0 : 1) ); // using bit 7 as flag for port
            break ;
        case 4: // port connected to a shift register
            port4_mem = port4_mem >> 8 | (uint16_t)value << 8 ;
            break ;

        default : cpu->port[port_number]->port_dir[1] = value ;
    }
}

void run_cpu ( cpu_8080 *cpu, int cycle_count )
{
  int x = 0 ;
  while ( x < cycle_count )
  {
    //printf("port1 read: %02x || port2 read :%02x \n", cpu->port[1]->port_dir[0] , cpu->port[2]->port_dir[0]  );
    x += execute_opcode ( cpu ) ;
  }
}

void extract_pixels ( uint32_t *pixels  )
{
    int x = 0 ;
    for ( int j = 0 ; j < W ; j++ )
        for ( int i = H-1 ; i >= 0 ; i-- , x++ )
            pixels[i*W+j] = 0xFFFFFF * ( (mem[x/8 + VIDEO_MEM_START ] & (1 << x%8)  )) ;
}

void set_clear_bit ( uint8_t *number , int bit_number , bool state )
{
    if ( state )
        *number |= 1 << bit_number ;
    else
        *number &= ~( 1 << bit_number);
}

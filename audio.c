#include "invaders.h"

Mix_Chunk **buffer ;
int ufo_channel ;
uint8_t last_out_port3 = 0 , last_out_port5 = 0 ; // used to handle audio


void init_audio (  )
{
    if ( SDL_Init ( SDL_INIT_AUDIO ) != 0 )
    {
        fprintf(stderr, "%s\n", "error setting up audio");
        exit(1) ;
    }

    int id = Mix_OpenAudio (  MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT,
        MIX_DEFAULT_CHANNELS, 1024 );
    if ( id != 0 )
    {
        fprintf(stderr, "error opening audio device\n");
        exit(1) ;
    }

    buffer = ( Mix_Chunk ** ) malloc ( sizeof( Mix_Chunk * ) * 8 ) ;// 8 sound files

    buffer[0] = Mix_LoadWAV ( "sound/explosion.wav" ) ;
    buffer[1] = Mix_LoadWAV ( "sound/fastinvader1.wav" ) ;
    buffer[2] = Mix_LoadWAV ( "sound/fastinvader2.wav" ) ;
    buffer[3] = Mix_LoadWAV ( "sound/fastinvader3.wav" ) ;
    buffer[4] = Mix_LoadWAV ( "sound/fastinvader4.wav" ) ;
    buffer[5] = Mix_LoadWAV ( "sound/invaderkilled.wav" ) ;
    buffer[6] = Mix_LoadWAV ( "sound/shoot.wav" ) ;
    buffer[7] = Mix_LoadWAV ( "sound/ufo_highpitch.wav" ) ;
}

void free_audio ()
{
    for ( int i = 0 ; i < 8 ; i++ )
        Mix_FreeChunk ( buffer[i] ) ;
}

void handle_sound ( uint8_t data )
{ // play sound when bit changes form 0 to 1
    if ( !(data&0x80) ) // port 3
    {
        switch (data^last_out_port3)
        {
            case 1:
                if ( data & 1 ) // ufo sound that loops until 1 -> 0
                    ufo_channel = Mix_PlayChannel ( -1 , buffer[7] , -1 );
                else // this bit was 1 and now is 0
                    Mix_HaltChannel ( ufo_channel ) ;
                break ;
            case 2:
                if ( data & 2 ) // shot sound
                    Mix_PlayChannel ( -1 , buffer[6] , 0 ) ; // no loop
                break ;
            case 4:
                if ( data & 4 ) // flash ( player dies ) sound
                    Mix_PlayChannel ( -1 , buffer[0] , 0 ) ;
                break ;
            case 8:
                if ( data & 8 ) // invader dies sound
                    Mix_PlayChannel ( -1 , buffer[5] , 0 ) ;
                break ;

        }
        last_out_port3 = data ;
    }
    else // must be port 5 :)
    {
        switch (data^last_out_port5)
        {
            case 1:
                if ( data & 1 ) // fleet movement sound 1
                    Mix_PlayChannel ( -1 , buffer[1] , 0 );
                break ;
            case 2:
                if ( data & 2 ) // fleet movement sound 2
                    Mix_PlayChannel ( -1 , buffer[2] , 0 );
                break ;
            case 4:
                if ( data & 4 ) // fleet movement sound 3
                    Mix_PlayChannel ( -1 , buffer[3] , 0 );
                break ;
            case 8:
                if ( data & 8) // fleet movement 4
                    Mix_PlayChannel ( -1 , buffer[4] , 0 );
                break ;
            case 16: // ufo hit sound , sound file was not found
                break ;

        }

        last_out_port5 = data ;
    }



}

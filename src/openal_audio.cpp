#include "audio.hpp"

#include "logging.hpp"

#include <AL/al.h>
#include <AL/alc.h>

#include <stdint.h>

#define BUFFER_COUNT 2
#define BUFFER_SIZE  2048
#define SAMPLE_RATE  44100

struct {
    ALCdevice * device;
    ALCcontext * context;

    unsigned int source;
    unsigned int buffer_list[ BUFFER_COUNT ];

    int next_buffer;

} intern;

// int source_from_wav( res_t wav )
//{
//     unsigned int source;
//
//     alGenSources( 1, &source );
//     alSourcef( source, AL_PITCH, 1 );
//     alSourcef( source, AL_GAIN, 1 );
//     alSource3f( source, AL_POSITION, 0, 0, 0 );
//     alSource3f( source, AL_VELOCITY, 0, 0, 0 );
//     alSourcei( source, AL_LOOPING, AL_FALSE );
//
//     alSourcei( source, AL_BUFFER, load_wav( wav.data, wav.size ) );
//
//     return (int) source;
// }

int audio_init()
{
    intern.device = alcOpenDevice( nullptr );
    if ( !intern.device ) {
        ERROR_LOG( "failed to open audio device" );
        return 1;
    }

    intern.context = alcCreateContext( intern.device, nullptr );
    if ( !intern.context ) {
        ERROR_LOG( "failed to create audio context" );
        return 1;
    }

    ALCboolean is_current = alcMakeContextCurrent( intern.context );
    if ( is_current != ALC_TRUE ) {
        ERROR_LOG( "failed to make audio context current" );
        return 1;
    }

    INFO_LOG(
        "openal device: %s",
        alcGetString( intern.device, ALC_ALL_DEVICES_SPECIFIER )
    );
    INFO_LOG(
        "openal version: %s (%s) (%s)",
        alGetString( AL_VERSION ),
        alGetString( AL_VENDOR ),
        alGetString( AL_RENDERER )
    );

    float orientation[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };

    alListener3f( AL_POSITION, 0, 0, 1.0f );
    alListener3f( AL_VELOCITY, 0, 0, 0 );
    alListenerfv( AL_ORIENTATION, orientation );

    unsigned int source;

    alGenSources( 1, &source );
    alSourcef( source, AL_PITCH, 1 );
    alSourcef( source, AL_GAIN, 1 );
    alSource3f( source, AL_POSITION, 0, 0, 0 );
    alSource3f( source, AL_VELOCITY, 0, 0, 0 );
    alSourcei( source, AL_LOOPING, AL_FALSE );

    intern.source = source;

    alGenBuffers( 2, intern.buffer_list );

    return 0;
}

static void fill_buffer( int i )
{
    static float callback_buffer[ BUFFER_SIZE ];
    static uint8_t internal_buffer[ BUFFER_SIZE ];

    audio_callback( callback_buffer, BUFFER_SIZE, SAMPLE_RATE );

    for ( int i = 0; i < BUFFER_SIZE; i++ ) {
        internal_buffer[ i ] = (uint8_t) ( callback_buffer[ i ] * 255.0f );
    }

    alBufferData(
        intern.buffer_list[ i ],
        AL_FORMAT_MONO8,
        internal_buffer,
        BUFFER_SIZE,
        SAMPLE_RATE
    );
}

static void fill_and_queue_next_buffer()
{
    fill_buffer( intern.next_buffer );
    alSourceQueueBuffers(
        intern.source,
        1,
        intern.buffer_list + intern.next_buffer
    );

    intern.next_buffer++;
    intern.next_buffer %= BUFFER_COUNT;
}

void audio_tick()
{
    int source_state;
    alGetSourcei( intern.source, AL_SOURCE_STATE, &source_state );

    if ( source_state != AL_PLAYING ) {
        DEBUG_LOG( "source play" );
        fill_and_queue_next_buffer();
        fill_and_queue_next_buffer();
        alSourcePlay( intern.source );
    }

    int buffers_processed;
    alGetSourcei( intern.source, AL_BUFFERS_PROCESSED, &buffers_processed );

    if ( buffers_processed == 1 ) {
        unsigned int buffer;
        alSourceUnqueueBuffers( intern.source, 1, &buffer );
        fill_and_queue_next_buffer();
    } else if ( buffers_processed > 1 ) {
        ERROR_LOG( "all buffers processed (too fast)" );
        static unsigned int buffers[ BUFFER_COUNT ];
        alSourceUnqueueBuffers( intern.source, buffers_processed, buffers );
    }
}

void audio_destroy()
{
    alcMakeContextCurrent( nullptr );
    alcDestroyContext( intern.context );
    alcCloseDevice( intern.device );
}

#include "audio.hpp"

#include <pa_ringbuffer.h>
#include <portaudio.h>

#include <math.h>
#include <stdio.h>

#define NUM_SECONDS       ( 5 )
#define SAMPLE_RATE       ( 44100 )
#define FRAMES_PER_BUFFER ( 64 )

#define OSC_TABLE_SIZE 4096

#ifndef M_PI
#define M_PI ( 3.14159265 )
#endif

struct synth_command_t {
    enum {
        MIDI_START,
        MIDI_STOP,
        MIDI_CONTROL,
    } type;

    int value;
};

struct synth_t {
    PaUtilRingBuffer command_queue;

    int midi_enable;
    int midi_no;
    int phase;

    /// voltage controlled oscillator
    struct vco_t {
        float pitch;
        float vco_wave;
        float pulse_width;
    } vco;

    /// voltage controlled filter
    struct vcf_t {
        float cutoff;
        float resonance;

        float out;

        float process( float in );
    } vcf;

    /// voltage controlled amplifier
    struct vca_t {
        float volume;
        int vca_mode; // 0 - ON   1 - EG
    } vca;

    /// low frequency oscillator
    struct lfo_t {
        float lfo_rate;
        float lfo_wave;
    } lfo;

    /// envelope generator
    struct eg_t {
        float attack;
        float decay;
        float sustain;
        float release;

        float pressed();
        float released();

        float out;
        float t;
    } eg;

    float sine[ OSC_TABLE_SIZE ];
    float sawtooth[ OSC_TABLE_SIZE ];
    float triangle[ OSC_TABLE_SIZE ];

    float envelope_factor();
};

float synth_t::eg_t::pressed()
{
    if ( t < attack ) {
        out += ( 1.0f / attack ) * ( 1.0f / SAMPLE_RATE );
    } else if ( t < attack + decay ) {
        out -= ( ( 1.0f - sustain ) / decay ) * ( 1.0f / SAMPLE_RATE );
    } else {
        out = sustain;
    }

    t += ( 1.0f / SAMPLE_RATE );

    if ( out > 1.0f ) out = 1.0f;
    if ( out < 0.0f ) out = 0.0f;

    return out;
}

float synth_t::eg_t::released()
{
    if ( t < release ) {
        out -= ( sustain / release ) * ( 1.0f / SAMPLE_RATE );
    } else {
        out = 0.0f;
    }

    t += ( 1.0f / SAMPLE_RATE );

    if ( out > 1.0f ) out = 1.0f;
    if ( out < 0.0f ) out = 0.0f;

    return out;
}

float synth_t::envelope_factor()
{
    if ( midi_enable ) {
        return eg.pressed();
    } else {
        return eg.released();
    }
}

float synth_t::vcf_t::process( float in )
{
    float rc = 1.0f / ( 2 * M_PI * cutoff );
    float dt = 1.0f / SAMPLE_RATE;
    float a = dt / ( rc + dt );

    out = a * in + ( 1 - a ) * out;

    return out;
}

struct {
    PaStream * stream;
    synth_t synth;

} intern;

static float midi_to_freq( int midi_no )
{
    return 440.00 * pow( 2.0, ( midi_no - 69.00 ) / 12.00 );
}

static int pa_callback(
    const void * input_buffer,
    void * output_buffer,
    unsigned long frames_per_buffer,
    const PaStreamCallbackTimeInfo * time_info,
    PaStreamCallbackFlags status_flags,
    void * user_data
)
{
    synth_t * s = (synth_t *) user_data;
    float * out = (float *) output_buffer;

    synth_command_t command;
    if ( PaUtil_ReadRingBuffer( &s->command_queue, &command, 1 ) ) {
        if ( command.type == synth_command_t::MIDI_START ) {
            if ( !s->midi_enable ) s->eg.t = 0.0f;
            s->midi_enable = 1;
            s->midi_no = command.value;
        }
        if ( command.type == synth_command_t::MIDI_STOP ) {
            s->midi_enable = 0;
            s->eg.t = 0.0f;
        }
        if ( command.type == synth_command_t::MIDI_CONTROL ) {
            s->vcf.cutoff =
                100.0f + ( (float) command.value / 0x7f ) * 5000.0f;
        }
    }

    for ( unsigned long i = 0; i < frames_per_buffer; i++ ) {
        float freq = midi_to_freq( s->midi_no );
        //*out++ = s->sine[ s->phase ];
        //*out++ = s->sine[ s->phase ];
        float x = s->triangle[ s->phase ] * s->envelope_factor();
        x = s->vcf.process( x );
        *out++ = x;
        *out++ = x;

        s->phase += (int) ( freq * OSC_TABLE_SIZE / SAMPLE_RATE );
        s->phase %= OSC_TABLE_SIZE;
    }

    return paContinue;
}

static void setup_osc_tables()
{
    for ( int i = 0; i < OSC_TABLE_SIZE; i++ ) {
        intern.synth.sine[ i ] =
            (float) sin( ( (double) i / (double) OSC_TABLE_SIZE ) * M_PI * 2. );
    }

    for ( int i = 0; i < OSC_TABLE_SIZE; i++ ) {
        intern.synth.sawtooth[ i ] =
            1.0f - 2.0f * ( i / (float) OSC_TABLE_SIZE );
    }

    for ( int i = 0; i < OSC_TABLE_SIZE; i++ ) {
        intern.synth.triangle[ i ] =
            -fabs( -1.0 + 2.0f * i / (float) OSC_TABLE_SIZE );
    }
}

void audio_send_control( int value )
{
    synth_command_t cmd;
    cmd.type = synth_command_t::MIDI_CONTROL;
    cmd.value = value;
    PaUtil_WriteRingBuffer( &intern.synth.command_queue, &cmd, 1 );
}

void audio_start_midi( int midi_no )
{
    synth_command_t cmd;
    cmd.type = synth_command_t::MIDI_START;
    cmd.value = midi_no;
    PaUtil_WriteRingBuffer( &intern.synth.command_queue, &cmd, 1 );
}

void audio_stop_midi()
{
    synth_command_t cmd;
    cmd.type = synth_command_t::MIDI_STOP;
    PaUtil_WriteRingBuffer( &intern.synth.command_queue, &cmd, 1 );
}

int audio_init()
{
    PaStreamParameters outputParameters;
    PaError err;
    int i;

    intern.synth.midi_no = 69;
    intern.synth.midi_enable = 1;

    intern.synth.eg.attack = 0.1f;
    intern.synth.eg.decay = 0.0f;
    intern.synth.eg.sustain = 1.0f;
    intern.synth.eg.release = 0.1f;

    intern.synth.vcf.cutoff = 500;

    static void * command_buffer = new synth_command_t[ 1024 ];
    PaUtil_InitializeRingBuffer(
        &intern.synth.command_queue,
        sizeof( synth_command_t ),
        1024,
        command_buffer
    );

    setup_osc_tables();

    printf(
        "PortAudio Test: output sine wave. SR = %d, BufSize = %d\n",
        SAMPLE_RATE,
        FRAMES_PER_BUFFER
    );

    err = Pa_Initialize();

    outputParameters.device =
        Pa_GetDefaultOutputDevice(); /* default output device */

    if ( outputParameters.device == paNoDevice ) {
        fprintf( stderr, "Error: No default output device.\n" );
    }

    outputParameters.channelCount = 2; /* stereo output */
    outputParameters.sampleFormat =
        paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency =
        Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
        &intern.stream,
        NULL, /* no input */
        &outputParameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff, /* we won't output out of range samples so don't bother
                      clipping them */
        pa_callback,
        &intern.synth
    );

    err = Pa_StartStream( intern.stream );

    return 0;
}

void audio_tick()
{
}

void audio_destroy()
{
    Pa_StopStream( intern.stream );

    Pa_CloseStream( intern.stream );

    Pa_Terminate();
    printf( "Test finished.\n" );
}

float audio_visual_1()
{
    // TODO: not exactly threadsafe
    return intern.synth.eg.out;
}
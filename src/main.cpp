#include "audio.hpp"
#include "hardware.hpp"
#include "logging.hpp"
#include "render.hpp"
#include "state.hpp"

#include <math.h>

#include <portmidi.h>
#include <porttime.h>

#define MIDI_CODE_MASK 0xf0
#define MIDI_CHN_MASK  0x0f

static struct {
    PmStream * midi_in;
} intern;

static void init_midi()
{
    int last_input;
    for ( int i = 0; i < Pm_CountDevices(); i++ ) {
        const PmDeviceInfo * info = Pm_GetDeviceInfo( i );
        if ( info->input ) {
            last_input = i;
            INFO_LOG( "%d: %s, %s\n", i, info->interf, info->name );
        }
    }

    Pt_Start( 1, 0, 0 );
    PmError err = Pm_OpenInput(
        &intern.midi_in,
        last_input,
        nullptr,
        512,
        nullptr,
        nullptr
    );

    if ( err ) {
        ERROR_LOG( "%s", Pm_GetErrorText( err ) );
    }

    // Pm_SetFilter( intern.midi_in, PM_FILT_NOTE );
    PmEvent event;
    while ( Pm_Poll( intern.midi_in ) ) {
        Pm_Read( intern.midi_in, &event, 1 );
    }
}

static void destroy_midi()
{
    Pm_Close( intern.midi_in );
}

static void loop()
{
    static int last_midi = 69;

    int midi = hardware_number() + 69;

    if ( midi != last_midi ) {
        if ( midi == 69 ) {
            audio_stop_midi();
        } else {
            audio_start_midi( midi );
        }

        last_midi = midi;
    }

    static int pressed_note_count = 0;

    PmEvent event;
    if ( Pm_Read( intern.midi_in, &event, 1 ) ) {
        int cmd = Pm_MessageStatus( event.message ) & MIDI_CODE_MASK;
        int chan = Pm_MessageStatus( event.message ) & MIDI_CHN_MASK;
        int data1 = Pm_MessageData1( event.message );
        int data2 = Pm_MessageData2( event.message );
        INFO_LOG( "[chan %d : %2x] : %2x %2x", chan, cmd, data1, data2 );

        if ( cmd == 0x90 ) {
            pressed_note_count++;
            audio_start_midi( data1 );
        }
        if ( cmd == 0x80 ) {
            pressed_note_count--;
            if (pressed_note_count == 0) audio_stop_midi();
        }
    }

    audio_tick();
    render();
}

#if defined( _WIN32 ) and RELEASE
int WinMain()
#else
int main()
#endif
{
    state.freq = 440.0f;
    INFO_LOG( "meow" );

    init_midi();

    hardware_init();

    audio_init();

    audio_stop_midi();

    hardware_set_loop( loop );

    audio_destroy();

    hardware_destroy();

    destroy_midi();

    return 0;
}

#pragma once

int audio_init();

void audio_tick();

void audio_destroy();

void audio_start_midi( int midi_no );

void audio_stop_midi();

float audio_visual_1();
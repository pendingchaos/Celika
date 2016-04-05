#pragma once
#ifndef AUDIO_H
#define AUDIO_H

#include <stddef.h>

typedef struct sound_t sound_t;

void audio_init(size_t freq, size_t sample_buf_size);
void audio_deinit();
sound_t* audio_create_sound(const char* filename);
void audio_del_sound(sound_t* sound);
void audio_play_sound(sound_t* sound, float volume);

#endif

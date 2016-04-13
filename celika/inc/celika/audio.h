#pragma once
#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <stddef.h>

typedef struct sound_t sound_t;
typedef struct audio_src_t audio_src_t;
typedef struct music_t music_t;

void audio_init(size_t freq, size_t sample_buf_size);
void audio_deinit();
sound_t* audio_create_sound(const char* filename);
void audio_del_sound(sound_t* sound);
audio_src_t* audio_play_sound(sound_t* sound, float volume, float offset, bool gc);
float audio_get_src_offset(audio_src_t* src);
void audio_set_src_offset(audio_src_t* src, float offset);
float audio_get_src_volume(audio_src_t* src);
void audio_set_src_volume(audio_src_t* src, float volume);
bool audio_get_src_done(audio_src_t* src);
void audio_del_src(audio_src_t* src);
music_t* audio_create_music(size_t track_count, const char** filenames);
void audio_del_music(music_t* music);
void audio_update_music(music_t* music);
#endif

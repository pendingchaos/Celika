#include "audio.h"
#include "list.h"

#include <vorbis/vorbisfile.h>
#include <SDL2/SDL_audio.h>
#include <stdbool.h>
#include <endian.h>
#include <stdlib.h>
#include <stdio.h>

struct sound_t {
    size_t ref_count;
    size_t sample_rate;
    size_t sample_count;
    short* samples; //interleaved stereo samples
};

typedef struct audio_src_t {
    sound_t* sound;
    size_t offset; //in samples with a frequency of spec.freq
    float volume;
    bool gc;
    bool done;
} audio_src_t;

typedef struct music_t {
    size_t cur_track;
    size_t track_count;
    sound_t** tracks;
    audio_src_t* source;
} music_t;

static SDL_AudioDeviceID dev;
static SDL_AudioSpec spec;
static list_t* sources; //list_t of audio_src_t

static void callback(void* userdata, Uint8* stream, int len) {
    bool clipping = false;
    
    memset(stream, 0, len);
    
    int16_t* data = (int16_t*)stream;
    size_t sample_count = len / 4;
    
    for (ptrdiff_t i = 0; i < list_len(sources); i++) {
        audio_src_t* source = list_nth(sources, i);
        if (source->done) continue;
        sound_t* sound = source->sound;
        
        bool end = false;
        for (size_t j = 0; j < sample_count; j++) {
            size_t index = (source->offset+j)/(double)spec.freq * source->sound->sample_rate;
            if (index >= sound->sample_count) {
                end = true;
                break;
            }
            int16_t l = sound->samples[index*2] * source->volume;
            int16_t r = sound->samples[index*2+1] * source->volume;
            
            data[j*2] += l;
            data[j*2+1] += r;
            
            clipping = clipping || data[j*2] == 32767;
            clipping = clipping || data[j*2] == -32768;
            clipping = clipping || data[j*2+1] == 32767;
            clipping = clipping || data[j*2+1] == -32768;
        }
        
        source->offset += sample_count;
        
        if (end && source->gc) {
            audio_del_sound(source->sound);
            list_remove(source);
            i--;
        } else if (end) {
            source->done = true;
        }
    }
    
    if (clipping) printf("Warning: Audio clipping\n");
}

void audio_init(size_t freq, size_t sample_buf_size) {
    sources = list_create(sizeof(audio_src_t));
    
    SDL_AudioSpec desired;
    desired.freq = freq;
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = sample_buf_size;
    desired.callback = callback;
    desired.userdata = NULL;
    
    dev = SDL_OpenAudioDevice(NULL, SDL_FALSE, &desired, &spec,
                              SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (!dev) {
        fprintf(stderr, "Failed to open audio device: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }
}

void audio_deinit() {
    SDL_CloseAudioDevice(dev);
    list_del(sources);
}

sound_t* audio_create_sound(const char* filename) {
    SDL_PauseAudioDevice(dev, 0);
    
    sound_t* sound = malloc(sizeof(sound_t));
    sound->ref_count = 1;
    
    OggVorbis_File vf;
    if (ov_fopen(filename, &vf) < 0) {
        fprintf(stderr, "Failed to open %s\n", filename);
        exit(EXIT_FAILURE);
    }
    
    sound->sample_count = ov_pcm_total(&vf, -1);
    sound->sample_rate = ov_info(&vf, -1)->rate;
    
    int channels = ov_info(&vf, -1)->channels;
    if (channels > 2) {
        fprintf(stderr, "Unsupported number of channels in %s\n", filename);
        exit(EXIT_FAILURE);
    }
    
    sound->samples = malloc(sound->sample_count*4);
    
    int stream;
    size_t offset = 0;
    while (true) {
        uint8_t buf[4096];
        #if __BYTE_ORDER == __BIG_ENDIAN
        int big_endian = 1;
        #else
        int big_endian = 0;
        #endif
        long count = ov_read(&vf, (char*)buf, sizeof(buf), big_endian, 2, 1, &stream);
        if (count < 0) {
            fprintf(stderr, "Failed to read from %s\n", filename);
            exit(EXIT_FAILURE);
        } else if (!count) break;
        
        int16_t* dest_samples = sound->samples + offset*2;
        
        if (channels == 2) {
            memcpy(dest_samples, buf, count);
            offset += count / 4;
        } else {
            for (size_t i = 0; i < count/2; i++) {
                int16_t v = ((int16_t*)buf)[i];
                dest_samples[i*2] = v;
                dest_samples[i*2+1] = v;
            }
            offset += count / 2;
        }
    }
    
    ov_clear(&vf);
    
    return sound;
}

void audio_del_sound(sound_t* sound) {
    sound->ref_count--;
    
    if (!sound->ref_count) {
        free(sound->samples);
        free(sound);
    }
}

audio_src_t* audio_play_sound(sound_t* sound, float volume, float offset, bool gc) {
    SDL_LockAudioDevice(dev);
    
    audio_src_t source;
    source.sound = sound;
    sound->ref_count++;
    source.offset = offset * spec.freq;
    source.volume = volume;
    source.gc = gc;
    source.done = false;
    list_append(sources, &source);
    
    audio_src_t* res = list_nth(sources, list_len(sources)-1);
    
    SDL_UnlockAudioDevice(dev);
    
    return res;
}

float audio_get_src_offset(audio_src_t* src) {
    SDL_LockAudioDevice(dev);
    float offset = src->offset / (double)src->sound->sample_rate;
    SDL_UnlockAudioDevice(dev);
    return offset;
}

void audio_set_src_offset(audio_src_t* src, float offset) {
    SDL_LockAudioDevice(dev);
    src->offset = offset * src->sound->sample_rate;
    SDL_UnlockAudioDevice(dev);
}

float audio_get_src_volume(audio_src_t* src) {
    SDL_LockAudioDevice(dev);
    float volume = src->volume;
    SDL_UnlockAudioDevice(dev);
    return volume;
}

void audio_set_src_volume(audio_src_t* src, float volume) {
    SDL_LockAudioDevice(dev);
    src->volume = volume;
    SDL_UnlockAudioDevice(dev);
}

bool audio_get_src_done(audio_src_t* src) {
    SDL_LockAudioDevice(dev);
    bool done = src->done;
    SDL_UnlockAudioDevice(dev);
    return done;
}

void audio_del_src(audio_src_t* src) {
    SDL_LockAudioDevice(dev);
    audio_del_sound(src->sound);
    list_remove(src);
    SDL_UnlockAudioDevice(dev);
}

music_t* audio_create_music(size_t track_count, const char** filenames) {
    music_t* music = malloc(sizeof(music_t));
    music->track_count = track_count;
    music->tracks = malloc(sizeof(sound_t*)*track_count);
    for (size_t i = 0; i < track_count; i++)
        music->tracks[i] = audio_create_sound(filenames[i]);
    music->cur_track = 0;
    music->source = audio_play_sound(music->tracks[0], 1, 0, false);
    return music;
}

void audio_del_music(music_t* music) {
    audio_del_src(music->source);
    for (size_t i = 0; i < music->track_count; i++)
        audio_del_sound(music->tracks[i]);
    free(music->tracks);
    free(music);
}

void audio_update_music(music_t* music) {
    if (audio_get_src_done(music->source)) {
        music->cur_track = (music->cur_track+1) % music->track_count;
        audio_del_src(music->source);
        music->source = audio_play_sound(music->tracks[music->cur_track], 1, 0, false);
    }
}

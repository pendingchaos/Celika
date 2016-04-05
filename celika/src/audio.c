#include "audio.h"
#include "list.h"

#include <vorbis/vorbisfile.h>
#include <SDL2/SDL_audio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

struct sound_t {
    size_t sample_rate;
    size_t sample_count;
    short* samples; //interleaved stereo samples
};

typedef struct source_t {
    sound_t* sound;
    double offset; //in samples with a frequency of spec.freq
    float volume;
} source_t;

static SDL_AudioDeviceID dev;
static SDL_AudioSpec spec;
static list_t* sources; //list_t of source_t

static void callback(void* userdata, Uint8* stream, int len) {
    memset(stream, 0, len);
    
    int16_t* data = (int16_t*)stream;
    size_t sample_count = len / 4;
    
    for (size_t i = 0; i < list_len(sources); i++) {
        source_t* source = list_nth(sources, i);
        sound_t* sound = source->sound;
        
        for (size_t j = 0; j < sample_count; j++) {
            size_t index = source->offset + j;
            if (index > sound->sample_count) break;
            int32_t l = sound->samples[index*2] / 32767.0 * source->volume * 32767;
            int32_t r = sound->samples[index*2+1] / 32767.0 * source->volume * 32767;
            if (l > 32768) l = 32768;
            if (l < -32767) l = -32767;
            if (r > 32768) r = 32768;
            if (r < -32767) r = -32767;
            data[j*2] = l;
            data[j*2+1] = r;
        }
        
        source->offset += sample_count;
    }
}

void audio_init(size_t freq, size_t sample_buf_size) {
    sources = list_new(sizeof(source_t), NULL);
    
    spec.freq = freq;
    spec.format = AUDIO_S16;
    spec.channels = 2;
    spec.samples = sample_buf_size;
    spec.callback = callback;
    spec.userdata = NULL;
    
    dev = SDL_OpenAudioDevice(NULL, SDL_FALSE, &spec, &spec,
                              SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (!dev) {
        fprintf(stderr, "Failed to open audio device: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }
}

void audio_deinit() {
    SDL_CloseAudioDevice(dev);
    list_free(sources);
}

sound_t* audio_create_sound(const char* filename) {
    sound_t* sound = malloc(sizeof(sound_t));
    
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
        long count = ov_read(&vf, (char*)buf, sizeof(buf), 0, 2, 1, &stream);
        if (count < 0) {
            fprintf(stderr, "Failed to read from %s\n", filename);
            exit(EXIT_FAILURE);
        } else if (!count) break;
        memcpy((uint8_t*)sound->samples+offset, buf, count);
        offset += count;
    }
    
    ov_clear(&vf);
    
    return sound;
}

void audio_del_sound(sound_t* sound) {
    free(sound->samples);
    free(sound);
}

void audio_play_sound(sound_t* sound, float volume) {
    SDL_LockAudioDevice(dev);
    
    source_t source;
    source.sound = sound;
    source.offset = 0;
    source.volume = volume;
    list_append(sources, &source);
    
    SDL_UnlockAudioDevice(dev);
}

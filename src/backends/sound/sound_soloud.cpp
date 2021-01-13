#include "sound_base.h"

#include <cstring>
#include <string>
#include <map>
#include <vorbis/vorbisfile.h>
#include "soloud.h"
#include "soloud_wav.h"
#include "soloud_wavstream.h"
#include "soloud_lofifilter.h"
#include "soloud_biquadresonantfilter.h"

using namespace SoLoud;

#define CHUNK_BUFFER_SIZE 8

Soloud soloud;

WavStream music;
int music_handle;
int chunkqueue_handle;
Bus filter_bus;
float music_speed = 1.0;

int whichchunk = 0;
Wav *chunks[CHUNK_BUFFER_SIZE];
Queue chunkqueue;

BiquadResonantFilter underwaterFilter;

std::map<std::string,Wav*> sfx_cache;


int Backend_Sound_Close() {
    soloud.deinit();
    return 1;
}

int Backend_Sound_Init() {
    soloud.init(
        Soloud::CLIP_ROUNDOFF,
        Soloud::AUTO,
        SOUND_FREQUENCY,
        SOUND_SAMPLES_SIZE,
        2
    );

    chunkqueue.setParams(SOUND_FREQUENCY, 2);
    chunkqueue.setInaudibleBehavior(true, false);
    soloud.play(filter_bus);
    underwaterFilter.setParams(SoLoud::BiquadResonantFilter::LOWPASS, 1000, 0);

    return 1;
}

bool soundrecover_triggered = false;

int Backend_Sound_Update(int size) {
    if (chunks[whichchunk] == NULL)
        chunks[whichchunk] = new Wav();

    Wav *chunk = chunks[whichchunk];

    if (chunkqueue.getQueueCount() >= CHUNK_BUFFER_SIZE - 1)
        return 1;

    if (chunk->mData == NULL) {
        chunk->mChannels = 2;
        chunk->mBaseSamplerate = SOUND_FREQUENCY;
        chunk->mData = new float[SOUND_SAMPLES_SIZE];
        chunk->setInaudibleBehavior(true, false);
    } else {
        chunks[whichchunk]->stop();
    }

    // Doing this manually because it's broken in soloud's loadRawWave16
    // (Also doing this directly is more efficient)
    soloud.lockAudioMutex_internal();
    chunk->mSampleCount = size / chunk->mChannels;
    for (int i = 0; i < size / 2; i++) {
        chunk->mData[i] = (
            ((signed short)soundframe[i*2]) /
            (float)0x8000
        );

        chunk->mData[i + (size / 2)] = (
            ((signed short)soundframe[(i*2)+1]) /
            (float)0x8000
        );
    }
    soloud.unlockAudioMutex_internal();

    chunkqueue.play(*chunk);
    whichchunk++;
    whichchunk %= CHUNK_BUFFER_SIZE;

    if (!soloud.isValidVoiceHandle(chunkqueue_handle))
        chunkqueue_handle = soloud.play(chunkqueue);
    
    return 1;
}

int Backend_Sound_IsPlayingMusic() {
    return soloud.isValidVoiceHandle(music_handle);
}

int Backend_Sound_PlayMusic(char *path) {
    if (Backend_Sound_IsPlayingMusic()) music.stop();

    // I tried reusing OggVorbis_File's fp before but I think DiskFile in SoLoud is fucked up
    // Oh well!
    if (music.load(path) != 0) {
        printf("Unable to load Ogg file: %s\n", path);
        return 0;
    }

    music.setLooping(1);

    OggVorbis_File music_ogg;
    ov_fopen(path, &music_ogg);
    vorbis_comment *music_comments = ov_comment(&music_ogg, -1);

    for (int i = 0; i < music_comments->comments; i++) {
        int comment_length = music_comments->comment_lengths[i];
        char *comment_line = music_comments->user_comments[i];
        char *comment_key = strtok(comment_line, "=");
        char *comment_value = strtok(NULL, "=");

        if (!strcmp(comment_key, "LOOPSTART")) {
            double loopstart = strtod(comment_value, NULL);
            if (loopstart < 0) {
                music.setLooping(0);
            } else {
                music.setLoopPoint(loopstart);
            }
        }
    }

    ov_clear(&music_ogg);

    music_handle = filter_bus.play(music, 1);
    soloud.setRelativePlaySpeed(music_handle, music_speed);
    return 1;
}

int Backend_Sound_PlaySFX(char *path) {
    std::string pathstr = path;
    Wav *sfx;
    std::map<std::string,Wav*>::iterator it = sfx_cache.find(pathstr);
    if(it != sfx_cache.end()) {
        sfx = it->second;
    } else {
        sfx = new Wav();
        if (sfx->load(path) != 0) {
            printf("Unable to load Wav file: %s\n", path);
            return 0;
        }
        sfx_cache.insert(it,std::pair<std::string,Wav*>(pathstr,sfx));
    }
    soloud.stopAudioSource(*sfx);
    filter_bus.play(*sfx);
    return 1;
}

int Backend_Sound_StopMusic() {
    music.stop();
    return 1;
}

int Backend_Sound_FadeOutMusic(int fadeTime) {
    soloud.fadeVolume(music_handle, 0, fadeTime / 1000.0);
    soloud.scheduleStop(music_handle, fadeTime / 1000.0);
    return 1;
}

int Backend_Sound_SetPause(int paused) {
    soloud.setPauseAll(paused);
    soloud.setPause(chunkqueue_handle, false);
    return 1;
}

int Backend_Sound_MusicSpeed(float speed) {
    if (music_speed == speed) return 1;
    music_speed = speed;
    soloud.setRelativePlaySpeed(music_handle, music_speed);
    return 1;
}

int Backend_Sound_MusicSetUnderwater(int isUnderwater) {
    if (isUnderwater) filter_bus.setFilter(0, &underwaterFilter);
    else filter_bus.setFilter(0, NULL);
    return 1;
}
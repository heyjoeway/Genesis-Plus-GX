#include "sound_base.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

Mix_Music *music;

struct {
  char* current_pos;
  char* buffer;
  int current_emulated_samples;
  Mix_Chunk chunk;
  int current_chunk;
} sdl_sound;

static void SDLMixer_callback(int channel)
{
  if (channel != 1) return;

  Mix_Chunk *chunk;
  chunk = &sdl_sound.chunk;

  if(sdl_sound.current_emulated_samples < chunk->alen) {
    memset(chunk->abuf, 0, chunk->alen);
  }
  else {
    memcpy(chunk->abuf, sdl_sound.buffer, chunk->alen);
    /* loop to compensate desync */
    do {
      sdl_sound.current_emulated_samples -= chunk->alen;
    } while(sdl_sound.current_emulated_samples > 2 * chunk->alen);
    memcpy(sdl_sound.buffer,
           sdl_sound.current_pos - sdl_sound.current_emulated_samples,
           sdl_sound.current_emulated_samples);
    sdl_sound.current_pos = sdl_sound.buffer + sdl_sound.current_emulated_samples;
  }

  Mix_PlayChannel(1, chunk, 0);
}

int Backend_Sound_PlayMusic(char *path) {
  music = Mix_LoadMUS(path);

  if (music == NULL) {
      printf("Unable to load Ogg file: %s\n", Mix_GetError());
      return 0;
  }

  if (Mix_PlayMusic(music, 0) == -1) {
      printf("Unable to play Ogg file: %s\n", Mix_GetError());
      return 0;
  }
  
  return 1;
}
int Backend_Sound_StopMusic() {
  Mix_HaltMusic();
  Mix_FreeMusic(music);
  
  return 1;
}
int Backend_Sound_FadeOutMusic(int fadeTime) {
  Mix_FadeOutMusic(fadeTime);
  return 1;
}
int Backend_Sound_PauseMusic() {
  Mix_PauseMusic(); 
  return 1;
}
int Backend_Sound_ResumeMusic() {
  Mix_ResumeMusic();
  return 1;
}

int Backend_Sound_Pause() {
  SDL_PauseAudio(0);
  return 1;
}

int Backend_Sound_Close() {
  SDL_PauseAudio(1);
  Mix_CloseAudio();
  if (sdl_sound.buffer)
    free(sdl_sound.buffer);
  return 1;
}

int Backend_Sound_Init() {
  int n;
  SDL_AudioSpec as_desired;

  if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
    printf("SDL Audio initialization failed\n");
    return 0;
  }

  if(Mix_OpenAudio(SOUND_FREQUENCY, AUDIO_S16LSB, 2, SOUND_SAMPLES_SIZE) < 0) {
    printf("SDL Audio open failed\n");
    return 0;
  }

  Mix_Init(MIX_INIT_OGG);

  sdl_sound.current_emulated_samples = 0;
  n = SOUND_SAMPLES_SIZE * 2 * sizeof(short) * 20;

  sdl_sound.chunk.allocated = 0;
  sdl_sound.chunk.abuf = (Uint8*)malloc(n / 20);
  sdl_sound.chunk.alen = n / 20;
  sdl_sound.chunk.volume = 128;

  sdl_sound.buffer = (char*)malloc(n);
  if(!sdl_sound.buffer) {
    printf("Can't allocate audio buffer\n");
    return 0;
  }
  memset(sdl_sound.buffer, 0, n);
  sdl_sound.current_pos = sdl_sound.buffer;

  Mix_ChannelFinished(SDLMixer_callback);
  Mix_PlayChannel(1, &sdl_sound.chunk, 0);

  return 1;
}

int Backend_Sound_Update(int size) {
  int i;
  short *out;

  SDL_LockAudio();
  out = (short*)sdl_sound.current_pos;
  for(i = 0; i < size; i++)
  {
    *out++ = soundframe[i];
  }
  sdl_sound.current_pos = (char*)out;
  sdl_sound.current_emulated_samples += size * sizeof(short);
  SDL_UnlockAudio();
  return 1;
}

int Backend_Sound_MusicSpeed(float speed) {
  return 1;
}

int Backend_Sound_MusicSetUnderwater(int isUnderwater) {
  return 1;
}

int Backend_Sound_PlaySFX(char *path) {
  return 1;
}

int Backend_Sound_SetPause(int paused) {
  return 1;
}

int Backend_Sound_IsPlayingMusic() {
  return 0;
}
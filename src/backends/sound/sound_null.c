#include "sound_base.h"

int Backend_Sound_Init() { return 1; }
int Backend_Sound_Update(int size) { return 1; }
int Backend_Sound_Close() { return 1; }
int Backend_Sound_PlayMusic(char *path) { return 1; }
int Backend_Sound_StopMusic() { return 1; }
int Backend_Sound_FadeOutMusic(int fadeTime) { return 1; }
int Backend_Sound_MusicSpeed(float speed) { return 1; }
int Backend_Sound_MusicSetUnderwater(int isUnderwater) { return 1; }
int Backend_Sound_PlaySFX(char *path) { return 1; }
int Backend_Sound_SetPause(int paused) { return 1; }
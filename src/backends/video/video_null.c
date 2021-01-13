#include "video_base.h"

#include "shared.h"

int Backend_Video_Close() { return 1; }
int Backend_Video_Init() {
  bitmap.data = (unsigned char *)malloc(bitmap.pitch * bitmap.height);
  return 1;
}
int Backend_Video_SetFullscreen(int arg_fullscreen) { return 1; }
int Backend_Video_ToggleFullscreen() { return 1; }
int Backend_Video_Update() { return 1; }
int Backend_Video_SetWindowTitle(char *caption) { return 1; }
void *Backend_Video_LoadImage(char *path) { }
void *Backend_Video_GetRenderer() { }
int Backend_Video_Clear() { return 1; }
int Backend_Video_Present() { return 1; }
#ifndef __BACKEND_VIDEO_BASE___
#define __BACKEND_VIDEO_BASE___

#define VIDEO_WIDTH  400
#define VIDEO_HEIGHT 224

#define WINDOW_SCALE 3

extern int option_mirrormode;
extern int option_scaling;

#ifdef __cplusplus
extern "C" {
#endif

int Backend_Video_Close();
int Backend_Video_Init();
int Backend_Video_SetFullscreen(int arg_fullscreen);
int Backend_Video_ToggleFullscreen();
int Backend_Video_Update();
int Backend_Video_Clear();
int Backend_Video_Present();
int Backend_Video_SetWindowTitle(char *caption);
void *Backend_Video_LoadImage(char *path);
void *Backend_Video_GetRenderer();
int Backend_Video_SetVsync(int vsync);
int Backend_Video_GetRefreshRate();

#ifdef __cplusplus
}
#endif

#endif
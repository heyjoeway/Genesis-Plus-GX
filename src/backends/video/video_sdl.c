#include "video_base.h"

#include <SDL/SDL.h>
#include "shared.h"

// #if defined(USE_8BPP_RENDERING)
//   #define SURFACE_FORMAT SDL_PIXELFORMAT_RGB332
// #elif defined(USE_15BPP_RENDERING)
//   #define SURFACE_FORMAT SDL_PIXELFORMAT_RGB555
// #elif defined(USE_16BPP_RENDERING)
//   #define SURFACE_FORMAT SDL_PIXELFORMAT_RGB565
// #elif defined(USE_32BPP_RENDERING)
//   #define SURFACE_FORMAT SDL_PIXELFORMAT_RGB888
// #endif

struct {
  SDL_Surface* surf_screen;
  SDL_Surface* surf_bitmap;
  SDL_Rect srect;
  SDL_Rect drect;
  Uint32 frames_rendered;
  Uint16 screen_width;
  Uint16 screen_height;
  int fullscreen;
} sdl_video;



int SDL_OnResize(void* data, SDL_Event* event) {
  if (event->type == SDL_VIDEORESIZE) {
      bitmap.viewport.changed = 1;
  }
  return 1;
}

int Backend_Video_Init() {
  if(SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
    printf("SDL Video initialization failed");
    return 0;
  }

  sdl_video.surf_screen = SDL_SetVideoMode(VIDEO_WIDTH, VIDEO_HEIGHT, 16, SDL_SWSURFACE);
  sdl_video.surf_bitmap = SDL_CreateRGBSurface(SDL_SWSURFACE, 720, 576, 16, 0, 0, 0, 0);
  sdl_video.frames_rendered = 0;
  SDL_ShowCursor(0);

  SDL_LockSurface(sdl_video.surf_bitmap);
  bitmap.data = (unsigned char *)sdl_video.surf_bitmap->pixels;
  SDL_UnlockSurface(sdl_video.surf_bitmap);

  return 1;
}

int Backend_Video_Update() {
  /* viewport size changed */
  if(bitmap.viewport.changed & 1)
  {
    sdl_video.surf_screen  = SDL_GetVideoSurface();

    /* source bitmap */
    sdl_video.srect.w = 400;
    sdl_video.srect.h = 240;
    sdl_video.srect.x = 0;
    sdl_video.srect.y = 0;

    /* destination bitmap */
    sdl_video.drect.w = sdl_video.srect.w;
    sdl_video.drect.h = sdl_video.srect.h;
    sdl_video.drect.x = 0;
    sdl_video.drect.y = 0;
  }

  // SDL_UnlockSurface(sdl_video.surf_bitmap);
  SDL_BlitSurface(sdl_video.surf_bitmap, &sdl_video.srect, sdl_video.surf_screen, &sdl_video.drect);

  ++sdl_video.frames_rendered;

  return 1;
}

int Backend_Video_Close() {
  SDL_FreeSurface(sdl_video.surf_bitmap);
  // SDL_DestroyWindow(sdl_video.window);
  SDL_Quit();

  return 1;
}

int Backend_Video_SetFullscreen(int arg_fullscreen) {
  sdl_video.fullscreen = arg_fullscreen;
  // SDL_SetWindowFullscreen(sdl_video.window, sdl_video.fullscreen);

  return 1;
}

int Backend_Video_ToggleFullscreen() {
  Backend_Video_SetFullscreen(!sdl_video.fullscreen);

  return 1;
}

int Backend_Video_SetWindowTitle(char *caption) {
  SDL_WM_SetCaption(caption, NULL);
  return 1;
}

int Backend_Video_Present() {
  SDL_UpdateRect(sdl_video.surf_screen, 0, 0, 0, 0);
  return 1;
}

int Backend_Video_Clear() {
  SDL_FillRect(sdl_video.surf_screen, 0, 0);
  return 1;
}

int Backend_Video_SetVsync(int vsync) { return -1 };
int Backend_Video_GetRefreshRate()  { return -1 };
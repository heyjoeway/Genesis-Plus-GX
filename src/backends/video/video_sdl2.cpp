#include "video_base.h"

#include <cstring>
#include <string>
#include <map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "shared.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#if defined(USE_8BPP_RENDERING)
  #define SURFACE_FORMAT SDL_PIXELFORMAT_RGB332
#elif defined(USE_15BPP_RENDERING)
  #define SURFACE_FORMAT SDL_PIXELFORMAT_RGB555
#elif defined(USE_16BPP_RENDERING)
  #define SURFACE_FORMAT SDL_PIXELFORMAT_RGB565
#elif defined(USE_32BPP_RENDERING)
  #define SURFACE_FORMAT SDL_PIXELFORMAT_RGB888
#endif

SDL_Window* sdl_window;
SDL_Renderer* sdl_renderer;
SDL_Surface* sdl_winsurf;
SDL_Surface* sdl_bitmap;
SDL_Texture* sdl_texture;
SDL_Rect rect_source;
SDL_Rect rect_dest;
Uint16 screen_width;
Uint16 screen_height;
int fullscreen;

Uint32 tex_fmt_id;
SDL_PixelFormat *dst_fmt;

int SDL_OnResize(void* data, SDL_Event* event) {
  if (
    event->type == SDL_WINDOWEVENT &&
    event->window.event == SDL_WINDOWEVENT_RESIZED
  ) bitmap.viewport.changed = 1;
  return 1;
}

void Init_Bitmap() {
  if (sdl_texture != NULL) SDL_DestroyTexture(sdl_texture);
  if (sdl_bitmap != NULL) SDL_FreeSurface(sdl_bitmap);

  sdl_bitmap = SDL_CreateRGBSurfaceWithFormat(
    0,
    VIDEO_WIDTH,
    (VIDEO_HEIGHT * 2) + 1, // idk but it stops a segfault in 2p
    SDL_BITSPERPIXEL(SURFACE_FORMAT),
    SURFACE_FORMAT
  );

  sdl_texture = SDL_CreateTexture(
    sdl_renderer, 
    SURFACE_FORMAT, 
    SDL_TEXTUREACCESS_STREAMING,
    VIDEO_WIDTH,
    (VIDEO_HEIGHT * 2) + 1
  );
  SDL_SetTextureBlendMode(sdl_texture, SDL_BLENDMODE_BLEND);

  SDL_LockSurface(sdl_bitmap);
  bitmap.data = (unsigned char *)sdl_bitmap->pixels;
  SDL_UnlockSurface(sdl_bitmap);
}

int Backend_Video_Init() {
  if(SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
    SDL_ShowSimpleMessageBox(
      SDL_MESSAGEBOX_ERROR,
      "Error",
      "SDL Video initialization failed.",
      sdl_window
    );
    return 0;
  }

  // IMG_Init(IMG_INIT_PNG);

  uint32 window_flags = SDL_WINDOW_RESIZABLE;
  #if !defined(SWITCH)
  window_flags |= SDL_WINDOW_ALLOW_HIGHDPI;
  #endif

  sdl_window = SDL_CreateWindow(
    "Loading...",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    VIDEO_WIDTH * WINDOW_SCALE,
    VIDEO_HEIGHT * WINDOW_SCALE,
    window_flags
  );

  sdl_renderer = SDL_CreateRenderer(
    sdl_window,
    -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );
  
  sdl_winsurf = SDL_GetWindowSurface(sdl_window);

  Init_Bitmap();

  SDL_ShowCursor(0);
  SDL_AddEventWatch(SDL_OnResize, sdl_window);

  SDL_QueryTexture(sdl_texture, &tex_fmt_id, NULL, NULL, NULL);
  dst_fmt = SDL_AllocFormat(tex_fmt_id);

  return 1;
}

bool needs_clear = false;

int Backend_Video_Clear() {
  if (!needs_clear) return 0;
  needs_clear = false;
  SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
  SDL_RenderClear(sdl_renderer);
  return 1;
}

void Update_Viewport() {
  if (!(bitmap.viewport.changed & 1)) return;

  SDL_Rect window_rect = { .x = 0, .y = 0 };
  SDL_GetWindowSize(sdl_window, &window_rect.w, &window_rect.h);

  screen_width = window_rect.w;
  screen_height = window_rect.h;

  #ifdef __EMSCRIPTEN__
  double dpi = emscripten_get_device_pixel_ratio();
  screen_width *= dpi;
  screen_height *= dpi;
  #endif

  bitmap.viewport.changed &= ~1;

  /* source bitmap */
  rect_source.w = bitmap.viewport.w + (2*bitmap.viewport.x);
  rect_source.h = bitmap.viewport.h + (2*bitmap.viewport.y);
  if (interlaced) rect_source.h += bitmap.viewport.h;
  rect_source.x = 0;
  rect_source.y = 0;
  rect_dest.h = screen_height;

  if (option_scaling == 2) { // Stretch
    rect_dest.w = screen_width;
    rect_dest.x = 0;
    rect_dest.y = 0;
  } else {
    int video_height = bitmap.viewport.h;
    if (interlaced) video_height *= 2;

    float aspect_ratio = bitmap.viewport.w / (float)video_height;
    rect_dest.w = aspect_ratio * rect_dest.h;

    if (rect_dest.w > screen_width) {
      rect_dest.w = screen_width;
      rect_dest.h = rect_dest.w / aspect_ratio;
    }

    if (option_scaling == 1) { // Integer scaling
      rect_dest.h = (rect_dest.h / video_height) * (float)video_height;
      rect_dest.w = aspect_ratio * rect_dest.h;
    }

    rect_dest.x = (screen_width / 2.0) - (rect_dest.w / 2.0);
    rect_dest.y = (screen_height / 2.0) - (rect_dest.h / 2.0);
  }

  needs_clear = true;
}

void Update_Texture() {
  /* Set up a destination surface for the texture update */
  SDL_LockSurface(sdl_bitmap);
  SDL_Surface *temp = SDL_ConvertSurface(sdl_bitmap, dst_fmt, 0);
  SDL_UnlockSurface(sdl_bitmap);

  Uint32 key_color = SDL_MapRGB(
    temp->format,
    0xFF, 0x00, 0xFF
  );

  // "Green screen" the image
  for (int i = 0; i < temp->w * temp->h; i++) {
    if (((Uint32*)temp->pixels)[i] != key_color) continue;
    ((Uint32*)temp->pixels)[i] &= 0x00FFFFFF; 
  }

  SDL_UpdateTexture(sdl_texture, NULL, temp->pixels, temp->pitch);
  SDL_FreeSurface(temp);
}

void Update_Renderer() {
  if (option_mirrormode) {
    SDL_RenderCopyEx(
      sdl_renderer,
      sdl_texture,
      &rect_source,
      &rect_dest,
      0,
      NULL,
      SDL_FLIP_HORIZONTAL
    );
  } else {
    SDL_RenderCopy(
      sdl_renderer,
      sdl_texture,
      &rect_source,
      &rect_dest
    );
  }
}

int Backend_Video_Update() {
  Update_Viewport();
  Update_Texture();
  Update_Renderer();

  return 1;
}

int Backend_Video_Present() {
  SDL_RenderPresent(sdl_renderer);
  return 1;
}

int Backend_Video_Close() {
  SDL_FreeFormat(dst_fmt);
  SDL_DestroyTexture(sdl_texture);
  SDL_FreeSurface(sdl_bitmap);
  SDL_DestroyWindow(sdl_window);
  SDL_Quit();

  return 1;
}

int Backend_Video_SetFullscreen(int arg_fullscreen) {
  fullscreen = arg_fullscreen;
  SDL_SetWindowFullscreen(
    sdl_window,
    fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0
  );
  return 1;
}

int Backend_Video_ToggleFullscreen() {
  Backend_Video_SetFullscreen(!fullscreen);
  return 1;
}

int Backend_Video_SetWindowTitle(char *caption) {
  SDL_SetWindowTitle(sdl_window, caption);
  return 1;
}

std::map<std::string,SDL_Texture*> tex_cache;

void *Backend_Video_LoadImage(char *path) {
    // std::string pathstr = path;
    // SDL_Texture *tex;
    // std::map<std::string,SDL_Texture*>::iterator it = tex_cache.find(pathstr);
    // if(it != tex_cache.end()) {
    //     tex = it->second;
    // } else {
    //   SDL_Surface* tmp_surface = IMG_Load(path);
    //   if (!tmp_surface) return NULL;
    //   tex = SDL_CreateTextureFromSurface(sdl_renderer, tmp_surface);
    //   SDL_FreeSurface(tmp_surface);
    //   tex_cache.insert(it,std::pair<std::string,SDL_Texture*>(pathstr,tex));
    // }
    // return (void *)tex;
    return NULL;
}

void *Backend_Video_GetRenderer() { return (void *)sdl_renderer; }

int vsync_prev = 0;

int Backend_Video_SetVsync(int vsync) {
  if (vsync == vsync_prev) return vsync;
  vsync_prev = vsync;

  SDL_SetHintWithPriority(
    SDL_HINT_RENDER_VSYNC,
    vsync ? "1" : "0",
    SDL_HINT_OVERRIDE
  );
  return vsync;
}

int Backend_Video_GetRefreshRate() {
  // https://davidgow.net/handmadepenguin/ch18.html
  SDL_DisplayMode mode;
  int displayIndex = SDL_GetWindowDisplayIndex(sdl_window);
  // If we can't find the refresh rate, we'll return this:
  int defaultRefreshRate = 60;
  if (SDL_GetDesktopDisplayMode(displayIndex, &mode) != 0)
      return defaultRefreshRate;

  if (mode.refresh_rate == 0)
      return defaultRefreshRate;

  return mode.refresh_rate;
}

int Backend_Video_GetActive() {
  return !(SDL_GetWindowFlags(sdl_window) & SDL_WINDOW_MINIMIZED);
}
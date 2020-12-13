#include "video_base.h"

#include <cstring>
#include <string>
#include <map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "shared.h"

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
SDL_Surface* sdl_bitmap_1;
SDL_Surface* sdl_bitmap_2;
SDL_Texture* sdl_texture_1;
SDL_Texture* sdl_texture_2;
SDL_Rect rect_source;
SDL_Rect rect_dest_1;
SDL_Rect rect_dest_2;
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
  if (sdl_texture_1 != NULL) SDL_DestroyTexture(sdl_texture_1);
  if (sdl_bitmap_1 != NULL) SDL_FreeSurface(sdl_bitmap_1);

  sdl_bitmap_1 = SDL_CreateRGBSurfaceWithFormat(
    0,
    VIDEO_WIDTH,
    (VIDEO_HEIGHT * 2) + 1, // idk but it stops a segfault in 2p
    SDL_BITSPERPIXEL(SURFACE_FORMAT),
    SURFACE_FORMAT
  );

  sdl_texture_1 = SDL_CreateTextureFromSurface(sdl_renderer, sdl_bitmap_1);
  SDL_SetTextureBlendMode(sdl_texture_1, SDL_BLENDMODE_BLEND);

  SDL_LockSurface(sdl_bitmap_1);
  bitmap.data1 = (unsigned char *)sdl_bitmap_1->pixels;
  SDL_UnlockSurface(sdl_bitmap_1);

  // ==========================================================================

  if (sdl_texture_2 != NULL) SDL_DestroyTexture(sdl_texture_2);
  if (sdl_bitmap_2 != NULL) SDL_FreeSurface(sdl_bitmap_2);

  sdl_bitmap_2 = SDL_CreateRGBSurfaceWithFormat(
    0,
    VIDEO_WIDTH,
    (VIDEO_HEIGHT * 2) + 1, // idk but it stops a segfault in 2p
    SDL_BITSPERPIXEL(SURFACE_FORMAT),
    SURFACE_FORMAT
  );

  sdl_texture_2 = SDL_CreateTextureFromSurface(sdl_renderer, sdl_bitmap_2);
  SDL_SetTextureBlendMode(sdl_texture_2, SDL_BLENDMODE_BLEND);

  SDL_LockSurface(sdl_bitmap_2);
  bitmap.data2 = (unsigned char *)sdl_bitmap_2->pixels;
  SDL_UnlockSurface(sdl_bitmap_2);
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
  #if !defined(SWITCH) && defined(__EMSCRIPTEN__)
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

  SDL_QueryTexture(sdl_texture_1, &tex_fmt_id, NULL, NULL, NULL);
  dst_fmt = SDL_AllocFormat(tex_fmt_id);

  return 1;
}

int Backend_Video_Clear() {
  SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
  SDL_RenderClear(sdl_renderer);
  return 1;
}

void Update_Viewport() {
  if (!(bitmap.viewport.changed & 1)) return;

  /* viewport size changed */
  sdl_winsurf  = SDL_GetWindowSurface(sdl_window);

  #if defined(SWITCH) || defined(MACOS)
  screen_width = 1280;
  screen_height = 720;
  #elif defined(__vita__)
  screen_width = 960;
  screen_height = 544;
  #else
  screen_width = sdl_winsurf->w;
  screen_height = sdl_winsurf->h;
  #endif

  bitmap.viewport.changed &= ~1;

  /* source bitmap */
  rect_source.w = bitmap.viewport.w + (2*bitmap.viewport.x);
  rect_source.h = bitmap.viewport.h + (2*bitmap.viewport.y);
  if (interlaced) rect_source.h += bitmap.viewport.h;
  rect_source.x = 0;
  rect_source.y = 0;
  rect_dest_1.h = screen_height;

  if (option_scaling == 2) { // Stretch
    rect_dest_1.w = screen_width;
    rect_dest_1.x = 0;
    rect_dest_1.y = 0;
  } else {
    int video_height = bitmap.viewport.h;
    if (interlaced) video_height *= 2;

    float aspect_ratio = bitmap.viewport.w / (float)video_height;
    if (machine_maxindex > 0) aspect_ratio /= 2.0;
    rect_dest_1.w = aspect_ratio * rect_dest_1.h;

    if (rect_dest_1.w > screen_width) {
      rect_dest_1.w = screen_width;
      rect_dest_1.h = (rect_dest_1.w / aspect_ratio);
    }

    if (option_scaling == 1) { // Integer scaling
      rect_dest_1.h = (rect_dest_1.h / video_height) * (float)video_height;
      rect_dest_1.w = aspect_ratio * rect_dest_1.h;
    }

    rect_dest_1.x = (screen_width / 2.0) - (rect_dest_1.w / 2.0);
    rect_dest_1.y = (screen_height / 2.0) - (rect_dest_1.h / 2.0);
  }
  #ifdef SWITCH // ??????? idk
    rect_dest_1.w *= 0.9343065693430657;
    rect_dest_1.h *= 0.9343065693430657;
    rect_dest_1.x *= 0.9343065693430657;
    rect_dest_1.y *= 0.9343065693430657;
  #endif

  if (machine_maxindex > 0) {
    rect_dest_1.h /= 2.0;

    rect_dest_2.w = rect_dest_1.w;
    rect_dest_2.h = rect_dest_1.h;
    rect_dest_2.x = rect_dest_1.x;
    rect_dest_2.y = rect_dest_1.y + rect_dest_1.h;
  }

  /* clear destination surface */
  SDL_FillRect(sdl_winsurf, 0, 0);
}

void Update_Texture() {
  {
    /* Set up a destination surface for the texture update */
    SDL_LockSurface(sdl_bitmap_1);
    SDL_Surface *temp = SDL_ConvertSurface(sdl_bitmap_1, dst_fmt, 0);
    SDL_UnlockSurface(sdl_bitmap_1);

    Uint32 key_color = SDL_MapRGB(
      temp->format,
      0xFF, 0x00, 0xFF
    );

    // "Green screen" the image
    for (int i = 0; i < temp->w * temp->h; i++) {
      if (((Uint32*)temp->pixels)[i] != key_color) continue;
      ((Uint32*)temp->pixels)[i] &= 0x00FFFFFF; 
    }

    SDL_UpdateTexture(sdl_texture_1, NULL, temp->pixels, temp->pitch);
    SDL_FreeSurface(temp);
  }

  // ==========================================================================

  if (machine_maxindex > 0) {
    /* Set up a destination surface for the texture update */
    SDL_LockSurface(sdl_bitmap_2);
    SDL_Surface *temp = SDL_ConvertSurface(sdl_bitmap_2, dst_fmt, 0);
    SDL_UnlockSurface(sdl_bitmap_2);

    Uint32 key_color = SDL_MapRGB(
      temp->format,
      0xFF, 0x00, 0xFF
    );

    // "Green screen" the image
    for (int i = 0; i < temp->w * temp->h; i++) {
      if (((Uint32*)temp->pixels)[i] != key_color) continue;
      ((Uint32*)temp->pixels)[i] &= 0x00FFFFFF; 
    }

    SDL_UpdateTexture(sdl_texture_2, NULL, temp->pixels, temp->pitch);
    SDL_FreeSurface(temp);
  }
}

void Update_Renderer() {
  if (option_mirrormode) {
    SDL_RenderCopyEx(
      sdl_renderer,
      sdl_texture_1,
      &rect_source,
      &rect_dest_1,
      0,
      NULL,
      SDL_FLIP_HORIZONTAL
    );
    if (machine_maxindex > 0) {
      SDL_RenderCopyEx(
        sdl_renderer,
        sdl_texture_2,
        &rect_source,
        &rect_dest_2,
        0,
        NULL,
        SDL_FLIP_HORIZONTAL
      );
    }
  } else {
    SDL_RenderCopy(
      sdl_renderer,
      sdl_texture_1,
      &rect_source,
      &rect_dest_1
    );
    if (machine_maxindex > 0) {
      SDL_RenderCopy(
        sdl_renderer,
        sdl_texture_2,
        &rect_source,
        &rect_dest_2
      );
    }
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
  SDL_DestroyTexture(sdl_texture_1);
  SDL_DestroyTexture(sdl_texture_2);
  SDL_FreeSurface(sdl_bitmap_1);
  SDL_FreeSurface(sdl_bitmap_2);
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
#include <time.h>
#include <errno.h>

#include <sys/stat.h>
#include <limits.h>
#include <jansson.h>

#ifdef __EMSCRIPTEN__
  #include <emscripten.h>
#endif

#ifdef ENABLE_NXLINK
  #ifdef __cplusplus
  extern "C" {
  #endif

  #include "switch/runtime/devices/socket.h"
  #include "switch/runtime/nxlink.h"

  #ifdef __cplusplus
  }
  #endif
#endif

#include "shared.h"
#include "sms_ntsc.h"
#include "md_ntsc.h"
#include "config.h"
#include "inputact.h"

#include "argparse.h"
#ifdef ENABLE_DIALOGS
  #include "portable-file-dialogs.h"
#endif

#include "backends/sound/sound_base.h"
short soundframe[SOUND_SAMPLES_SIZE];

#include "backends/video/video_base.h"
int option_mirrormode = 0;
int option_scaling = 0;

#include "backends/input/input_base.h"

#include "gamehacks.h"

#define STATIC_ASSERT(name, test) typedef struct { int assert_[(test)?1:-1]; } assert_ ## name ## _
#define M68K_MAX_CYCLES 1107
#define Z80_MAX_CYCLES 345
#define OVERCLOCK_FRAME_DELAY 100

#ifdef M68K_OVERCLOCK_SHIFT
#define HAVE_OVERCLOCK
STATIC_ASSERT(m68k_overflow,
              M68K_MAX_CYCLES <= UINT_MAX >> (M68K_OVERCLOCK_SHIFT + 1));
#endif

#ifdef Z80_OVERCLOCK_SHIFT
#ifndef HAVE_OVERCLOCK
#define HAVE_OVERCLOCK
#endif
STATIC_ASSERT(z80_overflow,
              Z80_MAX_CYCLES <= UINT_MAX >> (Z80_OVERCLOCK_SHIFT + 1));
#endif

#define FRAMERATE_TARGET  59.92

/* Some games appear to calibrate music playback speed for PAL/NTSC by
   actually counting CPU cycles per frame during startup, resulting in
   hilariously fast music.  Delay overclocking for a while as a
   workaround */
#ifdef HAVE_OVERCLOCK
static uint32 overclock_delay;
#endif

int log_error   = 0;
int debug_on    = 0;
int turbo_mode  = 0;
int use_sound   = 1;
bool vsync_on = false;

static uint8 brm_format[0x40] =
{
  0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x00,0x00,0x00,0x00,0x40,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x53,0x45,0x47,0x41,0x5f,0x43,0x44,0x5f,0x52,0x4f,0x4d,0x00,0x01,0x00,0x00,0x00,
  0x52,0x41,0x4d,0x5f,0x43,0x41,0x52,0x54,0x52,0x49,0x44,0x47,0x45,0x5f,0x5f,0x5f
};

/* video */
md_ntsc_t *md_ntsc;
sms_ntsc_t *sms_ntsc;

static const uint16 vc_table[4][2] =
{
  /* NTSC, PAL */
  {0xDA , 0xF2},  /* Mode 4 (192 lines) */
  {0xEA , 0x102}, /* Mode 5 (224 lines) */
  {0xDA , 0xF2},  /* Mode 4 (192 lines) */
  {0x106, 0x10A}  /* Mode 5 (240 lines) */
};

#ifdef HAVE_OVERCLOCK
static void update_overclock(void)
{
#ifdef M68K_OVERCLOCK_SHIFT
    m68k.cycle_ratio = 1 << M68K_OVERCLOCK_SHIFT;
#endif
#ifdef Z80_OVERCLOCK_SHIFT
    z80_cycle_ratio = 1 << Z80_OVERCLOCK_SHIFT;
#endif
    if (overclock_delay == 0)
    {
      /* Cycle ratios multiply per-instruction cycle counts, so use
         reciprocals */
#ifdef M68K_OVERCLOCK_SHIFT
      if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
        m68k.cycle_ratio = (100 << M68K_OVERCLOCK_SHIFT) / 200;
#endif
#ifdef Z80_OVERCLOCK_SHIFT
      if ((system_hw & SYSTEM_PBC) != SYSTEM_MD)
        z80_cycle_ratio = (100 << Z80_OVERCLOCK_SHIFT) / 200;
#endif
    }
}
#endif

#ifdef __EMSCRIPTEN__
  #define PATH_SRAM_ROOT "/home/web_user/"
#else
  #define PATH_SRAM_ROOT "./"
#endif

int running = 1;

#define SRAM_AUTOWRITE_INTERVAL 60 * 30 // In frames
int sram_autowrite_timer = 0;

extern "C" {

void sram_write() {
  FILE *fp;

  if (sram.on) {
    /* save SRAM */
    fp = fopen(PATH_SRAM_ROOT "game.srm", "wb");
    if (fp != NULL) {
      fwrite(sram.sram,0x10000,1, fp);
      fclose(fp);
    }
  }

  if (system_hw == SYSTEM_MCD) {
    /* save internal backup RAM (if formatted) */
    if (!memcmp(scd.bram + 0x2000 - 0x20, brm_format + 0x20, 0x20)) {
      fp = fopen(PATH_SRAM_ROOT "scd.brm", "wb");
      if (fp!=NULL) {
        fwrite(scd.bram, 0x2000, 1, fp);
        fclose(fp);
      }
    }

    /* save cartridge backup RAM (if formatted) */
    if (scd.cartridge.id) {
      if (!memcmp(scd.cartridge.area + scd.cartridge.mask + 1 - 0x20, brm_format + 0x20, 0x20)) {
        fp = fopen(PATH_SRAM_ROOT "cart.brm", "wb");
        if (fp!=NULL) {
          fwrite(scd.cartridge.area, scd.cartridge.mask + 1, 1, fp);
          fclose(fp);
        }
      }
    }
  }

	#ifdef __EMSCRIPTEN__
		EM_ASM(
			FS.syncfs(function (err) { console.log(err); });
		);
	#endif
}

}

void mainloop() {
  Backend_Input_MainLoop();

  if (!Backend_Video_GetActive()) {
      timespec spec = {
        .tv_sec = 0,
        .tv_nsec = 10000000
      };
      nanosleep(&spec, &spec);
      return;
  }
  gamehacks_update();

  #ifdef HAVE_OVERCLOCK
    /* update overclock delay */
    if (overclock_delay && --overclock_delay == 0)
        update_overclock();
  #endif
  Backend_Video_Clear();
  gamehacks_render();

  if (system_hw == SYSTEM_MCD) system_frame_scd(0);
  else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD) system_frame_gen(0);
  else system_frame_sms(0);

  Backend_Video_Update();
  Backend_Video_Present();
  int sound_update_size = audio_update(soundframe) * 2;
  if (use_sound) Backend_Sound_Update(sound_update_size);

  if (++sram_autowrite_timer >= SRAM_AUTOWRITE_INTERVAL) {
    sram_autowrite_timer = 0;
    sram_write();
  }
}

char *get_valid_filepath_jsonarray(json_t *patharr) {
  if (patharr == NULL) return NULL;
  if (!json_is_array(patharr)) return NULL;

  // Iterate over array
  struct stat statbuffer;
  for (int i = 0; i < json_array_size(patharr); i++) {
      // Ensure patharr[i] is a string
      json_t *element = json_array_get(patharr, i);
      if (!json_is_string(element)) continue;

      // Ensure patharr[i] is a valid path
      const char *path = json_string_value(element);
      int path_exists = stat(path, &statbuffer) == 0;
      // If it is, we've got our file
      if (path_exists) return (char *)path;
  }
  return NULL;
}

char *get_rom_path() {
  // Ensure config.rom exists
  json_t *config_rom = json_object_get(config_json, "rom");
  if (config_rom == NULL) return NULL;

  json_t *config_rompaths = json_object_get(config_rom, "paths");
  return get_valid_filepath_jsonarray(config_rompaths);
}

char *get_diff_path() {
  // Ensure config.rom exists
  json_t *config_rom = json_object_get(config_json, "rom");
  if (config_rom == NULL) return NULL;

  // Ensure config.rom.paths_patch exists
  json_t *config_patchpaths = json_object_get(config_rom, "paths_patch");
  return get_valid_filepath_jsonarray(config_patchpaths);
}

int main (int argc, char *argv[]) {
  char *rom_path = NULL;
  char *diff_path = NULL;

  #ifdef __EMSCRIPTEN__
    rom_path = "/home/web_user/rom.bin";
    diff_path = "/patch.ips";
  #endif

  char *config_path = "./config.json";

  // This isn't just disabled cause emscripten doesnt take args (kinda)
  // It's disabled because it causes a crash for some reason
  #ifndef __EMSCRIPTEN__
    struct argparse_option options[] = {
      OPT_HELP(),
      OPT_STRING('r', "rom", &rom_path, "Path to ROM file"),
      OPT_STRING('p', "patch", &diff_path, "Path to IPS patch file"),
      OPT_STRING('c', "config", &config_path, "Path to config file"),
      OPT_END(),
    };
    struct argparse argparse;
    argparse_init(&argparse, options, 0, 0);
    argparse_describe(
      &argparse,
      "\nGPGX Widescreen",
      ""
    );

    argc = argparse_parse(&argparse, argc, (const char **)argv);
  #endif

  if ((rom_path == NULL) && (argc > 1))
    rom_path = (char *)argv[1];

  #ifdef ENABLE_NXLINK
  socketInitializeDefault();
  nxlinkStdio();
  #endif

  /* set default config */
  error_init();
  config_load(config_path);

  if (!rom_path) rom_path = get_rom_path();
  if (!diff_path) diff_path = get_diff_path();

  /* Genesis BOOT ROM support (2KB max) */
  memset(boot_rom, 0xFF, 0x800);
  FILE *fp = fopen(MD_BIOS, "rb");
  if (fp != NULL) {
    /* read BOOT ROM */
    fread(boot_rom, 1, 0x800, fp);
    fclose(fp);

    /* check BOOT ROM */
    if (!memcmp((char *)(boot_rom + 0x120),"GENESIS OS", 10))
      system_bios = SYSTEM_MD; /* mark Genesis BIOS as loaded */

    /* Byteswap ROM */
    for (int i = 0; i < 0x800; i += 2) {
      uint8 temp = boot_rom[i];
      boot_rom[i] = boot_rom[i+1];
      boot_rom[i+1] = temp;
    }
  }

  // Ensure config.rom exists
  json_t *config_rom = json_object_get(config_json, "rom");
  if (config_rom == NULL) return 0;

  // Load rom and patch, show warning messages if any issues occur
  #ifdef ENABLE_DIALOGS
    json_t *config_warn_patch_missing = json_object_get(config_rom, "warn_patch_missing");
    if (
      (diff_path == NULL) &&
      (config_warn_patch_missing != NULL) &&
      json_boolean_value(config_warn_patch_missing)
    ) {
      pfd::message(
        "Patch Missing", 
        "You are missing the IPS patch for this game. Things will likely not work correctly. Check your config.json if you're looking for where to put the patch file.",
        pfd::choice::ok,
        pfd::icon::warning
      );
    }
  #endif

  if((rom_path == NULL) || !load_rom(rom_path, diff_path)) {
    char caption[256];
    sprintf(caption, "Error loading file `%s'.", rom_path);
    #ifdef ENABLE_DIALOGS
      pfd::message(
        "ROM Missing",
        "You're missing a ROM file. Check your config.json if you're looking for where the ROM should be placed.",
        pfd::choice::ok,
        pfd::icon::error
      );
    #endif
  
    #ifdef ENABLE_NXLINK
      socketExit();
    #endif

    return 1;
  }

  /* initialize Genesis virtual system */
  memset(&bitmap, 0, sizeof(t_bitmap));
  bitmap.width        = 400;
  bitmap.height       = 224;
#if defined(USE_8BPP_RENDERING)
  bitmap.pitch        = (bitmap.width * 1);
#elif defined(USE_15BPP_RENDERING)
  bitmap.pitch        = (bitmap.width * 2);
#elif defined(USE_16BPP_RENDERING)
  bitmap.pitch        = (bitmap.width * 2);
#elif defined(USE_32BPP_RENDERING)
  bitmap.pitch        = (bitmap.width * 4);
#endif
  /* initialize backends */
  Backend_Video_Init();
  Backend_Input_Init();
  inputact_init();
  Backend_Sound_Init();

  bitmap.viewport.changed = 3;

  char caption[100];
  sprintf(caption,"%s", rominfo.international);
  Backend_Video_SetWindowTitle(caption);

#ifdef HAVE_OVERCLOCK
    overclock_delay = OVERCLOCK_FRAME_DELAY;
#endif

  /* initialize system hardware */
  double framerate = Backend_Video_GetRefreshRate();
  vsync_on = true;
  if (abs(framerate - FRAMERATE_TARGET) > 1.0) {
    framerate = FRAMERATE_TARGET;
    vsync_on = false;
  }
  Backend_Video_SetVsync(vsync_on);
  printf("%i\n", vsync_on);

  audio_init(SOUND_FREQUENCY, framerate);
  system_init();

  /* Mega CD specific */
  if (system_hw == SYSTEM_MCD)
  {
    /* load internal backup RAM */
    fp = fopen(PATH_SRAM_ROOT "scd.brm", "rb");
    if (fp!=NULL)
    {
      fread(scd.bram, 0x2000, 1, fp);
      fclose(fp);
    }

    /* check if internal backup RAM is formatted */
    if (memcmp(scd.bram + 0x2000 - 0x20, brm_format + 0x20, 0x20))
    {
      /* clear internal backup RAM */
      memset(scd.bram, 0x00, 0x200);

      /* Internal Backup RAM size fields */
      brm_format[0x10] = brm_format[0x12] = brm_format[0x14] = brm_format[0x16] = 0x00;
      brm_format[0x11] = brm_format[0x13] = brm_format[0x15] = brm_format[0x17] = (sizeof(scd.bram) / 64) - 3;

      /* format internal backup RAM */
      memcpy(scd.bram + 0x2000 - 0x40, brm_format, 0x40);
    }

    /* load cartridge backup RAM */
    if (scd.cartridge.id)
    {
      fp = fopen(PATH_SRAM_ROOT "cart.brm", "rb");
      if (fp!=NULL)
      {
        fread(scd.cartridge.area, scd.cartridge.mask + 1, 1, fp);
        fclose(fp);
      }

      /* check if cartridge backup RAM is formatted */
      if (memcmp(scd.cartridge.area + scd.cartridge.mask + 1 - 0x20, brm_format + 0x20, 0x20))
      {
        /* clear cartridge backup RAM */
        memset(scd.cartridge.area, 0x00, scd.cartridge.mask + 1);

        /* Cartridge Backup RAM size fields */
        brm_format[0x10] = brm_format[0x12] = brm_format[0x14] = brm_format[0x16] = (((scd.cartridge.mask + 1) / 64) - 3) >> 8;
        brm_format[0x11] = brm_format[0x13] = brm_format[0x15] = brm_format[0x17] = (((scd.cartridge.mask + 1) / 64) - 3) & 0xff;

        /* format cartridge backup RAM */
        memcpy(scd.cartridge.area + scd.cartridge.mask + 1 - sizeof(brm_format), brm_format, sizeof(brm_format));
      }
    }
  }

  if (sram.on)
  {
    /* load SRAM */
    fp = fopen(PATH_SRAM_ROOT "game.srm", "rb");
    if (fp!=NULL)
    {
      fread(sram.sram,0x10000,1, fp);
      fclose(fp);
    }
  }

  /* reset system hardware */
  system_reset();

  //if (use_sound) Backend_Sound_Pause();

  long updatePeriod_nsec = (1000000000.0L / FRAMERATE_TARGET);

  gamehacks_init();

  /* emulation loop */
  #ifdef __EMSCRIPTEN__
   emscripten_set_main_loop(&mainloop, 60, 1);
  #else
    timespec timeAfter, timeBefore;

    while(running) {
      clock_gettime(CLOCK_MONOTONIC, &timeBefore);
      mainloop();
      clock_gettime(CLOCK_MONOTONIC, &timeAfter);

      timespec deltaSpec = {
        .tv_sec = timeAfter.tv_sec - timeBefore.tv_sec,
        .tv_nsec = timeAfter.tv_nsec - timeBefore.tv_nsec
      };

      if (deltaSpec.tv_nsec < 0) {
          --deltaSpec.tv_sec;
          deltaSpec.tv_nsec += 1000000000L;
      }

      Backend_Video_SetVsync(vsync_on && !turbo_mode);

      if (!turbo_mode && !vsync_on && ((updatePeriod_nsec - deltaSpec.tv_nsec) > 0)) {
        deltaSpec.tv_nsec = updatePeriod_nsec - deltaSpec.tv_nsec;

        while ( nanosleep(&deltaSpec, &deltaSpec) == EINTR ) {
          /* Keep running "nanosleep" in case interrupt signal was received */;
        }
      }
    }

    sram_write();

    gamehacks_deinit();

    audio_shutdown();
    error_shutdown();

    Backend_Input_Close();
    Backend_Sound_Close();
    Backend_Video_Close();

    #ifdef ENABLE_NXLINK
    socketExit();
    #endif
  #endif
  
  return 0;
}

#ifdef __EMSCRIPTEN__
extern "C" { // For calling from JS in Emscripten

int emscripten_main(int argc, char *argv[]) {
  return main(argc, argv);
}

}
#endif
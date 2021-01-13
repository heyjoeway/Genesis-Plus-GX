
#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "shared.h"
#include "jansson.h"

/****************************************************************************
 * Config Option 
 *
 ****************************************************************************/
typedef struct 
{
  uint8 padtype;
} t_input_config;

typedef struct 
{
  uint8 hq_fm;
  uint8 filter;
  uint8 hq_psg;
  uint8 ym2612;
  uint8 ym2413;
#ifdef HAVE_YM3438_CORE
  uint8 ym3438;
#endif
#ifdef HAVE_OPLL_CORE
  uint8 opll;
#endif
  int16 psg_preamp;
  int16 fm_preamp;
  uint32 lp_range;
  int16 low_freq;
  int16 high_freq;
  int16 lg;
  int16 mg;
  int16 hg;
  uint8 mono;
  uint8 system;
  uint8 region_detect;
  uint8 vdp_mode;
  uint8 master_clock;
  uint8 force_dtack;
  uint8 addr_error;
  uint8 bios;
  uint8 lock_on;
#ifdef HAVE_OVERCLOCK
  uint32 overclock;
#endif
  uint8 no_sprite_limit;
  uint8 hot_swap;
  uint8 invert_mouse;
  uint8 gun_cursor[2];
  uint8 overscan;
  uint8 gg_extra;
  uint8 ntsc;
  uint8 lcd;
  uint8 render;
  t_input_config input[MAX_INPUTS];
} t_config;

/* Global variables */
extern t_config config_legacy;
extern json_t *config_json;
extern int config_load(char *config_path);

#ifdef __cplusplus
}
#endif

#endif /* _CONFIG_H_ */


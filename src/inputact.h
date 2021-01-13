#ifndef __INPUTACT_H__
#define __INPUTACT_H__

#include "shared.h"

#ifdef __cplusplus
extern "C" {
#endif

void input_run_action(char *str, int press);
int input_process_keycode(char *keystr, int press);
void inputact_init();
void input_process_joystick(int joynum, int button, int press);
void input_update_pad(int padnum);

extern uint16 pad_action[MAX_DEVICES];
extern uint16 pad_analog[MAX_DEVICES];

#ifdef __cplusplus
}
#endif

#endif
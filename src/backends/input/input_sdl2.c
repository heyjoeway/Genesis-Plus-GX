#include "input_base.h"
#include "video_base.h"

#include "inputact.h"

#include <SDL2/SDL.h>
#include "shared.h"

#ifndef max
    #define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
    #define min(a,b) ((a) < (b) ? (a) : (b))
#endif


SDL_GameController *controllers[MAX_DEVICES];

void update_analog(int padnum) {
  int axis_lx = SDL_GameControllerGetAxis(controllers[padnum], SDL_CONTROLLER_AXIS_LEFTX);
  int axis_ly = SDL_GameControllerGetAxis(controllers[padnum], SDL_CONTROLLER_AXIS_LEFTY);

  double deadzone_l = 10000;

  // --------------------------------------------------------------------------

  double deadzone_lx_analog_upper = 10000;
  double intensity_lx_adj = min(1, max(0,
    (abs(axis_lx) - deadzone_l) /
    (32767.0 - deadzone_lx_analog_upper - deadzone_l)
  ));
  double lx_adj = (
    intensity_lx_adj *
    (axis_lx < 0 ? -1 : 1)
  );
  input.analog[padnum][0] = (lx_adj + 1) * (0X3F / 2.0);

  // --------------------------------------------------------------------------

  double deadzone_ly_analog_upper = 10000;
  double intensity_ly_adj = min(1, max(0,
    (abs(axis_ly) - deadzone_l) /
    (32767.0 - deadzone_ly_analog_upper - deadzone_l)
  ));
  double ly_adj = (
    intensity_ly_adj *
    (axis_ly < 0 ? -1 : 1)
  );
  input.analog[padnum][1] = (ly_adj + 1) * (0X3F / 2.0);

  // --------------------------------------------------------------------------

  double intensity_l = sqrt(pow(axis_lx, 2) + pow(axis_ly, 2));
  double angle_l = atan2((double)axis_ly, (double)axis_lx) + M_PI;
  double angle_overlap_x = M_PI_4 / 8.0;
  double angle_overlap_y = M_PI_4 / 8.0;

  pad_analog[padnum] = 0;

  if (intensity_l > deadzone_l) {
    if (
      (
        (angle_l < M_PI_4 + angle_overlap_x) ||
        (angle_l > ((2*M_PI) - M_PI_4 - angle_overlap_x))
      )
    ) pad_analog[padnum] |= INPUT_LEFT;

    if (
      (
        (angle_l < (M_PI + M_PI_4 + angle_overlap_x)) &&
        (angle_l > (M_PI - M_PI_4 - angle_overlap_x))
      )
    ) pad_analog[padnum] |= INPUT_RIGHT;

    if (
      (angle_l < (M_PI_2 + M_PI_4 + angle_overlap_y)) &&
      (angle_l > (M_PI_2 - M_PI_4 - angle_overlap_y))
    ) pad_analog[padnum] |= INPUT_UP;

    if (
      (angle_l < (M_PI + M_PI_2 + M_PI_4 + angle_overlap_y)) &&
      (angle_l > (M_PI + M_PI_2 - M_PI_4 - angle_overlap_y))
    ) pad_analog[padnum] |= INPUT_DOWN;
  }

  input_update_pad(padnum);
}

int Backend_Input_Update() {
  for (int i = 0; i < SDL_NumJoysticks(); i++)
    if (SDL_IsGameController(i) && (controllers[i] != NULL))
      update_analog(i);

  return 1;
}


void update_controller_connections() {
  for (int i = 0; i < SDL_NumJoysticks(); i++) {
      if (SDL_IsGameController(i)) {
          if (controllers[i] == NULL) {
            controllers[i] = SDL_GameControllerOpen(i);
            printf("New Controller: %i\n", i);
          }
      } else {
        if (controllers[i] != NULL) controllers[i] = NULL;
      }
  }
}

int Backend_Input_Init() {
  SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
  SDL_GameControllerEventState(SDL_ENABLE);
  SDL_GameControllerAddMappingsFromFile("./gamecontrollerdb.txt");

  for (int i=0;i < MAX_DEVICES;i++) controllers[i] = NULL;
  update_controller_connections();

  return 1;
}

int Backend_Input_Close() {
    return 1;
}

void key_callback(SDL_Keycode key, int scancode, int action) {
  char *keyname = (char *)SDL_GetKeyName(key);
	if (!input_process_keycode(keyname, action)) {
		char buffer[5];
		if (key >= 10000) return;
		sprintf(buffer, "%i", key);
		input_process_keycode(buffer, action);
	}
}

int Backend_Input_MainLoop() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch(event.type) {
      case SDL_QUIT:
        running = 0;
        break;
      case SDL_KEYUP:
      case SDL_KEYDOWN:
        key_callback(
          event.key.keysym.sym,
          event.key.keysym.scancode,
          event.type == SDL_KEYDOWN
        );
        break;
      case SDL_CONTROLLERDEVICEADDED:
      case SDL_CONTROLLERDEVICEREMOVED:
      case SDL_CONTROLLERDEVICEREMAPPED:
        update_controller_connections();
        break;
      case SDL_CONTROLLERBUTTONDOWN:
      case SDL_CONTROLLERBUTTONUP:
        input_process_joystick(
          event.cbutton.which,
          event.cbutton.button,
          event.cbutton.state == SDL_PRESSED
        );
    }
  }

  return 1;
}


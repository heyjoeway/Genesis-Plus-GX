#include "input_base.h"
#include "video_base.h"

#include <SDL/SDL.h>
#include "shared.h"

#ifndef max
    #define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
    #define min(a,b) ((a) < (b) ? (a) : (b))
#endif

// SDL_GameController* controller;
// int Backend_Input_Update() {
//   // const uint8 *keystate = SDL_GetKeyboardState(NULL);

//   /* reset input */
//   input.pad[joynum] = 0;

//   // I took out all the other input types for now, should probably be added back in later
//   switch (input.dev[joynum])
//   {   
//     default:
//     {
//       if(keystate[SDL_SCANCODE_A])  input.pad[joynum] |= INPUT_A;
//       if(keystate[SDL_SCANCODE_S])  input.pad[joynum] |= INPUT_B;
//       if(keystate[SDL_SCANCODE_D])  input.pad[joynum] |= INPUT_C;
//       if(keystate[SDL_SCANCODE_F])  input.pad[joynum] |= INPUT_START;
//       if(keystate[SDL_SCANCODE_Z])  input.pad[joynum] |= INPUT_X;
//       if(keystate[SDL_SCANCODE_X])  input.pad[joynum] |= INPUT_Y;
//       if(keystate[SDL_SCANCODE_C])  input.pad[joynum] |= INPUT_Z;
//       if(keystate[SDL_SCANCODE_V])  input.pad[joynum] |= INPUT_MODE;

//       if(keystate[SDL_SCANCODE_UP]) input.pad[joynum] |= INPUT_UP;
//       else
//       if(keystate[SDL_SCANCODE_DOWN]) input.pad[joynum] |= INPUT_DOWN;
//       if(keystate[SDL_SCANCODE_LEFT]) input.pad[joynum] |= INPUT_LEFT;
//       else
//       if(keystate[SDL_SCANCODE_RIGHT]) input.pad[joynum] |= INPUT_RIGHT;

//       if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A))
//         input.pad[joynum] |= INPUT_A;

//       if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B))
//         input.pad[joynum] |= INPUT_B;

//       if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X))
//         input.pad[joynum] |= INPUT_C;

//       if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START))
//         input.pad[joynum] |= INPUT_START;

//       if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP))
//         input.pad[joynum] |= INPUT_UP;

//       if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
//         input.pad[joynum] |= INPUT_DOWN;

//       if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
//         input.pad[joynum] |= INPUT_LEFT;

//       if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
//         input.pad[joynum] |= INPUT_RIGHT;

//       int axis_lx = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
//       int axis_ly = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);

//       double deadzone_l = 10000;
//       double intensity_l = sqrt(pow(axis_lx, 2) + pow(axis_ly, 2));
//       double angle_l = atan2((double)axis_ly, (double)axis_lx) + M_PI;

//       // printf("%i, %i, %f, %f\n", axis_lx, axis_ly, intensity_l, angle_l);
//       double angle_overlap_x = M_PI_4 / 8.0;
//       double angle_overlap_y = M_PI_4 / 8.0;

//       double deadzone_lx_analog_upper = 10000;
//       double intensity_lx_adj = min(1, max(0,
//         (abs(axis_lx) - deadzone_l) /
//         (32767.0 - deadzone_lx_analog_upper - deadzone_l)
//       ));

//       intensity_lx_adj = ((intensity_lx_adj - 0) * (1 - 0.9)) / ((1 - 0) + 0.9);
//       int allow_horiz = 1;
//       if (intensity_lx_adj < 1.0) {
//         double frametimer_period = 3.0;
//         allow_horiz = (
//           (int)(frametimer / frametimer_period) %
//           (
//             (int)(60.0 / frametimer_period) -
//             (int)(intensity_lx_adj * (60.0 / frametimer_period))
//           )
//         ) == 0;
//       }

//       if (intensity_l > deadzone_l) {
//         if (
//           (
//             (angle_l < M_PI_4 + angle_overlap_x) ||
//             (angle_l > ((2*M_PI) - M_PI_4 - angle_overlap_x))
//           ) && (allow_horiz)
//         ) input.pad[joynum] |= INPUT_LEFT;

//         if (
//           (
//             (angle_l < (M_PI + M_PI_4 + angle_overlap_x)) &&
//             (angle_l > (M_PI - M_PI_4 - angle_overlap_x))
//           ) && (allow_horiz)
//         ) input.pad[joynum] |= INPUT_RIGHT;

//         if (
//           (angle_l < (M_PI_2 + M_PI_4 + angle_overlap_y)) &&
//           (angle_l > (M_PI_2 - M_PI_4 - angle_overlap_y))
//         ) input.pad[joynum] |= INPUT_UP;

//         if (
//           (angle_l < (M_PI + M_PI_2 + M_PI_4 + angle_overlap_y)) &&
//           (angle_l > (M_PI + M_PI_2 - M_PI_4 - angle_overlap_y))
//         ) input.pad[joynum] |= INPUT_DOWN;

//       }

//       break;
//     }
//   }

//   if (mirrormode) {
//     int val_left = input.pad[joynum] & INPUT_LEFT;
//     int val_right = input.pad[joynum] & INPUT_RIGHT;
//     input.pad[joynum] &= ~(INPUT_LEFT | INPUT_RIGHT);
//     if (val_left) input.pad[joynum] |= INPUT_RIGHT;
//     if (val_right) input.pad[joynum] |= INPUT_LEFT;
//   }
//   return 1;
// }

int Backend_Input_Init() {
  // SDL_InitSubSystem(SDL_INIT_JOYSTICK);
  // SDL_JoystickEventState(SDL_ENABLE);
  // SDL_JoystickOpen(0);
  // controller = SDL_GameControllerOpen(0);

  return 1;
}

int Backend_Input_Close() {
    return 1;
}

void key_callback(int key, int scancode, int action) {
  char *keyname = (char *)SDL_GetKeyName(key, 0);
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
          (int)event.key.keysym.sym,
          event.key.keysym.scancode,
          event.type == SDL_KEYUP
        );
        break;
    }
  }  
  return 1;
}


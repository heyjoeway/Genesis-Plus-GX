#include "input_base.h"
#include "video_base.h"

#include <GLFW/glfw3.h> // GLFW helper library

#include "inputact.h"
#include "shared.h"

#define MAX_JOYSTICKS 16

int Backend_Input_Update() {
	glfwPollEvents();
	return 1;
}

uint16 joystates_prev[MAX_JOYSTICKS];
uint16 joystates[MAX_JOYSTICKS];

void Update_Joystick() {
	for (int j = 0; j < MAX_JOYSTICKS; j++) {
		joystates[j] = 0;

		if (!glfwJoystickPresent(j)) continue;

		int count = 0;
		const unsigned char* buttons = glfwGetJoystickButtons(j, &count);
		for (int i = 0; i < count; i++) joystates[j] |= buttons[i] << i;

		uint16 joystate_change = joystates[j] ^ joystates_prev[j];

		if (joystate_change) {
			uint16 joystate_prev = joystates_prev[j];
			uint16 joystate = joystates[j];
			for (int b = 0; b < count; b++) {
				int button = (joystate >> b) & 0b0000000000000001; 
				int button_prev = (joystate_prev >> b) & 0b0000000000000001; 

				if (button && !button_prev) input_process_joystick(j, b, TRUE);
				else if (!button && button_prev) input_process_joystick(j, b, FALSE);
			}
		}

		joystates_prev[j] = joystates[j];
	}

	// const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &count);

	// int axis_lx = 0;
	// int axis_ly = 0;

	// if (count >= 1) {
	// 	axis_lx = axes[0] * 32767;
	// 	axis_ly = axes[1] * 32767;
	// }

	// double deadzone_l = 10000;
	// double intensity_l = sqrt(pow(axis_lx, 2) + pow(axis_ly, 2));
	// // printf("%f\n",intensity_l);
	// double angle_l = atan2((double)axis_ly, (double)axis_lx) + M_PI;

	// double angle_overlap_x = M_PI_4 / 8.0;
	// double angle_overlap_y = M_PI_4 / 8.0;

	// double deadzone_lx_analog_upper = 10000;
	// double intensity_lx_adj = min(1, max(0,
	// 	(abs(axis_lx) - deadzone_l) /
	// 	(32767.0 - deadzone_lx_analog_upper - deadzone_l)
	// ));


	// int allow_horiz = 1;
	// if (intensity_lx_adj < 1.0) {
	// 	double frametimer_period = 3.0;
	// 	allow_horiz = (
	// 		(int)(frametimer / frametimer_period) %
	// 		(
	// 			(int)(60.0 / frametimer_period) -
	// 			(int)(intensity_lx_adj * (60.0 / frametimer_period))
	// 		)
	// 	) == 0;
	// }

	// if (intensity_l > deadzone_l) {
	// 	if (
	// 		(
	// 		(angle_l < M_PI_4 + angle_overlap_x) ||
	// 		(angle_l > ((2*M_PI) - M_PI_4 - angle_overlap_x))
	// 		) && (allow_horiz)
	// 	) input.pad[joynum] |= INPUT_LEFT;

	// 	if (
	// 		(
	// 		(angle_l < (M_PI + M_PI_4 + angle_overlap_x)) &&
	// 		(angle_l > (M_PI - M_PI_4 - angle_overlap_x))
	// 		) && (allow_horiz)
	// 	) input.pad[joynum] |= INPUT_RIGHT;

	// 	if (
	// 		(angle_l < (M_PI_2 + M_PI_4 + angle_overlap_y)) &&
	// 		(angle_l > (M_PI_2 - M_PI_4 - angle_overlap_y))
	// 	) input.pad[joynum] |= INPUT_UP;

	// 	if (
	// 		(angle_l < (M_PI + M_PI_2 + M_PI_4 + angle_overlap_y)) &&
	// 		(angle_l > (M_PI + M_PI_2 - M_PI_4 - angle_overlap_y))
	// 	) input.pad[joynum] |= INPUT_DOWN;
	// }

	// input.pad[joynum] |= input_j1;
}

int Backend_Input_MainLoop() {
	Update_Joystick();
	return 1;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    char *keyname = (char *)glfwGetKeyName(key, 0);
	if (!input_process_keycode(keyname, action)) {
		char buffer[5];
		if (key >= 10000) return;
		sprintf(buffer, "%i", key);
		input_process_keycode(buffer, action);
	}
}

int Backend_Input_Init() {
	GLFWwindow *window = (GLFWwindow *)window_shared;
	glfwSetKeyCallback(window, key_callback);

	return 1;
}

int Backend_Input_Close() {
	glfwTerminate();
	return 1;
}
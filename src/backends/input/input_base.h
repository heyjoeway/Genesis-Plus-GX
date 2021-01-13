#ifndef __BACKEND_INPUT_BASE___
#define __BACKEND_INPUT_BASE___

#ifdef __cplusplus
extern "C" {
#endif

int Backend_Input_Update();
int Backend_Input_Init();
int Backend_Input_Close();
int Backend_Input_MainLoop();

#ifdef __cplusplus
}
#endif

#endif
#include <stdlib.h>

void openmpt_module_destroy(void * mod) { }
void *openmpt_module_create_from_memory(const void * filedata, size_t filesize, void *logfunc, void * user, void * ctls) { }
int openmpt_module_read_float_stereo(void * mod, int samplerate, size_t count, float * left, float * right) {
	return 0;
}

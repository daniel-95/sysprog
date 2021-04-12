#ifndef __MACRO_H
#define __MACRO_H

#include <stdio.h>

#define errdump(msg) do { printf("[ERROR] %s:%d %s\n", __FILE__, __LINE__,  msg); exit(EXIT_FAILURE); } while (0)
#define memcheck(ptr, msg) {  \
	if(ptr == NULL)       \
		errdump(msg); \
}

#endif

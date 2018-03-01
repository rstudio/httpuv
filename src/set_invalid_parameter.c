#ifdef _WIN32

#include <stdlib.h>

// These definitions are to make libuv work without linking to msvcr90

typedef void (*_invalid_parameter_handler)(const wchar_t * expression, const wchar_t * function, const wchar_t * file, unsigned int line, uintptr_t pReserved);

_invalid_parameter_handler _set_invalid_parameter_handler(_invalid_parameter_handler pNew) {
	return NULL;
}

#else

// Need to declare something to avoid "ISO C forbids an empty translation unit"
// warning when compiling with -pedantic
typedef int _set_invalid_parameter_dummy;

#endif

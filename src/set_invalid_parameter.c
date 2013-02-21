#include <stdlib.h>

// These definitions are to make libuv work without linking to msvcr90

typedef void (*_invalid_parameter_handler)(const wchar_t * expression, const wchar_t * function, const wchar_t * file, unsigned int line, uintptr_t pReserved);

_invalid_parameter_handler _set_invalid_parameter_handler(_invalid_parameter_handler pNew) {
	return NULL;
}

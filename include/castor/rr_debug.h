#ifndef RR_DEBUG_H
#define RR_DEBUG_H 1

#include <sys/cdefs.h>

#include <castor/runtime.h>

#include "rr_fdprintf.h"

void __rr_assert(const char *, const char *, int, const char *) __dead2;

#define	rr_assert(exp)	do { if (__predict_false(rrMode == RRMODE_REPLAY)) {\
				if (!(exp)) {__rr_assert(__func__, __FILE__, \
				    __LINE__, #exp);\
				}\
			     }\
			} while (0)


#define	rr_printf(...)	do { if (__predict_false(rrMode == RRMODE_REPLAY)) {\
				rr_fdprintf(STDOUT_FILENO, __VA_ARGS__);\
			     }\
			} while(0)
#endif

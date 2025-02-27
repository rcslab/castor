#ifndef RR_DEBUG_H
#define RR_DEBUG_H 1

#include <stdarg.h>
#include <stdbool.h>
#include <threads.h>
#include <sys/cdefs.h>

#include <castor/archconfig.h>
#include <castor/rr_fdprintf.h>

extern enum RRMODE rrMode;

void __rr_assert(const char *, const char *, int, const char *) __dead2;

#define	rr_assert(exp)	do { \
			    if (__predict_false(rrMode != RRMODE_NORMAL)) {\
				if (!(exp)) {__rr_assert(__func__, __FILE__, \
				    __LINE__, #exp);\
				}\
			     }\
			} while (0)

#define	replay_assert(exp)	do { \
				    if (__predict_false(rrMode == RRMODE_REPLAY)) {\
					if (!(exp)) {__rr_assert(__func__, __FILE__, \
					    __LINE__, #exp);\
					}\
				    }\
				} while (0)

#define	rr_printf(...)	do { \
			    if (__predict_false(rrMode == RRMODE_REPLAY)) {\
				rr_fdprintf(STDOUT_FILENO, __VA_ARGS__);\
			    }\
			} while(0)

#define	replay_printf(...)	do { \
				    if (__predict_false(rrMode == RRMODE_REPLAY)) {\
					rr_fdprintf(STDOUT_FILENO, __VA_ARGS__);\
				    }\
				} while(0)

#endif


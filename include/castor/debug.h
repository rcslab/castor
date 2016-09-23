
#ifndef __CASTOR_DEBUG_H__
#define __CASTOR_DEBUG_H__

void Debug_PrintBacktrace();
void Debug_LogBacktrace();
void Debug_Log(int level, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
int Debug_Init(const char *logPath);

#define LEVEL_SYS       0 /* Assert/Panic/Abort/NotImplemented */
#define LEVEL_ERR       1 /* Error */
#define LEVEL_WRN       2 /* Warning */
#define LEVEL_MSG       3 /* Stdout */
#define LEVEL_LOG       4 /* Log */
#define LEVEL_DBG       5 /* Debug */
#define LEVEL_VRB       6 /* Verbose */

#define SYSERROR(fmt, ...) Debug_Log(LEVEL_ERR, fmt "\n", ##__VA_ARGS__)
#define WARNING(fmt, ...) Debug_Log(LEVEL_WRN, fmt "\n", ##__VA_ARGS__)
#define MSG(fmt, ...) Debug_Log(LEVEL_MSG, fmt "\n", ##__VA_ARGS__)
#define LOG(fmt, ...) Debug_Log(LEVEL_LOG, fmt "\n", ##__VA_ARGS__)

/*
 * Only DEBUG builds compile in DLOG messages
 */
#ifdef CASTOR_DEBUG
#define DLOG(fmt, ...) Debug_Log(LEVEL_DBG, fmt "\n", ##__VA_ARGS__)
#else
#define DLOG(fmt, ...)
#endif

#ifdef CASTOR_DEBUG
#define ASSERT(_x) \
    if (!(_x)) { \
        Debug_Log(LEVEL_SYS, "ASSERT("#_x"): %s %s:%d\n", \
                __FUNCTION__, __FILE__, __LINE__); \
        assert(_x); \
    }
#else
#define ASSERT(_x)
#endif

#define PANIC() { Debug_Log(LEVEL_SYS, "PANIC: " \
                         "function %s, file %s, line %d\n", \
                         __func__, __FILE__, __LINE__); abort(); }
#define NOT_IMPLEMENTED(_x) if (!(_x)) { Debug_Log(LEVEL_SYS, \
                                "NOT_IMPLEMENTED(" #_x "): %s %s:%d\n", \
                                __func__, __FILE__, __LINE__); abort(); }


#endif /* __CASTOR_DEBUG_H__ */


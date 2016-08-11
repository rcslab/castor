
#include <sys/types.h>
#include <sys/sysctl.h>

static inline uint64_t TSCDiffToMS(uint64_t diff)
{
    const char *tsc = "machdep.tsc_freq";
    uint64_t tscfreq;
    size_t len = 8;

    if (sysctlbyname(tsc, &tscfreq, &len, NULL, 0) < 0) {
	perror("sysctl");
	exit(1);
    }

    return 1000 * diff / tscfreq;
}

static inline uint64_t TSCDiffToUS(uint64_t diff)
{
    const char *tsc = "machdep.tsc_freq";
    uint64_t tscfreq;
    size_t len = 8;

    if (sysctlbyname(tsc, &tscfreq, &len, NULL, 0) < 0) {
	perror("sysctl");
	exit(1);
    }

    return 1000 * 1000 * diff / tscfreq;
}



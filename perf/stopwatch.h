
#include <stdatomic.h>
#include <sys/types.h>
#include <sys/sysctl.h>

#ifdef __aarch64__
#include <arm_acle.h>
#endif
//#include <x86intrin.h>

static uint64_t Stopwatch_Start(void)
{
    sleep(0);
#ifdef __amd64__
    return __rdtsc();
#elif defined(__aarch64__)
    uint64_t ui;
    __isb(15);
    __asm__ volatile("mrs %0, CNTVCT_EL0" : "=&r" (ui));
    atomic_signal_fence(memory_order_seq_cst);
    return ui;
#elif defined(__powerpc64__)
    uint64_t ui;
    __asm__ volatile("mfspr %0, 268" : "=r" (ui));
    atomic_signal_fence(memory_order_seq_cst);
    return ui;
#else
    return __builtin_readcyclecounter();
#endif
}

static uint64_t Stopwatch_Stop(void)
{
#if defined(__amd64__)
    unsigned int ui;
    return __builtin_ia32_rdtscp(&ui);
#elif defined(__aarch64__)
    uint64_t ui;
    atomic_signal_fence(memory_order_seq_cst);
    __isb(15);
    atomic_signal_fence(memory_order_seq_cst);
    __asm__ volatile("mrs %0, CNTVCT_EL0" : "=&r" (ui));
    return ui;
#elif defined(__powerpc64__)
    uint64_t ui;
    atomic_signal_fence(memory_order_seq_cst);
    __asm__ volatile("mfspr %0, 268" : "=r" (ui));
    atomic_signal_fence(memory_order_seq_cst);
    return ui;
#else
    return __builtin_readcyclecounter();
#endif
}

#if 0
static uint64_t Stopwatch_ToNS(uint64_t diff)
{
#if defined(__amd64__)
    const char *tsc = "machdep.tsc_freq";
    uint64_t tscfreq;
    size_t len = 8;

    if (sysctlbyname(tsc, &tscfreq, &len, NULL, 0) < 0) {
	perror("sysctl");
	exit(1);
    }

    return 1000000000ULL * diff / tscfreq;
#elif defined(__aarch64__)
    uint64_t cntfreq;
    __asm__ volatile("mrs %0, CNTFRQ_EL0" : "=&r" (cntfreq));
    return 1000000000ULL * diff / cntfreq;
#elif defined(__powerpc64__)
    return 1000000000ULL * diff / 512000000ULL;
#else
    return diff;
#endif
}
#endif

static uint64_t Stopwatch_ToMS(uint64_t diff)
{
#if defined(__amd64__)
    const char *tsc = "machdep.tsc_freq";
    uint64_t tscfreq;
    size_t len = 8;

    if (sysctlbyname(tsc, &tscfreq, &len, NULL, 0) < 0) {
	perror("sysctl");
	exit(1);
    }

    return 1000 * diff / tscfreq;
#elif
#error Unsupported
#endif
}

static void Stopwatch_Print(uint64_t start, uint64_t stop, uint64_t events)
{
    uint64_t baselineStart = Stopwatch_Start();
    uint64_t baselineStop = Stopwatch_Stop();
    uint64_t baseline = baselineStop - baselineStart;
    uint64_t diff = stop - start;
    uint64_t ms = Stopwatch_ToMS(diff - baseline);

    printf("Time: %lu Cycles, Baseline: %lu Cycles\n", diff - baseline, baseline);
    printf("Time: %lu ms\n", ms);
    printf("%lu events/second\n", events * 1000UL / ms);
}


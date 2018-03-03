
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

uint64_t readtsc(void)
{
    uint64_t ts;

    ts = __builtin_readcyclecounter();

    return ts;
}

int main(int argc, const char *argv[])
{
    printf("%016"PRIu64"\n", readtsc());
    return 0;
}



#include <stdint.h>
#include <assert.h>

int load(volatile int *addr)
{
    return *addr;
}

void store(volatile int *addr, int val)
{
    *addr = val;
}

int main(int argc, const char *argv[])
{
    volatile int a;

    store(&a, 5);
    assert(load(&a) == 5);

    return 0;
}


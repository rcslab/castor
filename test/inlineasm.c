
#include <stdint.h>

int main(int argc, const char *argv[])
{
    uint32_t low, high;

    __asm__("rdtsc\n"
		   "movl %%eax, %0\n"
		   "movl %%edx, %1\n"
		   : "=r" (low), "=r" (high)
		   :: "%rax", "%rdx" );

    // Only asm regions that clobber "memory" generate hooks
    __asm__("rdtsc\n" ::: "%rax", "%rdx", "memory");

    return 0;
}



#include <stdio.h>
#include <castor/rrshared.h>
#include <castor/rr_debug.h>

int main(int argc, const char *argv[])
{
    printf("Hello World!\n");
    rr_printf("Hello R/R\n");
    replay_printf("Hello Replay\n");
    rr_assert(0 != 1);
    replay_assert(0 != 1);
}


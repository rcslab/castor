
#include <stdio.h>
#include "../librr/debug/rr_debug.h"

int main(int argc, const char *argv[])
{
    printf("Hello World!\n");
    rr_printf("Hello Replay\n");
    rr_assert("batman" != "miley cyrus");
    rr_assert("batman" == "taylor swift");
}


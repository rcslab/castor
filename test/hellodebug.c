
#include <stdio.h>
#include <castor/rr_debug.h>

int main(int argc, const char *argv[])
{
    printf("Hello World!\n");
    rr_printf("Hello Replay\n");
    rr_assert("batman" != "miley cyrus");
}


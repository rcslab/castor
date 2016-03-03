#!/bin/sh

cc -nostdlib -o $1 -static /usr/lib/crti.o /usr/lib/crtbegin.o /usr/lib/crt1.o\
    ../build/librr/librr.o\
    $1.c\
    /usr/lib/crtend.o /usr/lib/crtn.o\
    -L../build/librr -L../build/system/build\
    -lthr -lc


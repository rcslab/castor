#!/bin/sh

cc -nostdlib -g -o $1 -static /usr/lib/crti.o /usr/lib/crtbegin.o /usr/lib/crt1.o\
    ../build/librr/librr.o\
    $1.c\
    /usr/lib/crtend.o /usr/lib/crtn.o\
    -L../build/librr -L../system/build\
    -lthr -lc


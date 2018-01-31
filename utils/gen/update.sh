#!/bin/sh
set -x
./gen_sal.py < syscalls.annotated
cp -f events_gen.c ../../lib/Runtime
cp -f events_gen.h ../../include/castor

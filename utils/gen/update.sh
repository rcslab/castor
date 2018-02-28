#!/bin/sh
set -x
./gen_sal.py < syscalls.master
cp -f events_gen.c ../../lib/Runtime
cp -f events_gen.h ../../include/castor

#!/bin/sh
set -x
./gen_sal.py < syscalls.master
cp -f events_gen.c ../../lib/Runtime
cp -f events_gen.h ../../include/castor
cp -f events_pretty_printer_gen.[ch] ../../tools/rrlog
cp -f castor_xlat.[ch] ../../tools/rrlog

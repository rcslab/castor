#!/bin/sh
(grep '__strong_reference(__rr_[a-z]' ../../lib/Runtime -r --exclude='*_gen.c' | cut -d',' -f2 | cut -d')' -f1 | sed 's/^[ _]*//'; grep 'BIND_REF' ../../lib/Runtime -r --exclude='*_gen.c' | grep -v '#define' | cut -d'(' -f2 | cut -d')' -f1) | sort | uniq

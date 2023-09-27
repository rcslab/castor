#!/bin/sh

if [ -f "lib/libc/sys/fstatat.c" ]; then
    patch -u -b lib/libc/sys/fstatat.c -i "$(dirname "$0")/libc/fstatat.c.diff"
else
    echo "lib/libc/sys/fstatat.c does not exist. Aborted."
fi

if [ -f "lib/libc/sys/getdents.c" ]; then
    patch -u -b lib/libc/sys/getdents.c -i "$(dirname "$0")/libc/getdents.c.diff"
else
    echo "lib/libc/sys/getdents.c does not exist. Aborted."
fi

if [ -f "lib/libc/sys/lstat.c" ]; then
    patch -u -b lib/libc/sys/lstat.c -i "$(dirname "$0")/libc/lstat.c.diff"
else
    echo "lib/libc/sys/lstat.c does not exist. Aborted."
fi

if [ -f "lib/libc/sys/stat.c" ]; then
    patch -u -b lib/libc/sys/stat.c -i "$(dirname "$0")/libc/stat.c.diff"
else
    echo "lib/libc/sys/stat.c does not exist. Aborted."
fi

if [ -f "lib/libthr/pthread.map" ]; then
    patch -u -b lib/libthr/pthread.map -i "$(dirname "$0")/libthr/pthread.map.diff"
else
    echo "lib/libthr/pthread.map does not exist. Aborted."
fi

if [ -f "lib/libthr/thread/thr_mutex.c" ]; then
    patch -u -b lib/libthr/thread/thr_mutex.c -i "$(dirname "$0")/libthr/thr_mutex.c.diff"
else
    echo "lib/libthr/thread/thr_mutex.c does not exist. Aborted."
fi


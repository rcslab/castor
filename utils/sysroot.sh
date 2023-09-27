#!/bin/csh -e

set BASE=`pwd`

echo "Name for Castor jail?"
set JAIL_NAME = $<

if ( ! -d $BASE/sysroot-src ) then
    echo "Requires FreeBSD source code to be installed."
    exit 1
endif 

set NCPU=`/sbin/sysctl -n hw.ncpu`
set NCPU=`expr 2 \* $NCPU`

cd $BASE/sysroot-src/

echo "Patching FreeBSD source.."
if ( -f "lib/libc/sys/fstatat.c" ) then
    patch -u -b lib/libc/sys/fstatat.c -i ../utils/patches/libc/fstatat.c.diff
else
    echo "lib/libc/sys/fstatat.c does not exist. Aborted."
    exit 1
endif

if ( -f "lib/libc/sys/getdents.c" ) then
    patch -u -b lib/libc/sys/getdents.c -i ../utils/patches/libc/getdents.c.diff
else
    echo "lib/libc/sys/getdents.c does not exist. Aborted."
    exit 1
endif

if ( -f "lib/libc/sys/lstat.c" ) then
    patch -u -b lib/libc/sys/lstat.c -i ../utils/patches/libc/lstat.c.diff
else
    echo "lib/libc/sys/lstat.c does not exist. Aborted."
    exit 1
endif

if ( -f "lib/libc/sys/stat.c" ) then
    patch -u -b lib/libc/sys/stat.c -i ../utils/patches/libc/stat.c.diff
else
    echo "lib/libc/sys/stat.c does not exist. Aborted."
    exit 1
endif

if ( -f "lib/libthr/pthread.map" ) then
    patch -u -b lib/libthr/pthread.map -i ../utils/patches/libthr/pthread.map.diff
else
    echo "lib/libthr/pthread.map does not exist. Aborted."
    exit 1
endif

if ( -f "lib/libthr/thread/thr_mutex.c" ) then
    patch -u -b lib/libthr/thread/thr_mutex.c -i ../utils/patches/libthr/thr_mutex.c.diff
else
    echo "lib/libthr/thread/thr_mutex.c does not exist. Aborted."
    exit 1
endif


echo "Building FreeBSD source.."
cd $BASE/sysroot-src
make -j $NCPU buildworld MAKEOBJDIRPREFIX=$BASE/sysroot-obj NO_FSCHG=YES
make installworld DESTDIR=$BASE/sysroot 

#include Castor runtime at loadtime
echo "Patching Castor jail.."
cd $BASE/sysroot
sed -i .backup 's;(;( /usr/lib/libCastorRuntime.a;' usr/lib/libc.so
rm usr/lib/libthr.so
echo "GROUP ( /usr/lib/libCastorThreadRuntime.a /lib/libthr.so.3)" >> usr/lib/libthr.so

echo "SYSROOT Build Complete"


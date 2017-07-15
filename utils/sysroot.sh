#!/bin/csh -e

set BASE=`pwd`

if ( ! -d /usr/src ) then
    echo "Requires FreeBSD source code to be installed."
    exit 1
endif 

set NCPU=`/sbin/sysctl -n hw.ncpu`
set NCPU=`expr 2 \* $NCPU`

if ( ! -d $BASE/sysroot-src ) then
    cp -r /usr/src $BASE/sysroot-src

    cd $BASE/sysroot-src/lib
    patch -p1 < $BASE/utils/patches/libthr.diff
endif

cd $BASE/sysroot-src
make -j $NCPU xdev XDEV=amd64 XDEV_ARCH=amd64 MAKEOBJDIRPREFIX=$BASE/sysroot-obj DESTDIR=$BASE/sysroot
cd ..

#include Castor runtime at loadtime
cd $BASE/sysroot-src
sed -i .backup 's;(;( /usr/lib/libCastorRuntime.a;' usr/amd64-freebsd/usr/lib/libc.so

echo "SYSROOT Build Complete"


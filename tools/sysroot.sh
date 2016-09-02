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
    patch -p1 < $BASE/tools/patches/libthr.diff
endif

cd $BASE/sysroot-src
make -j $NCPU xdev XDEV=amd64 XDEV_ARCH=amd64 MAKEOBJDIRPREFIX=$BASE/sysroot-obj DESTDIR=$BASE/sysroot
cd ..

echo "SYSROOT Build Complete"


#!/bin/csh -e

echo
echo "Castor sysroot installer"
echo
echo "Please enter your sudo password so that the installation will be finished automatically."
echo "Otherwise you will be asked multiple times during the installation."
echo "Please enter password for sudo: " 
stty -echo 
set SUDO_PASSWORD = $<
stty echo
echo

set BASE=`pwd`

if ( ! -d $BASE/sysroot-src ) then
    echo "Requires FreeBSD source code to be installed."
    exit 1
endif 

set NCPU=`/sbin/sysctl -n hw.ncpu`
set NCPU=`expr 2 \* $NCPU`

#include Castor runtime at loadtime
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
make -j $NCPU buildworld NO_FSCHG=YES
echo "Installing FreeBSD into Castor sysroot.."
if ( -z "$SUDO_PASSWORD" ) then
    echo "$SUDO_PASSWORD" | sudo -S  make installworld DESTDIR=$BASE/sysroot 
else
    sudo make installworld DESTDIR=$BASE/sysroot 
endif

#include Castor runtime at loadtime
echo "Patching Freebsd for Castor.."
if ( -z "$SUDO_PASSWORD" ) then
    echo "$SUDO_PASSWORD" | sudo -S sed -i .backup 's;(;( /usr/lib/libCastorRuntime.a;' $BASE/sysroot/usr/lib/libc.so
    if ( -f "$BASE/sysroot/usr/lib/libthr.so" ) then
	echo "$SUDO_PASSWORD" | sudo -S rm $BASE/sysroot/usr/lib/libthr.so
    endif
    echo "$SUDO_PASSWORD" | sudo -S touch $BASE/sysroot/usr/lib/libthr.so
    echo "$SUDO_PASSWORD" | sudo -S chmod g+w $BASE/sysroot/usr/lib/libthr.so 
    echo "$SUDO_PASSWORD" | sudo -S echo "GROUP ( /usr/lib/libCastorThreadRuntime.a /lib/libthr.so.3)" >> $BASE/sysroot/usr/lib/libthr.so
else
    sudo sed -i .backup 's;(;( /usr/lib/libCastorRuntime.a;' $BASE/sysroot/usr/lib/libc.so
    if ( -f "$BASE/sysroot/usr/lib/libthr.so" ) then
	sudo rm $BASE/sysroot/usr/lib/libthr.so
    endif
    sudo touch $BASE/sysroot/usr/lib/libthr.so
    sudo chmod g+w $BASE/sysroot/usr/lib/libthr.so 
    sudo echo "GROUP ( /usr/lib/libCastorThreadRuntime.a /lib/libthr.so.3)" >> $BASE/sysroot/usr/lib/libthr.so
endif

echo "SYSROOT Build Complete"

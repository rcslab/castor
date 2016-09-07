#!/bin/csh -e

set LLVMVER="3.8.1"
set SRC=llvm-$LLVMVER.src

set BASE=`pwd`

if ( -d $BASE/llvm ) then
    echo "Delete the llvm directory first if you are rebuilding the LLVM tree."
    exit 1
endif

cd $BASE/tools
if ( ! -e $SRC.tar.xz ) then
    fetch http://llvm.org/releases/3.8.1/$SRC.tar.xz
endif

tar zxvf $SRC.tar.xz
mv $SRC $BASE/llvm

cd $BASE/llvm
mkdir build
cd build
cmake ../
cmake --build .

echo "LLVM Build Complete"


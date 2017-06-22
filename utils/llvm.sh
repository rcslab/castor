#!/bin/csh -e

set LLVMVER="3.9.1"
set LLVM_SRC=llvm-$LLVMVER.src
set CFE_SRC=cfe-$LLVMVER.src

set BASE=`pwd`

if ( -d $BASE/llvm ) then
    echo "Delete the llvm directory first if you are rebuilding the LLVM tree."
    exit 1
endif

cd $BASE/utils
if ( ! -e $LLVM_SRC.tar.xz ) then
    fetch http://llvm.org/releases/$LLVMVER/$LLVM_SRC.tar.xz
endif
if ( ! -e $CFE_SRC.tar.xz ) then
    fetch http://llvm.org/releases/$LLVMVER/$CFE_SRC.tar.xz
endif

tar zxvf $LLVM_SRC.tar.xz
tar zxvf $CFE_SRC.tar.xz
mv $LLVM_SRC $BASE/llvm
mv $CFE_SRC $BASE/llvm/tools/clang

cd $BASE/llvm
if (! -e build ) then
  mkdir build
endif

if (! -e $BASE/../compiler-rt ) then
  echo "Unable to find compiler-rt repository, not building custom runtime!!!"
else
  ln -s $BASE/../compiler-rt $BASE/llvm/projects/compiler-rt
endif

cd build
cmake -G Ninja -DLLVM_TARGETS_TO_BUILD=X86 ../
cmake --build .

echo "LLVM Build Complete"


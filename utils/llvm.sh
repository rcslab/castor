#!/bin/csh -e

set LLVMVER="15.0.0"
set LLVM_SRC=llvm-$LLVMVER.src
set CFE_SRC=clang-$LLVMVER.src
set CMAKE_MODULE=cmake-$LLVMVER.src

set BASE=`pwd`

set URL_BASE="https://github.com/llvm/llvm-project/releases/download/llvmorg-$LLVMVER"

if ( -d $BASE/llvm ) then
    echo "Delete the llvm directory first if you are rebuilding the LLVM tree."
    exit 1
endif

cd $BASE/utils
if ( ! -e $LLVM_SRC.tar.xz ) then
    fetch $URL_BASE/$LLVM_SRC.tar.xz
endif
if ( ! -e $LLVM_SRC.tar.xz ) then
    fetch $URL_BASE/$LLVM_SRC.tar.xz
endif
if ( ! -e $CFE_SRC.tar.xz ) then
    fetch $URL_BASE/$CFE_SRC.tar.xz
endif
if ( ! -e $CMAKE_MODULE.tar.xz ) then
    fetch $URL_BASE/$CMAKE_MODULE.tar.xz 
endif

tar xvf $LLVM_SRC.tar.xz
tar xvf $CFE_SRC.tar.xz
tar xvf $CMAKE_MODULE.tar.xz
mv $LLVM_SRC $BASE/llvm
mv $CFE_SRC $BASE/llvm/tools/clang
mv $CMAKE_MODULE/Modules/* $BASE/llvm/cmake/modules

cd $BASE/llvm
if (! -e build ) then
  mkdir build
endif

#if (! -e $BASE/../compiler-rt ) then
#  echo "Unable to find compiler-rt repository, not building custom runtime!!!"
#else
#  ln -s $BASE/../compiler-rt $BASE/llvm/projects/compiler-rt
#endif

cd build
cmake -G Ninja -DLLVM_TARGETS_TO_BUILD=X86 -DCMAKE_BUILD_TYPE=Release -DLLVM_INCLUDE_BENCHMARKS=OFF ../
cmake --build .

echo "LLVM Build Complete"


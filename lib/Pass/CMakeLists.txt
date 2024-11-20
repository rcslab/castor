cmake_minimum_required(VERSION 3.5)
project(CastorPass CXX)

if (EXISTS "../../llvm/build")
list(APPEND CMAKE_PREFIX_PATH "../../llvm/build")
else()
list(APPEND CMAKE_PREFIX_PATH "/usr/local/llvm15")
endif()

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_library(CastorPass MODULE CastorPass.cc)

set_property(TARGET CastorPass PROPERTY CXX_STANDARD 14)


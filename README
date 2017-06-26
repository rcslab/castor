
***** DEPENDENCIES *****

Requirements:
 - FreeBSD 11+
 - Packages: git, python, scons, llvm39

1. Install necessary dependencies

# pkg install git python scons llvm39 ninja cmake

NOTE: If clang39 --version does not match 3.9.1 on your system then you need to 
update util/llvm.sh to match your version before proceeding.

***** BUILD *****

1. First initialize Local.sc with sane defaults:

Recommended settings for Local.sc:
    CC="clang39"
    CXX="clang++39"
    LINK="clang39"
    CLANGTIDY="clang-tidy39"
    PREFIX="/home/<USERNAME>/castor"

2. Create a sysroot with our patches by running

    # scons sysroot

3. Build the compiler

    # scons llvm

4. Build the system and associated tests

    # scons

5. Run the testsuite

    # scons testbench

NOTE: Due to a few bugs a few tests may spuriously fail.  Use 'ipcrm -W' to 
clear out any System V shared memory segments.

6. Install into a path

    # scons install


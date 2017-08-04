# Castor: a low overhead application record/replay system.

##Requirements

Castor is only supported on FreeBSD 11+ at this time, and has only been tested 
with clang 3.9.1. 

##Building Castor

Following these steps will allow you to build Castor and run the test suite.

### 1. Install the packages needed to build Castor

```
pkg install git python scons llvm39 ninja cmake
```

Note: If ```clang39 --version``` does not match 3.9.1 on your system and you still wish to proceed, then
you need to update util/llvm.sh to match your version before proceeding.


### 2. Create a sysroot with our patches by running

```
scons sysroot
```

### 3. Build the compiler

```
scons llvm
```

### 4. Build the system and associated tests

```
scons
```

### 5. Run the testsuite

```
scons testbench
```

Note: Sometimes bugs in the test suite cause certain tests to fail, which
can result in leaked SysV shared memory segments, the script in ./utils/cleanup.sh
will free up sys V shared memory, and kill any lingering test processes.

## Installing Castor

You will want to have easy access to the most common Castor commands: *record*,
*replay*, and *rrlog*, which live under the build directory. I suggest putting 
symlinks to these somewhere in your path.

```
mkdir ~/bin
ln -s $PWD/build/tools/record/record ~/bin/record
ln -s $PWD/build/tools/replay/replay ~/bin/replay
ln -s $PWD/build/tools/rrlog/rrlog ~/bin/rrlog

```


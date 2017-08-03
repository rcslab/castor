# Castor: a low overhead record/replay system for applications

##Requirements

Castor is only supported on FreeBSD 11+ at this time, and has only been tested 
with clang 3.9.1. 

##Building Castor

### 1. Install the packages needed to build Castor

```
pkg install git python scons llvm39 ninja cmake
```

Note: If ```clang39 --version``` does not match 3.9.1 on your system and you still wish to proceed, then
you need to update util/llvm.sh to match your version before proceeding.



### 2. Configure the build system

Initialize Local.sc with sane defaults:

Recommended settings for Local.sc:

```
	CC="clang39"
	CXX="clang++39"
	LINK="clang39"
	CLANGTIDY="clang-tidy39"
	PREFIX="/home/<USERNAME>/castor"
```

### 3. Create a sysroot with our patches by running

    scons sysroot

### 4. Build the compiler

```
scons llvm
```

### 5. Build the system and associated tests

```
scons
```

### 6. Run the testsuite

```
scons testbench
```

Note: Sometimes bugs in the test suite cause certain tests to fail, which
can result in leaked SysV shared memory segments, the script in ./utils/cleanup.sh
will free up sys V shared memory, and kill any lingering test processes.

## Installing Castor


Install into a path

    # scons install


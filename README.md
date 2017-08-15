# Castor: a low overhead application record/replay system.

##Requirements

Castor is only supported on FreeBSD 11+ at this time, and has only been tested 
with Clang 4.0. 

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

Put symlinks to the common Castor commands somewhwere in your path, for example.

```
ln -s $PWD/build/tools/record/record ~/bin/record
ln -s $PWD/build/tools/replay/replay ~/bin/replay
ln -s $PWD/build/tools/rrlog/rrlog ~/bin/rrlog

```

## Integration Tests

There are a set of test applications in the apps repository, check this out
into the castor root directory and follow the instructions in the README.

## Using Castor

To record:

```
    record command
```

To replay

```
    replay command
```

both take a -o flag, e.g. 

    record -o awesome-log-file.rr ./ls

to specify the name of the log file, if none is given, the logfile is assumed to have the
name default.rr.

To work properly, the application must have been built using our version of
clang, and linked with with the appropriate version of libc, libthread, etc.

To see working examples of how to build an app with our current setup, check out
the examples in the apps directory.

## Debugging Tips

The most common form of bug we encounter with Castor is a divergence, which
occurs when execution differs between record and replay, usually become some
source of non-determinism has not been correctly logged, often due to
incorrectly logging a system call. The easiest way to spot such problems is to
pull up from truss on record and replay side by side using a tool like vimdiff.


To examine the contents of the log file for debugging Castor we can use, the rrlog
command e.g.

    rrlog default.rr


## Build Tips
To see the different build options:

    scons -h

To clean up:

    scons -c

To create a file to set your own local build defaults.

    echo > Local.sc

To create a release build, set the following lines in Local.sc

    BUILDTYPE="RELEASE"
    RR="tsx"





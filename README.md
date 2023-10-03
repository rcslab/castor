# Castor: a low overhead application record/replay system.

##Requirements

Castor only supports FreeBSD 13.1, and is tested with Clang 15.

For development, you should use a sysroot, chroot or jail environment.  

##Building Castor

Following these steps will allow you to build Castor and run the test suite.

### 1. Install the packages needed to build Castor

```
pkg install git python scons-py39 llvm15 sudo cmake curl
```

If ```clang15 --version``` does not match 15.0.x on your system and you still 
wish to proceed, then you need to update util/llvm.sh to ignore our version 
check.


### 2. Prepare the OS environment

You may either use the sysroot or jail method to get your environment running.

#### 2.1 Sysroot (Method 1)
Create a sysroot with our patches by running
```
scons sysroot
```

After building the sysroot, set the sysroot or jail to use the FreeBSD build in
<Castor_Root_Dir>/sysroot/

You can use chroot or a FreeBSD jail to run inside the sysroot environment. 
Please see the man pages or FreeBSD handbook for details on how to set that up.

### 3. Build the compiler (Skip this step if you have llvm15 installed)

```
scons llvm
```

### 4. Build the system and associated tests

```
scons
```

### 5. Fix up the linker scripts (jail method only)
After the compliation, insert the Castor library path into the the system's 
libc.so linker script:
```
vim /usr/lib/libc.so
```
Insert the fullpath of libsys_castor.so into th group as shown below.
```
...
GROUP ( <path_to_libsys_castor.a> ..other libc so.. )
...
```

You also need to delete /usr/lib/libthr.so symbol link and replace it with a 
group file with the following contents.
```
GROUP ( <path_to_libCastorThreadRuntime.a> /lib/libthr.so.3 )
```

### 5. Run the testsuite

You may need to rebuild the testsuite after updating the linker script above 
using scons -c to clean the build and scons to rebuild everything.

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

## Matching Our Paper's Results

Our paper's measurements were taken using a release build with the TSX 
record/replay mode.  If you do not have Intel TSX support in your CPU, then the 
TSC option is closest to our benchmark.  It will only differ for workloads that 
make significant use of atomic instructions, which is not the case for any 
benchmark we ran.  We still need to merge our branch that supports replaying 
TSX/TSC modes by rewriting the timestamps into a sequence on replay.

CTR mode is the default for development, which uses a global counter that 
serializes all events.

Local.sc contents:
```
BUILDTYPE="RELEASE"
RR="tsx" # or "tsc" if you don't have Intel TSX
```


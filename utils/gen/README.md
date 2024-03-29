##The Autogenerator

Most of castor's system call handlers, i.e. that record system call results are auto generated
using the code in this directory.

This must be run manually instead of being run at build time. This is nice as it allows changes
to autogenerated code to be visible in commits and generally makes life simpler.

Its inputs are:
    autogenerate_includes -- stuff to include in autogenerated code.
    autogenerate_syscalls -- list of calls to generate.
    syscalls.annotated -- list of SAL annotations for system calls.

Invoke it as follows: 
```
./gen_sal.py < syscalls.annotated
```
Invoke like this for generous debugging output: 
```
./gen_sal.py -v < syscalls.annotated
```

To install updated handlers: 
```
./update.sh
```

This regenerates the handlers and copies:
    events_gen.c to lib/Runtime/events_gen.c
    events.h to include/castor/events_gen.h


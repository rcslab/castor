#!/usr/bin/env python

import sys
import os
import time
import subprocess

TIMEOUT = 2.0

CLEAR = "\r\033[K"
RED = "\033[0;31m"
GREEN = "\033[0;32m"
NORMAL = "\033[0;39m"

FORMAT = "%-32s [ %s%-9s"+ NORMAL + " ]"

tests = [ "helloworld", "read", "rand", "time" ]
failed = [ ]

def write(str):
    sys.stdout.write(str)
    sys.stdout.flush()

def CleanTest(name):
    try:
        os.unlink(name)
    except OSError:
        pass
    try:
        os.unlink(name + ".record")
    except OSError:
        pass
    try:
        os.unlink(name + ".replay")
    except OSError:
        pass
    try:
        os.unlink(name + ".rr")
    except OSError:
        pass

def BuildTest(name):
    write(FORMAT % (name, NORMAL, "Building"))
    t = subprocess.Popen(["./build.sh", name],
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    t.wait()
    if t.returncode != 0:
        write(CLEAR)
        write(FORMAT % (name, RED, "Build Fail") + "\n")
        print "***** BEGIN COMPILER OUTPUT *****"
        print t.communicate()
        print "***** END COMPILER OUTPUT *****"
        sys.exit(1)
    write(CLEAR)

def ReportError(name):
    write(CLEAR)
    write(FORMAT % (name, RED, "Failed") + "\n")
    failed.append(name)

def ReportTimeout(name):
    write(CLEAR)
    write(FORMAT % (name, RED, "Timeout") + "\n")
    failed.append(name)

def RunTest(name):
    write(FORMAT % (name, NORMAL, "Running"))

    # Record
    outputRecord = open(name + ".record", "w+")
    t = subprocess.Popen(["./" + name],
                         stdout=outputRecord,
                         stderr=outputRecord,
                         env = { "CASTOR_MODE":"RECORD", "CASTOR_LOGFILE":name+".rr"})
    start = time.time()
    while 1:
        t.poll()
        if t.returncode == 0:
            break
        if t.returncode != None:
            ReportError(name)
            return
        if (time.time() - start) > TIMEOUT:
            t.kill()
            ReportTimeout(name)
            return

    # Replay
    outputReplay = open(name + ".replay", "w+")
    t = subprocess.Popen(["./" + name],
                         stdout=outputReplay,
                         stderr=outputReplay,
                         env = { "CASTOR_MODE":"REPLAY", "CASTOR_LOGFILE":name+".rr"})
    start = time.time()
    while 1:
        t.poll()
        if t.returncode == 0:
            break
        if t.returncode != None:
            ReportError(name)
            return
        if (time.time() - start) > TIMEOUT:
            t.kill()
            ReportTimeout(name)
            return

    write(CLEAR)
    write(FORMAT % (name, GREEN, "Completed") + "\n")

basedir = os.getcwd()
if (basedir.split('/')[-1] != 'test'):
    os.chdir('test')

write("%-32s   %s\n" % ("Test", "Status"))
write("----------------------------------------------\n")
for t in tests:
    CleanTest(t)
    BuildTest(t)
    RunTest(t)

if len(failed) != 0:
    print str(len(failed)) + " tests failed"

sys.exit(len(failed))


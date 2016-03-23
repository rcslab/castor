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
TFORMAT = "%-32s [ %s%-9s"+ NORMAL + " ] %-10.6f %-10.6f %-10.6f"

tests = [ "helloworld", "read", "rand", "time", "thread_basic", "thread_mutex",
        "thread_print" ]
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

    # Normal
    outputNormal = open(name + ".normal", "w+")
    start = time.time()
    t = subprocess.Popen(["./" + name],
                         stdout=outputNormal,
                         stderr=outputNormal,
                         env = {})
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
    norm_time = time.time() - start

    # Record
    outputRecord = open(name + ".record", "w+")
    start = time.time()
    t = subprocess.Popen(["./" + name],
                         stdout=outputRecord,
                         stderr=outputRecord,
                         env = { "CASTOR_MODE":"RECORD", "CASTOR_LOGFILE":name+".rr"})
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
    rec_time = time.time() - start

    # Replay
    outputReplay = open(name + ".replay", "w+")
    start = time.time()
    t = subprocess.Popen(["./" + name],
                         stdout=outputReplay,
                         stderr=outputReplay,
                         env = { "CASTOR_MODE":"REPLAY", "CASTOR_LOGFILE":name+".rr"})
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
    rep_time = time.time() - start

    write(CLEAR)
    write(TFORMAT % (name, GREEN, "Completed", norm_time, rec_time, rep_time))
    write("\n")

basedir = os.getcwd()
if (basedir.split('/')[-1] != 'test'):
    os.chdir('test')

write("%-32s   %-9s   %-10s %-10s %-10s\n" %
        ("Test", "Status", "Normal", "Record", "Replay"))
write("------------------------------------------------------------------------------\n")
for t in tests:
    CleanTest(t)
    BuildTest(t)
    RunTest(t)

if len(failed) != 0:
    print str(len(failed)) + " tests failed"

sys.exit(len(failed))


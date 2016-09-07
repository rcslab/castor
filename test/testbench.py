#!/usr/bin/env python

import sys
import os
import time
import subprocess

TIMEOUT = 3.0

CLEAR = "\r\033[K"
RED = "\033[0;31m"
GREEN = "\033[0;32m"
NORMAL = "\033[0;39m"

FORMAT = "%-32s [ %s%-9s"+ NORMAL + " ]"
TFORMAT = "%-32s [ %s%-9s"+ NORMAL + " ] %-10.6f %-10.6f %-10.6f"

recordtool = "../build/tools/record/record"
replaytool = "../build/tools/replay/replay"

all_tests = ["getuid", "setuid", "helloworld","hellodebug", "read", "rand", "time", 
        "thread_basic", "thread_mutex", "thread_print", "network_basic",
        "fork_basic", "kqueue" ]
tests = [ ]
failed = [ ]

def write(str):
    sys.stdout.write(str)
    sys.stdout.flush()

def DeleteFile(name):
    try:
        os.unlink(name)
    except OSError:
        pass

def CleanTest(name):
    DeleteFile(name)
    DeleteFile(name + ".normal")
    DeleteFile(name + ".record")
    DeleteFile(name + ".replay")
    DeleteFile(name + ".rr")
    DeleteFile(name + ".core")

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

def Run(tool, name, output):
    outfile = open(output, "w+")
    start = time.time()
    t = subprocess.Popen(tool + ["./" + name],
                         stdout=outfile,
                         stderr=outfile)
    while 1:
        t.poll()
        if t.returncode == 0:
            return (time.time() - start)
        if t.returncode != None:
            ReportError(name)
            return None
        if (time.time() - start) > TIMEOUT:
            t.kill()
            ReportTimeout(name)
            return None

def RunTest(name):
    write(FORMAT % (name, NORMAL, "Running"))

    pname = "../build/test/" + name

    # Normal
    norm_time = Run([], pname, name + ".normal")
    if norm_time is None:
        return

    # Record
    rec_time = Run([recordtool, "-o", name + ".rr"], pname, name + ".record")
    if rec_time is None:
        return

    # Replay
    rep_time = Run([replaytool, "-o", name + ".rr"], pname, name + ".replay")
    if rep_time is None:
        return


    write(CLEAR)
    write(TFORMAT % (name, GREEN, "Completed", norm_time, rec_time, rep_time))
    write("\n")

basedir = os.getcwd()
if (basedir.split('/')[-1] != 'test'):
    os.chdir('test')

if len(sys.argv) > 1:
    for t in sys.argv[1:]:
        if all_tests.count(t) == 0:
            print "Test '%s' does not exist!" % (t)
            sys.exit(255)
        tests.append(t)
else:
    tests = all_tests

write("%-32s   %-9s   %-10s %-10s %-10s\n" %
        ("Test", "Status", "Normal", "Record", "Replay"))
write("------------------------------------------------------------------------------\n")
for t in tests:
    CleanTest(t)
    #BuildTest(t)
    RunTest(t)

if len(failed) != 0:
    print str(len(failed)) + " tests failed"

sys.exit(len(failed))


#!/usr/bin/env python

import sys
import os
import time
import subprocess

TIMEOUT = 60.0

CLEAR = "\r\033[K"
RED = "\033[0;31m"
GREEN = "\033[0;32m"
NORMAL = "\033[0;39m"

FORMAT = "%-32s [ %s%-9s"+ NORMAL + " ]"
RRFORMAT = "%-32s [ %s%-9s"+ NORMAL + " ] %-10.6f %-10.6f %-10.6f"
TFORMAT = "%-32s [ %s%-9s"+ NORMAL + " ] %-10.6f %-10s %-10s"

recordtool = "../build/tools/record/record"
replaytool = "../build/tools/replay/replay"

rr_tests = [ ]
tests = [ ]
for f in sorted(os.listdir('perf/')):
    if f.endswith(".c"):
        if f.startswith("rr_"):
            rr_tests.append(os.path.basename(f[0:-2]))
        else:
            tests.append(os.path.basename(f[0:-2]))

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
    DeleteFile(name + ".normal")
    DeleteFile(name + ".record")
    DeleteFile(name + ".replay")
    DeleteFile(name + ".rr")
    DeleteFile(name + ".core")

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

def RunRRTest(name):
    write(FORMAT % (name, NORMAL, "Running"))

    pname = "../build/perf/" + name

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
    write(RRFORMAT % (name, GREEN, "Completed", norm_time, rec_time, rep_time))
    write("\n")

def RunTest(name):
    write(FORMAT % (name, NORMAL, "Running"))

    pname = "../build/perf/" + name

    # Normal
    norm_time = Run([], pname, name + ".normal")
    if norm_time is None:
        return

    write(CLEAR)
    write(TFORMAT % (name, GREEN, "Completed", norm_time, "-", "-"))
    write("\n")

basedir = os.getcwd()
if (basedir.split('/')[-1] != 'perf'):
    os.chdir('perf')

#if len(sys.argv) > 1:
#    for t in sys.argv[1:]:
#        if all_tests.count(t) == 0:
#            print "Test '%s' does not exist!" % (t)
#            sys.exit(255)
#        tests.append(t)
#else:
#    tests = all_tests

write("%-32s   %-9s   %-10s %-10s %-10s\n" %
        ("Perf Test", "Status", "Normal", "Record", "Replay"))
write("------------------------------------------------------------------------------\n")
for t in rr_tests:
    CleanTest(t)
    RunRRTest(t)
for t in tests:
    CleanTest(t)
    RunTest(t)

if len(failed) != 0:
    print(str(len(failed)) + " tests failed")

sys.exit(len(failed))


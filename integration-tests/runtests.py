#!/usr/bin/env python3.6

import sys
import os
import subprocess
import time

CLEAR = "\r\033[K"
RED = "\033[0;31m"
GREEN = "\033[0;32m"
NORMAL = "\033[0;39m"

FORMAT = "%-32s [ %s%-9s"+ NORMAL + " ]"
RRFORMAT = "%-32s [ %s%-9s"+ NORMAL + " ] %-10.6f %-10.6f %-10.6f %-10.6f"
TFORMAT = "%-32s [ %s%-9s"+ NORMAL + " ] %-10.6f %-10s %-10s"

TESTDIR = 'tests'
TESTS = sorted(os.listdir(TESTDIR))
failed = [ ]
disabled = [ ]

TIMEOUT = 2.0

def Write(str):
    sys.stdout.write(str)
    sys.stdout.flush()

def ReportError(name):
    Write(CLEAR)
    Write(FORMAT % (name, RED, "Failed") + "\n")
    failed.append(name)

def ReportTimeout(name):
    Write(CLEAR)
    Write(FORMAT % (name, RED, "Timeout") + "\n")
    failed.append(name)

def ReportTimeout(name):
    Write(CLEAR)
    Write(FORMAT % (name, RED, "Timeout") + "\n")
    failed.append(name)

def ReportDisabled(name):
    Write(CLEAR)
    Write(FORMAT % (name, RED, "Disabled") + "\n")
    disabled.append(name)


def ReportNotFound(name):
    Write(CLEAR)
    Write(FORMAT % (name, RED, "Not Found") + "\n")
    failed.append(name)

def DeleteFile(name):
    try:
        os.unlink(name)
    except OSError:
        pass

def CleanTest(name):
    DeleteFile(name + ".normal")
    DeleteFile(name + ".record")
    DeleteFile(name + ".replay")
    DeleteFile(name + ".core")
    if os.path.isfile('./clean.sh'):
        subprocess.call('./clean.sh')

def Run(test, name, outfile):
    output = open(outfile, "w+")
    start = time.time()
    success = True

    if not os.path.isfile(name):
        ReportNotFound(test + " : " + name)
        return None

    t = subprocess.Popen(["./" + name],
                         stdout=output,
                         stderr=output)
    while 1:
        t.poll()
        if t.returncode == 0:
            break
        if t.returncode != None:
            ReportError(test + " : " + name + " [" + str(t.returncode) + "]")
            success = False
            break
        if (time.time() - start) > TIMEOUT:
            t.kill()
            ReportTimeout(test + " : " + name)
            break

    return [success, (time.time() - start)]

def PrintHeader():
    Write("%-32s   %-9s   %-10s %-10s %-10s %-10s\n" %
            ("Test", "Status", "Normal", "Record", "Replay", "Validate"))
    Write("---------------------------------------------------------------------------------------------\n")

def RunTest(name):
    Write(FORMAT % (name, NORMAL, "Running"))

    # Normal
    success, norm_time = Run(name, 'test-normal.sh', "normal.out")
    if not success:
        return

    # Record
    success, rec_time = Run(name, 'test-record.sh', "record.out")
    if not success:
        return

    # Replay
    success, rep_time = Run(name, 'test-replay.sh', "replay.out")
    if not success:
        return

    # Validate
    success, val_time = Run(name, 'test-validate.sh', "validate.out")
    if not success:
        return

    Write(CLEAR)
    Write(RRFORMAT % (name, GREEN, "Completed", norm_time, rec_time, rep_time, val_time))
    Write("\n")

def read_disabled_list():
    with open('DISABLED') as f:
                disabled_tests = f.read().splitlines()
    autogenerate_list = map(lambda x: x.strip(), disabled_tests)
    autogenerate_list = filter(lambda x:not x.startswith(';'), disabled_tests)
    return disabled_tests

def Cleanup():
    subprocess.call(['ipcrm','-W'])
    subprocess.getoutput('killall -9 -m record replay')

def RunTests():
    disabled_list = read_disabled_list()
    for t in TESTS:
        if t in disabled_list:
            ReportDisabled(t)
            continue
        if os.path.isfile(t):
            continue
        home = os.getcwd()
        tdir = os.path.join(TESTDIR, t)
        os.chdir(tdir)
        CleanTest(t)
        RunTest(t)
        os.chdir(home)
        Cleanup()
    print( str(len(disabled)) + " tests disabled")
    print( str(len(failed)) + " tests failed")

PrintHeader()
RunTests()


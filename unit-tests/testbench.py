#!/usr/bin/env python

import sys
import os
import signal
import time
import subprocess

TIMEOUT = 3.0

CLEAR = "\r\033[K"
RED = "\033[0;31m"
GREEN = "\033[0;32m"
NORMAL = "\033[0;39m"
YELLOW = "\033[0;33m"

FORMAT = "%-32s [ %s%-9s"+ NORMAL + " ]"
TFORMAT = "%-32s [ %s%-9s"+ NORMAL + " ] %-10.6f %-10.6f %-10.6f"

recordtool = "../build/tools/record/record"
replaytool = "../build/tools/replay/replay"
os.environ["CASTORDIR"] = "../build/lib/Runtime"

# Check ASLR
aslrcheck = subprocess.run(["/sbin/sysctl", "-n", "kern.elf64.aslr.enable"], capture_output=True)
if (aslrcheck.returncode == 0) and (aslrcheck.stdout != b"0\n"):
    print("ASLR is enabled!")
    sys.exit(1)

print(aslrcheck)

all_tests = []
for f in sorted(os.listdir('unit-tests/')):
    if f.endswith(".c"):
        all_tests.append(os.path.basename(f[0:-2]))

tests = [ ]
failed = [ ]
disabled = [ ]

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

def BuildTest(name):
    write(FORMAT % (name, NORMAL, "Building"))
    t = subprocess.Popen(["./build.sh", name],
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    t.wait()
    if t.returncode != 0:
        write(CLEAR)
        write(FORMAT % (name, RED, "Build Fail") + "\n")
        print("***** BEGIN COMPILER OUTPUT *****")
        print(t.communicate())
        print("***** END COMPILER OUTPUT *****")
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

def ReportDisabled(name):
    write(CLEAR)
    write(FORMAT % (name, YELLOW, "Disabled") + "\n")
    disabled.append(name)

def Run(tool, sname, name, output):
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
            ReportError(sname)
            return None
        if (time.time() - start) > TIMEOUT:
            t.kill()
            ReportTimeout(sname)
            return None

def read_disabled_list():
    with open('DISABLED') as f:
                disabled_tests = f.read().splitlines()
    autogenerate_list = map(lambda x: x.strip(), disabled_tests)
    autogenerate_list = filter(lambda x:not x.startswith(';'), disabled_tests)
    return disabled_tests

def RunTest(name):
    write(FORMAT % (name, NORMAL, "Running"))

    pname = "../build/unit-tests/" + name

    # Normal
    norm_time = Run([], name, pname, name + ".normal")
    if norm_time is None:
        return

    # Record
    rec_time = Run([recordtool, "-o", name + ".rr"], name, pname, name + ".record")
    if rec_time is None:
        return

    # Replay
    rep_time = Run([replaytool, "-o", name + ".rr"], name, pname, name + ".replay")
    if rep_time is None:
        return


    write(CLEAR)
    write(TFORMAT % (name, GREEN, "Completed", norm_time, rec_time, rep_time))
    write("\n")
    subprocess.run(["ipcrm", "-W"])


basedir = os.getcwd()
if (basedir.split('/')[-1] != 'unit-tests'):
    os.chdir('unit-tests')

if len(sys.argv) > 1:
    for t in sys.argv[1:]:
        if all_tests.count(t) == 0:
            print("Test '%s' does not exist!" % (t))
            sys.exit(255)
        tests.append(t)
else:
    tests = all_tests

write("%-32s   %-9s   %-10s %-10s %-10s\n" %
        ("Test", "Status", "Normal", "Record", "Replay"))
write("------------------------------------------------------------------------------\n")
os.setpgid(0, 0)
disabled_tests = read_disabled_list()
for t in tests:
    if t in disabled_tests:
        ReportDisabled(t)
        continue
    CleanTest(t)
    #BuildTest(t)
    RunTest(t)

print(str(len(disabled)) + " tests disabled")

if len(failed) > 0:
    print("\n>>>>>>> !!!!!TESTS FAILED !!!!!<<<<<<<<")
    print(str(len(failed)) + " tests failed")
    print(">>>>>>>> !!!!!!!!!!<<<<<<<<\n")

if len(failed) != 0:
    print("Killing any left over processes.")
    signal.signal(signal.SIGTERM, signal.SIG_IGN)
    os.killpg(0, signal.SIGTERM)
    print("Cleaning any left over System V shared memory segments.")
    subprocess.run(["ipcrm", "-W"])

sys.exit(len(failed))


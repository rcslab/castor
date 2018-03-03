#!/usr/bin/env python

import sys
import os
import subprocess

for d in sorted(os.listdir('tests')):
    target = 'tests/' + d
    if os.path.isdir(target):
        print "Cleaning " + target
        os.chdir(target)
        subprocess.call(["./clean.sh"])
        os.chdir("../..")

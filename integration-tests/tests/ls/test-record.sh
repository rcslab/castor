#!/bin/sh
mkdir -p logs
rm -f logs/*

record -o logs/startup.rr truss -o logs/record.truss ./ls > ls.record
record -o logs/startup1.rr truss -o logs/record1.truss ./ls -l >> ls.record
record -o logs/startup2.rr truss -o logs/record2.truss ./ls -L >> ls.record
record -o logs/startup3.rr truss -o logs/record3.truss ./ls -R >> ls.record

sleep 1
killall -9 -m record truss ls
exit 0

#!/bin/sh
mkdir -p logs
truss -o logs/normal.truss ./ls &
truss -o logs/normal1.truss ./ls -l &
truss -o logs/normal2.truss ./ls -L &
truss -o logs/normal3.truss ./ls -R &

sleep 1
killall -9 -m ls truss
exit 0

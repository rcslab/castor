#!/bin/sh
mkdir -p logs
truss -o logs/normal.truss ./simple-udp &

sleep 1
killall -9 -m truss simple-udp
exit 0

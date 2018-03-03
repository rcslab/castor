#!/bin/sh
mkdir -p logs
truss -o logs/normal.truss ./lua input.lua &

sleep 1
killall -q -9 -m truss lua
exit 0

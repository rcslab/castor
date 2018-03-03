#!/bin/sh
mkdir -p logs
replay -o logs/lua.rr truss -o logs/replay.truss ./lua input.lua &

sleep 1
killall -q -9 -m replay truss lua
exit 0

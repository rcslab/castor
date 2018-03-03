#!/bin/sh
mkdir -p logs
rm -f logs/*
record -o logs/lua.rr truss -o logs/record.truss ./lua input.lua &

sleep 1
killall -q -9 -m record truss lua
exit 0

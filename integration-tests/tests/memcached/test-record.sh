#!/bin/sh
mkdir -p logs
rm -f logs/*

record -o logs/startup.rr truss -o logs/record.truss ./memcached -vv &
record -o logs/testapp.rr truss -o logs/record1.truss ./testapp &

sleep 1
killall -9 -m record truss memcached testapp
exit 0

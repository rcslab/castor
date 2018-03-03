#!/bin/sh
replay -o logs/startup.rr truss -o logs/replay.truss ./memcached -vv &
replay -o logs/testapp.rr truss -o logs/replay1.truss ./testapp &

sleep 1
killall -9 -m replay truss memcached testapp

exit 0

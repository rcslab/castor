#!/bin/sh
mkdir -p logs

truss -o logs/normal.truss ./memcached -vv &
truss -o logs/normalapp.truss ./testapp &

sleep 1
killall -9 -m truss testapp memcached
exit 0

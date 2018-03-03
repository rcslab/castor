#!/bin/sh

replay -o logs/dbbench.rr truss -o logs/replay.truss ./db_bench > leveldb.replay &

sleep 5
killall -9 -m replay truss db_bench

exit 1

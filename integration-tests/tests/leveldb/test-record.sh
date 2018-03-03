#!/bin/sh
mkdir -p logs
rm -f logs/*

record -o logs/dbbench.rr truss -o logs/record.truss ./db_bench > leveldb.record &

sleep 5
killall -9 -m record truss db_bench
exit 0

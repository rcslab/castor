#!/bin/sh
mkdir -p logs
truss -o logs/normal.truss ./db_bench &

sleep 5
killall -9 -m truss db_bench
exit 0

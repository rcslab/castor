#!/bin/sh
mkdir -p logs
truss -o logs/truss.normal ./redis-server --loglevel debug &
sleep 1
killall -9 -m "redis-server"

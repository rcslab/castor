#!/bin/sh
mkdir -p logs
rm -f logs/*
record -o logs/redis.rr truss -o logs/record.truss ./redis-server --loglevel debug &
sleep 1
killall -9 -m record redis-server

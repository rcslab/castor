#!/bin/sh
replay -o logs/redis.rr truss -o logs/replay.truss ./redis-server --loglevel debug &
sleep 1
killall -9 -m replay redis-server


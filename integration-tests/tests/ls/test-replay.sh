#!/bin/sh
replay -o logs/startup.rr truss -o logs/replay.truss ./ls > ls.replay
replay -o logs/startup1.rr truss -o logs/replay1.truss ./ls -l >> ls.replay
replay -o logs/startup2.rr truss -o logs/replay2.truss ./ls -L >> ls.replay
replay -o logs/startup3.rr truss -o logs/replay3.truss ./ls -R >> ls.replay

sleep 1
killall -9 -m record truss ls

#diff -s ls.record ls.replay
exit 0
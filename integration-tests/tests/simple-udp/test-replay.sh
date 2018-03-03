#!/bin/sh
replay -o logs/simple-udp.rr truss -o logs/replay.truss ./simple-udp > simple-udp.replay

# diff -s simple-udp.record simple-udp.replay

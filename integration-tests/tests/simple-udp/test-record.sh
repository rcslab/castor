#!/bin/sh
mkdir -p logs
rm -f logs/*
record -o logs/simple-udp.rr truss -o logs/record.truss ./simple-udp

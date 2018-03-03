#!/bin/sh
mkdir -p logs
rm -f logs/*
record -o logs/dash.rr truss -o logs/dash.truss ./dash ./test-input.sh

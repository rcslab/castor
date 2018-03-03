#!/bin/sh
mkdir -p logs
rm -f logs/*
record -o logs/bash.rr truss -o logs/record.truss ./bash ./test-input.sh

#!/bin/sh
mkdir -p logs
truss -o logs/normal.truss ./bash ./test-input.sh

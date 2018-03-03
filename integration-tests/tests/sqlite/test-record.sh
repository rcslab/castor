#!/bin/sh
mkdir -p logs
rm -f logs/*
rm -f test.db
record -o logs/shell.rr truss -o logs/shell.truss ./shell test.db < ./test-input.sql

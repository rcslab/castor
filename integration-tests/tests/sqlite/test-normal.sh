#!/bin/sh
mkdir -p logs
rm -f test.db
./shell test.db < ./test-input.sql

#!/bin/sh
replay -o logs/shell.rr truss -o logs/shell.truss ./shell test.db < ./test-input.sql

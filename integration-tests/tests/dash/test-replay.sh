#!/bin/sh
replay -o logs/dash.rr truss -o logs/dash.truss ./dash ./test-input

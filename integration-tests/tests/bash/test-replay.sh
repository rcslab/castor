#!/bin/sh
replay -o logs/bash.rr truss -o logs/replay.truss ./bash ./test-input

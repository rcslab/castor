#!/bin/sh
mkdir -p logs

replay -o logs/pbzip2-compress.rr truss -o logs/replay1.truss sh -c "./pbzip2 -l -p2 words"
replay -o logs/pbzip2-decompress.rr truss -o logs/replay2.truss sh -c "./pbzip2 -l -p2 -d words.bz2"

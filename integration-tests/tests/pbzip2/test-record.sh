#!/bin/sh
mkdir -p logs
rm -rf logs/*

record -o logs/pbzip2-compress.rr truss -o logs/record1.truss sh -c "./pbzip2 -l -v -p2 words"
record -o logs/pbzip2-decompress.rr truss -o logs/record2.truss sh -c "./pbzip2 -l -v -p2 -d words.bz2"

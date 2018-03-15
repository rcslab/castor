#!/bin/sh
mkdir -p logs
rm -rf logs/*

record -o logs/zip.rr truss -o logs/truss.out sh -c "./pbzip2 -c -l -v -p2 words > words.bz2"
record -o logs/unzip.rr truss -o logs/truss.out sh -c "./pbzip2 -c -d -l -v -p2 words.bz2 > words.out"

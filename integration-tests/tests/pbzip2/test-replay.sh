#!/bin/sh
replay -o logs/zip.rr truss -o logs/replay-zip.truss sh -c "./pbzip2 -c -l -v -p2 words > words.bz2"
replay -o logs/unzip.rr truss -o logs/replay-unzip.truss sh -c "./pbzip2 -c -d -l -v -p2 words.bz2 > words.out"

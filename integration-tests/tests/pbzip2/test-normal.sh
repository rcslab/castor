#!/bin/sh
mkdir -p logs
rm -rf logs/*

truss -o logs/normal1.truss sh -c "./pbzip2 -l -v -p2 words"
truss -o logs/normal2.truss sh -c "./pbzip2 -l -v -p2 -d words.bz2"

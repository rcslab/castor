#!/bin/sh

for i in `ls *.txt`; do
    j=`echo $i | sed 's/txt/1/'`
    k=`echo $i | sed 's/.txt//'`
    echo "Formatting $i generating $j"
    txt2man -p -t $k -s 1 "./$i" > $j
done

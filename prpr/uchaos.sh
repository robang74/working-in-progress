#!/bin/sh
nfle=${1:-test.txt}
echo "Appending: $nfle"
time for i in $(seq 0 7); do
    printf "\n### using -s $i -d 7\n";
    cat uchaos.c | ./uchaos -T 10000 -s $i -d 7 | ent;
done >> $nfle 2>&1
time for i in $(seq 0 7); do
    printf "\n### using -s $i -d 15\n";
    cat uchaos.c | ./uchaos -T 10000 -s $i -d 15 | ent;
done >> $nfle 2>&1

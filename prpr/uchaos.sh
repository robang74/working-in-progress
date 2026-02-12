#!/bin/sh

nfle=${1:-test.txt}
echo "Appending: $nfle"
reseeding_test() {
    for i in $(seq 0 7); do
        printf "\n### using -T 1000 x8\n";
        { date +%N; cat uchaos.c; } | ./uchaos -T 1000;
    done 2>> $nfle | ent >> $nfle
}

if [ ! -x ./uchaos ]; then
    gcc uchaos.c -O3 -Wall -o uchaos
fi
time reseeding_test

if false; then
    time for i in $(seq 0 7); do
        printf "\n### using -s $i -d 7\n";
        cat uchaos.c | ./uchaos -T 10000 -s $i -d 7 | ent;
    done >> $nfle 2>&1
    time for i in $(seq 0 7); do
        printf "\n### using -s $i -d 15\n";
        cat uchaos.c | ./uchaos -T 10000 -s $i -d 15 | ent;
    done >> $nfle 2>&1
fi

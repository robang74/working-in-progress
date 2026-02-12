#!/bin/sh

nfle=${1:-test.txt}
echo "Appending: $nfle"
reseeding_test() {
    printf "\n### uschaos x8 using -T 100\n" >> $nfle;
    for j in $(seq 16); do
        for i in $(seq 8); do
            { date +%N; cat uchaos.c; } | ./uchaos -T 100 &
        done 2>> $nfle
    done | ent >> $nfle
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
    done
fi

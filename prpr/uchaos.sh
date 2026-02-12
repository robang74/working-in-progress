#!/bin/sh
time for i in $(seq 0 7); do
    printf "\n### using -s $i\n";
    cat uchaos.c | ./uchaos -T 10000 -s $i -d 7 | ent;
done >> test.txt 2>&1

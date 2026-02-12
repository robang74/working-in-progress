#!/bin/sh
# (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2 license

entgr() { ent "$@" | grep -E "error|samples|127.5 |Entropy|exceed|uncorrelated"; }

nfle=${1:-test.txt}
echo "uchaos.sh is appending to file: $nfle"

testfunc() {
    tcmd="./uchaos -T $((100*1024)) -d $i /\\_"
    printf "\n|\/ Testing with $tcmd" | tee -a $nfle.$i
    {
        printf "_%.0s" {1..32}; printf "\n|";
        cat uchaos.c | { $tcmd 2>&3; printf "\n|\n" >&3; } | entgr
    } 3>&1 | grep . >> $nfle.$i
}

for i in 0 3 7 16; do
    tcmd="./uchaos -T $((100*1024)) -d $i -r 64 /\\_"
    testfunc & sleep 0.01
done
time wait
for i in 0 3 7 16; do cat $nfle.$i >> $nfle; done
echo

if false; then
    hgstr="{ time date +%N 2>&1 | dd bs=1; } 2>&1"
    h() { date +%N | sha512sum | cut -f1 -d' '; sleep 0.01; }

    reseeding_test() {
        printf "\n### uschaos x8 using -T 1600\n" >> $nfle;
        for j in $(seq 1); do
            for i in $(seq 8); do
                ./mtrd -t4 "$hgstr" | ./uchaos -T 1600 | dd bs=1 status=none &
            done 2>> $nfle
        done | dd ibs=1 status=none | tee test.bin | entgr >> $nfle
    }

    if [ ! -x ./uchaos ]; then
        gcc uchaos.c -O3 -Wall -o uchaos
    fi

    time reseeding_test

### ATTENTION: most of these test are keen to fail becuase requires Nx GB of data
###            otherwise the test run-over the same dataset MANY times (like 80+)

    # Run dieharder (all tests, file input, 100 trials—~5-10min on VM)
    # dieharder -a -g 201 -f test.bin -n 100
    # Or targeted (e.g., collisions + runs)
    printf "\nExecuting dieharder's most sensitive -g 201 tests:\n" >> $nfle
    dieharder -d 2 -g 201 -f test.bin -n 50 2>&1 | grep diehard_ >$nfle.1 & # Birthday
    dieharder -d 3 -g 201 -f test.bin -n 50 2>&1 | grep diehard_ >$nfle.2 & # OPERM
    dieharder -d 5 -g 201 -f test.bin -n 50 2>&1 | grep diehard_ >$nfle.3 & # Runs
    time wait ; cat $nfle.[1-9] >> $nfle
    printf "\nDataset size in Kb: $(du -ks test.bin | tr '\t' ' ')\n" >> $nfle
fi

if false; then
    time for i in $(seq 0 7); do
        printf "\n### using -s $i -d 7\n";
        cat uchaos.c | ./uchaos -T 10000 -s $i -d 7 | entgr;
    done >> $nfle 2>&1
    time for i in $(seq 0 7); do
        printf "\n### using -s $i -d 15\n";
        cat uchaos.c | ./uchaos -T 10000 -s $i -d 15 | entgr;
    done
fi

test -f $nfle

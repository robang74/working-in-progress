#!/bin/sh

nfle=${1:-test.txt}
echo "Appending: $nfle"

time { cat uchaos.c | ./uchaos -T 10000 | ent >> $nfle; } 2>&1

if false; then
    hgstr="{ time date +%N 2>&1 | dd bs=1; } 2>&1"
    h() { date +%N | sha512sum | cut -f1 -d' '; sleep 0.01; }

    reseeding_test() {
        printf "\n### uschaos x8 using -T 1600\n" >> $nfle;
        for j in $(seq 1); do
            for i in $(seq 8); do
                ./mtrd -t4 "$hgstr" | ./uchaos -T 1600 | dd bs=1 status=none &
            done 2>> $nfle
        done | dd ibs=1 status=none | tee test.bin | ent >> $nfle
    }

    if [ ! -x ./uchaos ]; then
        gcc uchaos.c -O3 -Wall -o uchaos
    fi

    time reseeding_test

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
        cat uchaos.c | ./uchaos -T 10000 -s $i -d 7 | ent;
    done >> $nfle 2>&1
    time for i in $(seq 0 7); do
        printf "\n### using -s $i -d 15\n";
        cat uchaos.c | ./uchaos -T 10000 -s $i -d 15 | ent;
    done
fi

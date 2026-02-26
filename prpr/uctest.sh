#!/bin/bash
#
# (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, MIT license
#
# output to test with: cat *.dat | PractRand_0.96/PractRand/RNG_test stdin64
#

for i in $(seq 32); do

for t in $(seq 4); do
  n=$(printf "%04d" $i)
  echo "test-$t-$n.txt test-$t-$n.dat"
  ({ time cat dmesg.txt | ./uchaos -i16 -T $((256*1024)) | tee test-$t-$n.dat | ent; } > test-$t-$n.txt 2>&1) &
  sleep 0.1
done

wait

done

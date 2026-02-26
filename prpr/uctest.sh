#!/bin/sh
#
# (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, MIT license
#
# output to test with: cat *.dat | PractRand_0.96/PractRand/RNG_test stdin64
#

for x in $(seq 32); do

for t in $(seq 4); do

for i in $(seq 1); do

n=$(printf "%04d" $i)

{ time cat dmesg.txt | ./uchaos -i16 -T$[1024*256] | tee test-$t-$n.dat |  ent; } >> test-$t-$n.txt 2>&1 &

done

wait

done

done

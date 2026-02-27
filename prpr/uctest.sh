#!/bin/bash
#
# (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, MIT license
#
# preliminary internal check: grep KO test-*.txt
# output to test with: cat $(ls -1 test-*.dat | sort) | RNG_test stdin64
#

if [ ! -r "dmesg.txt" -o ! -x "uchaos" ]; then
  printf "\nERROR: dmesg.txt readable and uchaos executable are needed\n" >&2
fi

fn="test"
nh=$((512*1024))
ch="./uchaos -i 16 -d 3 -T $nh"
tf() { cat dmesg.txt | $ch | tee $1.dat | ent; }

for i in $(seq 32); do

  for t in $(seq 4); do
    n=$(printf "%04d" $i)
    nm="$fn-$t-$n"
    echo "$nm.txt $nm.dat"
    eval tf $nm >$nm.txt 2>&1 & sleep 0.1
  done

  wait
  eq=$(du -k $(find . -name $fn-\*.dat | sort) | cut -f1 | tr '\n' +)0
  echo "$(( ( $eq ) >> 10 )) MB"
  grep -n "KO" $fn-*.txt &&
    break

done

echo
echo 'to test with: cat $(ls -1 test-*.dat | sort) | RNG_test stdin64'
echo

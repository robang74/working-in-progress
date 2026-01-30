#!/bin/bash
# expected output: prpr.sh.txt

echo
echo ">gcc -o prpr -O3 prpr.c && strip prpr"
set -e
gcc -O3 prpr.c -o prpr && strip prpr
set +e

chktest() {
  j=$1
  for i in $(seq -$j $j); do printf -- "+$j o:%2d " $i
    printf ${string} | ./prpr -r  $j -o $i; echo;
  done
  for i in $(seq -$j $j); do printf -- "-$j o:%2d " $i
    printf ${string} | ./prpr -r -$j -o $i; echo;
  done
}

string="0123456789ABCDEF"

printf -- " 0+o:12 ";
printf ${string} | ./prpr -r  0 -o  16
echo
printf -- " 0-o:12 ";
printf ${string} | ./prpr -r  0 -o -16
echo

chktest 4
chktest 5

printf ">5 o: 6 Error expected 4: "
{
  printf ${string} | ./prpr -r  5 -o  6 >/dev/null
  printf ${string} | ./prpr -r  5 -o -6 >/dev/null
  printf ${string} | ./prpr -r -5 -o  6 >/dev/null
  printf ${string} | ./prpr -r -5 -o -6 >/dev/null
} 2>&1 | grep "Error: Offset exceeds window" | wc -l
echo


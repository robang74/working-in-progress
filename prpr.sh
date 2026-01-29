#!/bin/bash -e

echo
echo ">gcc -o prpr -O3 prpr.c && strip prpr"
set -e
gcc -O3 prpr.c -o prpr && strip prpr
string="0123456789"

printf -- "0+o:10 ";
printf ${string} | ./prpr -r  0 -o  10
echo
printf -- "0-o:10 ";
printf ${string} | ./prpr -r  0 -o -10
echo

for i in $(seq 0 5); do printf -- "+5+o:$i ";
  printf ${string} |   ./prpr -r  5 -o  $i; echo;
done
for i in $(seq 0 5); do printf -- "-5-o:$i ";
  printf ${string} |   ./prpr -r -5 -o -$i; echo;
done
for i in $(seq 0 5); do printf -- "+5-o:$i ";
  printf ${string} |   ./prpr -r  5 -o -$i; echo;
done
for i in $(seq 0 5); do printf -- "-5+o:$i ";
  printf ${string} |   ./prpr -r -5 -o  $i; echo;
done

printf ">5:o:6 Error expected 4: "
{
  printf ${string} | ./prpr -r  5 -o  6 >/dev/null
  printf ${string} | ./prpr -r  5 -o -6 >/dev/null
  printf ${string} | ./prpr -r -5 -o  6 >/dev/null
  printf ${string} | ./prpr -r -5 -o -6 >/dev/null
} 2>&1 | grep "Error: Offset exceeds window" | wc -l
echo

# EXPECTED OUTPUT
#
# >gcc -o prpr -O3 prpr.c && strip prpr
# 0+o:10 0123456789
# 0-o:10 9876543210
# +5+o:0 
# +5+o:1 05
# +5+o:2 0156
# +5+o:3 012567
# +5+o:4 01235678
# +5+o:5 0123456789
# -5-o:0 
# -5-o:1 01345689
# -5-o:2 014569
# -5-o:3 0459
# -5-o:4 05
# -5-o:5 
# +5-o:0 
# +5-o:1 49
# +5-o:2 3489
# +5-o:3 234789
# +5-o:4 12346789
# +5-o:5 0123456789
# -5+o:0 
# -5+o:1 01345689
# -5+o:2 034589
# -5+o:3 0459
# -5+o:4 49
# -5+o:5 
# >5:o:6 Error expected 4: 1


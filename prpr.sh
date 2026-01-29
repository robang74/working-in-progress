#!/bin/bash

echo
echo ">gcc -o prpr -O3 prpr.c && strip prpr"
gcc -O3 prpr.c -o prpr && strip prpr

for i in $(seq 0 5); do printf -- "+5+o:$i ";
  printf 1234567890 | ./prpr -r 5 -o $i; echo;
done
for i in $(seq 0 5); do printf -- "-5+o:$i ";
  printf 1234567890 | ./prpr -r -5 -o -$i; echo;
done
for i in $(seq 0 5); do printf -- "+5-o:$i ";
  printf 1234567890 | ./prpr -r 5 -o -$i; echo;
done
for i in $(seq 0 5); do printf -- "-5+o:$i ";
  printf 1234567890 | ./prpr -r -5 -o $i; echo;
done

printf ">5:o:6 Error expected 4: "
{
  printf 1234567890 | ./prpr -r  5 -o  6 >/dev/null
  printf 1234567890 | ./prpr -r  5 -o -6 >/dev/null
  printf 1234567890 | ./prpr -r -5 -o  6 >/dev/null
  printf 1234567890 | ./prpr -r -5 -o -6 >/dev/null
} 2>&1 | grep "Error: Offset exceeds window" | wc -l
echo

# EXPECTED OUTPUT
# >gcc -o prpr -O3 prpr.c && strip prpr
# +5+o:0 
# +5+o:1 16
# +5+o:2 1267
# +5+o:3 123678
# +5+o:4 12346789
# +5+o:5 1234567890
# -5+o:0 1234567890
# -5+o:1 12456790
# -5+o:2 125670
# -5+o:3 1560
# -5+o:4 16
# -5+o:5 
# +5-o:0 
# +5-o:1 50
# +5-o:2 4590
# +5-o:3 345890
# +5-o:4 23457890
# +5-o:5 1234567890
# -5+o:0 1234567890
# -5+o:1 12456790
# -5+o:2 145690
# -5+o:3 1560
# -5+o:4 50
# -5+o:5 
# >5:o:6 Error expected 4: 4


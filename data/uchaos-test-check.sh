#!/bin/bssh
# (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, MIT license

while true; do #################################################################

list=$(ls -1 uchaos-test-[0-9]*.zip)
test -n "$list" || break

nzipfiles=$(echo "$list" | wc -l)
echo "Removing $nzipfiles folders:"
if [ "x${1:-}" == "x--clean" ]; then
    for i in $list; do
        nme=${i/.zip/}
        printf "  $nme: "
        rm -rf $nme
        test -d $nme && echo "KO" || echo "OK"
    done
    break
fi

printf "Check the sha256 of zip files: "
sha256sum -c uchaos-test-sha256.txt 2>&1 | grep -v ": OK" |\
    grep -q . && { echo "KO"; break; } || { echo "OK"; }

printf "Unzip $nzipfiles files in folders: "
for i in $list; do
    nme=${i/.zip/}
    mkdir -p $nme
    unzip -qfd $nme $i || { echo "KO"; break; }
done; echo "OK"

printf "Check for tests outcomes:\n"
for i in $list; do
    nme=${i/.zip/}
    echo "  $nme:"
    grep -E "OK|KO" $nme/test.txt | sed -e "s/^./    &/"
done

break; done ####################################################################

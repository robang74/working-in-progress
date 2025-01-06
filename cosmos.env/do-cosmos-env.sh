#!/bin/bash -e
#
# (C) 2025, Roberto A. Foglietta <roberto.foglietta@gmail.com> - MIT license
#
################################################################################

function get_cosmotolitan_last_version() {
    local i vfnd="" strn=$(wget -O- $1 2>/dev/null |\
        sed -ne "s,.*<a href=[^>]*>cosmopolitan-\(.*\).tar.gz</a>.*,\\1,p")
    test -n "$strn"
    declare -A vern
    for i in 1 2 3; do
        vern[$i]=$(echo "$strn" | grep -e "^$vfnd" | cut -d. -f$i | sort -n | tail -n1)
        test -n "${vern[$i]}" || continue
        vfnd+="${vfnd:+.}${vern[$i]}"
    done
    echo $vfnd
}

################################################################################

durl="https://cosmo.zip/pub"
ccfn="cosmocc.zip"
bdir="cosmos.env"
pkgd="pkg.d"

printf "\n> INFO: creating cosmos enviroment in $bdir ...\n"

mkdir -p $bdir/$pkgd; cd $bdir

lstv=$(get_cosmotolitan_last_version $durl/cosmo/)
nxt="tgz"
ext="tar.gz"
nme="cosmopolitan"
pkg="$nme-$lstv.$ext"
test -e "$pkgd/$pkg" || wget -nc $durl/cosmo/$pkg -O $pkgd/$pkg
ln -sf $pkg $pkgd/$nme.tgz

for pkg in cosmocc; do
    if [ ! -e $pkgd/$pkg.zip ]; then
        wget -nc $durl/$pkg/$pkg.zip -o $pkgd/$pkg.zip
    fi
    printf "\n> INFO: decompressing $pkgd/$pkg.zip ... "
    unzip -o $pkgd/$pkg.zip | wc -l | tr -d '\n'
    printf " files\n"
done

ext="zip"
nme="cosmos"
pkg="$nme.$ext"
test -e "$pkgd/$pkg" || wget -nc $durl/$nme/$ext/$pkg -O $pkgd/$pkg
printf "\n> INFO: decompressing $pkgd/$pkg ... "
unzip -o $pkgd/$pkg | wc -l | tr -d '\n'
printf " files\n"

sysarch=$(uname -m)
errstrn="run-detectors: unable to find an interpreter"
xelfape="bin/ape-$sysarch.elf"
destbin="/usr/bin/ape"
sysregf="/proc/sys/fs/binfmt_misc/register"
wslregf="/proc/sys/fs/binfmt_misc/WSLInterop"
apestr1=":APE:M::MZqFpD::$destbin:"
apestr2=":APE-jart:M::jartsr::$destbin:"
cosmocc="bin/cosmocc"

test -e $sysregf || sysregf="/dev/null"
test -e $wslregf || wslregf="/dev/null"

printf "\n> INFO: testing the $cosmocc compiler ... "
printf '#include <stdio.h>\nint main() { printf("hello world\\n"); }' >hello.c
if $cosmocc -o hello hello.c 2>&1 | grep "$errstrn" >&2; then
    printf "\n\n> WARNING: this shell has not ape support, trying to solve ...\n"
    if [ -x bin/$xelfape ]; then
        for cmd in "cp -f $xelfape $destbin" "chmod +x $destbin" \
                         "bash -c \"echo '$apestr1' >$sysregf\"" \
                         "bash -c \"echo '$apestr2' >$sysregf\"" \
                         "bash -c \"echo '-1' >$wslregf\"" ;
        do
            echo "sudo $cmd <- execute? CTRL-C otherwise"
            read; eval sudo "$cmd"
        done
        $cosmocc -o hello hello.c
        if ./hello | grep "hello world"; then
            echo
        else
            printf "\n\n> ERROR: $xelfape is missing or cannot run\n" >&2
        fi
    else
        printf "\n\n> ERROR: $cosmocc is missing or cannot run\n" >&2
    fi
else
    printf " ok\n"
fi
rm -f hello.com.dbg hello.*.elf hello.c hello

mkdir -p com
for com in emulator.com tinyemu.com; do
    test -e com/$com ||\
        wget -nc https://justine.lol/$com -O com/$com &&
            chmod +x com/$com
done
printf "\n> INFO: downloading toybox ... \n\n"
wget -nc https://landley.net/toybox/bin/toybox-$sysarch -O bin/toybox
chmod a+x bin/toybox; bin/toybox; printf "\n$(du -ks bin/toybox)\n"

printf "\n> INFO: everything done.\n\n"
    

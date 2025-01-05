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

printf "\n> INFO: creating cosmos enviroment in $bdir...\n"

mkdir -p $bdir/$pkgd; cd $bdir

lstv=$(get_cosmotolitan_last_version $durl/cosmo/)
nxt="tgz"
ext="tar.gz"
nme="cosmopolitan"
pkg="$nme-$lstv.$ext"
test -e "$pkgd/$pkg" || wget -c $durl/cosmo/$pkg -O $pkgd/$pkg
ln -sf $pkg $pkgd/$nme.tgz

for pkg in cosmocc; do
    if [ ! -e $pkgd/$pkg.zip ]; then
        wget -c $durl/$pkg/$pkg.zip -o $pkgd/$pkg.zip
    fi
    #unzip -o $pkgd/$pkg.zip
done

ext="zip"
nme="cosmos"
pkg="$nme.$ext"
test -e "$pkgd/$pkg" || wget -c $durl/$nme/$ext/$pkg -O $pkgd/$pkg
#unzip -o $pkgd/$pkg

errstrn="run-detectors: unable to find an interpreter"
xelfape="bin/ape-$(uname -m).elf"
destbin="/usr/bin/ape"
sysregf="/proc/sys/fs/binfmt_misc/register"
apestr1=":APE:M::MZqFpD::$destbin:"
apestr2=":APE-jart:M::jartsr::$destbin:"
cosmocc="bin/cosmocc"

printf "\n> INFO: testing the $cosmocc compiler...\n"
printf '#include <stdio.h>\nint main() { printf("hello world\\n"); }' >hello.c
if $cosmocc -o hello hello.c 2>&1 | grep "$errstrn" >&2; then
    printf "\n\n> WARNING: this shell has not ape support, trying to solve...\n"
    if [ -x bin/$xelfape ]; then
        for cmd in "cp -f $xelfape $destbin" "chmod +x $destbin" \
                         "bash -c \"echo '$apestr1' >$sysregf\"" \
                         "bash -c \"echo '$apestr2' >$sysregf\"" ;
        do
            echo "sudo $cmd <- execute? CTRL-C otherwise"
            read; eval sudo "$cmd"
        done
        $cosmocc -o hello hello.c
        if ./hello | grep "hello world"; then
            rm -f hello.com.dbg hello.*.elf hello.c hello
            echo
        else
            printf "\n\n> ERROR: $xelfape is missing or cannot run\n" >&2
        fi
    else
        printf "\n\n> ERROR: $cosmocc is missing or cannot run\n" >&2
    fi
fi

printf "\n> INFO: done.\n\n"
    

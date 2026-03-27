#!/bin/bash
#
# (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, MIT license
#

append_for_kernel_debug="earlyprintk=serial nokaslr -pidfile vm.pid -panic=1"

kimg="bzImage"
rfsimg="initramfs.cpio"
qemubin="qemu-system-x86_64"
test -r $qemubin.gz.sh && qemubin="$qemubin.gz.sh"

export PATH=.:$PATH

test -r ${rfsimg}.gz && rfsimg="${rfsimg}.gz"
rfsdir=$(echo "$rfsimg" | sed 's/\.cpio\.gz//;s/\.cpio//')

chkmd5() { md5sum -c update/$rfsdir.md5 2>/dev/null; }

# Cope with the user's parametric input

test -r bzImage || ln -sf bzImage.orig bzImage

# Command line flags management

imgupdte=0
updtquit=0
zerokelv=0
zerotest=0
quietrun=0

while getopts "ZzuUqm:wM:" opt; do
  case $opt in
    u)
      imgupdte=1
      ;;
    U)
      imgupdte=1
      updtquit=1
      ;;
    z)
      zerokelv=1
      ;;
    Z)
      zerotest=1
      ;;
    q)
      quietrun=1
      ;;
    m)
      QMSZE=$OPTARG
      ;;
    M)
      QMACH=$OPTARG
      ;;
    w)
      ZWARM=1
      ;;
    *)
      echo "Unknown flag, ignored: $1"
      ;;
  esac
done; shift $((OPTIND - 1))

if [ $quietrun -ne 0 ]; then
  cmdlnx="quiet"
else
  cmdlnx=""
fi

if [ $zerotest -ne 0 ]; then
  export QZERO=1 QMSZE=${QMSZE:-256M} UCTEST=${UCTEST:-1}
  cmdlnx="$cmdlnx UCTEST=${UCTEST:-1}"
elif [ $zerokelv -ne 0 ]; then
  export QZERO=1 QMSZE=${QMSZE:-256M} UCTEST=${UCTEST:-0}
  cmdlnx="$cmdlnx UCTEST=${UCTEST:-1}"
elif [ -n "$UCTEST" ]; then
  cmdlnx="$cmdlnx UCTEST=$UCTEST"
fi

if [ $imgupdte -ne 0 ]; then
  sh -c "cd ..; ./cpio.sh -c"
  if [ $updtquit -ne 0 ]; then
    cp -f $(find ../musl -name bzImage -type f) .
    du -k bzImage | sed -e "s/\t/ KB /"
    exit 0
  fi
  read -p ">>> Updated, press ENTER to continue " key
fi

# Preparing the QEMU virtual machine configuration #############################

export QTTYUC=${QTTYUC:-console=ttyS0,115200n8}

cmdlnx="$cmdlnx HOST=x86_64 root=/dev/ram0 init=/init $QTTYUC net.ifnames=0 nokaslr"
nograp="-nographic -vga none -display none"

if [ "${QZERO:-0}" = "0" ]; then
  boxnme="-name tinylnx"
  qaccel="-enable-kvm -cpu host,migratable=off,+invtsc"
# netisl="-netdev user,id=net0,restrict=yes"
  if "$QMACH" = "microvm" ; then
    qaccel="$qaccel -M microvm,accel=kvm,x-option-roms=off,pit=off,pic=off,rtc=off,acpi=off"
#   netisl="$netisl -device virtio-net-device,netdev=net0"
  else
    qaccel="$qaccel -M q35,accel=kvm,usb=off,vmport=off,dump-guest-core=off -boot order=dc"
#   netisl="$netisl -device virtio-net-pci,netdev=net0"
  fi
else
  echo
  echo "Zero Kelvin Linux mode"
  echo
  boxnme="-name zroklnx"
  qaccel="-accel tcg -cpu qemu64 -smp 1 -icount shift=0,sleep=off,align=off"
  qaccel="$qaccel -rtc base=2026-03-01,clock=vm,driftfix=none"
  cmdlnx="$cmdlnx deferred_probe_timeout=0 page_alloc.shuffle=0 memtest=0"
  cmdlnx="lpj=2000000 noapic nolapic clocksource=pit video=off nomodeset $cmdlnx"
  cmdlnx="$cmdlnx random.trust_cpu=off mitigations=off"
  netisl="-net none -serial mon:stdio -nodefaults"

  if [ "${ZWARM:-0}" = "1" ]; then
    qaccel="-cpu qemu64 -smp 1";
  fi
  qaccel="${QMACH:+-M $QMACH} $qaccel"
fi

cmdlnx="-append '$cmdlnx ${KARGS:-}'"

# disable this line if it creates trouble because ulimit -l isn't friendly
grep -qi uchaos /etc/os-release || qaccel="$qaccel -overcommit mem-lock=on"

cmd="$qemubin -m ${QMSZE:-128M} -kernel ${kimg} -initrd ${rfsimg} ${nograp:-} \
              -no-reboot ${boxnme:-} ${qaccel:-} ${netisl:-} \
              ${cmdlnx:-} ${QARGS:-}"

# Starting the QEMU configuraed virtual machine ################################

sh -xc "$cmd"; stty sane; printf '\e[?7h'
echo; echo $cmd; echo


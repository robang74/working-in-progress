#!/bin/bash
# (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2 license

nfle=${1:-test.txt}
echo "sysinfo.sh is appending to file: $nfle"
exec >> $nfle 2>&1

echo "=== System identification ==="
echo
date +"date: %F, time: %T, zone: %z, unix time: %s"
echo "Kernel: $(uname -a)"
echo "CPU model: $(grep -m1 '^model name' /proc/cpuinfo 2>/dev/null | cut -d: -f2- | sed 's/^[ \t]*//')"
echo "Sys info: nproc: $(nproc), uptime: $(uptime), free mem:"; free -h
echo "Scaling governors:"
if ls /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor >/dev/null 2>&1; then
  cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor 2>/dev/null | sort | uniq
else
  echo "no cpufreq"
fi

echo "lscpu output:"
lscpu 2>/dev/null || echo "(lscpu not available)"

printf "OS release:"
cat /etc/issue 2>/dev/null || cat /etc/os-release 2>/dev/null || echo "(no release info)"

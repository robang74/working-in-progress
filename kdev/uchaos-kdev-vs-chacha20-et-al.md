### uCHAOS V0.5.4: SHOW ME YOUR CODE, INSTEAD!

- [LinkedIn post #1](https://www.linkedin.com/posts/robertofoglietta_uchaos-v054-show-me-your-code-instead-activity-7438251459344781313-6UQS) -- [Facebook post #1](https://www.facebook.com/roberto.a.foglietta/posts/10163068326183736)

First of all, the screenshots:

1. on the left a qemu machine with CPU passthrough running in RAM but without network and disk I/O by an essential kernel and a busybox as the whole system. In this system 18 process/threads are running on a single CPU, 15 are kernel threads and [kswapd0] is for sure inactive because no swap exists while [kblockd] functionally is needed for initramfs (possibly, but it is arguable that a cpio can be seen as a block device rather than a stream, it is a matter of implementation, anyway) and full [kdevtmpfs] usage. A serial device as user interface in which I/O are text console printouts and keyboard's keys pressing. This is a minimal (or near minimal) functional Linux VMs, much less than this, it is hard to imagine doing anything useful if any at all.

2. on the right, the same above but with the CPU virtualized by software and no direct access to the RAM. Moreover, every hardware has been removed from being simulated and the hardware clock is counting the instructions instead of keeping time. This is a minimal (or near minimal) system debugging Linux VMs and it has no reason to be used outside debugging. Also because it is something between 8x and 14x times slower than the real machine which hosts the VMs.

The "coldest" (the minimal action) way to check with PractRand the output of the uChaos kernel driver is to read /dev/uchaos and write its output as a stream on the root filesystem which is RAM based. Every I/O are thus totally virtual due the fact that RAM is emulated. Moreover, the /dev/uchaos has been initialised (activated) with 8-byte of zeros:

- `0x0000000000000000` <-- this is the initial input, at boot time

Despite these minimalistic conditions, and despite using NO ANY cryptographic functions but just fully deterministic and linear functions, in both cases 8GB of randomness passed the PractRand test. Every LCG (linear consequential generator) would have failed the test in the first megabyte unless its output would be cryptographically obscured (and it might fail, anyway the test in the long run). Certification of perfection is issued at infinite time after infinite length check. Failures happen almost immediately, and 8GB is a WAY bigger than the few KB a kernel might need before its cryptographic engine would enter in function after being seeded by real entropy.

How is this "magic" possible? Theory of the Chaos. Which is also the main difference between jitterentropy and seedrng. These tools deal with entropy by its standardised concept, while uChaos creates dynamics of chaos as a source of entropy.

```sh
root@u-bmls:/# ls -1d /proc/[0-9]* | wc -l ;  ps -T
18
PID  USER   TIME COMMAND
  1 root   0:00 {linuxrc} init
...
  13 root   0:00 [kswapd0]
...
  84 root   0:00 /bin/sh
  89 root   0:00 ps -T

root@u-bmls:/# free
       total    used    free   shared buff/cache  available
Mem:    9556776    24064   9511172      4    21540   9454404
Swap:       0      0      0
```

---

### uCHAOS V0.5.4: SHOW ME YOUR CODE, INSTEAD! #2

- [LinkedIn post #2](https://www.linkedin.com/posts/robertofoglietta_uchaos-v054-show-me-your-code-instead-activity-7438256141433729024-LTW1) -- [Facebook post #2](https://www.facebook.com/roberto.a.foglietta/posts/10163068385103736)

Every novelty challenges and shackes our state of arts, this create frictions, resistance, doubts and fears. We are humans, after all. We do dramas.

Innovation is novelty that earn the market even before people or experts accept the idea behind it. Most of the time, it is not an invention but a novel way of integrating existing and well-known technologies.

uChaos does both, integrate existing technologies with well-known principles and move them into a playground in which more conservative approaches were always taken, because certification, because standardisation, because dogmas or whatever.

In this specific case prove cannot provided because "entropy" or "randomness" have a fundamental characteristics in common "no structure" and as you can imagine the only prove that can be provided is "we have found a structure where was supposed no one should be". This is when PractRand fails. However, if it doesn fail in 1MB, it might in 1GB or 1TB, or more. Unfortunately this is a never ending testing unless fail, never fails, never ends the test.

uChaos as character device for the Linux kernel with murmur3 as whitening final function move on the high ground by claiming: no conceiling stuff beyhind a critpgraphic layer but murmur3, zero input, real-time live-system always-available check of the output. If it fails, it fails almost immediately.

This is a great way for a scientific a-dogmatic approach to entropy in the kernel space. Therefore, I show you my code for inviting others people provide the same raw output debuging / inspectable char device. If it fails, it fails almost immediately.

It is a char device, not a jetplane cockpit. For sure it can be provided within a reasonable effort as a debug tool.

In such a way the XYZ's RNG is better than uChaos becuase it is algorithm is certified under lab conditions set by NIST & al? Let us to check its raw output with no ChaCha20 & Co. but murmur3 on a live system...

...then it fails, it fails fast? Ahi, Ahi, Ahi, bonito! 😘

---

### Screenshot n.1 (sx, OCR)

```text
Run /init as init process
MemTotal: 9556776 kB
MemAvailable: 9489144 kB
Temporary/dev/shm size is 65536 kB of RAM

Linux u-bmls 5.15.202 #4 Thu Mar 12 06:03:34 CET 2026 x86_64 GNU/Linux

uChaos128 v0.5.6 w/sb!/pr s:0, q:1, d+p(0):3+1 ns, r:31, 1:16, Z:19
dae0409c4d178d38ff4ed6d94985c9aa --> /dev/random by uChaos
Active console: ttyS0, entropy: 1 -> 256 bits
[ 0.229398] random: crng init done

Load, init with 8-zeros and test the /dev/uchaos
--------------------------------------------------------------------------------

filename: /lib/modules/uchaos_dev.ko
author: Roberto A. Foglietta <roberto.foglietta@gmail.com>>>
description: Stochastic jitter-based chaos stream device
license: GPL v2
parm: loop_mult: Number of repetitions of the hashing loop (default=1)
parm: min_delta: Minimum delta otherwise do an extra passage (default=3)
parm: init_runs: Number of initial runs for stat stabilization (default=7)
version: 0.5.3
srcversion: 40F787AC1477BBB531EAD53
depends:
vermagic: 5.15.202 mod_unload

16777216 bytes (16.0MB) copied, 0.205691 seconds, 77.8MB/s

BFN using PractRand version 0.96
RNG RNG_stdin64, seed = unknown
test set core, folding standard (64 bit)

rng=RNG_stdin64, seed=unknown
length 16 megabytes (2^24 bytes), time 0.2 seconds
no anomalies in 147 test result(s)

--------------------------------------------------------------------------------

      ===============================================
      Welcome to Minimal Linux System for QEMU x86_64
       System dmsg time was 0.20s at the 1st console
      ===============================================
       type command 'reboot -f' to shutdown this VM

root@u-bmls:/# dd if=/dev/uchaos bs=8k count=1M of=/d.ata 
1048576+0 records in 
1048576+0 records out 
8589934592 bytes (8.0GB) copied, 113.563875 seconds, 72.1MB/s 

root@u-bmls:/# RNG_test.gz.sh stdin64 < /d.ata 
BFN using PractRand version 0.96 
RNG RNG_stdin64, seed = unknown 
test set core, folding standard (64 bit)

rng-RNG_stdin64, seed=unknown 
length= 256 megabytes (2^28 bytes), time 2.6 seconds
no anomalies in 199 test result(s)

rng-RNG_stdin64, seed=unknown 
length 512 megabytes (2^29 bytes), time 5.4 seconds 
no anomalies in 213 test result(s)

rng=RNG_stdin64, seed=unknown
length 1 gigabyte (2^30 bytes), time 10.6 seconds 
no anomalies in 227 test result(s)

rng-RNG_stdin64, seed=unknown
length 2 gigabytes (2^31 bytes), time 22.6 seconds 
no anomalies in 242 test result(s)

rng=RNG_stdin64, seed=unknown
length 4 gigabytes (2^32 bytes), time 49.0 seconds 
no anomalies in 256 test result(s)

rng=RNG_stdin64, seed=unknown 
length 8 gigabytes (2^33 bytes), time 99.0 seconds 
no anomalies in 270 test result(s)

error reading from file
root@u-bmls:/# []
```

---

### Screenshot n.2 (dx, OCR)

```sh
Linux u-bmls 5.15.202 #4 Thu Mar 12 06:03:34 CET 2026 x86_64 GNU/Linux

uChaos128 v0.5.6 w/sb!/pr s:0, q:1, d+p(0):3+1 ns, r:31, 1:16, Z:19
67c7e7e9c3410d5b0c5b46620335171d --> /dev/random by uChaos
Active console: ttyS0, entropy: 1 -> 256 bits
[ 0.520368] random: crng init done

Load, init with 8-zeros and test the /dev/uchaos
--------------------------------------------------------------------------------

filename: /lib/modules/uchaos_dev.ko
author: Roberto A. Foglietta <roberto.foglietta@gmail.com>>>
description: Stochastic jitter-based chaos stream device
license: GPL v2
parm: loop_mult: Number of repetitions of the hashing loop (default=1)
parm: min_delta: Minimum delta otherwise do an extra passage (default=3)
parm: init_runs: Number of initial runs for stat stabilization (default=7)
version: 0.5.3
srcversion: 40F787AC1477BBB531EAD53
depends:
vermagic: 5.15.202 mod_unload

16777216 bytes (16.0MB) copied, 0.665947 seconds, 24.0MB/s

BFN using PractRand version 0.96
RNG RNG_stdin64, seed = unknown
test set core, folding standard (64 bit)

rng=RNG_stdin64, seed=unknown
length 16 megabytes (2^24 bytes), time 1.4 seconds
no anomalies in 147 test result(s)

--------------------------------------------------------------------------------

      ===============================================
      Welcome to Minimal Linux System for QEMU x86_64
       System dmsg time was 0.47s at the 1st console
      ===============================================
       type command 'reboot -f' to shutdown this VM

root@u-bmls:/# dd if=/dev/uchaos bs=8k count=1M of=/d.ata
1048576+0 records in
1048576+0 гecords out
8589934592 bytes (8.0GB) copied, 2058.133844 seconds, 4.0MB/s

root@u-bmls:/# RNG_test.gz.sh stdin64 </d.ata
BFN using PractRand version 0.96
RNG RNG_stdin64, seed = unknown
test set core, folding standard (64 bit)

rng=RNG_stdin64, seed=unknown 
length= 32 megabytes (2^25 bytes), time 2.7 seconds 
no anomalies in 159 test result(s)

rng=RNG_stdin64, seed=unknown
length= 512 megabytes (2^29 bytes), time= 58.8 seconds 
no anomalies in 213 test result(s)

rng-RNG_stdin64, seed=unknown 
length 1 gigabyte (2^30 bytes), time= 107 seconds 
no anomalies in 227 test result(s)

rng=RNG_stdin64, seed=unknown 
length 2 gigabytes (2^31 bytes), time= 199 seconds 
no anomalies in 242 test result(s)

rng-RNG_stdin64, seed=unknown 
length 4 gigabytes (2^32 bytes), time= 380 seconds 
no anomalies in 256 test result(s)

rng=RNG_stdin64, seed=unknown 
length 8 gigabytes (2^33 bytes), time= 735 seconds 
no anomalies in 270 test result(s)
```

---

(c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, CC BY-ND-NC 4.0

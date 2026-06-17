## uchaosys.qemu

This folder contains a binary copy of the embedded system (bzImage **v0.6.3** + glib-musl static elf64 uqemu **v0.6.6** + initramfs w/ uchaos .ko & kit **v0.7.0**) provided by sources and compilation recipes available in this project:

- [github.com/robang74/uchaosys](https://github.com/robang74/uchaosys/tree/v063)

The project above linked is able to provide the same output in about 20m, depending on the download transfer rate. Unfortunately, these days many GNU repositories are experiencing severe downtime.

Considering that the system footprint is below 2MB, offering a binary sample makes sense independently from the outages. This snapshot is not supposed to be updated often, therefore refers to the above project link.

### Quick start

```sh
sh start.sh -qm 32

sh uckaos.gz.sh 128  | ent
sh umkaos32.gz.sh 18 | ent

sh uckaos.gz.sh $((128 << 10)) |
  sh practrand_rng_test.gz.sh stdin64
sh umkaos32.gz.sh 28 |
  sh practrand_rng_test.gz.sh stdin64
```

The `uqemu` system emulator, here provided, supports `microvm` and `q35` machines, the `tgc` (sw) and the `kvm` (hw) acceleration, as long as the kernel module for `kvm` support and userland access privileges are granted, obviously.

It is a "frankestain" glibc-musl elf64 static binary which has **not** been extensively tested and it was designed for embedded systems in mind, not desktops. However, it passed the self-contained self-hosted test which can be considered the ultimate health check in terms of a self-sufficient static binary: it works also for the embedded systems for which it has been designed for.

**Warning**: the uchaosys v0.7.0 correctly includes the uchaos_dev.ko v0.6.5, despite the two version seems similar they are indipendent. While `ent` is a pseudorandom number sequence test provided by 3rd party, as well as the PractRand suite.

### License

- **`(c)`** 2026 – Roberto A. Foglietta &lt;roberto.foglietta@gmail.com&gt;

The overall license for the uChaoSys binaries is dependent on the system components thus the GPLv2 is the reference as the most demanding license among those involved as long as "GPLv2 or later" means GPLv2 as an option.

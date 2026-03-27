## uchaosys.qemu

This folder contains a binary copy of the embedded system v0.6.3 provided by sources and compilation recipes available in this project:

- [github.com/robang74/uchaosys](https://github.com/robang74/uchaosys/tree/v063)

The project above linked is able to provide the same output in about 20m, depending on the download transfer rate. Unfortunately, these days many GNU repositories are experiencing severe downtime.

Considering that the system footprint is below 2MB, offering a binary sample makes sense independently from the outages. This snapshot is not supposed to be updated often, therefore refers to the above project link.

### Quick start

```sh
tar xvzf u*-roms.tgz
sh start.sh -q -m 32
```

The uqemu system emulator, here provided, supports microvm and q35 machines, the tgc (sw) and the kvm (hw) acceleration, provided the kernel module for kvm support and userland access privileges granted.

It is a "frankestain" glibc-musl elf64 static binary which has **not** been extensively tested and it was designed for embedded systems in mind, not desktops. However, it passed the self-contained self-hosted test which can be considered the ultimate health check in terms of a self-sufficient static binary: it works also for the embedded systems for which it has been designed for.

### License

- **`(c)`** 2026 – Roberto A. Foglietta &lt;roberto.foglietta@gmail.com&gt;

The overall license for the uChaoSys binaries is dependent on the system components thus the GPLv2 is the reference as the most demanding license among those involved as long as "GPLv2 or later" means GPLv2 as an option.

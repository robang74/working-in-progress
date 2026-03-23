# uchaosys.qemu

This folder contains a binary copy of the embedded system provided by sources and compilation recipes available in this project:

- [github.com/robang74/uchaosys](https://github.com/robang74/uchaosys)

The project above linked is able to provide the same output in about 20m, depending on the download transfer rate. Unfortunately, these days many GNU repositories are experiencing severe downtime.

Considering that the system footprint is below 2MB, offering a binary sample makes sense independently from the outages. This snapshot is not supposed to be updated often, therefore refers to the above project link.

### Quick start

```sh
sh start.sh -q -m 32
```

## License

The overall license for the uChaoSys binaries is dependent on the system components thus the GPLv2 is the reference as the most demanding license among those involved as long as "GPLv2 or later" means GPLv2 as an option.

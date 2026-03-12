obj-m += uchaos_dev.o

# Point to your compiled kernel source or headers
KDIR := linux-kernel/
PWD  := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_prepare modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Define the base module name here
MODULE_NAME := uchaos_dev

obj-m += $(MODULE_NAME).o

# Add GDB flags to force symbol generation during compilation
# EXTRA_CFLAGS += -g -O0 -Wall

# Point to your compiled kernel source or headers
KO_FILE := $(MODULE_NAME).ko
KDIR := linux-kernel/
PWD  := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_prepare modules

dist: default
#	@echo "Stripping debug symbols from uchaos_dev.ko..."
#	strip --strip-debug $(KO_FILE)
	@echo "Compressing with gzip -9 ... "
	gzip -9 -c $(KO_FILE) > $(KO_FILE).gz
	@echo -n "Final size: "
	@du -b $(KO_FILE).gz | tr '\t' ' '

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f $(KO_FILE).gz

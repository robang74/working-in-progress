# Define the base module name here
MODULE_NAME := uchaos_dev

obj-m += $(MODULE_NAME).o

# Add GDB flags to force symbol generation during compilation
# EXTRA_CFLAGS += -g -O0 -Wall

KERNVER ?= $(shell uname -r)

# Point to your compiled kernel source or headers
KO_FILE := $(MODULE_NAME).ko
KDIR := linux-kernel/
PWD  := $(shell pwd)

KMAP := $(KDIR)/System.map System.map
ADDR := 0x$(shell grep " _credit_init_bits" $(KMAP) 2>/dev/null |\
            tail -n1 | cut -d' ' -f1 | cut -d: -f2)
ccflags-y += -D_CREDIT_INIT_ADDR=$(ADDR)

KCFG  := $(KDIR)/.config .config
KASLR := $(shell grep "CONFIG_RANDOMIZE_BASE=y" $(KCFG) 2>/dev/null)
ifneq ($(KASLR),)
    KASLR := 1
    ccflags-y += -D_KASLR_ENABLED=1
else
    KASLR := 0
    ccflags-y += -D_KASLR_ENABLED=0
endif

PRINTK := 0x$(shell grep -w "_printk" $(KMAP) 2>/dev/null |\
             tail -n1 | cut -d' ' -f1 | cut -d: -f2)
ccflags-y += -D_STATIC_PRINTK=$(PRINTK)

default:
	@echo "_CREDIT_INIT_ADDR=$(ADDR) w/ KASLR=$(KASLR),$(PRINTK)"
	$(MAKE) -C $(KDIR) M=$(PWD) modules_prepare modules

dist: default
	@echo "Stripping debug symbols from uchaos_dev.ko..."
	strip --strip-debug $(KO_FILE)
	@echo "Compressing with gzip -9 ... "
	gzip -9 -c $(KO_FILE) > $(KO_FILE).gz
	@echo -n "Final size: "
	@du -b $(KO_FILE).gz | tr '\t' ' '

sign: default
	@echo "Signing the kernel module requires root priviledges"
	sudo /usr/src/linux-headers-$(KERNVER)/scripts/sign-file sha256 /var/lib/shim-signed/mok/MOK.priv /var/lib/shim-signed/mok/MOK.der $(KO_FILE) && modinfo $(KO_FILE)
	@echo ""
	@echo "The signature is valid only if it is enrolled in UEFI"
	@echo "Check the signature with 'sudo mokutil --list-enrolled'"

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f $(KO_FILE).gz

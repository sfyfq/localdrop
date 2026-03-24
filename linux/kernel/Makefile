obj-m += localdrop.o
localdrop-y := main.o \
				localdrop_ioctl.o \
				localdrop_dev.o \
				access_control.o \
				interceptor.o \
				ed25519/src/fe.o \
				ed25519/src/ge.o \
				ed25519/src/sc.o \
				ed25519/src/sha512.o \
				ed25519/src/keypair.o \
				ed25519/src/sign.o
ccflags-y += -I$(src)/ed25519/src

PWD := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod localdrop.ko

unload:
	sudo rmmod localdrop

reload: unload load

log:
	sudo dmesg --follow --human

.PHONY: all clean load unload reload log
obj-m += localdrop_interceptor.o
localdrop_interceptor-y := main.o interceptor.o

PWD := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod localdrop_interceptor.ko

unload:
	sudo rmmod localdrop_interceptor

reload: unload load

log:
	sudo dmesg --follow --human

.PHONY: all kernel clean load unload reload log
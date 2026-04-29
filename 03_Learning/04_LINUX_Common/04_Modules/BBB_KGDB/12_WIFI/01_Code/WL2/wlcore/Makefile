# SPDX-License-Identifier: GPL-2.0
wlc-y		:= main.o cmd.o io.o event.o tx.o rx.o ps.o acx.o ops.o wl18.o \
			  boot.o init.o debugfs.o scan.o sysfs.o vendor_cmd.o \
			  sdio.o
obj-m += wlc.o

KDIR := /home/vinh/build_BBB_custom/linux-stable-rcn-ee
PWD := $(shell pwd)
CROSS_COMPILE := /home/vinh/build_BBB_custom/gcc-11.3.0-nolibc/arm-linux-gnueabi/bin/arm-linux-gnueabi-

# Add debug flags
# ccflags-y += -O0 -g -fsanitize=address -fno-omit-frame-pointer
# ccflags-y += -O0 -g 
all:
	$(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

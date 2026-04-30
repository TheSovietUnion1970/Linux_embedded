# SPDX-License-Identifier: GPL-2.0
wl18-y		:= main.o io.o
obj-m += wl18.o

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

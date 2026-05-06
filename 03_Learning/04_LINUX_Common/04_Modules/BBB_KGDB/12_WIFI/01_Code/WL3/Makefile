# Makefile

KDIR := /home/vinh/build_BBB_custom/linux-stable-rcn-ee
PWD := $(shell pwd)
CROSS_COMPILE := /home/vinh/build_BBB_custom/gcc-11.3.0-nolibc/arm-linux-gnueabi/bin/arm-linux-gnueabi-

# ==================== SDIO Glue Module ====================
s-y := sdio.o
obj-m += s.o

# ==================== Main wl3 Module ====================
wl3-y := wl18xx.o main.o cmd.o io.o event.o tx.o rx.o ps.o acx.o ops.o wl18.o \
         boot.o init.o scan.o vendor_cmd.o

obj-m += wl3.o

all:
	$(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# sudo cp *.c *.h Makefile /home/vinh/D/LINUX/Linux_embedded/03_Learning/04_LINUX_Common/04_Modules/BBB_KGDB/12_WIFI/01_Code/WL3

# sudo cp -r wl18xx wlcore /home/vinh/build_BBB_custom/linux-stable-rcn-ee/wl3
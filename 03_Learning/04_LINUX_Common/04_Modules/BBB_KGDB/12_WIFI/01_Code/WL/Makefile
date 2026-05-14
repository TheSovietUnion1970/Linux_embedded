# Makefile

KDIR := /home/vinh/build_BBB_custom/linux-stable-rcn-ee
PWD := $(shell pwd)
CROSS_COMPILE := /home/vinh/build_BBB_custom/gcc-11.3.0-nolibc/arm-linux-gnueabi/bin/arm-linux-gnueabi-

# ==================== SDIO Glue Module ====================
obj-m += sdio.o

# ==================== Main wl3 Module ====================
wifi0-y := wl18xx.o main.o cmd.o io.o event.o tx.o rx.o ps.o acx.o ops.o wl18.o \
         boot.o init.o scan.o 

obj-m += wifi0.o

all:
	$(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# sudo cp *.c *.h Makefile /home/vinh/D/LINUX/Linux_embedded/03_Learning/04_LINUX_Common/04_Modules/BBB_KGDB/12_WIFI/01_Code/WL

# sudo cp *.c *.h /home/vinh/build_BBB_custom/linux-stable-rcn-ee/wl3
f-y := t2.o u1.o
obj-m += f.o

KDIR := /home/vinh/build_BBB_custom/linux-stable-rcn-ee
PWD := $(shell pwd)
CROSS_COMPILE := /home/vinh/build_BBB_custom/gcc-11.3.0-nolibc/arm-linux-gnueabi/bin/arm-linux-gnueabi-

# Add debug flags
ccflags-y += -O0 -g

all:
	$(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean


# === Rebuild device tree ===

# export CC_x86_for_arm=/home/vinh/build_BBB_custom/gcc-11.3.0-nolibc/arm-linux-gnueabi/bin/arm-linux-gnueabi-
# make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} am335x-boneblack.dtb
# scp ~/build_BBB_custom/linux-stable-rcn-ee/arch/arm/boot/dts/am335x-boneblack.dtb debian@192.168.137.2:/boot/dtbs/5.15.177/

# === Update U-boot to load new dtb ===
# (Press SPACE to abort the default u-boot) -> '=>'

# setenv fdtfile am335x-boneblack.dtb
# saveenv
# boot

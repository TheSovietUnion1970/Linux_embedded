export CC_x86_for_arm=/home/vinh/build_BBB_custom/gcc-11.3.0-nolibc/arm-linux-gnueabi/bin/arm-linux-gnueabi-
make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} am335x-boneblack.dtb
scp /home/vinh/build_BBB_custom/linux-stable-rcn-ee/arch/arm/boot/dts/am335x-boneblack.dtb debian@192.168.137.2:/boot/dtbs/5.15.177+/

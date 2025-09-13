# need to make sure to have modules.symvers 

export CC_x86_for_arm=/home/vinh/build_BBB_custom/gcc-11.3.0-nolibc/arm-linux-gnueabi/bin/arm-linux-gnueabi-

echo "Rebuild *.ko"
sudo make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} -C /home/vinh/build_BBB_custom/linux-stable-rcn-ee M=drivers/usb/musb modules
sudo make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} -C /home/vinh/build_BBB_custom/linux-stable-rcn-ee M=drivers/usb/phy modules
sudo make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} -C /home/vinh/build_BBB_custom/linux-stable-rcn-ee M=drivers/usb/core modules

echo "Cpy *.ko to BBB"
scp drivers/usb/musb/*.ko debian@192.168.137.2:/home/debian
scp drivers/usb/phy/*.ko debian@192.168.137.2:/home/debian
scp drivers/usb/core/*.ko debian@192.168.137.2:/home/debian

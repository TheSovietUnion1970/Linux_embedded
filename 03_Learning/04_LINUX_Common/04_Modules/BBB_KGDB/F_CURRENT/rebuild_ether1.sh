# need to make sure to have modules.symvers 

export CC_x86_for_arm=/home/vinh/build_BBB_custom/gcc-11.3.0-nolibc/arm-linux-gnueabi/bin/arm-linux-gnueabi-

echo "Rebuild  drivers/net/ethernet/ti/*.ko"
sudo make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} -C /home/vinh/build_BBB_custom/linux-stable-rcn-ee M=drivers/net/ethernet/ti modules
sudo make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} -C /home/vinh/build_BBB_custom/linux-stable-rcn-ee M=drivers/phy/ti modules
sudo make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} -C /home/vinh/build_BBB_custom/linux-stable-rcn-ee M=drivers/net/phy modules
sudo make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} -C /home/vinh/build_BBB_custom/linux-stable-rcn-ee M=drivers/net/mdio modules

echo "Cpy *.ko to BBB"
scp \
  drivers/net/ethernet/ti/*.ko \
  drivers/phy/ti/*.ko \
  drivers/net/phy/*.ko \
  drivers/net/mdio/*.ko \
  debian@192.168.137.2:/home/debian/ether_modules/share/


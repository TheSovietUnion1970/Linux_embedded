export DIR=/home/vinh
export CC_x86_for_arm=/home/vinh/build_BBB_custom/gcc-11.3.0-nolibc/arm-linux-gnueabi/bin/arm-linux-gnueabi-
# export CC_x86_for_arm=arm-linux-gnueabihf-

${CC_x86_for_arm}gcc --version
sudo apt-get install flex bison build-essential
mkdir -p ${DIR}/media_home/rootfs
...

# inside ${DIR}/BBB/linux-stable-rcn-ee
cd linux-stable-rcn-ee
make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} -j$(nproc) clean

make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} olddefconfig
# (CHECK) - CONFIG_GPIO_OMAP, CONFIG_MODULES=, CONFIG_LOCALVERSION

KERNEL_VERSION=$(make kernelrelease)
if [ -z "$KERNEL_VERSION" ]; then
    echo "Error: Failed to determine kernel version using make kernelrelease."
    exit 1
fi
export KERNEL_VERSION
echo "Kernel version set to: $KERNEL_VERSION"

# make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} -j$(nproc) zImage
echo " === Build zImage === "
make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} -j$(nproc) zImage 
echo " === Done zImage === "
# (CHECK) - ls -lh ./arch/arm/boot/zImage

# make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} -j$(nproc) modules
make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} -j$(nproc) modules 
# (CHECK) - ls -lh ./drivers/gpio/gpio-omap.ko

sudo rm -rf /tmp/modules
make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} INSTALL_MOD_PATH=/tmp/modules  modules_install
# (CHECK) - ls -lh /tmp/modules/lib/modules/${KERNEL_VERSION}/kernel/drivers/gpio/gpio-omap.ko.xz

# copy everything from /tmp/modules/lib/modules/${KERNEL_VERSION}/ -> ${DIR}/media_home/rootfs/lib/modules/${KERNEL_VERSION}/
sudo rm -rf ${DIR}/media_home/rootfs/lib/modules/${KERNEL_VERSION}
sudo mkdir -p ${DIR}/media_home/rootfs/lib/modules/${KERNEL_VERSION}
sudo cp -a /tmp/modules/lib/modules/${KERNEL_VERSION}/* ${DIR}/media_home/rootfs/lib/modules/${KERNEL_VERSION}/
sudo rm -rf /tmp/modules # not use anymore
# (CHECK) - ls -lh ${DIR}/media_home/rootfs/lib/modules/${KERNEL_VERSION}/kernel/drivers/gpio/gpio-omap.ko.xz

# Uncompress the .ko.xz files to .ko and delete Compressed ko
echo "Uncompress the .ko.xz..."
cd ${DIR}/media_home/rootfs/lib/modules/${KERNEL_VERSION}
find . -type f -name "*.ko.xz" -exec sudo unxz {} \;
# (CHECK) - ls -lh ${DIR}/media_home/rootfs/lib/modules/${KERNEL_VERSION}/kernel/drivers/gpio/gpio-omap.ko
# (CHECK) - find ${DIR}/media_home/rootfs/lib/modules/${KERNEL_VERSION} -type f -name "*.ko.xz"

# copy ./../linux-stable-rcn-ee to ${DIR}/media_home/rootfs
sudo mkdir -p ${DIR}/media_home/rootfs/usr/src

# #[1] option more memory
# sudo cp -a ${DIR}/build_BBB_custom/linux-stable-rcn-ee ${DIR}/media_home/rootfs/usr/src/linux-headers-${KERNEL_VERSION}

#[2] option less memory
sudo rsync -aAXH ${DIR}/build_BBB_custom/linux-stable-rcn-ee/Makefile ${DIR}/media_home/rootfs/usr/src/linux-headers-${KERNEL_VERSION}/
sudo rsync -aAXH ${DIR}/build_BBB_custom/linux-stable-rcn-ee/Module.symvers ${DIR}/media_home/rootfs/usr/src/linux-headers-${KERNEL_VERSION}/
sudo rsync -aAXH ${DIR}/build_BBB_custom/linux-stable-rcn-ee/arch ${DIR}/media_home/rootfs/usr/src/linux-headers-${KERNEL_VERSION}/
sudo rsync -aAXH ${DIR}/build_BBB_custom/linux-stable-rcn-ee/include ${DIR}/media_home/rootfs/usr/src/linux-headers-${KERNEL_VERSION}/
sudo rsync -aAXH ${DIR}/build_BBB_custom/linux-stable-rcn-ee/scripts ${DIR}/media_home/rootfs/usr/src/linux-headers-${KERNEL_VERSION}/
sudo rsync -aAXH ${DIR}/build_BBB_custom/linux-stable-rcn-ee/drivers ${DIR}/media_home/rootfs/usr/src/linux-headers-${KERNEL_VERSION}/


sudo rm ${DIR}/media_home/rootfs/lib/modules/${KERNEL_VERSION}/build
sudo rm ${DIR}/media_home/rootfs/lib/modules/${KERNEL_VERSION}/source
sudo ln -sf /usr/src/linux-headers-${KERNEL_VERSION} ${DIR}/media_home/rootfs/lib/modules/${KERNEL_VERSION}/build
sudo ln -sf /usr/src/linux-headers-${KERNEL_VERSION} ${DIR}/media_home/rootfs/lib/modules/${KERNEL_VERSION}/source

#copy ethernet_init.sh
sudo cp -a ${DIR}/build_BBB_custom/ethernet_init.sh ${DIR}/media_home/rootfs/usr/src/ethernet_init.sh
sudo chmod +x ${DIR}/media_home/rootfs/usr/src/ethernet_init.sh

# #		=== initramfs ===
# # Use mkinitramfs configurations in /media/rootfs
# echo "Generating initrd.img..."
# sudo mkdir -p /media/rootfs/
# sudo chroot /media/rootfs/ /bin/bash -c "depmod ${KERNEL_VERSION}"
# sudo chroot /media/rootfs/ /bin/bash -c "mkinitramfs -o /boot/initrd.img-${KERNEL_VERSION} ${KERNEL_VERSION}"

echo "✅ Done!."

# # copy initrd.img from /media/rootfs to /media/rootfs/boot/initrd.img-${KERNEL_VERSION} 
# echo "Copying initrd.img..."
# sudo mkdir -p ${DIR}/media_home/rootfs/boot/
# sudo rsync -aAXH /media/rootfs/boot/initrd.img-${KERNEL_VERSION} ${DIR}/media_home/rootfs/boot/initrd.img-${KERNEL_VERSION} 

# # Step 4: Verify the initramfs contents
# echo "Verifying initramfs contents..."
# zcat ${DIR}/media_home/rootfs/boot/initrd.img-${KERNEL_VERSION} | cpio -t | grep -E "modprobe|cat|sh|init|scripts|gpio-omap|modules.dep"





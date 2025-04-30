# inside /home/vinh/BBB

${CC_x86_for_arm}gcc --version
sudo apt-get install flex bison build-essential
mkdir -p /home/vinh/media_home/rootfs
...

# inside /home/vinh/BBB/linux-stable-rcn-ee
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

make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} -j$(nproc) zImage
# (CHECK) - ls -lh ./arch/arm/boot/zImage

make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} -j$(nproc) modules
# (CHECK) - ls -lh ./drivers/gpio/gpio-omap.ko

sudo rm -rf /tmp/modules
make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} INSTALL_MOD_PATH=/tmp/modules  modules_install
# (CHECK) - ls -lh /tmp/modules/lib/modules/${KERNEL_VERSION}/kernel/drivers/gpio/gpio-omap.ko.xz

# copy everything from /tmp/modules/lib/modules/${KERNEL_VERSION}/ -> /home/vinh/media_home/rootfs/lib/modules/${KERNEL_VERSION}/
sudo rm -rf /home/vinh/media_home/rootfs/lib/modules/${KERNEL_VERSION}
sudo mkdir -p /home/vinh/media_home/rootfs/lib/modules/${KERNEL_VERSION}
sudo cp -a /tmp/modules/lib/modules/${KERNEL_VERSION}/* /home/vinh/media_home/rootfs/lib/modules/${KERNEL_VERSION}/
# (CHECK) - ls -lh /home/vinh/media_home/rootfs/lib/modules/${KERNEL_VERSION}/kernel/drivers/gpio/gpio-omap.ko.xz

# Uncompress the .ko.xz files to .ko and delete Compressed ko
echo "Uncompress the .ko.xz..."
cd /home/vinh/media_home/rootfs/lib/modules/${KERNEL_VERSION}
find . -type f -name "*.ko.xz" -exec sudo unxz {} \;
# (CHECK) - ls -lh /home/vinh/media_home/rootfs/lib/modules/${KERNEL_VERSION}/kernel/drivers/gpio/gpio-omap.ko
# (CHECK) - find /home/vinh/media_home/rootfs/lib/modules/${KERNEL_VERSION} -type f -name "*.ko.xz"

#		=== initramfs ===
# Use mkinitramfs configurations in /media/rootfs
echo "Generating initrd.img..."
sudo chroot /media/rootfs/ /bin/bash -c "depmod ${KERNEL_VERSION}"
sudo chroot /media/rootfs/ /bin/bash -c "mkinitramfs -o /boot/initrd.img-${KERNEL_VERSION} ${KERNEL_VERSION}"

# copy initrd.img from /media/rootfs to /media/rootfs/boot/initrd.img-${KERNEL_VERSION} 
echo "Copying initrd.img..."
sudo rsync -aAXH /media/rootfs/boot/initrd.img-${KERNEL_VERSION} /home/vinh/media_home/rootfs/boot/initrd.img-${KERNEL_VERSION} 

# Step 4: Verify the initramfs contents
echo "Verifying initramfs contents..."
zcat /home/vinh/media_home/rootfs/boot/initrd.img-${KERNEL_VERSION} | cpio -t | grep -E "modprobe|cat|sh|init|scripts|gpio-omap|modules.dep"





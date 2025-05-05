# git clone --depth=1 --branch 5.15.177-bone43 https://github.com/RobertCNelson/linux-stable-rcn-ee.git

export DIR=/home/vinh
export DIR_TARGET=BBB
export DIR_MOUNT=/media/rootfs
export DIR_MOUNT_HOME=media_home_gdb1/rootfs
export CC_x86_for_arm=/home/vinh/BBB/gcc-11.3.0-nolibc/arm-linux-gnueabi/bin/arm-linux-gnueabi-

${CC_x86_for_arm}gcc --version
sudo apt-get install flex bison build-essential
mkdir -p ${DIR}/${DIR_MOUNT_HOME}

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

make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} -j$(nproc) zImage
# (CHECK) - ls -lh ./arch/arm/boot/zImage

make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} -j$(nproc) modules
# (CHECK) - ls -lh ./drivers/gpio/gpio-omap.ko

sudo rm -rf /tmp/modules
make ARCH=arm CROSS_COMPILE=${CC_x86_for_arm} INSTALL_MOD_PATH=/tmp/modules  modules_install
# (CHECK) - ls -lh /tmp/modules/lib/modules/${KERNEL_VERSION}/kernel/drivers/gpio/gpio-omap.ko.xz

# copy everything from /tmp/modules/lib/modules/${KERNEL_VERSION}/ -> ${DIR}/${DIR_MOUNT_HOME}/lib/modules/${KERNEL_VERSION}/
sudo rm -rf ${DIR}/${DIR_MOUNT_HOME}/lib/modules/${KERNEL_VERSION}
sudo mkdir -p ${DIR}/${DIR_MOUNT_HOME}/lib/modules/${KERNEL_VERSION}
sudo cp -a /tmp/modules/lib/modules/${KERNEL_VERSION}/* ${DIR}/${DIR_MOUNT_HOME}/lib/modules/${KERNEL_VERSION}/
# (CHECK) - ls -lh ${DIR}/${DIR_MOUNT_HOME}/lib/modules/${KERNEL_VERSION}/kernel/drivers/gpio/gpio-omap.ko.xz

# Uncompress the .ko.xz files to .ko and delete Compressed ko
echo "Uncompress the .ko.xz..."
cd ${DIR}/${DIR_MOUNT_HOME}/lib/modules/${KERNEL_VERSION}
find . -type f -name "*.ko.xz" -exec sudo unxz {} \;
# (CHECK) - ls -lh ${DIR}/${DIR_MOUNT_HOME}/lib/modules/${KERNEL_VERSION}/kernel/drivers/gpio/gpio-omap.ko
# (CHECK) - find ${DIR}/${DIR_MOUNT_HOME}/lib/modules/${KERNEL_VERSION} -type f -name "*.ko.xz"

# copy ./../linux-stable-rcn-ee to ${DIR}/${DIR_MOUNT_HOME}
sudo mkdir -p ${DIR}/${DIR_MOUNT_HOME}/usr/src
sudo cp -a ${DIR}/${DIR_TARGET}/linux-stable-rcn-ee ${DIR}/${DIR_MOUNT_HOME}/usr/src/linux-headers-${KERNEL_VERSION}

sudo rm ${DIR}/${DIR_MOUNT_HOME}/lib/modules/${KERNEL_VERSION}/build
sudo rm ${DIR}/${DIR_MOUNT_HOME}/lib/modules/${KERNEL_VERSION}/source
sudo ln -sf /usr/src/linux-headers-${KERNEL_VERSION} ${DIR}/${DIR_MOUNT_HOME}/lib/modules/${KERNEL_VERSION}/build
sudo ln -sf /usr/src/linux-headers-${KERNEL_VERSION} ${DIR}/${DIR_MOUNT_HOME}/lib/modules/${KERNEL_VERSION}/source

#copy ethernet_init.sh
sudo cp -a ${DIR}/${DIR_TARGET}/ethernet_init.sh ${DIR}/${DIR_MOUNT_HOME}/usr/src/ethernet_init.sh
sudo chmod +x ${DIR}/${DIR_MOUNT_HOME}/usr/src/ethernet_init.sh

#		=== initramfs ===
# Copy /lib/modules/${KERNEL_VERSION} -> ${DIR_MOUNT}/lib/modules/${KERNEL_VERSION}
echo "Copying /lib/modules/${KERNEL_VERSION} to ${DIR_MOUNT}/lib/modules/..."
sudo mkdir -p ${DIR_MOUNT}/lib/modules
sudo rsync -aAXH ${DIR}/${DIR_MOUNT_HOME}/lib/modules/${KERNEL_VERSION} ${DIR_MOUNT}/lib/modules/

# Use mkinitramfs configurations in ${DIR_MOUNT}
echo "Generating initrd.img..."
sudo chroot ${DIR_MOUNT}/ /bin/bash -c "depmod ${KERNEL_VERSION}"
sudo chroot ${DIR_MOUNT}/ /bin/bash -c "mkinitramfs -o /boot/initrd.img-${KERNEL_VERSION} ${KERNEL_VERSION}"

# copy initrd.img from ${DIR_MOUNT} to ${DIR_MOUNT}/boot/initrd.img-${KERNEL_VERSION} 
echo "Copying initrd.img..."
sudo mkdir -p ${DIR}/${DIR_MOUNT_HOME}/boot/
sudo rsync -aAXH ${DIR_MOUNT}/boot/initrd.img-${KERNEL_VERSION} ${DIR}/${DIR_MOUNT_HOME}/boot/

# Step 4: Verify the initramfs contents
echo "Verifying initramfs contents..."
zcat ${DIR}/${DIR_MOUNT_HOME}/boot/initrd.img-${KERNEL_VERSION} | cpio -t | grep -E "modprobe|cat|sh|init|scripts|gpio-omap|modules.dep"




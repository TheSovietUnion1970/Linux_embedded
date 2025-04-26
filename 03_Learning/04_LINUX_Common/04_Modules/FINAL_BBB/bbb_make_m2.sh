#!/bin/bash

set -e

# === CONFIGURATION ===
export DISK=/dev/sda
export KERNEL_VERSION=5.15.177-bone43
export ROOTFS_TAR=$(ls debian-*-*-armhf-*/armhf-rootfs-*.tar)
export DEPLOY_DIR=./kernelbuildscripts/deploy

# === WIPE FIRST 10MB OF DISK ===
echo "Wiping the first 10MB of $DISK..."
sudo dd if=/dev/zero of=${DISK} bs=1M count=10

# === WRITE BOOTLOADER ===
echo "Writing MLO and u-boot-dtb.img..."
sudo dd if=./u-boot/MLO of=${DISK} count=2 seek=1 bs=128k
sudo dd if=./u-boot/u-boot-dtb.img of=${DISK} count=4 seek=1 bs=384k

# === PARTITION ===
echo "Creating rootfs partition..."
sudo sfdisk ${DISK} <<-__EOF__
4M,,L,*
__EOF__

# === FORMAT PARTITION ===
echo "Formatting ${DISK}1 to ext4..."
sudo mkfs.ext4 -L rootfs -O ^metadata_csum,^64bit ${DISK}1

# === MOUNT ROOTFS ===
echo "Mounting ${DISK}1 to /media/rootfs..."
sudo mkdir -p /media/rootfs/
sudo mount ${DISK}1 /media/rootfs/

# === BACKUP BOOTLOADER FILES ===
sudo mkdir -p /media/rootfs/opt/backup/uboot/
sudo cp -v ./u-boot/MLO ./u-boot/u-boot-dtb.img /media/rootfs/opt/backup/uboot/

# === EXTRACT ROOT FILESYSTEM ===
echo "Extracting rootfs tarball..."
sudo tar xfvp ${ROOTFS_TAR} -C /media/rootfs/
sync

# === SET uEnv.txt ===
echo "Setting uEnv.txt..."
sudo sh -c "echo 'uname_r=${KERNEL_VERSION}' >> /media/rootfs/boot/uEnv.txt"

# === INSTALL KERNEL ===
echo "Installing kernel image..."
sudo cp -v ./linux-stable-rcn-ee/arch/arm/boot/zImage /media/rootfs/boot/vmlinuz-${KERNEL_VERSION}

# === INSTALL DTBs ===
echo "Installing DTBs..."
sudo mkdir -p /media/rootfs/boot/dtbs/${KERNEL_VERSION}/
sudo tar xfv ${DEPLOY_DIR}/${KERNEL_VERSION}-dtbs.tar.gz -C /media/rootfs/boot/dtbs/${KERNEL_VERSION}/

# === INSTALL MODULES ===
echo "Copying kernel modules..."
sudo mkdir -p /media/rootfs/lib/modules/5.15.177+
sudo cp -a /tmp/modules/lib/modules/5.15.177+/* /media/rootfs/lib/modules/5.15.177+/

echo "Running depmod..."
sudo chroot /media/rootfs/ /bin/bash -c "depmod 5.15.177+"

echo "Generating initramfs..."
sudo chroot /media/rootfs/ /bin/bash -c "apt update && apt install -y initramfs-tools"
sudo chroot /media/rootfs/ /bin/bash -c "mkinitramfs -o /boot/initrd.img-${KERNEL_VERSION} 5.15.177+"

# === CREATE FSTAB ===
echo "Writing /etc/fstab..."
sudo sh -c "echo '/dev/mmcblk0p1  /  auto  errors=remount-ro  0  1' >> /media/rootfs/etc/fstab"

# === NETWORK CONFIGURATION ===
echo "Writing /etc/network/interfaces..."
sudo tee /media/rootfs/etc/network/interfaces > /dev/null <<EOF
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet dhcp
EOF

sync

# === UNMOUNT ===
echo "Unmounting..."
sudo umount /media/rootfs

echo "✅ Done! Your SD card is ready."

#!/bin/bash

set -e

# === CONFIGURATION ===
export DISK=/dev/sda
export KERNEL_VERSION=5.15.177-bone43
export ROOTFS_TAR=$(ls debian-*-*-armhf-*/armhf-rootfs-*.tar)
export DEPLOY_DIR=./kernelbuildscripts/deploy

# === INSTALL MODULES ===
echo "Copying kernel modules... Done in previous step"
# sudo mkdir -p /media/rootfs/lib/modules/5.15.177+
# (CHECK - output from bbb_build_m)
# sudo cp -a /tmp/modules/lib/modules/5.15.177+/* /media/rootfs/lib/modules/5.15.177+/

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

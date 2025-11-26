#!/bin/bash

set -e

KERNEL_VERSION="5.15.177+"
export DIR=/home/vinh
mount_path="/media/vinh/rootfs"

echo "Copying kernel modules..."
sudo mkdir -p ${mount_path}/lib/modules/${KERNEL_VERSION}
sudo cp -a ${DIR}/media_home/rootfs/lib/modules/${KERNEL_VERSION}/* \
    ${mount_path}/lib/modules/${KERNEL_VERSION}/

echo "Preparing for initramfs generation..."
# Ensure required mounts inside chroot
sudo mount --bind /dev ${mount_path}/dev
sudo mount -t proc none ${mount_path}/proc
sudo mount -t sysfs none ${mount_path}/sys
sudo mount --bind /run ${mount_path}/run

echo "Installing initramfs-tools inside chroot..."
sudo chroot ${mount_path} /bin/bash -c \
    "apt update && apt install -y initramfs-tools"

echo "Running depmod inside chroot..."
sudo chroot ${mount_path} /bin/bash -c \
    "depmod ${KERNEL_VERSION}"

echo "Generating initramfs..."
sudo chroot ${mount_path} /bin/bash -c \
    "mkinitramfs -o /boot/initrd.img-${KERNEL_VERSION} ${KERNEL_VERSION}"

sync

echo "Unmounting..."
sudo umount ${mount_path}/proc || true
sudo umount ${mount_path}/sys || true
sudo umount ${mount_path}/dev || true
sudo umount ${mount_path}/run || true
sudo umount ${mount_path} || true

echo "✅ Done! Your SD card is ready."


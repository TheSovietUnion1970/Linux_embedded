#!/bin/bash

set -e

export DISK=/dev/sda

sudo umount /media/rootfs
sudo dd if=/dev/zero of=${DISK} bs=1M count=10

echo "✅ Done! Clean."

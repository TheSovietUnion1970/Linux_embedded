# build-vinh.bb - Debian rootfs + your custom kernel + U-Boot

SUMMARY = "Custom BBB image - Debian rootfs + Vinh's kernel + U-Boot"
DESCRIPTION = "Uses your local Debian Bullseye rootfs tarball"

inherit core-image

# Use absolute path to your existing tarball (no need to copy into layer)
SRC_URI += "file:///home/vinh/build_BBB_custom/debian-11.5-minimal-armhf-2022-10-06/armhf-rootfs-debian-bullseye.tar"
ROOTFS_DIR = "/home/vinh/build_BBB_custom/debian-11.5-minimal-armhf-2022-10-06"

# Unpack your Debian tarball as the complete rootfs
ROOTFS_POSTPROCESS_COMMAND += "unpack_debian_rootfs; "

unpack_debian_rootfs() {
    bbwarn "build-vinh -> unpack_debian_rootfs: ${IMAGE_ROOTFS}"
    tar --numeric-owner -xpf ${ROOTFS_DIR}/armhf-rootfs-debian-bullseye.tar -C ${IMAGE_ROOTFS}
}

# Debug messages
do_image_wic:append() {
    bbwarn "build-vinh -> do_image_wic"
}

do_image_tar:append() {
    bbwarn "build-vinh -> do_image_wic"
}

# Optional nice features
IMAGE_FEATURES += "ssh-server-dropbear"
IMAGE_FSTYPES += "wic wic.bmap tar.bz2"

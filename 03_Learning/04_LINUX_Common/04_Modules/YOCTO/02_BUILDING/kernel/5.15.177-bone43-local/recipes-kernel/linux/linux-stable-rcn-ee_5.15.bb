# linux-stable-rcn-ee_5.15.bb
# Local source – disables both problematic QA checks

SUMMARY = "BeagleBone enhanced kernel from Robert Nelson"
DESCRIPTION = "Patched linux-stable for AM335x/BeagleBone with capes support"

LICENSE = "GPL-2.0-only"

# Skip license checksum QA (safe for local/custom source)
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"
# LICENSE_CHECKSUM_ALLOW_EMPTY = "1"


# Skip kernel version sanity check (safe for custom/local tree)
KERNEL_VERSION_SANITY_SKIP = "1"

inherit kernel kernel-yocto siteinfo

# Disable fetch/unpack/check-out – local source only
do_fetch[noexec] = "1"
do_unpack[noexec] = "1"
do_kernel_checkout[noexec] = "1"

# Local source – direct path
SRC_URI = "file:///home/vinh/build_BBB_custom/linux-stable-rcn-ee"

# Set here to use custom config
SRC_URI += "file:///home/vinh/Yocto/poky/meta-mybbb-local/recipes-kernel/linux/files/defconfig"

# Direct path to your kernel tree
S = "/home/vinh/build_BBB_custom/linux-stable-rcn-ee"


DEPENDS += "lzop-native"

LINUX_VERSION ?= "5.15"
PV = "${LINUX_VERSION}+git0c84cce45188aea4ffaf839b7f6a14a900ca2df3"

COMPATIBLE_MACHINE = "(beaglebone-yocto|beaglebone-black|am335x-evm)"

KERNEL_DEVICETREE = " \
    am335x-bonegreen-wireless.dtb \
    am335x-pepper.dtb \
    am335x-boneblack.dtb \
    am335x-bonegreen.dtb \
"

PROVIDES += "virtual/kernel"
RPROVIDES:${PN} += "kernel-vmlinux kernel-base kernel-modules kernel-image-zImage"

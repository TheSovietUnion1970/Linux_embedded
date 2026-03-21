# linux-stable-rcn-ee_5.15.bb - Fixed for local source
SUMMARY = "BeagleBone enhanced kernel from Robert Nelson"
DESCRIPTION = "Patched linux-stable for AM335x/BeagleBone with capes support"

LICENSE = "GPL-2.0-only"

# === STRONG PROTECTION FOR LOCAL SOURCE ===
CLEANBROKEN = "1"
do_clean[noexec] = "1"
do_cleanall[noexec] = "1"
do_cleansstate[noexec] = "1"
do_populate_lic[noexec] = "1"

# Skip unnecessary checks for local source
KERNEL_VERSION_SANITY_SKIP = "1"
LIC_FILES_CHKSUM = ""

inherit kernel siteinfo

# Disable fetch/unpack for local source
do_fetch[noexec] = "1"
do_unpack[noexec] = "1"
do_kernel_checkout[noexec] = "1"

# === LOCAL SOURCE ===
SRC_URI = "file:///home/vinh/build_BBB_custom/linux-stable-rcn-ee"

# Your custom defconfig (relative path inside the layer)
SRC_URI += "file://defconfig"

S = "/home/vinh/build_BBB_custom/linux-stable-rcn-ee"
B = "${WORKDIR}/build"

DEPENDS += "lzop-native"

LINUX_VERSION ?= "5.15"
PV = "${LINUX_VERSION}-bone43"

COMPATIBLE_MACHINE = "(beaglebone-yocto|beaglebone-black|am335x-evm)"

KERNEL_DEVICETREE = " \
    am335x-boneblack.dtb \
    am335x-bonegreen.dtb \
    am335x-bonegreen-wireless.dtb \
    am335x-pepper.dtb \
"

PROVIDES += "virtual/kernel"
RPROVIDES:${PN} += "kernel-vmlinux kernel-base kernel-modules kernel-image-zImage"

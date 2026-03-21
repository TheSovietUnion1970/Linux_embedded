# linux-stable-rcn-ee_5.15.bb - Fixed for local source
SUMMARY = "BeagleBone enhanced kernel from Robert Nelson"
DESCRIPTION = "Patched linux-stable for AM335x/BeagleBone with capes support"

LICENSE = "GPL-2.0-only"

inherit kernel

# Skip license checksum QA (safe for local/custom source)
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"
# LICENSE_CHECKSUM_ALLOW_EMPTY = "1"

# inherit kernel

do_symlink_kernsrc[noexec] = "1"
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


# === Custom Kernel Tasks for Local Source ===

# Set correct values for ARM BeagleBone
ARCH = "arm"
KERNEL_IMAGETYPE = "zImage"

do_configure() {
    bbwarn "kernel-v -> Starting do_configure"
    oe_runmake -C ${S} O=${B} olddefconfig
}

do_compile() {
    bbwarn "kernel-v -> Starting do_compile"
    oe_runmake -C ${S} O=${B} all
}

do_compile_kernelmodules() {
    bbwarn "kernel-v -> Starting do_compile_kernelmodules"
    oe_runmake -C ${S} O=${B} modules
}

do_deploy:append() {
    bbwarn "kernel-v -> Done do_deploy"
}


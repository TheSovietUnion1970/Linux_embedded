SUMMARY = "BeagleBone enhanced kernel from Robert Nelson"
DESCRIPTION = "Patched linux-stable for AM335x/BeagleBone with capes support"

LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

inherit kernel

# For remote
# SRC_URI = "git://github.com/RobertCNelson/linux-stable-rcn-ee.git;protocol=https;branch=6.18.16-bone23;nobranch=1"
# SRCREV = "e19127ebdad3c32ddd93ee4f5715e89c7a0f8a30"

# For local
SRC_URI = "file:///home/vinh/build_BBB_custom/linux-stable-rcn-ee;subdir=git;protocol=file"

# For Config
SRC_URI += "file://defconfig"

# For using config
do_configure:prepend() {
    cp ${WORKDIR}/defconfig ${B}/.config
}

do_configure:append() {
    oe_runmake_call -C ${S} CC="${KERNEL_CC}" O=${B} olddefconfig
}

# For providing dependencies
DEPENDS += "lzop-native"


LINUX_VERSION ?= "6.18"
PV = "${LINUX_VERSION}+git${SRCPV}"

S = "${WORKDIR}/git"

COMPATIBLE_MACHINE = "(beaglebone-yocto|beaglebone-black|am335x-evm)"

KERNEL_CONFIG_COMMAND = "oe_runmake_call -C ${S} CC='${KERNEL_CC}' O=${B} olddefconfig"

# Override KERNEL_DEVICETREE to match existing DTS files in the tree (under ti/omap/)
# From your find output: am335x-bonegreen-wireless.dts exists → build its .dtb
# am335x-boneblack.dts does NOT exist in your tree, so do NOT include it!
# If boneblack support is needed, it may come from bonegreen + HDMI include or need patches.
KERNEL_DEVICETREE = " \
    ti/omap/am335x-bonegreen-wireless.dtb \
    ti/omap/am335x-pepper.dtb \
    ti/omap/am335x-boneblack.dtb \
    ti/omap/am335x-bonegreen.dtb \
"



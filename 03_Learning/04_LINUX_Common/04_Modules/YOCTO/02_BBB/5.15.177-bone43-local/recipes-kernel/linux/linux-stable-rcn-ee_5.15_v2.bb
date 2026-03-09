# linux-stable-rcn-ee_5.15.bb
# Build with DEFAULT kernel config (no custom defconfig)

SUMMARY = "BeagleBone enhanced kernel from Robert Nelson"
DESCRIPTION = "Patched linux-stable for AM335x/BeagleBone with capes support"

LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

inherit kernel kernel-yocto siteinfo

# Disable fetch/unpack/check-out – local source only
do_fetch[noexec] = "1"
do_unpack[noexec] = "1"
do_kernel_checkout[noexec] = "1"

# Local source – direct path
SRC_URI = "file:///home/vinh/build_BBB_custom/linux-stable-rcn-ee"

# NO defconfig in SRC_URI → no custom config copied

S = "/home/vinh/build_BBB_custom/linux-stable-rcn-ee"

# Do NOT copy custom defconfig – let kernel use its own default
# Remove or comment these lines:
# do_configure:prepend() {
#     cp ${WORKDIR}/defconfig ${B}/.config
# }

# Still run olddefconfig to finalize the default config
do_configure:append() {
    oe_runmake_call -C ${S} CC="${KERNEL_CC}" O=${B} olddefconfig
}

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

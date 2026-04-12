# kernel-v_5.15.bb - Clean version for local source + proper module packaging
SUMMARY = "BeagleBone enhanced kernel from Robert Nelson"
DESCRIPTION = "Patched linux-stable for AM335x/BeagleBone with capes support"

LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

inherit kernel

do_symlink_kernsrc[noexec] = "1"
do_fetch[noexec] = "1"
do_unpack[noexec] = "1"
do_kernel_checkout[noexec] = "1"
CLEANBROKEN = "1"
do_clean[noexec] = "1"

SRC_URI = "file:///home/vinh/build_BBB_custom/linux-stable-rcn-ee"
SRC_URI += "file://defconfig"

S = "/home/vinh/build_BBB_custom/linux-stable-rcn-ee"
B = "${WORKDIR}/build"

DEPENDS += "lzop-native xz-native"

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

ARCH = "arm"
KERNEL_IMAGETYPE = "zImage"

# === Custom Tasks ===

do_configure() {
    bbwarn "kernel-v -> Starting do_configure"
    oe_runmake -C ${S} O=${B} mrproper
    oe_runmake -C ${S} O=${B} olddefconfig
}

do_compile() {
    bbwarn "kernel-v -> Starting do_compile"
    oe_runmake -C ${S} O=${B} zImage
}

do_compile_kernelmodules() {
    bbwarn "kernel-v -> Starting do_compile_kernelmodules"
    oe_runmake -C ${S} O=${B} modules
}

# Install modules to image directory
do_install:append() {
    bbwarn "kernel-v -> Installing kernel modules"
    oe_runmake -C ${S} O=${B} INSTALL_MOD_PATH=${D} modules_install
}

# Fix QA error: explicitly ship all module files
FILES:${PN} += "${base_libdir}/modules/*"
FILES:${PN}-modules = "${base_libdir}/modules/*"

INSANE_SKIP:${PN} += "installed-vs-shipped"

# Deploy modules as tarball
do_deploy:append() {
    bbwarn "kernel-v -> Deploying kernel modules tarball"
    mkdir -p ${DEPLOYDIR}
    tar -C ${D}/lib/modules -czf ${DEPLOYDIR}/modules-${PV}.tgz .
    ln -sf modules-${PV}.tgz ${DEPLOYDIR}/modules-${MACHINE}.tgz
}

do_deploy:append() {
    bbwarn "kernel-v -> Done do_deploy"
}

# Final working U-Boot for BeagleBone Black - Local Source
require u-boot-common.inc
require u-boot.inc

SUMMARY = "U-Boot for BeagleBone Black (local source)"
DESCRIPTION = "U-Boot from local directory for AM335x / BeagleBone Black"

LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://Licenses/README;md5=5a7450c57ffe5ae63fd732446b988025"

# === LOCAL SOURCE SETTINGS ===
SRC_URI = "file:///home/vinh/build_BBB_custom/u-boot"
do_fetch[noexec] = "1"
do_unpack[noexec] = "1"
S = "/home/vinh/build_BBB_custom/u-boot"
B = "${WORKDIR}/build"

# === BeagleBone Black specific ===
UBOOT_MACHINE = "am335x_evm_defconfig"

# Disable initial-env to avoid errors
UBOOT_INITIAL_ENV = ""

# SPL and main image names
SPL_BINARY = "spl/u-boot-spl.bin"
UBOOT_BINARY = "u-boot.img"

# Proper out-of-tree build (use "all" instead of "MLO" target)
do_configure() {
    oe_runmake -C ${S} O=${B} ${UBOOT_MACHINE}
}

do_compile() {
    oe_runmake -C ${S} O=${B} all
}

# Deploy MLO and u-boot.img correctly
do_deploy:append() {
    install -d ${DEPLOYDIR}

	# Prioritize the wrapped MLO
    if [ -f "${B}/MLO" ]; then
        bbwarn "DEBUG: Found MLO at ${B}/MLO → copying to deploy"
        install -m 0644 ${B}/MLO ${DEPLOYDIR}/MLO
    fi

    # Main U-Boot image
    if [ -f "${B}/u-boot.img" ]; then
        bbwarn "DEBUG: Found u-boot.img at ${B} → copying to deploy"
        install -m 0644 ${B}/u-boot.img ${DEPLOYDIR}/u-boot.img
    elif [ -f "${B}/u-boot.bin" ]; then
    	bbwarn "DEBUG: Found u-boot.bin at ${B} → copying to deploy"
        install -m 0644 ${B}/u-boot.bin ${DEPLOYDIR}/u-boot.img
    fi
}

PROVIDES += "virtual/bootloader u-boot"

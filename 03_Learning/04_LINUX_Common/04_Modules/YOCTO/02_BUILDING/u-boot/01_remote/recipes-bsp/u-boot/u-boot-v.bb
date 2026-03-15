# Custom U-Boot for BeagleBone Black using BeagleBoard fork
require u-boot-common.inc
require u-boot.inc

SUMMARY = "U-Boot for BeagleBone Black (am335x_evm_defconfig)"
DESCRIPTION = "U-Boot from BeagleBoard fork for AM335x / BeagleBone Black"

LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://Licenses/README;md5=5a7450c57ffe5ae63fd732446b988025"

# Source from BeagleBoard fork
SRCREV = "5509547b2c249c9a8641b5a564762e5c80f4a96b"
SRC_URI = "git://github.com/beagleboard/u-boot.git;protocol=https;branch=v2022.04-bbb.io-am335x-am57xx"

# === BeagleBone specific settings ===
UBOOT_MACHINE = "am335x_evm_defconfig"

# Tell U-Boot where the SPL and main image are
SPL_BINARY = "spl/u-boot-spl.bin"
UBOOT_BINARY = "u-boot.img"

# Force MLO and u-boot.img in deploy directory (critical for AM335x)
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

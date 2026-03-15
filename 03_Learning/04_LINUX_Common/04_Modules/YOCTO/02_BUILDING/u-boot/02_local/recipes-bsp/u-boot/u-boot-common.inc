HOMEPAGE = "http://www.denx.de/wiki/U-Boot/WebHome"
DESCRIPTION = "U-Boot, a boot loader for Embedded boards based on PowerPC, \
ARM, MIPS and several other processors, which can be installed in a boot \
ROM and used to initialize and test the hardware or to download and run \
application code."
SECTION = "bootloaders"
DEPENDS += "flex-native bison-native"

LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://Licenses/README;md5=5a7450c57ffe5ae63fd732446b988025"
PE = "1"

# =============================================
# LOCAL SOURCE CONFIGURATION (no git fetch)
# =============================================
SRC_URI = "file:///home/vinh/build_BBB_custom/u-boot"

# Skip network fetch and unpack - source is already on disk
do_fetch[noexec] = "1"
do_unpack[noexec] = "1"

# Point S directly to your local directory
S = "/home/vinh/build_BBB_custom/u-boot"
B = "${WORKDIR}/build"

inherit pkgconfig

do_configure[cleandirs] = "${B}"

SUMMARY = "Universal Boot Loader for embedded devices"

PACKAGE_ARCH = "${MACHINE_ARCH}"

DEPENDS += "${@bb.utils.contains('UBOOT_ENV_SUFFIX', 'scr', 'u-boot-mkimage-native', '', d)}"

inherit uboot-config uboot-extlinux-config uboot-sign deploy python3native kernel-arch

EXTRA_OEMAKE = 'CROSS_COMPILE=${TARGET_PREFIX} CC="${TARGET_PREFIX}gcc ${TOOLCHAIN_OPTIONS}" V=1'
EXTRA_OEMAKE += 'HOSTCC="${BUILD_CC} ${BUILD_CFLAGS} ${BUILD_LDFLAGS}"'
EXTRA_OEMAKE += 'STAGING_INCDIR=${STAGING_INCDIR_NATIVE} STAGING_LIBDIR=${STAGING_LIBDIR_NATIVE}'

PACKAGECONFIG ??= "openssl"
# u-boot will compile its own tools during the build, with specific
# configurations (aka when CONFIG_FIT_SIGNATURE is enabled) openssl is needed as
# a host build dependency.
PACKAGECONFIG[openssl] = ",,openssl-native"

# Allow setting an additional version string that will be picked up by the
# u-boot build system and appended to the u-boot version.  If the .scmversion
# file already exists it will not be overwritten.
UBOOT_LOCALVERSION ?= ""

# Default name of u-boot initial env, but enable individual recipes to change
# this value.
UBOOT_INITIAL_ENV ?= "${PN}-initial-env"


PACKAGE_BEFORE_PN += "${PN}-env ${PN}-extlinux"

RPROVIDES:${PN}-env += "u-boot-default-env"
ALLOW_EMPTY:${PN}-env = "1"
FILES:${PN}-env = " \
    ${@ '${sysconfdir}/${UBOOT_INITIAL_ENV}*' if d.getVar('UBOOT_INITIAL_ENV') else ''} \
    ${sysconfdir}/fw_env.config \
"

FILES:${PN}-extlinux = "${UBOOT_EXTLINUX_INSTALL_DIR}/${UBOOT_EXTLINUX_CONF_NAME}"
RDEPENDS:${PN} += "${@bb.utils.contains('UBOOT_EXTLINUX', '1', '${PN}-extlinux', '', d)}"

FILES:${PN} = "/boot ${datadir}"
RDEPENDS:${PN} += "${PN}-env"



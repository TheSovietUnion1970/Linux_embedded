#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x943b2a58, "module_layout" },
	{ 0x8a5d7bf8, "genphy_resume" },
	{ 0xf7e1143e, "genphy_suspend" },
	{ 0x6f517969, "phy_drivers_unregister" },
	{ 0x29c66741, "phy_drivers_register" },
	{ 0x5579eb25, "phy_resolve_aneg_linkmode" },
	{ 0x62175221, "genphy_read_lpa" },
	{ 0x62754071, "mdiobus_read" },
	{ 0x85f1a514, "genphy_update_link" },
	{ 0xe54c520d, "phy_start_aneg" },
	{ 0x87804a16, "phy_init_hw" },
	{ 0xba259fb0, "genphy_soft_reset" },
	{ 0x26e4fe3, "mdiobus_write" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "libphy");

MODULE_ALIAS("mdio:00000000001110110001100001100001");
MODULE_ALIAS("mdio:00000000001110110001100010000001");
MODULE_ALIAS("mdio:0000000000111011000110000100????");

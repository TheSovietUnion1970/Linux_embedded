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
	{ 0x6f517969, "phy_drivers_unregister" },
	{ 0x29c66741, "phy_drivers_register" },
	{ 0x328b5ae7, "genphy_setup_forced" },
	{ 0x4e303cc8, "__genphy_config_aneg" },
	{ 0xb7d4257f, "phy_error" },
	{ 0x9186f895, "phy_trigger_machine" },
	{ 0x62754071, "mdiobus_read" },
	{ 0xc3d4a03a, "__mdiobus_read" },
	{ 0xeab6bda4, "__mdiobus_write" },
	{ 0x878d6df5, "phy_modify" },
	{ 0x26e4fe3, "mdiobus_write" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "libphy");

MODULE_ALIAS("mdio:????????????1111110001100010????");
MODULE_ALIAS("mdio:????????????11111100011011??????");
MODULE_ALIAS("mdio:????????????0111000001001101????");
MODULE_ALIAS("mdio:????????????0111000001000101????");
MODULE_ALIAS("mdio:????????????0111000001001000????");
MODULE_ALIAS("mdio:????????????0111000001010101????");
MODULE_ALIAS("mdio:????????????0111000001011000????");
MODULE_ALIAS("mdio:????????????0111000001100110????");
MODULE_ALIAS("mdio:????????????1111110001010101????");
MODULE_ALIAS("mdio:????????????1111110001001011????");

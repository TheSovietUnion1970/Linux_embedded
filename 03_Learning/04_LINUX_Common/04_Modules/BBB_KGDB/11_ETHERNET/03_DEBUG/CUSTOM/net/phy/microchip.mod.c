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
	{ 0x6f517969, "phy_drivers_unregister" },
	{ 0x29c66741, "phy_drivers_register" },
	{ 0x3c655c5d, "phy_write_mmd" },
	{ 0xcb710da4, "_dev_warn" },
	{ 0x87db2a63, "phy_restore_page" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0x568980e7, "phy_save_page" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x5a0b28b7, "phy_read_mmd" },
	{ 0x86830506, "of_property_read_variable_u32_array" },
	{ 0xce888b09, "devm_kmalloc" },
	{ 0xf7e1143e, "genphy_suspend" },
	{ 0x4e303cc8, "__genphy_config_aneg" },
	{ 0xb7d4257f, "phy_error" },
	{ 0x9186f895, "phy_trigger_machine" },
	{ 0xe05a0988, "devm_kfree" },
	{ 0x62754071, "mdiobus_read" },
	{ 0x26e4fe3, "mdiobus_write" },
	{ 0xc3d4a03a, "__mdiobus_read" },
	{ 0xeab6bda4, "__mdiobus_write" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "libphy");

MODULE_ALIAS("mdio:0000000000000111110000010011????");

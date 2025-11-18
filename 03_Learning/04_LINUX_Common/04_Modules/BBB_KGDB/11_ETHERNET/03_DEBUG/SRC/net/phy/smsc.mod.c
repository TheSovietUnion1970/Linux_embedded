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
	{ 0xb077e70a, "clk_unprepare" },
	{ 0xf7e1143e, "genphy_suspend" },
	{ 0x815588a6, "clk_enable" },
	{ 0x65423cfa, "genphy_read_status" },
	{ 0x8eb8cb89, "dev_err_probe" },
	{ 0x9186f895, "phy_trigger_machine" },
	{ 0xb6e6d99d, "clk_disable" },
	{ 0xb43f9365, "ktime_get" },
	{ 0x29c66741, "phy_drivers_register" },
	{ 0x2e1ca751, "clk_put" },
	{ 0x26e4fe3, "mdiobus_write" },
	{ 0x4e303cc8, "__genphy_config_aneg" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0x5a07b2ff, "of_find_property" },
	{ 0x328a05f1, "strncpy" },
	{ 0x62754071, "mdiobus_read" },
	{ 0x8a5d7bf8, "genphy_resume" },
	{ 0x92997ed8, "_printk" },
	{ 0x7c9a7371, "clk_prepare" },
	{ 0x6f517969, "phy_drivers_unregister" },
	{ 0xba259fb0, "genphy_soft_reset" },
	{ 0x75b5427b, "clk_get" },
	{ 0x76d9b876, "clk_set_rate" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0xce888b09, "devm_kmalloc" },
	{ 0xb7d4257f, "phy_error" },
};

MODULE_INFO(depends, "libphy");

MODULE_ALIAS("mdio:0000000000000111110000001010????");
MODULE_ALIAS("mdio:0000000000000111110000001011????");
MODULE_ALIAS("mdio:0000000000000111110000001100????");
MODULE_ALIAS("mdio:0000000000000111110000001101????");
MODULE_ALIAS("mdio:0000000000000111110000001111????");
MODULE_ALIAS("mdio:0000000000000111110000010001????");

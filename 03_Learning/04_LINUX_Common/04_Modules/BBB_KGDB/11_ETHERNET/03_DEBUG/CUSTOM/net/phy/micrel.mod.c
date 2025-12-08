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
	{ 0xb46f882f, "genphy_write_mmd_unsupported" },
	{ 0x456ef966, "genphy_read_mmd_unsupported" },
	{ 0xba259fb0, "genphy_soft_reset" },
	{ 0x6f517969, "phy_drivers_unregister" },
	{ 0x29c66741, "phy_drivers_register" },
	{ 0xf9a482f9, "msleep" },
	{ 0x844be617, "ethnl_cable_test_fault_length" },
	{ 0x5df7141d, "ethnl_cable_test_result" },
	{ 0xb43f9365, "ktime_get" },
	{ 0x49ebacbd, "_clear_bit" },
	{ 0xd3f57a2, "_find_next_bit_le" },
	{ 0x6d662533, "_find_first_bit_le" },
	{ 0x39b52d19, "__bitmap_and" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0x8a5d7bf8, "genphy_resume" },
	{ 0x87804a16, "phy_init_hw" },
	{ 0x4e303cc8, "__genphy_config_aneg" },
	{ 0x556e4390, "clk_get_rate" },
	{ 0x5fe35fcd, "devm_clk_get" },
	{ 0xce888b09, "devm_kmalloc" },
	{ 0xb7d4257f, "phy_error" },
	{ 0x9186f895, "phy_trigger_machine" },
	{ 0x73e20c1c, "strlcpy" },
	{ 0x4d1b956e, "genphy_read_abilities" },
	{ 0xcb710da4, "_dev_warn" },
	{ 0x183c479d, "genphy_restart_aneg" },
	{ 0x5a07b2ff, "of_find_property" },
	{ 0xd794197b, "_dev_err" },
	{ 0x26e4fe3, "mdiobus_write" },
	{ 0x589577df, "phy_modify_mmd" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x5a0b28b7, "phy_read_mmd" },
	{ 0x3c655c5d, "phy_write_mmd" },
	{ 0x86830506, "of_property_read_variable_u32_array" },
	{ 0x878d6df5, "phy_modify" },
	{ 0x65423cfa, "genphy_read_status" },
	{ 0x62754071, "mdiobus_read" },
	{ 0xf7e1143e, "genphy_suspend" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "libphy");

MODULE_ALIAS("mdio:????????????0010000101100001000?");
MODULE_ALIAS("mdio:????????00100010000101100010????");
MODULE_ALIAS("mdio:????????00100010000101100100????");
MODULE_ALIAS("mdio:????????0010001000010110000110??");
MODULE_ALIAS("mdio:????????00100010000101110010????");
MODULE_ALIAS("mdio:????????001000100001010101010101");
MODULE_ALIAS("mdio:????????001000100001010101010110");
MODULE_ALIAS("mdio:????????00100010000101010001????");
MODULE_ALIAS("mdio:????????00100010000101010101????");
MODULE_ALIAS("mdio:????????00100010000101010111????");
MODULE_ALIAS("mdio:????????00100010000101010110????");
MODULE_ALIAS("mdio:????????00001110011100100011????");
MODULE_ALIAS("mdio:????????00100010000101000011????");
MODULE_ALIAS("mdio:????????00100010000101100011????");
MODULE_ALIAS("mdio:????????00100010000101100110????");

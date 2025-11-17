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
	{ 0xde54a6ad, "regulator_list_voltage_table" },
	{ 0x6f517969, "phy_drivers_unregister" },
	{ 0x29c66741, "phy_drivers_register" },
	{ 0xba259fb0, "genphy_soft_reset" },
	{ 0x4e303cc8, "__genphy_config_aneg" },
	{ 0x3257a3d, "phy_resolve_aneg_pause" },
	{ 0x62175221, "genphy_read_lpa" },
	{ 0x85f1a514, "genphy_update_link" },
	{ 0xe759aff8, "__dynamic_dev_dbg" },
	{ 0x9c5ce893, "mdio_device_reset" },
	{ 0x4d1b956e, "genphy_read_abilities" },
	{ 0x9618ede0, "mutex_unlock" },
	{ 0x828ce6bb, "mutex_lock" },
	{ 0x589577df, "phy_modify_mmd" },
	{ 0x71e8a57c, "rdev_get_drvdata" },
	{ 0x745ae0a8, "devm_regulator_get_optional" },
	{ 0x42483122, "devm_regulator_register" },
	{ 0xe781b62, "regulator_enable" },
	{ 0x86830506, "of_property_read_variable_u32_array" },
	{ 0x5a07b2ff, "of_find_property" },
	{ 0xce888b09, "devm_kmalloc" },
	{ 0x878d6df5, "phy_modify" },
	{ 0xb7d4257f, "phy_error" },
	{ 0x9186f895, "phy_trigger_machine" },
	{ 0x96511ee6, "regulator_disable" },
	{ 0xf9a482f9, "msleep" },
	{ 0x844be617, "ethnl_cable_test_fault_length" },
	{ 0x5df7141d, "ethnl_cable_test_result" },
	{ 0xd794197b, "_dev_err" },
	{ 0xd3f57a2, "_find_next_bit_le" },
	{ 0x49ebacbd, "_clear_bit" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0xb43f9365, "ktime_get" },
	{ 0x6d662533, "_find_first_bit_le" },
	{ 0x87804a16, "phy_init_hw" },
	{ 0x9092327e, "phy_modify_changed" },
	{ 0xc3d4a03a, "__mdiobus_read" },
	{ 0xac1d4ed6, "__phy_modify" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x26e4fe3, "mdiobus_write" },
	{ 0x3c655c5d, "phy_write_mmd" },
	{ 0xeea0399, "strscpy" },
	{ 0x5a0b28b7, "phy_read_mmd" },
	{ 0x62754071, "mdiobus_read" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "libphy");

MODULE_ALIAS("mdio:000000000100110111010000011?0110");
MODULE_ALIAS("mdio:00000000010011011101000001110100");
MODULE_ALIAS("mdio:00000000010011011101000000100011");
MODULE_ALIAS("mdio:00000000010011011101000001110010");
MODULE_ALIAS("mdio:00000000010011011101000001000001");

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
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x943b2a58, "module_layout" },
	{ 0xeeb117ef, "get_phy_device" },
	{ 0xe0c63600, "fwnode_irq_get" },
	{ 0xd0988ba1, "fwnode_handle_put" },
	{ 0x474aab1d, "of_parse_phandle_with_fixed_args" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0x8c39a473, "fwnode_property_present" },
	{ 0xb1ed8586, "phy_device_register" },
	{ 0xc8548578, "phy_device_create" },
	{ 0x872bc951, "fwnode_handle_get" },
	{ 0xcb59c1b3, "phy_device_free" },
	{ 0x1a89195d, "fwnode_property_read_u32_array" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0xfc5c74b6, "device_set_node" },
	{ 0xe759aff8, "__dynamic_dev_dbg" },
	{ 0xeb573f0d, "fwnode_property_match_string" },
	{ 0x25a9b6d6, "fwnode_get_phy_id" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0xee447610, "of_node_put" },
	{ 0xdeac4839, "of_fwnode_ops" },
	{ 0xc3a3183e, "driver_deferred_probe_check_state" },
};

MODULE_INFO(depends, "libphy");


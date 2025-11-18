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
	{ 0xbf340679, "mdio_device_free" },
	{ 0xd0988ba1, "fwnode_handle_put" },
	{ 0x63166ded, "of_parse_phandle" },
	{ 0x4009c73c, "__mdiobus_register" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0x5d93f0ab, "of_device_is_compatible" },
	{ 0x5db2af1f, "of_get_phy_mode" },
	{ 0x4d21a70d, "mdiobus_unregister" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x872bc951, "fwnode_handle_get" },
	{ 0xcb59c1b3, "phy_device_free" },
	{ 0xcb6bb6ae, "mdiobus_is_registered_device" },
	{ 0x8bb0d82c, "of_device_is_available" },
	{ 0xa97c85f1, "of_match_node" },
	{ 0x20b6216, "fwnode_mdiobus_phy_device_register" },
	{ 0x5a07b2ff, "of_find_property" },
	{ 0x505616d, "of_property_read_string" },
	{ 0x6e007e23, "mdio_device_create" },
	{ 0xa6007443, "fixed_phy_register" },
	{ 0xd794197b, "_dev_err" },
	{ 0xdbcb68ba, "of_get_child_by_name" },
	{ 0x6bba55cd, "mdio_device_register" },
	{ 0x2c50a5a5, "_dev_info" },
	{ 0x38516dcf, "phy_connect_direct" },
	{ 0x10566cf9, "put_device" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x208ac1a8, "fixed_phy_unregister" },
	{ 0x92997ed8, "_printk" },
	{ 0xfc5c74b6, "device_set_node" },
	{ 0xc39a172c, "netdev_err" },
	{ 0xa6e5e7b5, "of_get_property" },
	{ 0xe759aff8, "__dynamic_dev_dbg" },
	{ 0x9b448d23, "fwnode_mdio_find_device" },
	{ 0xec26a59f, "of_get_next_available_child" },
	{ 0x198fee1c, "fwnode_phy_find_device" },
	{ 0xa1986d7f, "of_node_get" },
	{ 0x51aeb6b6, "fwnode_mdiobus_register_phy" },
	{ 0x25a9b6d6, "fwnode_get_phy_id" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x86830506, "of_property_read_variable_u32_array" },
	{ 0xee447610, "of_node_put" },
};

MODULE_INFO(depends, "libphy,fwnode_mdio,fixed_phy");


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
	{ 0xeeb117ef, "get_phy_device" },
	{ 0x8bba04f8, "kmalloc_caches" },
	{ 0x18e4f8aa, "swphy_read_reg" },
	{ 0x4009c73c, "__mdiobus_register" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0xd5ff12f6, "platform_device_register_full" },
	{ 0x4d21a70d, "mdiobus_unregister" },
	{ 0xb1ed8586, "phy_device_register" },
	{ 0xcb59c1b3, "phy_device_free" },
	{ 0x5f754e5a, "memset" },
	{ 0xe1537255, "__list_del_entry_valid" },
	{ 0x79fa7ba8, "mdiobus_free" },
	{ 0xa24491bf, "ida_free" },
	{ 0xbebb3413, "platform_device_unregister" },
	{ 0x55141517, "phy_device_remove" },
	{ 0x68f31cbd, "__list_add_valid" },
	{ 0xdbcb68ba, "of_get_child_by_name" },
	{ 0x45db4797, "phy_advertise_supported" },
	{ 0x270dc87f, "fwnode_gpiod_get_index" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x92997ed8, "_printk" },
	{ 0xe4e48b12, "swphy_validate_state" },
	{ 0x448f3791, "kmem_cache_alloc_trace" },
	{ 0xa1986d7f, "of_node_get" },
	{ 0x37a0cba, "kfree" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0xee447610, "of_node_put" },
	{ 0x4756260d, "ida_destroy" },
	{ 0x31067169, "gpiod_put" },
	{ 0xa5684076, "ida_alloc_range" },
	{ 0xe914e41e, "strcpy" },
	{ 0xc011d46d, "gpiod_get_value_cansleep" },
	{ 0x34299969, "mdiobus_alloc_size" },
};

MODULE_INFO(depends, "libphy");


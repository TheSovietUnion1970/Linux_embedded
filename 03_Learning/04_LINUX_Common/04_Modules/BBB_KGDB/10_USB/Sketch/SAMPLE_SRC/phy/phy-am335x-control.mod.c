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
	{ 0x63166ded, "of_parse_phandle" },
	{ 0x5a0ae4af, "platform_bus_type" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0x9a510eed, "__platform_driver_register" },
	{ 0xaab9911f, "devm_platform_ioremap_resource_byname" },
	{ 0xa97c85f1, "of_match_node" },
	{ 0x8e865d3c, "arm_delay_ops" },
	{ 0x7172add1, "bus_find_device" },
	{ 0x10566cf9, "put_device" },
	{ 0x92997ed8, "_printk" },
	{ 0xae577d60, "_raw_spin_lock" },
	{ 0x822137e2, "arm_heavy_mb" },
	{ 0x2cfde9a2, "warn_slowpath_fmt" },
	{ 0x8fc1d4ae, "platform_driver_unregister" },
	{ 0xee447610, "of_node_put" },
	{ 0xce888b09, "devm_kmalloc" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cti,am335x-usb-ctrl-module");
MODULE_ALIAS("of:N*T*Cti,am335x-usb-ctrl-moduleC*");

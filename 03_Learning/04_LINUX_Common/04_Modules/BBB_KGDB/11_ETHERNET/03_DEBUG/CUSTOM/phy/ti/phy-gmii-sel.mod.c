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
	{ 0x8fc1d4ae, "platform_driver_unregister" },
	{ 0x9a510eed, "__platform_driver_register" },
	{ 0xcb710da4, "_dev_warn" },
	{ 0x833c7c42, "regmap_field_update_bits_base" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0xe4bb92a7, "__devm_of_phy_provider_register" },
	{ 0x83ed0521, "devm_phy_create" },
	{ 0xee1f170f, "devm_regmap_field_alloc" },
	{ 0x47b23a97, "__of_get_address" },
	{ 0xd794197b, "_dev_err" },
	{ 0x9e0fe483, "syscon_node_to_regmap" },
	{ 0xce888b09, "devm_kmalloc" },
	{ 0xa97c85f1, "of_match_node" },
	{ 0x92997ed8, "_printk" },
	{ 0xe759aff8, "__dynamic_dev_dbg" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cti,am3352-phy-gmii-sel");
MODULE_ALIAS("of:N*T*Cti,am3352-phy-gmii-selC*");
MODULE_ALIAS("of:N*T*Cti,dra7xx-phy-gmii-sel");
MODULE_ALIAS("of:N*T*Cti,dra7xx-phy-gmii-selC*");
MODULE_ALIAS("of:N*T*Cti,am43xx-phy-gmii-sel");
MODULE_ALIAS("of:N*T*Cti,am43xx-phy-gmii-selC*");
MODULE_ALIAS("of:N*T*Cti,dm814-phy-gmii-sel");
MODULE_ALIAS("of:N*T*Cti,dm814-phy-gmii-selC*");
MODULE_ALIAS("of:N*T*Cti,am654-phy-gmii-sel");
MODULE_ALIAS("of:N*T*Cti,am654-phy-gmii-selC*");

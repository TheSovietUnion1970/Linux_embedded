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
	{ 0xd794197b, "_dev_err" },
	{ 0xa91e506e, "usb_add_phy_dev" },
	{ 0xcf5d6d5b, "device_set_wakeup_enable" },
	{ 0x93952e16, "device_init_wakeup" },
	{ 0x70126c7c, "usb_phy_gen_create_phy" },
	{ 0x2824b50d, "of_usb_get_dr_mode_by_phy" },
	{ 0x2b96370d, "of_alias_get_id" },
	{ 0x48c2b804, "am335x_get_phy_control" },
	{ 0xce888b09, "devm_kmalloc" },
	{ 0xafe12b4a, "usb_remove_phy" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "phy-am335x-control");

MODULE_ALIAS("of:N*T*Cti,am335x-usb-phy");
MODULE_ALIAS("of:N*T*Cti,am335x-usb-phyC*");

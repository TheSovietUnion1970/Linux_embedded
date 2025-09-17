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
	{ 0x9d36dc83, "led_trigger_unregister" },
	{ 0xd35c44c, "led_trigger_register" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0xd794197b, "_dev_err" },
	{ 0xcb710da4, "_dev_warn" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x68f31cbd, "__list_add_valid" },
	{ 0xbabececf, "sysfs_add_file_to_group" },
	{ 0xc358aaf8, "snprintf" },
	{ 0x2d6fcc06, "__kmalloc" },
	{ 0x97255bdf, "strlen" },
	{ 0x5082c78e, "of_parse_phandle_with_args" },
	{ 0x8a00d88, "of_count_phandle_with_args" },
	{ 0xee447610, "of_node_put" },
	{ 0x9fb75183, "usb_of_get_device_node" },
	{ 0x89bbafc6, "usb_register_notify" },
	{ 0x6567abdf, "led_set_brightness" },
	{ 0x542614c6, "usb_for_each_dev" },
	{ 0x27bc9c5, "sysfs_create_group" },
	{ 0x448f3791, "kmem_cache_alloc_trace" },
	{ 0x8bba04f8, "kmalloc_caches" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x811dc334, "usb_unregister_notify" },
	{ 0x5d546110, "sysfs_remove_group" },
	{ 0x37a0cba, "kfree" },
	{ 0x1a88444b, "sysfs_remove_file_from_group" },
	{ 0xe1537255, "__list_del_entry_valid" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "usbcore");


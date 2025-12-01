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
	{ 0x2d3385d3, "system_wq" },
	{ 0xbb688643, "cdev_del" },
	{ 0xb3cf015c, "cdev_init" },
	{ 0xf9a482f9, "msleep" },
	{ 0x47884890, "system_power_efficient_wq" },
	{ 0x202c96d, "page_address" },
	{ 0xa8082b4a, "xdp_rxq_info_reg" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0xc4a48f81, "device_destroy" },
	{ 0x28c3a1b3, "page_pool_alloc_pages" },
	{ 0x8a8bfc0f, "xdp_rxq_info_unreg" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x29d9f26e, "cancel_delayed_work_sync" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x9a510eed, "__platform_driver_register" },
	{ 0x526c3a6c, "jiffies" },
	{ 0xcb710da4, "_dev_warn" },
	{ 0x5f754e5a, "memset" },
	{ 0xa6295366, "page_pool_create" },
	{ 0xe97c4103, "ioremap" },
	{ 0xea4a09cb, "mod_delayed_work_on" },
	{ 0x6fca8a6a, "register_netdev" },
	{ 0xac204644, "device_create" },
	{ 0xdc1fc562, "netif_napi_add" },
	{ 0x24d273d1, "add_timer" },
	{ 0x8e865d3c, "arm_delay_ops" },
	{ 0xd794197b, "_dev_err" },
	{ 0x70738cfc, "cdev_add" },
	{ 0xb6d7cce8, "devm_alloc_etherdev_mqs" },
	{ 0x9f56c268, "xdp_rxq_info_is_reg" },
	{ 0x2c50a5a5, "_dev_info" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x92997ed8, "_printk" },
	{ 0x2aeeed1b, "xdp_rxq_info_reg_mem_model" },
	{ 0x82ee90dc, "timer_delete_sync" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0x822137e2, "arm_heavy_mb" },
	{ 0xe583d8af, "page_pool_destroy" },
	{ 0x9d669763, "memcpy" },
	{ 0xedc03953, "iounmap" },
	{ 0x42a9a53f, "dma_sync_single_for_device" },
	{ 0x7fbea683, "class_destroy" },
	{ 0xac34a21e, "unregister_netdev" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0xb2d48a2e, "queue_work_on" },
	{ 0x6b3b40ef, "platform_get_irq" },
	{ 0xdf6ec450, "consume_skb" },
	{ 0x8fc1d4ae, "platform_driver_unregister" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0xce888b09, "devm_kmalloc" },
	{ 0x1afb48ae, "devm_request_threaded_irq" },
	{ 0xa56a9c3f, "__class_create" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cether-based");
MODULE_ALIAS("of:N*T*Cether-basedC*");

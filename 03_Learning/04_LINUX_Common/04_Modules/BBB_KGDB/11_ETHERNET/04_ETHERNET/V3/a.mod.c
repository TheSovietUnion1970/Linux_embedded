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
	{ 0xff178f6, "__aeabi_idivmod" },
	{ 0x47884890, "system_power_efficient_wq" },
	{ 0xdf1668c8, "mem_map" },
	{ 0x372c7bfa, "napi_disable" },
	{ 0x5f3c2e1, "napi_schedule_prep" },
	{ 0xa8082b4a, "xdp_rxq_info_reg" },
	{ 0x7ad01db8, "netif_carrier_on" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0x2f5b0fdb, "gen_pool_alloc_algo_owner" },
	{ 0x8eaaa15, "netif_carrier_off" },
	{ 0xc4a48f81, "device_destroy" },
	{ 0x28c3a1b3, "page_pool_alloc_pages" },
	{ 0x8a8bfc0f, "xdp_rxq_info_unreg" },
	{ 0xf705fa49, "gen_pool_free_owner" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x29d9f26e, "cancel_delayed_work_sync" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x9a510eed, "__platform_driver_register" },
	{ 0x526c3a6c, "jiffies" },
	{ 0x4883b258, "devm_gen_pool_create" },
	{ 0x7d6c2636, "gen_pool_add_owner" },
	{ 0xcb710da4, "_dev_warn" },
	{ 0x5f754e5a, "memset" },
	{ 0xa6295366, "page_pool_create" },
	{ 0xa7513d23, "netif_tx_wake_queue" },
	{ 0xfed9817, "netif_tx_stop_all_queues" },
	{ 0xf3d0b495, "_raw_spin_unlock_irqrestore" },
	{ 0xe97c4103, "ioremap" },
	{ 0xea4a09cb, "mod_delayed_work_on" },
	{ 0x6fca8a6a, "register_netdev" },
	{ 0x4e4dbe1f, "napi_enable" },
	{ 0xac204644, "device_create" },
	{ 0xde79d39e, "netif_set_real_num_tx_queues" },
	{ 0xdc1fc562, "netif_napi_add" },
	{ 0x24d273d1, "add_timer" },
	{ 0x8e865d3c, "arm_delay_ops" },
	{ 0xd794197b, "_dev_err" },
	{ 0x70738cfc, "cdev_add" },
	{ 0xb6d7cce8, "devm_alloc_etherdev_mqs" },
	{ 0x9f56c268, "xdp_rxq_info_is_reg" },
	{ 0x2c50a5a5, "_dev_info" },
	{ 0xc53f20a5, "__napi_schedule" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x92997ed8, "_printk" },
	{ 0x5f31efa4, "napi_complete_done" },
	{ 0x68767571, "dma_map_page_attrs" },
	{ 0x2aeeed1b, "xdp_rxq_info_reg_mem_model" },
	{ 0x1964b44, "dev_driver_string" },
	{ 0xde55e795, "_raw_spin_lock_irqsave" },
	{ 0x82ee90dc, "timer_delete_sync" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0x822137e2, "arm_heavy_mb" },
	{ 0x2cfde9a2, "warn_slowpath_fmt" },
	{ 0xe583d8af, "page_pool_destroy" },
	{ 0x9d669763, "memcpy" },
	{ 0xedc03953, "iounmap" },
	{ 0x42a9a53f, "dma_sync_single_for_device" },
	{ 0x1a83fa43, "devm_ioremap" },
	{ 0x7fbea683, "class_destroy" },
	{ 0xd6b3e92b, "dma_unmap_page_attrs" },
	{ 0xac34a21e, "unregister_netdev" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0xb2d48a2e, "queue_work_on" },
	{ 0xc358aaf8, "snprintf" },
	{ 0x6b3b40ef, "platform_get_irq" },
	{ 0xdf6ec450, "consume_skb" },
	{ 0x8fc1d4ae, "platform_driver_unregister" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0x8899dfa0, "skb_tstamp_tx" },
	{ 0xce888b09, "devm_kmalloc" },
	{ 0x1afb48ae, "devm_request_threaded_irq" },
	{ 0xa56a9c3f, "__class_create" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xc31db0ce, "is_vmalloc_addr" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cether-based");
MODULE_ALIAS("of:N*T*Cether-basedC*");

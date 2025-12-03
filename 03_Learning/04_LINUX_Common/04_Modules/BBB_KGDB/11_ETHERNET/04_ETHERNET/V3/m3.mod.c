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
	{ 0xac34a21e, "unregister_netdev" },
	{ 0x372c7bfa, "napi_disable" },
	{ 0x4e4dbe1f, "napi_enable" },
	{ 0x1a83fa43, "devm_ioremap" },
	{ 0x4883b258, "devm_gen_pool_create" },
	{ 0x42a9a53f, "dma_sync_single_for_device" },
	{ 0x202c96d, "page_address" },
	{ 0xe583d8af, "page_pool_destroy" },
	{ 0x2aeeed1b, "xdp_rxq_info_reg_mem_model" },
	{ 0xa8082b4a, "xdp_rxq_info_reg" },
	{ 0x8a8bfc0f, "xdp_rxq_info_unreg" },
	{ 0x9f56c268, "xdp_rxq_info_is_reg" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0xa6295366, "page_pool_create" },
	{ 0x5f754e5a, "memset" },
	{ 0x6fca8a6a, "register_netdev" },
	{ 0xdc1fc562, "netif_napi_add" },
	{ 0xb6d7cce8, "devm_alloc_etherdev_mqs" },
	{ 0xdf6ec450, "consume_skb" },
	{ 0xde79d39e, "netif_set_real_num_tx_queues" },
	{ 0x92997ed8, "_printk" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0xf705fa49, "gen_pool_free_owner" },
	{ 0x2f5b0fdb, "gen_pool_alloc_algo_owner" },
	{ 0x7d6c2636, "gen_pool_add_owner" },
	{ 0x9d669763, "memcpy" },
	{ 0x8899dfa0, "skb_tstamp_tx" },
	{ 0x599487a3, "page_pool_put_page" },
	{ 0x28c3a1b3, "page_pool_alloc_pages" },
	{ 0xce888b09, "devm_kmalloc" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cether-based");
MODULE_ALIAS("of:N*T*Cether-basedC*");

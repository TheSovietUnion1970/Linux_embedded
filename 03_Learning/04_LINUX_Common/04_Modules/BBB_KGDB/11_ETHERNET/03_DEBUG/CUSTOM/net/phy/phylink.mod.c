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
	{ 0x92b57248, "flush_work" },
	{ 0x8bba04f8, "kmalloc_caches" },
	{ 0x92995232, "phy_disconnect" },
	{ 0x47884890, "system_power_efficient_wq" },
	{ 0x538d073d, "phy_duplex_to_str" },
	{ 0x7a8c0759, "_dev_printk" },
	{ 0x2c972f72, "phy_stop" },
	{ 0xd6ced0fc, "phy_attach_direct" },
	{ 0xd0988ba1, "fwnode_handle_put" },
	{ 0x18e4f8aa, "swphy_read_reg" },
	{ 0xda7e7242, "phy_ethtool_ksettings_set" },
	{ 0x9946d82b, "phy_ethtool_ksettings_get" },
	{ 0x7ad01db8, "netif_carrier_on" },
	{ 0xf90b7d17, "phy_ethtool_get_wol" },
	{ 0x8eaaa15, "netif_carrier_off" },
	{ 0x4205ad24, "cancel_work_sync" },
	{ 0x26e4fe3, "mdiobus_write" },
	{ 0xdb940b34, "phy_get_pause" },
	{ 0x32d48238, "phy_support_asym_pause" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x9618ede0, "mutex_unlock" },
	{ 0x8c39a473, "fwnode_property_present" },
	{ 0xb4ab0cf6, "fwnode_property_read_string" },
	{ 0x526c3a6c, "jiffies" },
	{ 0x3aa585c1, "__dynamic_netdev_dbg" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x19dae054, "phy_ethtool_get_eee" },
	{ 0x63e8f82f, "phy_set_asym_pause" },
	{ 0xda60f043, "fwnode_get_named_child_node" },
	{ 0xa084749a, "__bitmap_or" },
	{ 0xcb59c1b3, "phy_device_free" },
	{ 0xf4689d50, "linkmode_set_pause" },
	{ 0x5f754e5a, "memset" },
	{ 0x82290b07, "phy_start" },
	{ 0x49c1af41, "fwnode_get_phy_node" },
	{ 0xde4bf88b, "__mutex_init" },
	{ 0xd61eeee, "__bitmap_subset" },
	{ 0x432fa570, "mdiobus_modify" },
	{ 0x62754071, "mdiobus_read" },
	{ 0x3238a70e, "phy_speed_down" },
	{ 0x828ce6bb, "mutex_lock" },
	{ 0x7e9a5c05, "netdev_printk" },
	{ 0x1ac12e63, "phy_attached_info_irq" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0x122be8a4, "phy_detach" },
	{ 0x50fe36cd, "phy_init_eee" },
	{ 0x9ee1670a, "phy_request_interrupt" },
	{ 0x1a89195d, "fwnode_property_read_u32_array" },
	{ 0x6209f49, "phy_lookup_setting" },
	{ 0xd59a1587, "linkmode_resolve_pause" },
	{ 0x270dc87f, "fwnode_gpiod_get_index" },
	{ 0x2a57cee2, "phy_speed_up" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x92997ed8, "_printk" },
	{ 0xf3e6402e, "__bitmap_equal" },
	{ 0xd64466ae, "phy_get_eee_err" },
	{ 0xe4b818c3, "phy_speed_to_str" },
	{ 0x448f3791, "kmem_cache_alloc_trace" },
	{ 0xe759aff8, "__dynamic_dev_dbg" },
	{ 0x198fee1c, "fwnode_phy_find_device" },
	{ 0x82ee90dc, "timer_delete_sync" },
	{ 0x39b52d19, "__bitmap_and" },
	{ 0x2cfde9a2, "warn_slowpath_fmt" },
	{ 0x945e7bef, "phy_ethtool_set_wol" },
	{ 0x37a0cba, "kfree" },
	{ 0xae797f52, "gpiod_to_irq" },
	{ 0xfff06604, "phy_ethtool_set_eee" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x3edd9359, "phy_mii_ioctl" },
	{ 0x676bbc0f, "_set_bit" },
	{ 0xb2d48a2e, "queue_work_on" },
	{ 0x85670f1d, "rtnl_is_locked" },
	{ 0x49ebacbd, "_clear_bit" },
	{ 0x31067169, "gpiod_put" },
	{ 0x7b3ae962, "phy_restart_aneg" },
	{ 0xc1514a3b, "free_irq" },
	{ 0xc011d46d, "gpiod_get_value_cansleep" },
};

MODULE_INFO(depends, "libphy");


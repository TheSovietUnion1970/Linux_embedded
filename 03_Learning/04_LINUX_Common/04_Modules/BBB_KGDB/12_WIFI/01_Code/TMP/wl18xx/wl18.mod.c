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
	{ 0x8ea59ac6, "wl12xx_cmd_build_probe_req" },
	{ 0x8bba04f8, "kmalloc_caches" },
	{ 0x6d3b27e, "wl12xx_debug_level" },
	{ 0x27dbd666, "wlcore_set_key" },
	{ 0x528c709d, "simple_read_from_buffer" },
	{ 0xe6b6c5a6, "generic_file_llseek" },
	{ 0x7dfb7c30, "debugfs_create_dir" },
	{ 0x9eb15df6, "wlcore_event_beacon_loss" },
	{ 0x570fe861, "param_ops_int" },
	{ 0x3ec6ef34, "__pm_runtime_suspend" },
	{ 0x3e82f3e9, "get_random_bytes" },
	{ 0xb190eb85, "wlcore_event_dummy_packet" },
	{ 0xdba724e5, "wlcore_event_inactive_sta" },
	{ 0xcfbbdc93, "wlcore_event_ba_rx_constraint" },
	{ 0x599e30e0, "ieee80211_radar_detected" },
	{ 0xf13dfccc, "wlcore_event_fw_logger" },
	{ 0xc1b99792, "ieee80211_channel_to_freq_khz" },
	{ 0x90d8b391, "wlcore_event_rssi_trigger" },
	{ 0x8d49882b, "__dynamic_pr_debug" },
	{ 0xb1b939e, "kmemdup" },
	{ 0xf99b4188, "wlcore_cmd_wait_for_event_or_timeout" },
	{ 0x47231f57, "wlcore_probe" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0x505d8b8f, "param_ops_bool" },
	{ 0x7020489f, "wlcore_alloc_hw" },
	{ 0x9618ede0, "mutex_unlock" },
	{ 0xd67e85b9, "__pm_runtime_resume" },
	{ 0xbdca1042, "debugfs_create_file" },
	{ 0x20351125, "wlcore_get_native_channel_type" },
	{ 0x9a510eed, "__platform_driver_register" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xb124a692, "wl1271_cmd_configure" },
	{ 0xa19ba7d7, "param_ops_charp" },
	{ 0x5f754e5a, "memset" },
	{ 0xa15d0131, "cancel_delayed_work" },
	{ 0x202f1d59, "default_llseek" },
	{ 0x9b8c79e3, "wlcore_event_sched_scan_completed" },
	{ 0xe2d7885a, "wlcore_boot_upload_firmware" },
	{ 0x2484f3e, "wlcore_set_partition" },
	{ 0x328a05f1, "strncpy" },
	{ 0xb23c43f0, "wlcore_enable_interrupts" },
	{ 0x28dcedb0, "nla_put" },
	{ 0x828ce6bb, "mutex_lock" },
	{ 0xa5b48a25, "kfree_skb_reason" },
	{ 0xaf3ed6d8, "irq_get_irq_data" },
	{ 0x8e865d3c, "arm_delay_ops" },
	{ 0x72d91967, "skb_pull" },
	{ 0x8b12b6e9, "simple_open" },
	{ 0x7dc9e5fa, "ieee80211_queue_delayed_work" },
	{ 0x50668806, "__cfg80211_send_event_skb" },
	{ 0x4059792f, "print_hex_dump" },
	{ 0xa466f53a, "skb_queue_tail" },
	{ 0x88db665b, "kstrtoul_from_user" },
	{ 0xdcbaac7, "wlcore_event_channel_switch" },
	{ 0xa693df1c, "__cfg80211_alloc_event_skb" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0xc2d1cdbb, "wlcore_disable_interrupts" },
	{ 0xf131c1df, "wlcore_remove" },
	{ 0x92997ed8, "_printk" },
	{ 0x75fff721, "ieee80211_find_sta" },
	{ 0x34ca145c, "kstrtou8_from_user" },
	{ 0xf4c0323b, "wlcore_event_max_tx_failure" },
	{ 0x2ab5ebd7, "wl1271_free_tx_id" },
	{ 0x448f3791, "kmem_cache_alloc_trace" },
	{ 0x3c2f8634, "ieee80211_stop_rx_ba_session" },
	{ 0xd22ea8b7, "wlcore_scan_sched_scan_results" },
	{ 0x8deb4e1d, "ieee80211_get_hdrlen_from_skb" },
	{ 0x62e227f, "wlcore_free_hw" },
	{ 0x2cfde9a2, "warn_slowpath_fmt" },
	{ 0x37a0cba, "kfree" },
	{ 0xc1db71fa, "wl1271_format_buffer" },
	{ 0x9d669763, "memcpy" },
	{ 0x662c83e7, "wl1271_debugfs_update_stats" },
	{ 0x610d84eb, "wlcore_set_scan_chan_params" },
	{ 0x9c5e5e7b, "request_firmware" },
	{ 0xf1fa9557, "wlcore_boot_run_firmware" },
	{ 0xbf466818, "wlcore_translate_addr" },
	{ 0xa1feaf1, "wlcore_scan_sched_scan_ssid_list" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x676bbc0f, "_set_bit" },
	{ 0xb2d48a2e, "queue_work_on" },
	{ 0xc358aaf8, "snprintf" },
	{ 0xa4680a46, "wlcore_event_roc_complete" },
	{ 0x99bb8806, "memmove" },
	{ 0x8ddb5893, "wl12xx_is_dummy_packet" },
	{ 0x6ebe366f, "ktime_get_mono_fast_ns" },
	{ 0x8fc1d4ae, "platform_driver_unregister" },
	{ 0x189f8419, "wl1271_cmd_send" },
	{ 0xd0e9fb09, "release_firmware" },
	{ 0xf74ecebb, "ieee80211_connection_loss" },
};

MODULE_INFO(depends, "wlcore,mac80211,cfg80211");

MODULE_ALIAS("platform:wl18xx");

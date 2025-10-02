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
	{ 0x6d003da6, "__pm_runtime_idle" },
	{ 0x2b68bd2f, "del_timer" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x93a841f1, "phy_power_on" },
	{ 0x82eb0b0c, "phy_init" },
	{ 0x68e4a66c, "devm_phy_get" },
	{ 0xecbd37ab, "devm_usb_get_phy_by_phandle" },
	{ 0x81282f6d, "devm_ioremap_resource" },
	{ 0x2f0d9053, "usb_otg_state_string" },
	{ 0x3efeda66, "musb_interrupt" },
	{ 0xe97c4103, "ioremap" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x526c3a6c, "jiffies" },
	{ 0x1afb48ae, "devm_request_threaded_irq" },
	{ 0xf7531ba, "platform_get_irq_byname" },
	{ 0xfd8e89e5, "usb_get_dr_mode" },
	{ 0xa3c39227, "pm_runtime_enable" },
	{ 0x2e929a84, "of_iomap" },
	{ 0x5d93f0ab, "of_device_is_compatible" },
	{ 0xa97c85f1, "of_match_node" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xa3c0b80f, "platform_device_put" },
	{ 0xcb710da4, "_dev_warn" },
	{ 0x607ba4a3, "platform_device_add" },
	{ 0x6df858db, "platform_device_add_data" },
	{ 0x8bda0060, "usb_get_maximum_speed" },
	{ 0x41d995b3, "musb_get_mode" },
	{ 0x86830506, "of_property_read_variable_u32_array" },
	{ 0xce888b09, "devm_kmalloc" },
	{ 0x2e6ed703, "platform_device_add_resources" },
	{ 0x25a62d05, "device_set_of_node_from_dev" },
	{ 0x484a8a81, "platform_device_alloc" },
	{ 0x9875a98e, "platform_get_resource_byname" },
	{ 0x5f754e5a, "memset" },
	{ 0x3ec6ef34, "__pm_runtime_suspend" },
	{ 0x6ebe366f, "ktime_get_mono_fast_ns" },
	{ 0xf3d0b495, "_raw_spin_unlock_irqrestore" },
	{ 0xe180ff93, "musb_queue_resume_work" },
	{ 0xde55e795, "_raw_spin_lock_irqsave" },
	{ 0xd67e85b9, "__pm_runtime_resume" },
	{ 0x8e865d3c, "arm_delay_ops" },
	{ 0xe759aff8, "__dynamic_dev_dbg" },
	{ 0xb4a8834, "musb_writeb" },
	{ 0x2734197f, "musb_readb" },
	{ 0x5a38bfa, "debugfs_create_regset32" },
	{ 0x7dfb7c30, "debugfs_create_dir" },
	{ 0x80a8fbc0, "usb_debug_root" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0xd8b2a7c2, "debugfs_remove" },
	{ 0x9aed17c9, "phy_exit" },
	{ 0xe0ea07e8, "phy_power_off" },
	{ 0x82ee90dc, "timer_delete_sync" },
	{ 0xd794197b, "_dev_err" },
	{ 0x92997ed8, "_printk" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x9d669763, "memcpy" },
	{ 0xf0f95e51, "musb_readl" },
	{ 0xeb03b389, "__raw_readsl" },
	{ 0xedc03953, "iounmap" },
	{ 0x9df81e40, "__pm_runtime_disable" },
	{ 0xbebb3413, "platform_device_unregister" },
	{ 0x6af8c6dc, "musb_writel" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "musb_hdrc");

MODULE_ALIAS("of:N*T*Cti,musb-am33xx");
MODULE_ALIAS("of:N*T*Cti,musb-am33xxC*");
MODULE_ALIAS("of:N*T*Cti,musb-dm816");
MODULE_ALIAS("of:N*T*Cti,musb-dm816C*");

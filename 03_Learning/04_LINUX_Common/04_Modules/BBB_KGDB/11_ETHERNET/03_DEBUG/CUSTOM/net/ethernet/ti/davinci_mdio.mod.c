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
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x69246874, "mdiobus_get_phy" },
	{ 0x5b1d2eff, "__of_mdiobus_register" },
	{ 0x45f69e1f, "of_get_next_child" },
	{ 0xa3c39227, "pm_runtime_enable" },
	{ 0x435989cd, "__pm_runtime_use_autosuspend" },
	{ 0xd4e1580f, "pm_runtime_set_autosuspend_delay" },
	{ 0xf1969a8e, "__usecs_to_jiffies" },
	{ 0x556e4390, "clk_get_rate" },
	{ 0x1a83fa43, "devm_ioremap" },
	{ 0xf55ccda5, "platform_get_resource" },
	{ 0x5fe35fcd, "devm_clk_get" },
	{ 0xad92c237, "of_device_get_match_data" },
	{ 0xc358aaf8, "snprintf" },
	{ 0x86830506, "of_property_read_variable_u32_array" },
	{ 0xa2a9fa6a, "devm_mdiobus_alloc_size" },
	{ 0x53736763, "alloc_mdio_bitbang" },
	{ 0x2f62d1ce, "soc_device_match" },
	{ 0xce888b09, "devm_kmalloc" },
	{ 0xe707d823, "__aeabi_uidiv" },
	{ 0x92997ed8, "_printk" },
	{ 0x2c50a5a5, "_dev_info" },
	{ 0xf9a482f9, "msleep" },
	{ 0xcb710da4, "_dev_warn" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0x526c3a6c, "jiffies" },
	{ 0x822728dc, "mdiobb_write" },
	{ 0x3ec6ef34, "__pm_runtime_suspend" },
	{ 0x6ebe366f, "ktime_get_mono_fast_ns" },
	{ 0xcf4c8ea1, "mdiobb_read" },
	{ 0xc49bc0ed, "free_mdio_bitbang" },
	{ 0x9df81e40, "__pm_runtime_disable" },
	{ 0x6158109, "__pm_runtime_set_status" },
	{ 0xd67e85b9, "__pm_runtime_resume" },
	{ 0x4d21a70d, "mdiobus_unregister" },
	{ 0x83775ada, "pinctrl_pm_select_sleep_state" },
	{ 0x9fba763e, "pm_runtime_force_suspend" },
	{ 0xa7c80ef6, "pm_runtime_force_resume" },
	{ 0x10e2382e, "pinctrl_pm_select_default_state" },
	{ 0xd794197b, "_dev_err" },
	{ 0xb43f9365, "ktime_get" },
	{ 0x822137e2, "arm_heavy_mb" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "libphy,of_mdio,mdio_devres,mdio-bitbang");

MODULE_ALIAS("of:N*T*Cti,davinci_mdio");
MODULE_ALIAS("of:N*T*Cti,davinci_mdioC*");
MODULE_ALIAS("of:N*T*Cti,cpsw-mdio");
MODULE_ALIAS("of:N*T*Cti,cpsw-mdioC*");

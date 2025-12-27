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
	{ 0xb077e70a, "clk_unprepare" },
	{ 0x2d3385d3, "system_wq" },
	{ 0xbb688643, "cdev_del" },
	{ 0xb3cf015c, "cdev_init" },
	{ 0xf9a482f9, "msleep" },
	{ 0x815588a6, "clk_enable" },
	{ 0x349cba85, "strchr" },
	{ 0x97255bdf, "strlen" },
	{ 0xf7802486, "__aeabi_uidivmod" },
	{ 0x4205ad24, "cancel_work_sync" },
	{ 0xc4a48f81, "device_destroy" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0x556e4390, "clk_get_rate" },
	{ 0x9a510eed, "__platform_driver_register" },
	{ 0x526c3a6c, "jiffies" },
	{ 0xe707d823, "__aeabi_uidiv" },
	{ 0x5f754e5a, "memset" },
	{ 0xe97c4103, "ioremap" },
	{ 0x84b183ae, "strncmp" },
	{ 0xac204644, "device_create" },
	{ 0xd794197b, "_dev_err" },
	{ 0x59e5070d, "__do_div64" },
	{ 0x70738cfc, "cdev_add" },
	{ 0x2c50a5a5, "_dev_info" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x92997ed8, "_printk" },
	{ 0x7c9a7371, "clk_prepare" },
	{ 0x5fe35fcd, "devm_clk_get" },
	{ 0x822137e2, "arm_heavy_mb" },
	{ 0x9d669763, "memcpy" },
	{ 0xedc03953, "iounmap" },
	{ 0x7fbea683, "class_destroy" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0xb2d48a2e, "queue_work_on" },
	{ 0xc358aaf8, "snprintf" },
	{ 0x6b3b40ef, "platform_get_irq" },
	{ 0x8fc1d4ae, "platform_driver_unregister" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0xce888b09, "devm_kmalloc" },
	{ 0x1afb48ae, "devm_request_threaded_irq" },
	{ 0xa56a9c3f, "__class_create" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xe914e41e, "strcpy" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cusb1-based");
MODULE_ALIAS("of:N*T*Cusb1-basedC*");

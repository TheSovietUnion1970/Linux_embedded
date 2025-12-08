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
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0x79fa7ba8, "mdiobus_free" },
	{ 0x8e865d3c, "arm_delay_ops" },
	{ 0x18b30b69, "module_put" },
	{ 0xecb10561, "__module_get" },
	{ 0x34299969, "mdiobus_alloc_size" },
};

MODULE_INFO(depends, "libphy");


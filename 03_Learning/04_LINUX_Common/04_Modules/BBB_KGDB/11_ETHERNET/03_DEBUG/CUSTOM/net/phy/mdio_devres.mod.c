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
	{ 0xd9d79181, "devres_find" },
	{ 0x4009c73c, "__mdiobus_register" },
	{ 0xe93e49c3, "devres_free" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0x4d21a70d, "mdiobus_unregister" },
	{ 0x79fa7ba8, "mdiobus_free" },
	{ 0x1de05de9, "__devres_alloc_node" },
	{ 0x5b1d2eff, "__of_mdiobus_register" },
	{ 0x78f333f9, "devres_add" },
	{ 0x2cfde9a2, "warn_slowpath_fmt" },
	{ 0x34299969, "mdiobus_alloc_size" },
};

MODULE_INFO(depends, "libphy,of_mdio");


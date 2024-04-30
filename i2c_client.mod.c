#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
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
__used
__attribute__((section("__versions"))) = {
	{ 0x90fd01f4, "module_layout" },
	{ 0x18444bdc, "i2c_del_driver" },
	{ 0x5d6d6b45, "i2c_register_driver" },
	{ 0x39a07896, "cdev_add" },
	{ 0x598f868a, "cdev_init" },
	{ 0x5e119b49, "device_create" },
	{ 0xe4d58cf5, "__class_create" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x681a4cbb, "devm_kmalloc" },
	{ 0xdb7305a1, "__stack_chk_fail" },
	{ 0x5e25215f, "i2c_transfer" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x42fa2de0, "class_destroy" },
	{ 0x46360476, "device_destroy" },
	{ 0xbffbcbd3, "cdev_del" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x7c32d0f0, "printk" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("i2c:chrdev_client");

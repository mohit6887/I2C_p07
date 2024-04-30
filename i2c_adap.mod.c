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
	{ 0xe43cb5ee, "platform_driver_unregister" },
	{ 0xc0b9519e, "__platform_driver_register" },
	{ 0xdb7c810d, "i2c_add_numbered_adapter" },
	{ 0x681a4cbb, "devm_kmalloc" },
	{ 0xd6b8e852, "request_threaded_irq" },
	{ 0x85b3b7bb, "of_property_read_variable_u32_array" },
	{ 0xe97c4103, "ioremap" },
	{ 0x90d48ea8, "platform_get_resource" },
	{ 0xddece99e, "__init_waitqueue_head" },
	{ 0xd68bb7b3, "_dev_err" },
	{ 0x51b40ba, "platform_get_irq" },
	{ 0xdb7305a1, "__stack_chk_fail" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x8e865d3c, "arm_delay_ops" },
	{ 0x5054c285, "wait_for_completion_timeout" },
	{ 0xf9a482f9, "msleep" },
	{ 0xda02d67, "jiffies" },
	{ 0xbb2242fc, "complete" },
	{ 0x7c32d0f0, "printk" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0xc92cc35d, "i2c_del_adapter" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


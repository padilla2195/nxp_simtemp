#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

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



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xf02290ec, "__register_chrdev" },
	{ 0x535f4f5f, "hrtimer_init" },
	{ 0x5fa07cc0, "hrtimer_start_range_ns" },
	{ 0x52b15b3b, "__unregister_chrdev" },
	{ 0x36a36ab1, "hrtimer_cancel" },
	{ 0xe4de56b4, "__ubsan_handle_load_invalid_value" },
	{ 0x97acb853, "ktime_get" },
	{ 0x5a844b26, "__x86_indirect_thunk_rax" },
	{ 0x49fc4616, "hrtimer_forward" },
	{ 0xd272d446, "__fentry__" },
	{ 0xe8213e80, "_printk" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0x70eca2ca, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0xf02290ec,
	0x535f4f5f,
	0x5fa07cc0,
	0x52b15b3b,
	0x36a36ab1,
	0xe4de56b4,
	0x97acb853,
	0x5a844b26,
	0x49fc4616,
	0xd272d446,
	0xe8213e80,
	0xd272d446,
	0x70eca2ca,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"__register_chrdev\0"
	"hrtimer_init\0"
	"hrtimer_start_range_ns\0"
	"__unregister_chrdev\0"
	"hrtimer_cancel\0"
	"__ubsan_handle_load_invalid_value\0"
	"ktime_get\0"
	"__x86_indirect_thunk_rax\0"
	"hrtimer_forward\0"
	"__fentry__\0"
	"_printk\0"
	"__x86_return_thunk\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "2367F76B799454B64005014");

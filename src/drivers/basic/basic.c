#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

int __init basic_init(void)
{
	pr_info("Basic module init\n");
	return 0;
}

void __exit basic_exit(void)
{
	pr_info("Basic module exit\n");
	return;
}

module_init(basic_init);
module_exit(basic_exit);

MODULE_AUTHOR("George Castillo");
MODULE_DESCRIPTION("Demonstrate module build, insertion and removal functionality");
MODULE_LICENSE("MIT");


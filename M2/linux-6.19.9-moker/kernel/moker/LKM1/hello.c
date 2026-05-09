#include <linux/module.h> /* Needed by all modules */
#include <linux/kernel.h> /* Needed for KERN_INFO */

static int
__init hello_init(void)
{
	printk(KERN_INFO "LKM: I am in the Linux kernel.\n");
	return 0;
}
static void
__exit hello_exit(void)
{
	printk(KERN_INFO "LKM: I am no longer in the Linux kernel.\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PBS");
MODULE_DESCRIPTION("The simplest kernel module ");

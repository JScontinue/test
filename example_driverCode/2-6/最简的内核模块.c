#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("Dual BSD/GPL");

static int hello_init(void)
{
	printk(KERN_ALERT " hello world enter\n");
	return 0;
}

static int hello_exit(void)
{
	printk(KERN_ALERT " hello world exit\n");
	return 0;
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("Deng Jinshui");
MODULE_DESCRIPTION("A simple Hello World Module");
MODULE_ALITS("a simple module");


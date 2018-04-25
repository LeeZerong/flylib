#include <linux/module.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/kernel.h>

struct timer_list dri_timer;
unsigned int i = 0;

//定时器超时函数
static void dri_timer_function(unsigned long data)
{
	printk(KERN_INFO"%d ", i++);
	mod_timer(&dri_timer, jiffies + HZ/5);
}

static int __init dri_init()
{
	//初始化定时器
	init_timer(&dri_timer);
	dri_timer.function = dri_timer_function;
	
	//注册定时器
	add_timer(&dri_timer);
	
	//启动定时器
	mod_timer(&dri_timer, jiffies + HZ/5);
}

static void __exit dri_exit()
{
	
} 



module_init(dri_init);
module_exit(dri_exit);
MODULE_LICENSE("GPL");
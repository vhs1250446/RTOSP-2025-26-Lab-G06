#include <linux/module.h> /* Needed by all modules */
#include <linux/kernel.h> /* Needed for KERN_INFO */
#include<linux/workqueue.h>


int proc_init(void);
void  proc_exit(void);

static struct workqueue_struct *queue;

 static void normal_work_handler(struct work_struct *work)
 {
 	printk(KERN_INFO "LKM: Hi! I'm handler of normal work! [%d]\n", current->pid);
 }
 static void delayed_work_handler(struct work_struct *work)
{
	printk(KERN_INFO "LKM:Hi! I'm handler of delayed work! [%d]\n", current->pid);
}

static DECLARE_WORK(normal_work, normal_work_handler);
static DECLARE_DELAYED_WORK(delayed_work, delayed_work_handler);



int __init proc_init(void)
{
	printk(KERN_INFO "LKM: I am in the Linux kernel.\n");
	 /* Schedule the Bottom Half */
  	queue = create_singlethread_workqueue("works");
  	if(!queue_work(queue,&normal_work))
		printk(KERN_INFO "The normal work was already queued!\n");
 	if(!queue_delayed_work(queue,&delayed_work,10*HZ))
 		printk(KERN_INFO "The delayed work was already queued!\n");

	return 0;
}
void __exit proc_exit(void)
{
	printk(KERN_INFO "LKM: I am no longer in the Linux kernel.\n");
	if(cancel_work_sync(&normal_work))
 		printk(KERN_INFO "The normal work has not been done yet!\n");
 	if(cancel_delayed_work_sync(&delayed_work))
 		printk(KERN_INFO "The delayed work has not been done yet!\n");
 	destroy_workqueue(queue);
}
module_init(proc_init);
module_exit(proc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("PBS");
MODULE_DESCRIPTION("The simplest kernel module ");




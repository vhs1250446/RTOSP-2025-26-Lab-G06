#include <linux/module.h> /* Needed by all modules */
#include <linux/kernel.h> /* Needed for KERN_INFO */
#include <linux/kthread.h>
#include <linux/delay.h>


int  proc_init(void);
void proc_exit(void);
int  kth_function(void *pv);

struct task_struct *kth1, *kth2;
int th_id1 = 1, th_id2 = 2;

/*
** Thread Function
*/
int kth_function(void *data)
{
    int i=0;
    int id = *((int *) data);
    while(!kthread_should_stop()) {
    	printk(KERN_INFO "LKM:[%d]:[%d] Kthread function [%d] \n", current->pid, id, i++);
        msleep(id * 1000);
    }
    return 0;
}

int __init proc_init(void)
{

	kth1 = kthread_create(kth_function,&th_id1,ENTRY_NAME);
        if(kth1) {
            printk(KERN_INFO "LKM:[%d]:[%d] Kthread Created Successfully\n", current->pid, th_id1);
            wake_up_process(kth1);
            printk(KERN_INFO "LKM:[%d]:[%d] Kthread is executing \n", current->pid, th_id1);
        } else {
            printk(KERN_INFO "LKM:[%d]:[%d] Cannot create kthread\n", current->pid, th_id1);
        }
        kth2 = kthread_run(kth_function,&th_id2,ENTRY_NAME);
        if(kth2) {
            printk(KERN_INFO "LKM:[%d]:[%d] Kthread Created Successfully\n", current->pid, th_id2);
        } else {
            printk(KERN_INFO "LKM:[%d]:[%d] Cannot create kthread\n", current->pid, th_id2);
        }
	return 0;
}

void __exit proc_exit(void)
{
	if(kth1){

		kthread_stop(kth1);
		printk(KERN_INFO "LKM:[%d]:[%d] Kthread Stopped Successfully\n", current->pid, th_id1);
	}
	if(kth2){
		kthread_stop(kth2);
		printk(KERN_INFO "LKM:[%d]:[%d] Kthread Stopped Successfully\n", current->pid, th_id2);
	}
}



module_init(proc_init);
module_exit(proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PBS");
MODULE_DESCRIPTION("The kernel thread example");

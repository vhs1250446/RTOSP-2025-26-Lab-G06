#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/semaphore.h>
	


#define ENTRY_NAME "waitq"

void burn_cpu(void);

int proc_open(struct inode *inode, struct file *filp);
ssize_t  proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t  proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
int proc_close(struct inode *inode, struct file *filp);

int proc_init(void);
void proc_exit(void);


wait_queue_head_t wq;
unsigned int flag = 0;


void burn_cpu(void)
{
	unsigned int i;
	for(i=0; i<UINT_MAX;i++){
		asm volatile  ("nop" ::); 
		asm volatile  ("nop" ::); 
		asm volatile  ("nop" ::); 
		asm volatile  ("nop" ::); 
		asm volatile  ("nop" ::); 
		asm volatile  ("nop" ::); 
		asm volatile  ("nop" ::); 
		asm volatile  ("nop" ::); 
		asm volatile  ("nop" ::); 
		asm volatile  ("nop" ::); 

	}
}


int proc_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "LKM:%s:[%d] open\n",ENTRY_NAME, current->pid);
	
	return 0;
}
ssize_t  proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO "LKM:%s:[%d] read\n",ENTRY_NAME, current->pid);
	printk(KERN_INFO "LKM:%s:[%d] wait[flag=%d]\n",ENTRY_NAME, current->pid, flag);
	wait_event_interruptible(wq, flag!=0);
	printk(KERN_INFO "LKM:%s:[%d] wait[flag=%d]\n",ENTRY_NAME, current->pid, flag);
	flag = 0;
	return 0;
	
}
ssize_t  proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO "LKM:%s:[%d] write\n",ENTRY_NAME, current->pid);
	printk(KERN_INFO "LKM:%s:[%d] up[flag=%d]\n",ENTRY_NAME, current->pid, flag);
	burn_cpu();
	flag = 1;
	wake_up_interruptible(&wq);
	printk(KERN_INFO "LKM:%s:[%d] up[flag=%d]\n",ENTRY_NAME, current->pid, flag);
	
    	return count;
}

int proc_close(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "LKM:%s:[%d] release\n",ENTRY_NAME, current->pid);
	return 0; 
}
static const struct proc_ops proc_ops = {
 	.proc_open  	= proc_open,
 	.proc_read  	= proc_read,
	.proc_write  	= proc_write,
 	.proc_release  	= proc_close,
};
int proc_init(void)
{
	struct proc_dir_entry *proc_entry = NULL;
   	proc_entry = proc_create(ENTRY_NAME,0, NULL, &proc_ops);
	if(proc_entry == NULL)
   		return -ENOMEM;

	init_waitqueue_head(&wq);
  
   	printk(KERN_INFO "LKM: /proc/%s created\n", ENTRY_NAME);
	
   	return 0;
}

void proc_exit(void)
{
   	remove_proc_entry(ENTRY_NAME, NULL);
   	printk(KERN_INFO "LKM: Removing /proc/%s.\n", ENTRY_NAME);
}

module_init(proc_init);
module_exit(proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LKM");
MODULE_DESCRIPTION("Waitqueue example");

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>


#define ENTRY_NAME "atomic"


int proc_open(struct inode *inode, struct file *filp);
ssize_t  proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t  proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
int proc_close(struct inode *inode, struct file *filp);
int proc_init(void);
void proc_exit(void);


atomic_t nr;

int
proc_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "LKM: %s:[%d] open\n",ENTRY_NAME, current->pid);

	return 0;
}

ssize_t
proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO "LKM: %s:[%d] read: nr: %d\n",
	       ENTRY_NAME,
	       current->pid,
	       atomic_dec_return(&nr));
	return 0;

}

ssize_t
proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO "LKM: %s:[%d] write: nr: %d\n",
	       ENTRY_NAME,
	       current->pid,
	       atomic_inc_return(&nr));
    	return count;
}

int
proc_close(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "LKM: %s:[%d] close\n",ENTRY_NAME, current->pid);
	return 0;
}

static const struct proc_ops proc_ops = {
 	.proc_open  	= proc_open,
 	.proc_read  	= proc_read,
	.proc_write  	= proc_write,
 	.proc_release  	= proc_close,
};

int
proc_init(void)
{
	struct proc_dir_entry *proc_entry = NULL;
   	proc_entry = proc_create(ENTRY_NAME,0666, NULL, &proc_ops);
	if(proc_entry == NULL)
   		return -ENOMEM;

	atomic_set(&nr,0);

   	printk(KERN_INFO "LKM: /proc/%s created\n", ENTRY_NAME);
	printk(KERN_INFO "LKM:%s:[%d] init: nr: %d\n",ENTRY_NAME, current->pid,atomic_read(&nr));
   	return 0;
}

void proc_exit(void)
{
   	remove_proc_entry(ENTRY_NAME, NULL);
   	printk(KERN_INFO "LKM: Removing /proc/%s.\n", ENTRY_NAME);
	printk(KERN_INFO "LKM:%s:[%d] exit: nr: %d\n",ENTRY_NAME, current->pid,atomic_read(&nr));
}

module_init(proc_init);
module_exit(proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PBS");
MODULE_DESCRIPTION("Atomic_t variables example");

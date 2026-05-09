#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/uaccess.h>	/* copy_(to,from)_user */

#define BUFFER_LEN	100
#define ENTRY_NAME "hello3"

int hello_proc_init(void);
void hello_proc_exit(void);
int proc_open(struct inode *inode, struct file *filp);
ssize_t  proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t  proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t  proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
int proc_close(struct inode *inode, struct file *filp);

/*
  When module loads, proc_init() runs and performs proc_create("hello3", ...)
  This created an entry under root/proc/ called "hello3"
  and connects it to the handlers via proc_ops.

  Use cases:
  User opens file: open handler runs.
  e.g. user runs "cat /proc/hello3"
  1 - first thing first, proc_open runs
  2 - then proc_read runs

*/

struct proc_dir_entry *proc_entry = NULL;

static const struct proc_ops proc_ops = {
 	.proc_open  	= proc_open,
 	.proc_read  	= proc_read,
	.proc_write  	= proc_write,
 	.proc_release  	= proc_close,
};

char buffer[BUFFER_LEN] ="I want to be a kernel hacker!!\n";

int
proc_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "LKM: %s:[%d] open\n", ENTRY_NAME, current->pid);
	return 0;
}

ssize_t
proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int ret, len;

	printk(KERN_INFO "LKM: %s:[%d] read\n",ENTRY_NAME, current->pid);

	len = strlen(buffer);

	if(*f_pos > len)
		return 0;

	if(len <= 0 || count < len)
		return -EFAULT;

	ret = raw_copy_to_user(buf,buffer,len);
	if(ret != 0){
		return -EFAULT;
	}

	*f_pos += count - len;

	return len;
}

ssize_t
proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int ret;
	printk(KERN_INFO "LKM: %s:[%d] write\n",ENTRY_NAME, current->pid);
	if (count > BUFFER_LEN)
        	return -EINVAL;
	ret = raw_copy_from_user(buffer, buf, count);
    	if (ret != 0)
        	return -EFAULT;
	buffer[count]=0;
    	return count;
}

int
proc_close(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "LKM: %s:[%d] close\n",ENTRY_NAME, current->pid);
	return 0;
}

int
hello_proc_init(void)
{
	proc_entry = proc_create(ENTRY_NAME,0, NULL, &proc_ops);
	if(proc_entry == NULL)
		return -ENOMEM;

	printk(KERN_INFO "LKM: /proc/%s created\n", ENTRY_NAME);
	return 0;
}

void
hello_proc_exit(void)
{
	remove_proc_entry(ENTRY_NAME, NULL);
	printk(KERN_INFO "LKM: /proc/%s removed\n", ENTRY_NAME);
}

module_init(hello_proc_init); // run when insmod
module_exit(hello_proc_exit); // run when rmmod

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PBS");
MODULE_DESCRIPTION("Proc entry module with interaction");

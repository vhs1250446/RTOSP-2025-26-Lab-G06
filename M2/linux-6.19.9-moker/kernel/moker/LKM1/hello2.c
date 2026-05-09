#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>

#define ENTRY_NAME "hello2"


int
proc_open(struct inode *inode, struct file *filp);

ssize_t
proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);

ssize_t
proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

int
proc_close(struct inode *inode, struct file *filp);

int
hello_proc_init(void);

void
hello_proc_exit(void);

////////////////////////////////////////////////////////////////////////////////////////

struct proc_dir_entry *proc_entry = NULL;

int
proc_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "LKM: %s:[%d] open\n", ENTRY_NAME, current->pid);
	return 0;
}

ssize_t
proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO "LKM: %s:[%d] read\n", ENTRY_NAME, current->pid);
	return 0;
}

ssize_t
proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO "LKM: %s:[%d] write\n", ENTRY_NAME, current->pid);
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
hello_proc_init(void)
{
  proc_entry = proc_create(ENTRY_NAME, 0, NULL, &proc_ops);
  if(proc_entry == NULL)
    return -ENOMEM;

  printk(KERN_INFO "LKM: /proc/%s created\n", ENTRY_NAME);
  return 0;
}

void
hello_proc_exit(void)
{
  remove_proc_entry(ENTRY_NAME, NULL);
  printk(KERN_INFO "LKM: /proc/%s removed.\n", ENTRY_NAME);
}

module_init(hello_proc_init);

module_exit(hello_proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PBS");
MODULE_DESCRIPTION("Proc entry module");

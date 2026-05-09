#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/uaccess.h>	/* copy_(to,from)_user */

#define BUFFER_LEN  100
#define ENTRY_NAME "tasklist"

/*
  defined in in include/linux/sched.h
  struct task_struct {
  ...
  struct list_head tasks;
  ...
  pid_t pid;
  ...
  struct task_struct __rcu *parent; // recipient of SIGCHLD, wait4() reports

  //children/sibling forms the list of my natural children

  struct list_head children;// list of my children
  struct list_head sibling;// linkage in my parent's children list

  char comm[TASK_COMM_LEN];// executable name excluding path ...
  ...
  }
*/



int proc_open(struct inode *inode, struct file *filp);

ssize_t  proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);

ssize_t  proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

int proc_close(struct inode *inode, struct file *filp);

int proc_init(void);

void proc_exit(void);


char buffer[BUFFER_LEN];
struct task_struct  *task;
int nr_tasks = 0;

int
proc_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "LKM:%s:[%d] open\n",ENTRY_NAME, current->pid);
	task	= NULL;
	nr_tasks = 0;
	return 0;
}

ssize_t
proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int ret, len  = 0;
	printk(KERN_INFO "LKM:%s:[%d] read\n",ENTRY_NAME, current->pid);
	if(nr_tasks == 0) {
		task = &init_task; //defined in /init/init_task.c
	} else {
		if(task == &init_task) {
			return 0;
		}
	}

	len =  sprintf( buffer+len, "#%3d ", ++nr_tasks );
	len += sprintf( buffer+len, "%5d ", task->pid );
	len += sprintf( buffer+len, "%5d ", task->parent->pid );
	len += sprintf( buffer+len, "%-15s ", task->comm );
	len += sprintf( buffer+len, "%lx ", (unsigned long)task);
	len += sprintf( buffer+len, "\n" );
	if(len <= 0)
		return -EFAULT;
	if(count < len)
		return -EFAULT;
	ret = raw_copy_to_user(buf,buffer,len);
	if(ret != 0){
		return -EFAULT;
	}
	task = list_entry(task->tasks.next, struct task_struct, tasks);
	return len;
}

ssize_t
proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    	return count;
}

int
proc_close(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "LKM:%s:[%d] close\n",ENTRY_NAME, current->pid);
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
   	proc_entry = proc_create(ENTRY_NAME,0, NULL, &proc_ops);
	if(proc_entry == NULL)
   		return -ENOMEM;

   	printk(KERN_INFO "LKM: /proc/%s created\n", ENTRY_NAME);
   	return 0;
}

void
proc_exit(void)
{
   	remove_proc_entry(ENTRY_NAME, NULL);
   	printk(KERN_INFO "LKM: Removing /proc/%s.\n", ENTRY_NAME);
}

module_init(proc_init);
module_exit(proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LKM");
MODULE_DESCRIPTION("Task list");

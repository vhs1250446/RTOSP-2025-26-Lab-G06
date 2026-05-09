#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/uaccess.h>	/* copy_(to,from)_user */
#include <linux/jiffies.h>	/*jiffies*/
#include <linux/timer.h> 	/*timer*/
#include <asm/atomic.h>		/*atomic*/


#define ENTRY_NAME "timer"
#define BUFFER_LEN	100

static void timer_callback(struct timer_list * t);

int proc_open(struct inode *inode, struct file *filp);
ssize_t  proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t  proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
int proc_close(struct inode *inode, struct file *filp);

int proc_init(void);
void proc_exit(void);

struct proc_dir_entry *proc_entry = NULL;

atomic_t secs;

struct timer_list timer;
unsigned long timer_interval;

static void timer_callback(struct timer_list * t)
{
	atomic_inc(&secs);
	mod_timer(&timer,timer.expires + timer_interval);
}

int proc_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "LKM:%s:[%d] open\n",ENTRY_NAME, current->pid);
	return 0;
}
ssize_t  proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int ret, len;
	char buffer[BUFFER_LEN];
	printk(KERN_INFO "LKM:%s:[%d] read\n",ENTRY_NAME, current->pid);
	sprintf(buffer,"%d",atomic_read(&secs));
	len = strlen(buffer);
	if(*f_pos > len)
		return 0;
	if(len <= 0)
		return -EFAULT;
	if(count < len)
		return -EFAULT;
	ret=copy_to_user(buf,buffer,len);
	if(ret != 0){ 
		return -EFAULT;
	}
	*f_pos += count - len;
	return len;
}

int proc_close(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "LKM:%s:[%d] release\n",ENTRY_NAME, current->pid);
	return 0; 
}
static const struct proc_ops proc_ops = {
 	.proc_open  	= proc_open,
 	.proc_read  	= proc_read,
 	.proc_release  	= proc_close,
};
int proc_init(void)
{
	struct proc_dir_entry *proc_entry = NULL;
   	proc_entry = proc_create(ENTRY_NAME,0, NULL, &proc_ops);
	if(proc_entry == NULL)
   		return -ENOMEM;
	printk(KERN_INFO "LKM: /proc/%s created\n", ENTRY_NAME);
  	atomic_set(&secs, 0);
	timer_interval = msecs_to_jiffies(1000);//one second
	timer_setup(&timer, timer_callback, 0);
	timer.expires = jiffies + timer_interval;
	add_timer(&timer); //start timer
   	
   	return 0;
}

void proc_exit(void)
{
   	remove_proc_entry(ENTRY_NAME, NULL);
   	printk(KERN_INFO "LKM: Removing /proc/%s.\n", ENTRY_NAME);
	del_timer_sync(&timer);
}

module_init(proc_init);
module_exit(proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LKM");
MODULE_DESCRIPTION("Timer");

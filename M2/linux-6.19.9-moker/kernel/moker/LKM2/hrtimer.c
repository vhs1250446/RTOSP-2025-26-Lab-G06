#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

#define NS_PER_SEC  1000000000

#define ENTRY_NAME "hrtimer"
#define BUFFER_LEN	100



enum hrtimer_restart timer_callback( struct hrtimer *t);

int proc_open(struct inode *inode, struct file *filp);
ssize_t  proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t  proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
int proc_close(struct inode *inode, struct file *filp);

int proc_init(void);
void proc_exit(void);

struct proc_dir_entry *proc_entry = NULL;

atomic_t secs;
struct hrtimer timer;
unsigned long timer_interval_ns = NS_PER_SEC; //1 seconds



enum hrtimer_restart timer_callback( struct hrtimer *t)
{
  	ktime_t currtime, interval;
  	currtime  = ktime_get();
	atomic_inc(&secs);
	
  	interval = ktime_set(0,timer_interval_ns); 
  	hrtimer_forward(t, currtime, interval);
	return HRTIMER_RESTART;
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
	ktime_t interval;
	struct proc_dir_entry *proc_entry = NULL;
   	proc_entry = proc_create(ENTRY_NAME,0, NULL, &proc_ops);
	if(proc_entry == NULL)
   		return -ENOMEM;
	printk(KERN_INFO "LKM: /proc/%s created\n", ENTRY_NAME);
  	atomic_set(&secs, 0);
	interval = ktime_set(0,timer_interval_ns); 
	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer.function = &timer_callback;
 	hrtimer_start(&timer, interval, HRTIMER_MODE_REL);
	return 0;
}

 void proc_exit(void) {
	remove_proc_entry(ENTRY_NAME, NULL);
   	printk(KERN_INFO "LKM: Removing /proc/%s.\n", ENTRY_NAME);
// hrtimer_try_to_cancel: return: 0 when the timer was not active 1 when the timer was active -1 when the timer is currently excuting the callback function and cannot be stopped 
	while ((hrtimer_try_to_cancel(&timer))==-1){
		printk(KERN_INFO "LKM: The timer was still in use...\n");
	}
  	printk(KERN_INFO "LKM: HR Timer module uninstalling\n");
	
}

module_init(proc_init);
module_exit(proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LKM");
MODULE_DESCRIPTION("Hrtimer implementation");

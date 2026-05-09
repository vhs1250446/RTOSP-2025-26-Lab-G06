#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/jiffies.h>
#include <linux/timer.h>

#define ENTRY_NAME "ring_buffer"
#define BUFFER_LEN	100
#define QUEUE_LEN	5

////////////////////////////////////////////////////////////////////////////////
// Dynamic Kernel Module loading callbacks
//

int proc_init(void);
int proc_open(struct inode *inode, struct file *filp);
ssize_t  proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t  proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
int proc_close(struct inode *inode, struct file *filp);
void proc_exit(void);

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Circular Buffer Data Type(s)

struct queue_item{
	char buffer[BUFFER_LEN];
};
struct ring {
	struct queue_item queue[QUEUE_LEN];
	int write_item;
	int read_item;
};

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Local Function Prototypes

static void timer_callback(struct timer_list* t);
static int increment(int * item);
static int is_empty(int r, int w);
static int is_full(int r, int w);
static int enqueue (char *buffer);
static int dequeue (char *buffer);

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Local Global Variables == Kernel-side Storage

struct proc_dir_entry *proc_entry = NULL;
struct ring ring;
struct timer_list timer;
unsigned long TIMER_INTERVAL;
spinlock_t lock;
char LAST_MESSAGE[BUFFER_LEN];

////////////////////////////////////////////////////////////////////////////////

static void
timer_callback(struct timer_list* t)
{
	int ret = 0;
	char last_msg[BUFFER_LEN];

	///////////////////////////////////////////////////
	spin_lock(&lock);

	ret = dequeue(LAST_MESSAGE);
	strcpy(last_msg, LAST_MESSAGE);

	spin_unlock(&lock);
	///////////////////////////////////////////////////

	if (ret == 1) { // only print when new message is dequeued
		printk(KERN_INFO "LKM: %s:[%d] DEQUEUE: %s\n",
		       ENTRY_NAME,
		       current->pid,
		       last_msg);
	}

	mod_timer(&timer, timer.expires + TIMER_INTERVAL);
}

static int
increment(int* item)
{
	int ret;
	ret = *item;
	*item = (*item + 1) % QUEUE_LEN;
	return ret;
}

static int
is_empty(int r, int w)
{
	int ret;

	ret = !(r ^ w); // not xor

	return ret;
}

static int
is_full(int r, int w)
{
	int ret, write;
	write = (w + 1) % QUEUE_LEN;
	ret = (write == r);
	return ret;
}


static int
dequeue (char *destination)
{
	int ret = 0;
	char* source = NULL;

	if(!is_empty(ring.read_item, ring.write_item)) {
		source = ring.queue[ring.read_item].buffer;

		strcpy(destination, source);

		increment(&ring.read_item);

		ret = 1;
	}

	return ret;
}

static int
enqueue (char *buffer)
{
	if(is_full(ring.read_item, ring.write_item)) {
		increment(&ring.read_item); //remove from buffer
	}

	strcpy(ring.queue[ring.write_item].buffer, buffer);

	increment(&ring.write_item);

	return 1;
}

int
proc_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "LKM: %s:[%d] open\n", ENTRY_NAME, current->pid);
	return 0;
}


ssize_t
proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int ret = 0;
	int len = 0;
	char temp[BUFFER_LEN];

	printk(KERN_INFO "LKM: %s:[%d] read\n", ENTRY_NAME, current->pid);

	if (*f_pos > 0) {
		*f_pos = 0;
		return 0;
	}

	//////////////////////////////////////////////////////
	spin_lock(&lock);

	strcpy(temp, LAST_MESSAGE);

	spin_unlock(&lock);
	//////////////////////////////////////////////////////

	len = strlen(temp);
	if(len <= 0 || count < len) {
		return -EFAULT;
	}

	ret = raw_copy_to_user(buf, temp, len);
	if(ret != 0)
		return -EFAULT;

	*f_pos = len;

	return len;
}

ssize_t
proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int ret;
	char temp[BUFFER_LEN];

	printk(KERN_INFO "LKM: %s:[%d] write\n", ENTRY_NAME, current->pid);

	if (count >= BUFFER_LEN) {
        	return -EINVAL;
	}

	ret = raw_copy_from_user(temp, buf, count);
    	if (ret != 0) {
        	return -EFAULT;
	}

	temp[count] = 0;

	//////////////////////////////////////////////////////
	spin_lock(&lock);

	ret = enqueue(temp);

	spin_unlock(&lock);
	//////////////////////////////////////////////////////

	if(ret <= 0) {
		return ret;
	}

	return count;
}

int
proc_close(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "LKM: %s:[%d] close\n", ENTRY_NAME, current->pid);
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
   	proc_entry = proc_create(ENTRY_NAME, 0666, NULL, &proc_ops);
	if (proc_entry == NULL) {
   		return -ENOMEM;
	}

   	printk(KERN_INFO "LKM: /proc/%s created\n", ENTRY_NAME);

	spin_lock_init(&lock);

	ring.write_item = 0;
	ring.read_item 	= 0;

	TIMER_INTERVAL = msecs_to_jiffies(1000); // 1s
	timer_setup(&timer, timer_callback, 0);
	timer.expires = jiffies + TIMER_INTERVAL;

	add_timer(&timer);

   	return 0;
}


void
proc_exit(void)
{
   	remove_proc_entry(ENTRY_NAME, NULL);
   	printk(KERN_INFO "LKM: /proc/%s removed\n", ENTRY_NAME);
	timer_delete_sync(&timer);
}

module_init(proc_init);
module_exit(proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("VHS");
MODULE_DESCRIPTION("Ring buffer implementation. Dequeue via timer callback");

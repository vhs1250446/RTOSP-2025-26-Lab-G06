#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define BUFFER_LEN 100
#define ENTRY_NAME "fifo_queue"

static int pop (char *buffer);
static int push (char *buffer);

int proc_open(struct inode *inode, struct file *filp);
int proc_close(struct inode *inode, struct file *filp);
ssize_t proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

struct proc_dir_entry *proc_entry = NULL;

struct queue_item {
	char buffer[BUFFER_LEN];
	struct list_head node;
};

static DEFINE_MUTEX(mut);
static LIST_HEAD(head);


static int pop (char *buffer)
{
	struct queue_item *node_queue;

	/* safeguard against race conditions
	   let a = pop, b = push
	   race conditions = {x: x in a * b} where * is cartesian product
	*/
	pr_info("LKM[pid:%d] Locking for POP\n", current->pid);
	mutex_lock(&mut);

	if(list_empty(&head)) {
		goto list_empty;
	}

	node_queue = list_first_entry(&head, struct queue_item, node);

	list_del(&node_queue->node);

	mutex_unlock(&mut);
	pr_info("LKM[pid:%d] Unlocked for POP\n", current->pid);

	strcpy(buffer, node_queue->buffer);

	kfree(node_queue);

	return 1;

list_empty:
	mutex_unlock(&mut);
	return 0;
}


static int push (char *buffer)
{
	struct queue_item *node_queue;

	node_queue = kmalloc(sizeof(struct queue_item), GFP_KERNEL);
	if(!node_queue) {
		goto kmalloc_err;
	}

	pr_info("LKM[pid:%d] Locking for PUSH\n", current->pid);
	mutex_lock(&mut);

	strcpy(node_queue->buffer, buffer);

	list_add_tail(&node_queue->node, &head);

	mutex_unlock(&mut);
	pr_info("LKM[pid:%d] Unlocked for PUSH\n", current->pid);

	return 1;

kmalloc_err:
	pr_err("LKM[pid:%d]: Couldn't allocate memory.", current->pid);
	return 0;
}


int proc_open(struct inode *inode, struct file *filp)
{
	pr_info("LKM: %s:[%d] open\n", ENTRY_NAME, current->pid);
	return 0;
}

ssize_t proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	char local_buffer[BUFFER_LEN];
	int ret = 0;
	int len = 0;

	pr_info("LKM: %s:[%d] read\n", ENTRY_NAME, current->pid);

	if(*f_pos > 0) {
		return 0;
	}

	if(!pop(local_buffer)) {
		return 0;
	}

	len = strlen(local_buffer);
	if(len <= 0) {
		return -EFAULT;
	}

	if(count < len) {
		return -EFAULT;
	}

	ret = copy_to_user(buf, local_buffer, len);
	if(ret != 0) {
		return -EFAULT;
	}

	*f_pos += len;

	return len;
}

ssize_t proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int ret;
	char local_buffer[BUFFER_LEN];

	printk(KERN_INFO "LKM: %s:[%d] write\n",ENTRY_NAME, current->pid);

	if (count >= BUFFER_LEN) {
        	return -EINVAL;
	}

	// raw_copy_from/to_user is arch dependent
	ret = copy_from_user(local_buffer, buf, count);
    	if (ret != 0) {
        	return -EFAULT;
	}

	local_buffer[count] = '\0';

	if(!push(local_buffer)) {
		return -EFAULT;
	}

	return count;
}


int proc_close(struct inode *inode, struct file *filp)
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


static int __init when_insmod(void)
{
	int ret;

   	proc_entry = proc_create(ENTRY_NAME, 0666, NULL, &proc_ops);
	if(!proc_entry) {
   		ret = -ENOMEM;
		goto err;
	}
   	printk(KERN_INFO "LKM: /proc/%s created\n", ENTRY_NAME);

   	return 0;
err:
	pr_err("LKM: Failed to create /proc/%s.\n", ENTRY_NAME);
	return ret;
}

static void __exit when_rmmod(void)
{
	struct queue_item *tmp;
	struct list_head *pos, *q;

   	remove_proc_entry(ENTRY_NAME, NULL);
   	pr_info("LKM: /proc/%s removed\n", ENTRY_NAME);

	list_for_each_safe(pos, q, &head) {
		tmp = list_entry(pos, struct queue_item, node);
		list_del(pos);
		kfree(tmp);
	}
}

module_init(when_insmod);
module_exit(when_rmmod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("vhs-1250446");
MODULE_DESCRIPTION("Exercise 2.a - Synchronized FIFO queue.");

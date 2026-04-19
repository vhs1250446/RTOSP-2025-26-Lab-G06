#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/rbtree.h>
#include <linux/uaccess.h>


#define BUFFER_LEN 100
#define ENTRY_NAME "sorted_queue"


static int proc_open(struct inode *inode, struct file *filp);
static int proc_close(struct inode *inode, struct file *filp);
static ssize_t proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

static int pop (char *buffer);
static int push (char *buffer);


struct queue_item{
	char buffer[BUFFER_LEN];
	struct rb_node node;
};

static struct proc_dir_entry *proc_entry = NULL;

static struct rb_root root = RB_ROOT; // RB_ROOT inits the tree

static DEFINE_SPINLOCK(lock);

typedef int (*compar)(const char *this, const char *that);
static compar cmp;

static int alpha_order (const char *this, const char *that)
{
	return strcmp(this, that);
}


static int pop (char *buffer)
{
	struct rb_node *node;
	struct queue_item *elem;

	pr_info("LKM[pid:%d] Locking for POP\n", current->pid);
	spin_lock(&lock);

	node = rb_first(&root);
	if(!node) {
		goto empty_tree;
	}

	elem = rb_entry(node, struct queue_item, node);
	strcpy(buffer, elem->buffer);
	rb_erase(node, &root);

	spin_unlock(&lock);
	pr_info("LKM[pid:%d] Unlocked for POP\n", current->pid);

	kfree(elem);

	return 1;

empty_tree:
	spin_unlock(&lock);
	pr_info("LKM[pid:%d] Unlocked for POP\n", current->pid);
	pr_err("LKM[pid:%d] Tree is empty\n", current->pid);
	return 0;
}


static int push(char *buffer)
{
        struct rb_node **link;
        struct rb_node *parent = NULL;
        struct queue_item *entry;
        struct queue_item *elem;
        int cmp_res;

        elem = kmalloc(sizeof(struct queue_item), GFP_KERNEL);
        if (!elem) {
		goto kmalloc_err;
	}

        strcpy(elem->buffer, buffer);

	pr_info("LKM[pid:%d] Locking for PUSH\n", current->pid);
        spin_lock(&lock);

        link = &root.rb_node;

        while(*link != NULL) {
                parent = *link;
                entry = rb_entry(parent, struct queue_item, node);

                cmp_res = cmp(elem->buffer, entry->buffer);

                if (cmp_res < 0) {
                        link = &parent->rb_left;
		} else {
                        link = &parent->rb_right;
		}
        }

        rb_link_node(&elem->node, parent, link);
        rb_insert_color(&elem->node, &root);

        spin_unlock(&lock);
	pr_info("LKM[pid:%d] Unlocked for PUSH\n", current->pid);

        return 1;

kmalloc_err:
	pr_err("LKM[pid:%d] Couldn't allocate memory\n", current->pid);

	return 0;
}


static int proc_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "LKM: %s:[%d] open\n", ENTRY_NAME, current->pid);

	return 0;
}


static ssize_t proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	char local_buffer[BUFFER_LEN];
	int ret = 0;
	int len = 0;

	printk(KERN_INFO "LKM: %s:[%d] read\n",ENTRY_NAME, current->pid);

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

	pr_info("LKM[pid:%d]: Sending:%s\n", current->pid, local_buffer);

	*f_pos += len;

	return len;
}


static ssize_t proc_write(struct file *filp, const char __user *buf, size_t count,
			  loff_t *f_pos)
{
	int ret;
	char local_buffer[BUFFER_LEN];

	printk(KERN_INFO "LKM: %s:[%d] write\n",ENTRY_NAME, current->pid);
	if(count >= BUFFER_LEN) {
        	return -EINVAL;
	}

	ret = copy_from_user(local_buffer, buf, count);
    	if(ret != 0) {
        	return -EFAULT;
	}

	local_buffer[count] = '\0';
	if(!push(local_buffer)) {
		return -EFAULT;
	}

	pr_info("LKM[pid:%d]: Pushed:%s\n", current->pid, local_buffer);

	return count;
}


static int proc_close(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "LKM: %s:[%d] release\n",ENTRY_NAME, current->pid);
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
   	proc_entry = proc_create(ENTRY_NAME,0, NULL, &proc_ops);
	if(proc_entry == NULL) {
   		return -ENOMEM;
	}

	cmp = alpha_order; // set comparator as the alphabetic order

   	printk(KERN_INFO "LKM: /proc/%s created\n", ENTRY_NAME);

   	return 0;
}


static void __exit when_rmmod(void)
{
	struct queue_item *tmp;
	struct rb_node *node;

   	remove_proc_entry(ENTRY_NAME, NULL);
   	printk(KERN_INFO "LKM: /proc/%s removed\n", ENTRY_NAME);

	node = rb_first(&root);

	while (node != NULL) {
		tmp = rb_entry(node, struct queue_item, node);
		rb_erase(node, &root);
		kfree(tmp);
		node = rb_first(&root);
	}
}

module_init(when_insmod);
module_exit(when_rmmod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("vhs");
MODULE_DESCRIPTION("Sorted queue implementation");

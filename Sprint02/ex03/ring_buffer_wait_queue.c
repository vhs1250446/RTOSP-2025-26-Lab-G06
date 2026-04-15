#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/wait.h>   
#include <linux/mutex.h>  

#define ENTRY_NAME "ring_buffer"
#define BUFFER_LEN  100
#define QUEUE_LEN   5

struct queue_item{ 
    char buffer[BUFFER_LEN];
}; 

struct ring{
    struct queue_item queue[QUEUE_LEN];
    int write_item;
    int read_item;
};

static int increment(int * item);
static int is_empty(int r, int w);
static int is_full(int r, int w);
static int enqueue (char *buffer);
static int dequeue (char *buffer);

int proc_open(struct inode *inode, struct file *filp);
ssize_t  proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t  proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
int proc_close(struct inode *inode, struct file *filp);
int proc_init(void);
void proc_exit(void);

struct ring ring;
struct proc_dir_entry *proc_entry = NULL;

/* Declaration of Wait Queues and Mutex */
DECLARE_WAIT_QUEUE_HEAD(read_queue);
DECLARE_WAIT_QUEUE_HEAD(write_queue);
DEFINE_MUTEX(ring_mutex);

static int increment(int * item) {
    int ret = *item;
    *item = (*item + 1) % QUEUE_LEN;
    return ret;
}

static int is_empty(int r, int w) {
    return !(r ^ w); // xor
}

static int is_full(int r, int w) {
    int write = (w + 1) % QUEUE_LEN;
    return (write == r);
}

static int dequeue (char *buffer) {
    int ret = 0;
    if(!is_empty(ring.read_item,ring.write_item)){ 
        strcpy(buffer, ring.queue[ring.read_item].buffer);
        increment(&ring.read_item);
        ret = 1;
    }
    return ret;
}

static int enqueue (char *buffer) {
    /* enqueue no longer overwrites old items if full */
    strcpy(ring.queue[ring.write_item].buffer,buffer);
    increment(&ring.write_item);
    return 1;
}

int proc_open(struct inode *inode, struct file *filp) {
    printk(KERN_INFO "LKM: %s:[%d] open\n",ENTRY_NAME, current->pid);
    return 0;
}

ssize_t  proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    char buffer[BUFFER_LEN];
    int ret = 0, len = 0;
    
    if(*f_pos > 0) return 0;

    /* If empty, the reader sleeps on the read_queue */
    if (wait_event_interruptible(read_queue, !is_empty(ring.read_item, ring.write_item))) {
        return -ERESTARTSYS; /* Interrupted by a signal */
    }

    /* Enter critical section to remove the item */
    mutex_lock(&ring_mutex);
    ret = dequeue(buffer);
    mutex_unlock(&ring_mutex);

    /* We just read and freed space, so wake up sleeping writers */
    wake_up_interruptible(&write_queue);

    if(ret <= 0) return ret;
    
    len = strlen(buffer);
    if(count < len) return -EFAULT;
    if(raw_copy_to_user(buf,buffer,len) != 0) return -EFAULT;
    
    *f_pos += len;
    return len;
}

ssize_t  proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    int ret;
    char buffer[BUFFER_LEN];
    
    if (count > BUFFER_LEN) return -EINVAL;
    if (raw_copy_from_user(buffer, buf, count) != 0) return -EFAULT;
    buffer[count] = 0;

    /* If full, the writer sleeps on the write_queue */
    if (wait_event_interruptible(write_queue, !is_full(ring.read_item, ring.write_item))) {
        return -ERESTARTSYS;
    }

    /* Enter critical section to insert the item */
    mutex_lock(&ring_mutex);
    ret = enqueue(buffer);
    mutex_unlock(&ring_mutex);

    /* We just wrote, so wake up sleeping readers */
    wake_up_interruptible(&read_queue);

    if(ret <= 0) return ret;
    return count;
}

int proc_close(struct inode *inode, struct file *filp) {
    printk(KERN_INFO "LKM: %s:[%d] close\n",ENTRY_NAME, current->pid);
    return 0; 
}

static const struct proc_ops proc_ops = {
    .proc_open      = proc_open,
    .proc_read      = proc_read,
    .proc_write     = proc_write,
    .proc_release   = proc_close,
};

int proc_init(void) {
    proc_entry = proc_create(ENTRY_NAME,0666, NULL, &proc_ops);
    if(proc_entry == NULL) return -ENOMEM;
  
    ring.write_item = 0;
    ring.read_item  = 0;
    return 0;
}

void proc_exit(void) {
    remove_proc_entry(ENTRY_NAME, NULL);
    printk(KERN_INFO "LKM: /proc/%s removed\n", ENTRY_NAME);
}

module_init(proc_init);
module_exit(proc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("PBS");
MODULE_DESCRIPTION("Ring buffer with Wait Queues");
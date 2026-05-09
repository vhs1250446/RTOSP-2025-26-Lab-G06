#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/semaphore.h> 

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

/* Declaration of the 3 Semaphores */
struct semaphore empty_slots; // Controls the free slots
struct semaphore full_slots;  // Controls the messages ready to read
struct semaphore mutex_sem;   // Guarantees mutual exclusion (like a Mutex)

static int increment(int * item)
{
    int ret;
    ret = *item;
    *item = (*item + 1) % QUEUE_LEN;
    return ret;
}

/* dequeue no longer needs is_empty. Semaphores block if it is empty! */
static int dequeue (char *buffer)
{
    strcpy(buffer, ring.queue[ring.read_item].buffer);
    increment(&ring.read_item);
    return 1;
}

/* enqueue no longer overwrites data or uses is_full. Semaphores block if it is full! */
static int enqueue (char *buffer)
{
    strcpy(ring.queue[ring.write_item].buffer,buffer);
    increment(&ring.write_item);
    return 1;
}

int proc_open(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "LKM: %s:[%d] open\n",ENTRY_NAME, current->pid);
    return 0;
}

ssize_t  proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    char buffer[BUFFER_LEN];
    int ret = 0, len = 0;
    printk(KERN_INFO "LKM: %s:[%d] read\n",ENTRY_NAME, current->pid);

    if(*f_pos > 0)
        return 0;

    /* 1. The Consumer requests 1 item (waits if full_slots is 0) */
    if (down_interruptible(&full_slots))
        return -ERESTARTSYS;

    /* 2. Lock the critical section */
    if (down_interruptible(&mutex_sem)) {
        up(&full_slots); /* Return the ticket if the thread is canceled */
        return -ERESTARTSYS;
    }

    /* 3. Read the data safely */
    ret = dequeue(buffer);

    /* 4. Unlock the critical section */
    up(&mutex_sem);

    /* 5. Notify Producers that there is 1 more empty slot! */
    up(&empty_slots);

    if(ret <= 0)
        return ret;
        
    len = strlen(buffer);
    if(len <= 0)
        return -EFAULT;
    if(count < len)
        return -EFAULT;
        
    ret = raw_copy_to_user(buf, buffer, len);
    if(ret != 0)
        return -EFAULT;
        
    *f_pos += count - len;
    return len;
}

ssize_t  proc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    int ret;
    char buffer[BUFFER_LEN];
    printk(KERN_INFO "LKM: %s:[%d] write\n",ENTRY_NAME, current->pid);
    
    if (count > BUFFER_LEN)
            return -EINVAL;
            
    ret = raw_copy_from_user(buffer, buf, count);
    if (ret != 0)
        return -EFAULT;
            
    buffer[count] = 0;

    /* 1. The Producer requests 1 empty slot (waits if empty_slots is 0) */
    if (down_interruptible(&empty_slots))
        return -ERESTARTSYS;

    /* 2. Lock the critical section */
    if (down_interruptible(&mutex_sem)) {
        up(&empty_slots);
        return -ERESTARTSYS;
    }

    /* 3. Write the data into the queue */
    ret = enqueue(buffer);

    /* 4. Unlock the critical section */
    up(&mutex_sem);

    /* 5. Notify Consumers that there is 1 more message to read! */
    up(&full_slots);

    if(ret <= 0)
        return ret;
    return count;
}

int proc_close(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "LKM: %s:[%d] close\n",ENTRY_NAME, current->pid);
    return 0; 
}

static const struct proc_ops proc_ops = {
    .proc_open      = proc_open,
    .proc_read      = proc_read,
    .proc_write     = proc_write,
    .proc_release   = proc_close,
};

int proc_init(void)
{
    proc_entry = proc_create(ENTRY_NAME,0666, NULL, &proc_ops);
    if(proc_entry == NULL)
        return -ENOMEM;
  
    printk(KERN_INFO "LKM: /proc/%s created\n", ENTRY_NAME);

    ring.write_item = 0;
    ring.read_item  = 0;

    /* Initialization of Semaphores */
    sema_init(&empty_slots, QUEUE_LEN); /* Starts with full capacity (5 slots) */
    sema_init(&full_slots, 0);          /* Starts with 0 items to read */
    sema_init(&mutex_sem, 1);           /* Binary semaphore starts unlocked (1) */

    return 0;
}

void proc_exit(void)
{
    remove_proc_entry(ENTRY_NAME, NULL);
    printk(KERN_INFO "LKM: /proc/%s removed\n", ENTRY_NAME);
}

module_init(proc_init);
module_exit(proc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("PBS");
MODULE_DESCRIPTION("Ring buffer implementation with Semaphores");
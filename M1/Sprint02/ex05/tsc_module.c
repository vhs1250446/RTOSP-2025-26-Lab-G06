#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/msr.h> 

#define ENTRY_NAME "tsc_value"

int proc_open(struct inode *inode, struct file *filp)
{
    return 0;
}

int proc_close(struct inode *inode, struct file *filp)
{
    return 0; 
}

ssize_t proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    char buffer[100];
    int len;
    unsigned long long tsc_val;

    /* If we already read, return 0 to indicate End Of File (EOF) */
    if (*f_pos > 0)
        return 0;

    /* 1. Read the Time Stamp Counter directly from the CPU */
    tsc_val = rdtsc();

    /* 2. Format the 64-bit number (%llu) into a text string */
    len = sprintf(buffer, "Current TSC: %llu CPU cycles\n", tsc_val);

    if (count < len)
        return -EFAULT;
        
    /* 3. Send the formatted text to the user space */
    if (raw_copy_to_user(buf, buffer, len) != 0)
        return -EFAULT;
        
    *f_pos += len;
    return len;
}

static const struct proc_ops proc_ops = {
    .proc_open      = proc_open,
    .proc_read      = proc_read,
    .proc_release   = proc_close,
};

struct proc_dir_entry *proc_entry = NULL;

int proc_init(void)
{
    /* 0444 Permissions (Read-Only) since we only read data */
    proc_entry = proc_create(ENTRY_NAME, 0444, NULL, &proc_ops);
    if(proc_entry == NULL)
        return -ENOMEM;
  
    printk(KERN_INFO "LKM: /proc/%s created successfully\n", ENTRY_NAME);
    return 0;
}

void proc_exit(void)
{
    remove_proc_entry(ENTRY_NAME, NULL);
    printk(KERN_INFO "LKM: /proc/%s removed successfully\n", ENTRY_NAME);
}

module_init(proc_init);
module_exit(proc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("PBS");
MODULE_DESCRIPTION("TSC Reader Module");


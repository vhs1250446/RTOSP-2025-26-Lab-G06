#include <linux/module.h> /* Needed by all modules */
#include <linux/kernel.h> /* Needed for KERN_INFO */
#include <linux/kthread.h>      
#include <linux/delay.h>
#include<linux/wait.h>

#define FN_NAME 30
#define NTHREADS 2

int  proc_init(void);
void proc_exit(void);
int simple_thread(void *data);
int waking_thread(void *data);

struct data{
	char name[FN_NAME];
	int id;
};
struct function{
	struct data data;
	int (*fn)(void *data);
};

struct function kthfns[NTHREADS]= {
	[0].data.id = 1,
	[0].data.name = "waking_thread",
	[0].fn = waking_thread,
	[1].data.id = 2,
	[1].data.name = "simple_thread",
	[1].fn = simple_thread,
};
struct task_struct *kths[NTHREADS];
wait_queue_head_t wq; 
unsigned int flag = 0;

int simple_thread(void *data){
	struct data *ptr = (struct data *) data;
	int i = 1;
	while(1){
		printk(KERN_INFO "LKM:[%d]:[%d]: %s waiting [%d] \n", current->pid, ptr->id, ptr->name, i);
	    	wait_event_interruptible(wq, flag!=0);
	    	printk(KERN_INFO "LKM:[%d]:[%d]: %s waking up [%d] \n", current->pid, ptr->id, ptr->name, i);
	    	printk(KERN_INFO "LKM:[%d]:[%d]: %s executing [%d] \n", current->pid, ptr->id, ptr->name, i);
		if(kthread_should_stop()) {
		    	printk(KERN_INFO "LKM:[%d]:[%d]: %s exiting [%d] \n", current->pid, ptr->id, ptr->name, i);
		       	return 0;    
	      	}
	      	flag = 0;
	      	i++;
   	}
}
int waking_thread(void *data){
	
	struct data *ptr = (struct data *) data;
	int i = 1;
	while(1){
		printk(KERN_INFO "LKM:[%d]:[%d]: %s sleeping [%d] \n", current->pid, ptr->id, ptr->name, i);
	      	msleep(ptr->id * 1000);
	      	printk(KERN_INFO "LKM:[%d]:[%d]: %s waking up [%d] \n", current->pid, ptr->id, ptr->name, i);
	      	printk(KERN_INFO "LKM:[%d]:[%d]: %s executing [%d] \n", current->pid, ptr->id, ptr->name, i);
		flag = 1;
		printk(KERN_INFO "LKM:[%d]:[%d]: %s signaling [%d] \n", current->pid, ptr->id, ptr->name, i);
		wake_up_interruptible(&wq);
		if(kthread_should_stop()) {
			printk(KERN_INFO "LKM:[%d]:[%d]: %s exiting [%d] \n", current->pid, ptr->id, ptr->name, i);
		       	return 0;    
	      	}
	     	i++;
   	}
}

int __init proc_init(void){
	
	init_waitqueue_head(&wq);
	for(int i = 0; i < NTHREADS; i++){
		kths[i] = kthread_create(kthfns[i].fn,&kthfns[i].data,kthfns[i].data.name);
	}
	for(int i = 0; i < NTHREADS; i++){
		if(kths[i] != NULL){
			printk(KERN_INFO "LKM:[%d]:[%d]: %s created successfully\n", current->pid, kthfns[i].data.id, kthfns[i].data.name);
            		wake_up_process(kths[i]);
		}
		else {
            		printk(KERN_INFO "LKM:[%d]:[%d]: %s not created\n", current->pid, kthfns[i].data.id, kthfns[i].data.name);
        	}
		
	}
	return 0;
}
void __exit proc_exit(void){
	for(int i = 0; i < NTHREADS; i++){
		if(kths[i] != NULL){
			kthread_stop(kths[i]);
			printk(KERN_INFO "LKM:[%d]:[%d]: %s stopped successfully\n", current->pid, kthfns[i].data.id, kthfns[i].data.name);
		}
	}
}
module_init(proc_init);
module_exit(proc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("PBS");
MODULE_DESCRIPTION("The kernel thread example");




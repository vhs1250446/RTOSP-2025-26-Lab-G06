#include <linux/module.h> /* Needed by all modules */
#include <linux/kernel.h> /* Needed for KERN_INFO */
#include<linux/workqueue.h>
#define NTASKS 2

int proc_init(void);
void  proc_exit(void);
void task_function(struct work_struct *work);

static struct workqueue_struct *wq;

typedef struct task {
    struct work_struct work;
    int data;
} task_t;

void task_function(struct work_struct *work) {
    task_t *task = container_of(work, task_t, work);
    int id = task->data;
    printk(KERN_INFO "LKM: %s:[%d]:[%d] Work function \n",current->comm, current->pid, id);
    kfree(task);
}

int __init proc_init(void)
{	
	wq = create_workqueue("LKM_WQ");
	if (wq) {
	task_t *tasks[NTASKS];
	for(int i = 0; i < NTASKS; i++){
		tasks[i] = kmalloc(sizeof(task_t), GFP_KERNEL);
		if (tasks[i]) {
		    INIT_WORK(&tasks[i]->work, task_function);
		    tasks[i]->data = i + 1;
		    queue_work(wq, &tasks[i]->work);
		}
	}
    }
	return 0;
}
void __exit proc_exit(void)
{
   	flush_workqueue(wq);
    	destroy_workqueue(wq);
}
module_init(proc_init);
module_exit(proc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("PBS");
MODULE_DESCRIPTION("A worqueue kernel module example");




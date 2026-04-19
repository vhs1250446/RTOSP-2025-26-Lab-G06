#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/atomic.h> // for atomics
#include <linux/wait.h> // for waitq

#define ENTRY_NAME "Waker of 2"

static int kth_waker_fn(void *args);
static int kth_increment_fn(void *args);
static int kth_decrement_fn(void *args);

static struct task_struct *kth_inc, *kth_dec, *kth_waker;
static int th_inc = 1, th_dec = 2, th_waker = 3;

static atomic64_t shared = ATOMIC64_INIT(42);

static DECLARE_WAIT_QUEUE_HEAD(inc_wq);
static DECLARE_WAIT_QUEUE_HEAD(dec_wq);
static atomic_t inc_condition = ATOMIC_INIT(0);
static atomic_t dec_condition = ATOMIC_INIT(0);


static int kth_waker_fn(void *data)
{
	// int id = *((int *) data);

	while(!kthread_should_stop()) {

		atomic_inc(&inc_condition);
		wake_up(&inc_wq);
		pr_info("LKM:[%d]: Waking up INCREMENT Kthread[pid:%d]\n",
			current->pid,
			th_inc);

		atomic_inc(&dec_condition);
		wake_up(&dec_wq);
		pr_info("LKM:[%d]: Waking up DECREMENT Kthread[%d]\n",
			current->pid,
			th_dec);

		msleep(1000 * 1);
	}

	return 0;
}

static int kth_increment_fn(void *args)
{
	int id = *((int *) args);
	s64 val;

	/*
	  This is so noisy...
	  Due to sporious wakes from the scheduler, we have 2 call sites for
	  kthread_should_stop().
	  1 call site is for the "keep alive" loop
	  the other is for the waitQ condition.
	 */

	while(!kthread_should_stop()) {
		// wait_event_interruptible - sleep until a condition gets true
		wait_event_interruptible(inc_wq,
					 atomic_read(&inc_condition) > 0 ||
					 kthread_should_stop());

		if (kthread_should_stop()) {
			pr_info("LKM:[pid:%d]: Kthread [id:%d] Stopping\n",
				current->pid, id);
			break;
		}

		atomic_dec(&inc_condition);

		val = atomic64_inc_return(&shared);
		pr_info("LKM:[pid:%d]:Kthread [id:%d] Incremented shared to %lld\n",
			current->pid, id, val);
	}

	return 0;
}

static int kth_decrement_fn(void *args)
{
	s64 val;
	int id = *((int *) args);

	while(!kthread_should_stop()) {
		wait_event_interruptible(dec_wq,
					 atomic_read(&dec_condition) > 0 ||
					 kthread_should_stop());

		if(kthread_should_stop()) {
			pr_info("LKM:[pid:%d]: Kthread [id:%d] Stopping\n",
				current->pid, id);
			break;
		}

		atomic_dec(&dec_condition);

		val = atomic64_dec_return(&shared);
		pr_info("LKM:[pid:%d]: Kthread [id:%d] Decremented shared to %lld\n",
			current->pid, id, val);

	}

	return 0;
}

static int __init when_insmod(void)
{
	int ret;

	kth_inc = kthread_run(kth_increment_fn, &th_inc, ENTRY_NAME);
        if(IS_ERR(kth_inc)) {
		ret = PTR_ERR(kth_inc);
		kth_inc = NULL;
		goto err_kth_inc;
	}
	pr_info("LKM:[%d]:[%d] Kthread Created Successfully\n",
		current->pid,
		th_inc);

	kth_dec = kthread_run(kth_decrement_fn, &th_dec, ENTRY_NAME);
        if(IS_ERR(kth_dec)) {
		ret = PTR_ERR(kth_dec);
		kth_dec = NULL;
		goto err_kth_dec;
	}
	pr_info("LKM:[%d]:[%d] Kthread Created Successfully\n",
		current->pid,
		th_dec);

        kth_waker = kthread_run(kth_waker_fn, &th_waker, ENTRY_NAME);
        if(IS_ERR(kth_waker)) {
		ret = PTR_ERR(kth_waker);
		kth_waker = NULL;
		goto err_kth_waker;
        }
	pr_info("LKM:[%d]:[%d] Kthread Created Successfully\n",
		current->pid,
		th_waker);

	return 0;

err_kth_waker:
	kthread_stop(kth_dec);
	kth_dec = NULL;
err_kth_dec:
	kthread_stop(kth_inc);
	kth_inc = NULL;
err_kth_inc:
	pr_err("LKM:[%d]:[%d] Couldn't create thread\n", current->pid, ret);
	return ret;
}

static void __exit when_rmmod(void)
{
	if(kth_waker) {
		kthread_stop(kth_waker);
		kth_waker = NULL;
		pr_info("LKM:[%d]:[%d] Kthread Stopped Successfully\n",
			current->pid,
			th_waker);
	}

	wake_up(&inc_wq);
	pr_info("LKM:[%d]: Kthread [%d] Stopped Successfully\n",
		current->pid,
		th_inc);
	if(kth_inc) {
		kthread_stop(kth_inc);
		kth_inc = NULL;
		pr_info("LKM:[%d]: Kthread [%d] Stopped Successfully\n",
			current->pid,
			th_inc);
	}

	wake_up(&dec_wq);
	if(kth_dec) {
		kthread_stop(kth_dec);
		kth_dec = NULL;
		pr_info("LKM:[%d]: Kthread [%d] Stopped Successfully\n",
			current->pid,
			th_dec);
	}

}



module_init(when_insmod);
module_exit(when_rmmod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("vhs");
MODULE_DESCRIPTION("The kernel thread example");

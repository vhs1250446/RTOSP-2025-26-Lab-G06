#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#include <linux/atomic.h> // for atomics
#include <linux/wait.h> // for waitq

#define ENTRY_NAME "Waking Writer Reader"


static int kthread_waker_fn(void *args);
static int kthread_writer_fn(void *args);
static int kthread_reader_fn(void *args);

static struct task_struct *kthread_waker, *kthread_writer, *kthread_reader;
static int kthread_waker_id = 1, kthread_writer_id = 2, kthread_reader_id = 3;

static atomic64_t shared = ATOMIC64_INIT(42);

static DECLARE_WAIT_QUEUE_HEAD(writer_wq);
static DECLARE_WAIT_QUEUE_HEAD(reader_wq);

static atomic_t writer_go_condition = ATOMIC_INIT(0);
static atomic_t writer_finished_condition = ATOMIC_INIT(0);



static int kthread_waker_fn(void *args)
{
	int id = *((int*) args);

	while(!kthread_should_stop()) {

		atomic_inc(&writer_go_condition);
		wake_up(&writer_wq);
		pr_info("LKM:[pid:%d]:[id:%d] Waking up Writer Kthread\n",
			current->pid,
			id);

		msleep(1000 * 1);
	}

	return 0;
}

static int kthread_writer_fn(void *args)
{
	int id = *((int*) args);
	s64 val;

	while(!kthread_should_stop()) {
		wait_event_interruptible(writer_wq,
					 atomic_read(&writer_go_condition) > 0 ||
					 kthread_should_stop());

		if (kthread_should_stop()) {
			pr_info("LKM:[pid:%d]:[id:%d] Stopping\n",
				current->pid,
				id);
			break;
		}

		atomic_dec(&writer_go_condition);

		val = atomic64_inc_return(&shared);

		atomic_inc(&writer_finished_condition); // signal work done

		wake_up(&reader_wq);
		pr_info("LKM:[pid:%d]:[id:%d] Waking up Reader Kthread\n",
			current->pid,
			id);
	}

	return 0;
}

static int kthread_reader_fn(void *args)
{
	int id = *((int *) args);
	s64 val;

	while(!kthread_should_stop()) {
		wait_event_interruptible(reader_wq,
					 atomic_read(&writer_finished_condition) > 0 ||
					 kthread_should_stop());

		if(kthread_should_stop()) {
			pr_info("LKM:[pid:%d]:[id:%d] Stopping\n",
				current->pid, id);
			break;
		}

		atomic_dec(&writer_finished_condition);

		val = atomic64_read(&shared);
		pr_info("LKM:[pid:%d]:[id:%d] SHARED has value %lld\n",
			current->pid,
			id,
			val);

	}

	return 0;
}

static int __init when_insmod(void)
{
	int ret;

	pr_info("LKM:[pid:%d] Kthread[id:%d] is Waker\n",
		current->pid,
		kthread_waker_id);
	pr_info("LKM:[pid:%d] Kthread[id:%d] is Writer\n",
		current->pid,
		kthread_writer_id);
	pr_info("LKM:[pid:%d] Kthread[id:%d] is Reader\n",
		current->pid,
		kthread_reader_id);

	kthread_writer = kthread_run(kthread_writer_fn, &kthread_writer_id, ENTRY_NAME);
        if(IS_ERR(kthread_writer)) {
		ret = PTR_ERR(kthread_writer);
		kthread_writer = NULL;
		goto err_kth_writer;
	}
	pr_info("LKM:[pid:%d]: Kthread[id:%d] Created Successfully\n",
		current->pid,
		kthread_writer_id);

	kthread_reader = kthread_run(kthread_reader_fn, &kthread_reader_id, ENTRY_NAME);
        if(IS_ERR(kthread_reader)) {
		ret = PTR_ERR(kthread_reader);
		kthread_reader = NULL;
		goto err_kth_reader;
	}
	pr_info("LKM:[pid:%d]: Kthread[id:%d] Created Successfully\n",
		current->pid,
		kthread_reader_id);

        kthread_waker = kthread_run(kthread_waker_fn, &kthread_waker_id, ENTRY_NAME);
        if(IS_ERR(kthread_waker)) {
		ret = PTR_ERR(kthread_waker);
		kthread_waker = NULL;
		goto err_kth_waker;
        }
	pr_info("LKM:[pid:%d]: Kthread[id:%d] Created Successfully\n",
		current->pid,
		kthread_waker_id);

	return 0;

err_kth_waker:
	kthread_stop(kthread_reader);
	kthread_reader = NULL;
err_kth_reader:
	kthread_stop(kthread_writer);
	kthread_writer = NULL;
err_kth_writer:
	pr_err("LKM:[%d]:[%d] Couldn't create thread\n", current->pid, ret);
	return ret;
}

static void __exit when_rmmod(void)
{
	if(kthread_waker) {
		kthread_stop(kthread_waker);
		kthread_waker = NULL;
		pr_info("LKM:[%d]:[%d] Kthread Stopped Successfully\n",
			current->pid,
			kthread_waker_id);
	}

	wake_up(&writer_wq);
	if(kthread_writer) {
		kthread_stop(kthread_writer);
		kthread_writer = NULL;
		pr_info("LKM:[%d]: Kthread [%d] Stopped Successfully\n",
			current->pid,
			kthread_writer_id);
	}

	wake_up(&reader_wq);
	if(kthread_reader) {
		kthread_stop(kthread_reader);
		kthread_reader = NULL;
		pr_info("LKM:[%d]: Kthread [%d] Stopped Successfully\n",
			current->pid,
			kthread_reader_id);
	}
}



module_init(when_insmod);
module_exit(when_rmmod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("vhs");
MODULE_DESCRIPTION("exercise 6.b");

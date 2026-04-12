#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/atomic.h>
#include <linux/spinlock.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

#define NTHREADS 3

int proc_init(void);
void proc_exit(void);

static int controller_thread(void *data);
static int increment_thread(void *data);
static int decrement_thread(void *data);
static enum hrtimer_restart activator_timer_callback(struct hrtimer *timer);

static struct task_struct *kths[NTHREADS];
static wait_queue_head_t wq;
static wait_queue_head_t controller_wq;
static spinlock_t token_lock;
static atomic_t shared_value = ATOMIC_INIT(0);
static int inc_tokens;
static int dec_tokens;
static unsigned long activation_cycle;
static int controller_tick;
static struct hrtimer activator_timer;
static ktime_t activator_period;

static enum hrtimer_restart
activator_timer_callback(struct hrtimer *timer)
{
	spin_lock(&token_lock);
	controller_tick = 1;
	spin_unlock(&token_lock);

	wake_up_interruptible(&controller_wq);
	hrtimer_forward_now(timer, activator_period);

	return HRTIMER_RESTART;
}

static int
increment_thread(void *data)
{
	while (!kthread_should_stop()) {
		unsigned long cycle = 0;
		int has_token = 0;

		wait_event_interruptible(wq, kthread_should_stop() || inc_tokens > 0);
		if (kthread_should_stop())
			break;

		spin_lock(&token_lock);
		if (inc_tokens > 0) {
			inc_tokens--;
			cycle = activation_cycle;
			has_token = 1;
		}
		spin_unlock(&token_lock);

		if (!has_token)
			continue;

		printk(KERN_INFO "LKM:[%d] increment thread: cycle=%lu value=%d\n",
		       current->pid,
		       cycle,
		       atomic_inc_return(&shared_value));
	}

	return 0;
}

static int
decrement_thread(void *data)
{
	while (!kthread_should_stop()) {
		unsigned long cycle = 0;
		int has_token = 0;

		wait_event_interruptible(wq, kthread_should_stop() || dec_tokens > 0);
		if (kthread_should_stop())
			break;

		spin_lock(&token_lock);
		if (dec_tokens > 0) {
			dec_tokens--;
			cycle = activation_cycle;
			has_token = 1;
		}
		spin_unlock(&token_lock);

		if (!has_token)
			continue;

		printk(KERN_INFO "LKM:[%d] decrement thread: cycle=%lu value=%d\n",
		       current->pid,
		       cycle,
		       atomic_dec_return(&shared_value));
	}

	return 0;
}

static int
controller_thread(void *data)
{
	while (!kthread_should_stop()) {
		unsigned long cycle;

		wait_event_interruptible(controller_wq,
			kthread_should_stop() || controller_tick != 0);
		if (kthread_should_stop())
			break;

		spin_lock(&token_lock);
		if (controller_tick == 0) {
			spin_unlock(&token_lock);
			continue;
		}
		controller_tick = 0;
		activation_cycle++;
		cycle = activation_cycle;
		inc_tokens++;
		dec_tokens++;
		spin_unlock(&token_lock);

		wake_up_interruptible(&wq);
		printk(KERN_INFO "LKM:[%d] controller thread: cycle=%lu activated workers\n",
		       current->pid,
		       cycle);
	}

	return 0;
}

int __init
proc_init(void)
{
	init_waitqueue_head(&wq);
	init_waitqueue_head(&controller_wq);
	spin_lock_init(&token_lock);
	inc_tokens = 0;
	dec_tokens = 0;
	activation_cycle = 0;
	controller_tick = 0;

	kths[0] = kthread_run(controller_thread, NULL, "controller_thread");
	kths[1] = kthread_run(increment_thread, NULL, "increment_thread");
	kths[2] = kthread_run(decrement_thread, NULL, "decrement_thread");

	if (IS_ERR(kths[0]) || IS_ERR(kths[1]) || IS_ERR(kths[2])) {
		int i;
		for (i = 0; i < NTHREADS; i++) {
			if (!IS_ERR_OR_NULL(kths[i]))
				kthread_stop(kths[i]);
		}
		return -ENOMEM;
	}

	activator_period = ktime_set(1, 0);
	hrtimer_init(&activator_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	activator_timer.function = activator_timer_callback;
	hrtimer_start(&activator_timer, activator_period, HRTIMER_MODE_REL);

	printk(KERN_INFO "LKM: 3 kernel threads started with hrtimer activator\n");
	return 0;
}

void __exit
proc_exit(void)
{
	int i;

	hrtimer_cancel(&activator_timer);
	wake_up_interruptible(&controller_wq);
	wake_up_interruptible(&wq);

	for (i = 0; i < NTHREADS; i++) {
		if (!IS_ERR_OR_NULL(kths[i]))
			kthread_stop(kths[i]);
	}

	printk(KERN_INFO "LKM: threads stopped, final value=%d\n",
	       atomic_read(&shared_value));
}

module_init(proc_init);
module_exit(proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PBS");
MODULE_DESCRIPTION("3 kthreads: hrtimer-driven controller + atomic workers");

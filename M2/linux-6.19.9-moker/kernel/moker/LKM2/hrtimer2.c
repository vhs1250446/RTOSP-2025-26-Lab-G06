#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

#define NS_PER_SEC  1000000000

unsigned long timer_interval_ns = 2 * NS_PER_SEC; //2 seconds

struct hrtimer timer;
unsigned long long  my_rdtsc(void);
enum hrtimer_restart timer_callback( struct hrtimer *t);
int  timer_init(void);
void timer_exit(void);


unsigned long long  my_rdtsc(void) 
{
 	unsigned int lo, hi;
	asm ("rdtsc" : "=a" (lo), "=d" (hi));
 	
 	return (unsigned long long)hi << 32 | lo;
 }

enum hrtimer_restart timer_callback( struct hrtimer *t)
{
  	ktime_t currtime, interval;
	unsigned long long tsc;
  	currtime  = ktime_get();
	tsc = my_rdtsc();
	printk(KERN_INFO "LKM: timer running at jiffies=%ld\n", jiffies);
	printk(KERN_INFO "LKM: timer running at nanosecond=%llu\n", ktime_to_ns(currtime));
	printk(KERN_INFO "LKM: timer running at tsc=%llu\n", tsc);
	
  	interval = ktime_set(0,timer_interval_ns); 
  	hrtimer_forward(t, currtime, interval);
	return HRTIMER_RESTART;
}

int  timer_init(void) {
	
	ktime_t currtime, interval;
	unsigned long long tsc;
  	currtime  = ktime_get();
	tsc = my_rdtsc();
	printk(KERN_INFO "LKM: [%d] init\n",current->pid);
	printk(KERN_INFO "LKM: timer initialization at jiffies=%ld\n", jiffies);
	printk(KERN_INFO "LKM: timer initialization at nanosecond=%llu\n", ktime_to_ns(currtime));
	printk(KERN_INFO "LKM: timer initialization at tsc=%llu\n",tsc);
	interval = ktime_set(0,timer_interval_ns); 
	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer.function = &timer_callback;
 	hrtimer_start(&timer, interval, HRTIMER_MODE_REL);
	return 0;
}

 void timer_exit(void) {
	printk(KERN_INFO "LKM: [%d] exit\n",current->pid);
// hrtimer_try_to_cancel: return: 0 when the timer was not active 1 when the timer was active -1 when the timer is currently excuting the callback function and cannot be stopped 
	while ((hrtimer_try_to_cancel(&timer))==-1){
		printk(KERN_INFO "LKM: The timer was still in use...\n");
	}
  	printk(KERN_INFO "LKM: HR Timer module uninstalling\n");
	
}

module_init(timer_init);
module_exit(timer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LKM");
MODULE_DESCRIPTION("Hrtimer implementation");

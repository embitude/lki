#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/sched/task.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/random.h>
#include <linux/mutex.h>

DEFINE_MUTEX(my_mutex);

static int counter = 0;
static int thread_id[2];

static struct task_struct *thread_st[2];

static int thread_fn(void *id)
{
	static unsigned int i;
	allow_signal(SIGKILL);

	while (!kthread_should_stop()) 
	{
		mutex_lock(&my_mutex);
		counter++;
		printk("Job %d started\n", counter);
		get_random_bytes(&i, sizeof(i));
		ssleep(i % 5);
		printk("Job %d finished\n", counter);
		mutex_unlock(&my_mutex);
		if (signal_pending(current))
			return -EINTR;
	}

	printk(KERN_INFO "Thread Stopping\n");
	return 0;
}


static int __init init_thread(void)
{
	int i;
	for (i = 0; i < 2; i++)
	{
		printk(KERN_INFO "Creating Thread %d\n", i);
		thread_id[i] = i;
		thread_st[i] = kthread_run(thread_fn, &thread_id[i], "mythread");
		if (thread_st[i]) {
			printk(KERN_INFO "Thread %d created\n", i);
			get_task_struct(thread_st[i]);
		}
		else 
		{
			printk(KERN_INFO "Thread creation failed\n");
			return -1;
		}
	}
	return 0;
}

static void __exit cleanup_thread(void)
{
	int i;

	printk("Cleaning Up\n");
	for (i = 0; i < 2; i++)
	{
		kthread_stop(thread_st[i]);
		put_task_struct(thread_st[i]);
		printk(KERN_INFO "Thread %d stopped", i);
	}
}

module_init(init_thread);
module_exit(cleanup_thread);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embitude Trainings <info@embitude.in>");
MODULE_DESCRIPTION("Concurreny issue demo");

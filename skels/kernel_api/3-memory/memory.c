/*
 * SO2 lab3 - task 3
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

MODULE_DESCRIPTION("Memory processing");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

struct task_info {
	pid_t pid;
	unsigned long timestamp;
};

static struct task_info *ti1, *ti2, *ti3, *ti4;

static struct task_info *task_info_alloc(int pid)
{
	struct task_info *ti;

	/* TODO 1: allocated and initialize a task_info struct */
    ti = (struct task_info*)kmalloc(sizeof(struct task_info), GFP_KERNEL);
    ti->pid = pid;
    ti->timestamp = jiffies;
    printk("pid: %d, timestamp: %lu\n", ti->pid, ti->timestamp);

	return ti;
}

static int memory_init(void)
{
    struct task_struct *p;
	/* TODO 2: call task_info_alloc for current pid */
    ti1 = task_info_alloc(current->pid);

	/* TODO 2: call task_info_alloc for parent PID */
    p = current->parent;
    if(p) {
        ti2 = task_info_alloc(p->pid);
    }

	/* TODO 2: call task_info alloc for next process PID */
    p = next_task(current);
    if(p) {
        ti3 = task_info_alloc(p->pid);
    }

	/* TODO 2: call task_info_alloc for next process of the next process */
    p = next_task(current);
    if(p && (p = next_task(p))) {
        ti4 = task_info_alloc(p->pid);
    }

	return 0;
}

static void memory_exit(void)
{

	/* TODO 3: print ti* field values */
    if(ti1) {
        printk(KERN_DEBUG "ti1 pid: %d, timestamp: %lu\n", ti1->pid, ti1->timestamp);
        kfree(ti1);
    }
    if(ti2) {
        printk(KERN_DEBUG "ti2 pid: %d, timestamp: %lu\n", ti2->pid, ti2->timestamp);
        kfree(ti2);
    }
    if(ti3) {
        printk(KERN_DEBUG "ti3 pid: %d, timestamp: %lu\n", ti3->pid, ti3->timestamp);
        kfree(ti3);
    }
    if(ti4) {
        printk(KERN_DEBUG "ti4 pid: %d, timestamp: %lu\n", ti4->pid, ti4->timestamp);
        kfree(ti4);
    }

	/* TODO 4: free ti* structures */
}

module_init(memory_init);
module_exit(memory_exit);

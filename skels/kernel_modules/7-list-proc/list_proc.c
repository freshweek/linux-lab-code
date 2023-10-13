#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
/* TODO: add missing headers */
#include <linux/sched.h>

MODULE_DESCRIPTION("List current processes");
MODULE_AUTHOR("Kernel Hacker");
MODULE_LICENSE("GPL");

void display_pid_name(void)
{
    printk(KERN_DEBUG "current name: %s, pid: %d\n",
            current->comm, current->pid);
}

static int my_proc_init(void)
{
	struct task_struct *p;

	/* TODO: print current process pid and its name */
    display_pid_name();

	/* TODO: print the pid and name of all processes */

	return 0;
}

static void my_proc_exit(void)
{
	/* TODO: print current process pid and name */
    display_pid_name();
}

module_init(my_proc_init);
module_exit(my_proc_exit);

/*
 * Character device drivers lab
 *
 * All tasks
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/wait.h>

#include "../include/so2_cdev.h"

MODULE_DESCRIPTION("SO2 character device");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_INFO

#define MY_MAJOR		42
#define MY_MINOR		0
#define NUM_MINORS		1
#define MODULE_NAME		"so2_cdev"
#define MESSAGE			"hello\n"
#define IOCTL_MESSAGE		"Hello ioctl"

#ifndef BUFSIZ
#define BUFSIZ		4096
#endif


struct so2_device_data {
	/* TODO 2: add cdev member */
    struct cdev dev;
	/* TODO 4: add buffer with BUFSIZ elements */
    char buffer[BUFSIZ];
	/* TODO 7: extra members for home */
    size_t size;
	/* TODO 3: add atomic_t access variable to keep track if file is opened */
    atomic_t flag;
    wait_queue_head_t wq;
    int wq_flag;
};

struct so2_device_data devs[NUM_MINORS];
// DECLARE_WAIT_QUEUE_HEAD(wq);
// int wq_flag = 0;


static int so2_cdev_open(struct inode *inode, struct file *file)
{
	struct so2_device_data *data;

	/* TODO 2: print message when the device file is open. */
    printk(KERN_DEBUG "cdev is open...\n");

	/* TODO 3: inode->i_cdev contains our cdev struct, use container_of to obtain a pointer to so2_device_data */
    data = container_of(inode->i_cdev, struct so2_device_data, dev);

	file->private_data = data;

#ifndef EXTRA
	/* TODO 3: return immediately if access is != 0, use atomic_cmpxchg */
    /*
    if(atomic_cmpxchg(&data->flag, 0, 1)){
        printk(KERN_DEBUG "Already lock open\n");
        return -EBUSY;
    }
    */
    // wait_event_interruptible(data->wq, data->wq_flag == 0);
    // data->wq_flag = 1;
#endif

	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(1 * HZ);

	return 0;
}

static int
so2_cdev_release(struct inode *inode, struct file *file)
{
	/* TODO 2: print message when the device file is closed. */
    printk(KERN_DEBUG "cdev is close...\n");

#ifndef EXTRA
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;

	/* TODO 3: reset access variable to 0, use atomic_set */
    // atomic_set(&data->flag, 0);
    // data->wq_flag = 0;
    // wake_up_interruptible(&data->wq);
#endif
	return 0;
}

static ssize_t
so2_cdev_read(struct file *file,
		char __user *user_buffer,
		size_t size, loff_t *offset)
{
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;
	size_t to_read;
    size_t capacity;
#ifdef EXTRA
	/* TODO 7: extra tasks for home */
#endif
    while(data->size < *offset || data->size == 0) {
        if(file->f_flags & O_NONBLOCK) {
            printk(KERN_DEBUG "would block\n");
            return -EWOULDBLOCK;
        } else {
            printk(KERN_DEBUG "wait\n");
            if(wait_event_interruptible(data->wq, data->size >= *offset && data->size != 0)){
                printk(KERN_DEBUG "interrupt\n");
                return 0;
            }
        }
        printk(KERN_DEBUG "be waked up\n");
    }

	/* TODO 4: Copy data->buffer to user_buffer, use copy_to_user */
    printk(KERN_DEBUG "offset: %llu\n", *offset);
    to_read = min(data->size - (size_t)*offset, size);
    printk(KERN_DEBUG "read %u bytes\n", to_read);
    if(to_read <= 0)
        return 0;

    if(copy_to_user(user_buffer, data->buffer + *offset, to_read)) {
        return -EFAULT;
    }

    *offset += to_read;

	return to_read;
}

static ssize_t
so2_cdev_write(struct file *file,
		const char __user *user_buffer,
		size_t size, loff_t *offset)
{
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;
    size_t to_write;

	/* TODO 5: copy user_buffer to data->buffer, use copy_from_user */
    to_write = min(BUFSIZ - (size_t)*offset, size);
    if(to_write <= 0)
        return 0;

    if(copy_from_user(data->buffer + *offset, user_buffer, to_write)){
        return -EFAULT;
    }
	/* TODO 7: extra tasks for home */
    *offset += to_write;
    data->size = max((size_t)*offset, data->size);
    wake_up_interruptible(&data->wq);
    printk(KERN_DEBUG "write done. wake up\n");
    
	return size;
}

static long
so2_cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;
	int ret = 0;
	int remains;

	switch (cmd) {
	/* TODO 6: if cmd = MY_IOCTL_PRINT, display IOCTL_MESSAGE */
    case MY_IOCTL_PRINT:
        printk(KERN_DEBUG "ioctl message: %s\n", IOCTL_MESSAGE);
        break;
    case MY_IOCTL_SET_BUFFER:
        if(copy_from_user(data->buffer, (void*)arg, BUFFER_SIZE)){
            ret = -EFAULT;
        }
        break;
    case MY_IOCTL_GET_BUFFER:
        if(copy_to_user((void*)arg, data->buffer, BUFFER_SIZE)){
            ret = -EFAULT;
        }
        break;
    case MY_IOCTL_DOWN:
        printk(KERN_DEBUG "thread wait\n");
        data->wq_flag = 1;
        wait_event_interruptible(data->wq, data->wq_flag == 0);
        break;
    case MY_IOCTL_UP:
        data->wq_flag = 0;
        wake_up_interruptible(&data->wq);
        break;
	/* TODO 7: extra tasks, for home */
	default:
		ret = -EINVAL;
	}

	return ret;
}

static const struct file_operations so2_fops = {
	.owner = THIS_MODULE,
/* TODO 2: add open and release functions */
    .open = so2_cdev_open,
    .release = so2_cdev_release,
/* TODO 4: add read function */
    .read = so2_cdev_read,
/* TODO 5: add write function */
    .write = so2_cdev_write,
/* TODO 6: add ioctl function */
    .unlocked_ioctl = so2_cdev_ioctl,
};

static int so2_cdev_init(void)
{
	int err;
	int i;

	/* TODO 1: register char device region for MY_MAJOR and NUM_MINORS starting at MY_MINOR */
    register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS, MODULE_NAME);
    printk(KERN_DEBUG "register device\n");

	for (i = 0; i < NUM_MINORS; i++) {
#ifdef EXTRA
		/* TODO 7: extra tasks, for home */
#else
		/*TODO 4: initialize buffer with MESSAGE string */
        memcpy(devs[i].buffer, MESSAGE, strlen(MESSAGE));
        devs[i].size = 0;
		/* TODO 3: set access variable to 0, use atomic_set */
        atomic_set(&devs[i].flag, 0);
        init_waitqueue_head(&devs[i].wq);
#endif
		/* TODO 7: extra tasks for home */
		/* TODO 2: init and add cdev to kernel core */
        cdev_init(&devs[i].dev, &so2_fops);
        cdev_add(&devs[i].dev, MKDEV(MY_MAJOR, i), 1);
	}

	return 0;
}

static void so2_cdev_exit(void)
{
	int i;

    printk(KERN_DEBUG "unregister device\n");
	for (i = 0; i < NUM_MINORS; i++) {
		/* TODO 2: delete cdev from kernel core */
        cdev_del(&devs[i].dev);
	}

	/* TODO 1: unregister char device region, for MY_MAJOR and NUM_MINORS starting at MY_MINOR */
    unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS);
}

module_init(so2_cdev_init);
module_exit(so2_cdev_exit);

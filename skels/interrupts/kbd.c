#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>

MODULE_DESCRIPTION("KBD");
MODULE_AUTHOR("Kernel Hacker");
MODULE_LICENSE("GPL");

#define MODULE_NAME		"kbd"

#define KBD_MAJOR		42
#define KBD_MINOR		0
#define KBD_NR_MINORS	1

#define I8042_KBD_IRQ		1
// #define I8042_STATUS_REG	0x64
// #define I8042_DATA_REG		0x60
#define I8042_STATUS_REG	0x65
#define I8042_DATA_REG		0x61

#define BUFFER_SIZE		1024
#define SCANCODE_RELEASED_MASK	0x80

struct kbd {
	struct cdev cdev;
	/* TODO 3: add spinlock */
	spinlock_t lock;
	char buf[BUFFER_SIZE];
	size_t put_idx, get_idx, count;
} devs[1];

/*
 * Checks if scancode corresponds to key press or release.
 */
static int is_key_press(unsigned int scancode)
{
	return !(scancode & SCANCODE_RELEASED_MASK);
}

/*
 * Return the character of the given scancode.
 * Only works for alphanumeric/space/enter; returns '?' for other
 * characters.
 */
static int get_ascii(unsigned int scancode)
{
	static char *row1 = "1234567890";
	static char *row2 = "qwertyuiop";
	static char *row3 = "asdfghjkl";
	static char *row4 = "zxcvbnm";

	scancode &= ~SCANCODE_RELEASED_MASK;
	if (scancode >= 0x02 && scancode <= 0x0b)
		return *(row1 + scancode - 0x02);
	if (scancode >= 0x10 && scancode <= 0x19)
		return *(row2 + scancode - 0x10);
	if (scancode >= 0x1e && scancode <= 0x26)
		return *(row3 + scancode - 0x1e);
	if (scancode >= 0x2c && scancode <= 0x32)
		return *(row4 + scancode - 0x2c);
	if (scancode == 0x39)
		return ' ';
	if (scancode == 0x1c)
		return '\n';
	return '?';
}

static void put_char(struct kbd *data, char c)
{
	if (data->count >= BUFFER_SIZE)
		return;

	data->buf[data->put_idx] = c;
	data->put_idx = (data->put_idx + 1) % BUFFER_SIZE;
	data->count++;
}

static bool get_char(char *c, struct kbd *data)
{
	/* TODO 4: get char from buffer; update count and get_idx */
	if(data->get_idx < data->put_idx) {
		*c = data->buf[data->get_idx];
		data->get_idx += 1;
		return true;
	}
	return false;
}

static void reset_buffer(struct kbd *data)
{
	/* TODO 5: reset count, put_idx, get_idx */
	data->get_idx = 0;
	data->put_idx = 0;
	data->count = 0;
}

/*
 * Return the value of the DATA register.
 */
static inline u8 i8042_read_data(void)
{
	u8 val;
	/* TODO 3: Read DATA register (8 bits). */
	val = inb(I8042_DATA_REG-1);
	return val;
}

/* TODO 2: implement interrupt handler */
irqreturn_t kbd_handler(int irq_no, void *dev_id)
{
	unsigned int scancode;
	struct kbd *data = (struct kbd*)dev_id;
	char ch;

	scancode = i8042_read_data();
	pr_info("IRQ:% d, scancode = 0x%x (%u,%c)\n",
				irq_no, scancode, scancode, scancode);
	if(is_key_press(scancode)) {
		ch = get_ascii(scancode);
		spin_lock(&data->lock);
		put_char(data, ch);
		spin_unlock(&data->lock);

		pr_info("IRQ %d: scancode=0x%x (%u) pressed=%d ch=%c\n",
				irq_no, scancode, scancode, 1, ch); 
	}	

	return IRQ_NONE;
}
	/* TODO 3: read the scancode */
	/* TODO 3: interpret the scancode */
	/* TODO 3: display information about the keystrokes */
	/* TODO 3: store ASCII key to buffer */

static int kbd_open(struct inode *inode, struct file *file)
{
	struct kbd *data = container_of(inode->i_cdev, struct kbd, cdev);

	file->private_data = data;
	pr_info("%s opened\n", MODULE_NAME);
	return 0;
}

static int kbd_release(struct inode *inode, struct file *file)
{
	pr_info("%s closed\n", MODULE_NAME);
	return 0;
}

/* TODO 5: add write operation and reset the buffer */
static ssize_t kbd_write(struct file *file, const char __user *user_buffer,
			size_t size, loff_t *offset)
{
	struct kbd *data = (struct kbd *) file->private_data;
	size_t write;
	char buf[6];
	unsigned long flag;

	write = min(6-(size_t)*offset, size);
	if(copy_from_user(buf+*offset, user_buffer, write)) {
		return -EFAULT;
	}
	*offset += write;
	if(!strcmp(buf, "clear")) {
		spin_lock_irqsave(&data->lock, flag);
		reset_buffer(data);
		spin_unlock_irqrestore(&data->lock, flag);
	}
	return size;
}

static ssize_t kbd_read(struct file *file, char __user *user_buffer,
			size_t size, loff_t *offset)
{
	struct kbd *data = (struct kbd *) file->private_data;
	size_t read = 0;
	unsigned long flag;
	/* TODO 4: read data from buffer */
	char ch;
	while(size > 0) {
		spin_lock_irqsave(&data->lock, flag);
		if(!get_char(&ch, data)) {
			spin_unlock_irqrestore(&data->lock, flag);
			break;
		}
		spin_unlock_irqrestore(&data->lock, flag);

		if(copy_to_user(user_buffer+read, &ch, 1)) {
			return -EBUSY;
		}
		*offset ++;
		read ++;
	}
	return read;
}

static const struct file_operations kbd_fops = {
	.owner = THIS_MODULE,
	.open = kbd_open,
	.release = kbd_release,
	.read = kbd_read,
	/* TODO 5: add write operation */
	.write = kbd_write,
};

static int kbd_init(void)
{
	int err;

	err = register_chrdev_region(MKDEV(KBD_MAJOR, KBD_MINOR),
				     KBD_NR_MINORS, MODULE_NAME);
	if (err != 0) {
		pr_err("register_region failed: %d\n", err);
		goto out;
	}

	/* TODO 1: request the keyboard I/O ports */
	if(! request_region(I8042_DATA_REG, 1, MODULE_NAME)) {
		return -ENODEV;
	}
	if(! request_region(I8042_STATUS_REG, 1, MODULE_NAME)) {
		release_region(I8042_DATA_REG, 1);
		return -ENODEV;
	}

	/* TODO 3: initialize spinlock */
	spin_lock_init(&devs[0].lock);

	/* TODO 2: Register IRQ handler for keyboard IRQ (IRQ 1). */
	err = request_irq(I8042_KBD_IRQ, kbd_handler, IRQF_SHARED,
					MODULE_NAME, &devs[0]);
	if(err < 0) {
		goto out_irq;
	}

	cdev_init(&devs[0].cdev, &kbd_fops);
	cdev_add(&devs[0].cdev, MKDEV(KBD_MAJOR, KBD_MINOR), 1);

	pr_notice("Driver %s loaded\n", MODULE_NAME);
	return 0;

	/*TODO 2: release regions in case of error */
out_irq:
	release_region(I8042_STATUS_REG, 1);
	release_region(I8042_DATA_REG, 1);

out_unregister:
	unregister_chrdev_region(MKDEV(KBD_MAJOR, KBD_MINOR),
				 KBD_NR_MINORS);
out:
	return err;
}

static void kbd_exit(void)
{
	cdev_del(&devs[0].cdev);

	/* TODO 2: Free IRQ. */
	free_irq(I8042_KBD_IRQ, &devs[0]);

	/* TODO 1: release keyboard I/O ports */
	release_region(I8042_STATUS_REG, 1);
	release_region(I8042_DATA_REG, 1);


	unregister_chrdev_region(MKDEV(KBD_MAJOR, KBD_MINOR),
				 KBD_NR_MINORS);
	pr_notice("Driver %s unloaded\n", MODULE_NAME);
}

module_init(kbd_init);
module_exit(kbd_exit);

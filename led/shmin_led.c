/*
 * shmin_led: SH7706LSR LED driver
 * 
 * Copyright (C) 2011 @oraccha
 * 
 * How to compile it:
 * $ make KERNELDIR=../../linux-2.6.28.10 CROSS_COMPILE=sh3-linux- ARCH=sh
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <asm/uaccess.h> 

#define LED_MOD_NAME "shmin_led"
#define LED_MOD_DESC "SH7706LSR LED"
#define LED_VERSION "0.1"
#define LED_MAX_DEVS (1)

#define SCPCR (0xA4000116)
#define SCPDR (0xA4000136)

MODULE_AUTHOR("@oraccha");
MODULE_DESCRIPTION(LED_MOD_DESC);
MODULE_LICENSE("GPL v2");
MODULE_VERSION(LED_VERSION);

static struct cdev led_dev;
static int led_major;

/*
 * File system operation functions.
 */

static ssize_t led_read(struct file *filp, char *buf, size_t len,
			loff_t *offset)
{
	return 0;
}

static ssize_t led_write(struct file *filp, const char *buf, size_t count,
			 loff_t *offset)
{
	unsigned char on;
	int err;

	err = get_user(on, buf);
	if (err)
		return err;

	if (on == '0')
		ctrl_outb(ctrl_inb(SCPDR) & ~0x10, SCPDR);
	if (on == '1')
		ctrl_outb(ctrl_inb(SCPDR) | 0x10, SCPDR);

	return 1;
}

static int led_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int led_close( struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations led_fops = {
	.owner   = THIS_MODULE,
	.open    = led_open,
	.release = led_close,
	.read    = led_read,
	.write   = led_write,
};

static int __init led_init(void)
{
	dev_t dev;
	int err = -ENOMEM;

	err = alloc_chrdev_region(&dev, 0, LED_MAX_DEVS, "led");
	if (err < 0) {
		printk(KERN_WARNING "%s: alloc_chrdev_region failed\n",
		       LED_MOD_NAME);
		return err;
	}
	led_major = MAJOR(dev);

	cdev_init(&led_dev, &led_fops);
	led_dev.owner = THIS_MODULE;
	err = cdev_add(&led_dev, MKDEV(led_major, 0), LED_MAX_DEVS);
	if (err < 0) {
		printk(KERN_WARNING "%s: cdev_add failed: %d\n",
		       LED_MOD_NAME, err);
		return err;
	}

	/* SC port 4 mode: output */
	ctrl_outw((ctrl_inw(SCPCR) & ~(0x0300)) | 0x0100, SCPCR);

	printk(KERN_INFO "%s driver %s loaded (major %d)\n",
	       LED_MOD_DESC, LED_VERSION, led_major);

	return 0;
}

static void __exit led_exit(void)
{
	cdev_del(&led_dev);
	unregister_chrdev_region(MKDEV(led_major, 0), LED_MAX_DEVS);
}


module_init(led_init);
module_exit(led_exit);

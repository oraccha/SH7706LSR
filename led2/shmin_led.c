/*
 * shmin_led: SH7706LSR LED driver (blink using TMU2 timer)
 * 
 * Copyright (C) 2011 @oraccha
 * 
 * How to compile it:
 * $ make KERNELDIR=../../linux-2.6.28.10 CROSS_COMPILE=sh3-linux- ARCH=sh
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <asm/irq.h>
#include <asm/uaccess.h> 
#include <asm/timer.h> 

#define LED_MOD_NAME "shmin_led"
#define LED_MOD_DESC "SH7706LSR LED"
#define LED_VERSION "0.2"
#define LED_MAX_DEVS (1)

#define SCPCR (0xA4000116)
#define SCPDR (0xA4000136)

#define TMU_TCR_INIT	0x0020

#define MODULE_CLOCK (40000000)
#define TIMER_CLOCK (MODULE_CLOCK/4)
#define BLINK_HZ (1)

MODULE_AUTHOR("@oraccha");
MODULE_DESCRIPTION(LED_MOD_DESC);
MODULE_LICENSE("GPL v2");
MODULE_VERSION(LED_VERSION);

static struct cdev led_dev;
static int led_major;

static int turn;

/*
 * Timer management unit functions.
 */

static irqreturn_t tmu2_timer_interrupt(int irq, void *dummy)
{
	if (turn) {
		ctrl_outb(ctrl_inb(SCPDR) & ~0x10, SCPDR); /* LED on. */
		turn = 0;
	} else {
		ctrl_outb(ctrl_inb(SCPDR) | 0x10, SCPDR); /* LED off. */
		turn = 1;
	}

	/* reset tmu2. */
	ctrl_outw(TMU_TCR_INIT, TMU2_TCR);

	return IRQ_HANDLED;
}

static inline void tmu2_start(void)
{
	ctrl_outb(ctrl_inb(TMU_012_TSTR) | (0x1<<2), TMU_012_TSTR);
}

static inline void tmu2_stop(void)
{
	ctrl_outb(ctrl_inb(TMU_012_TSTR) & ~(0x1<<2), TMU_012_TSTR);
}

static inline void tmu2_init(void)
{
	int interval;

	tmu2_stop();
	request_irq(CONFIG_SH_TIMER_IRQ + 2, tmu2_timer_interrupt, 0,
		    "tmu2", 0);

	interval = TIMER_CLOCK / BLINK_HZ;
	ctrl_outw(TMU_TCR_INIT, TMU2_TCR);
	ctrl_outl(interval, TMU2_TCOR);
	ctrl_outl(interval, TMU2_TCNT);
}

static inline void tmu2_fini(void)
{
	tmu2_stop();
	free_irq(CONFIG_SH_TIMER_IRQ + 2, 0);
}

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
	char val[16];
	int n, interval;

	if (copy_from_user(val, buf, sizeof(val)))
		return -EFAULT;

	n = simple_strtoul(val, NULL, 10);
	interval = TIMER_CLOCK / n;
	ctrl_outl(interval, TMU2_TCOR);
	ctrl_outl(interval, TMU2_TCNT);

	return count;
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
	/* SC port 4: low */
	ctrl_outb(ctrl_inb(SCPDR) & ~0x10, SCPDR);

	tmu2_init();
	tmu2_start();

	printk(KERN_INFO "%s driver %s loaded (major %d)\n",
	       LED_MOD_DESC, LED_VERSION, led_major);

	return 0;
}

static void __exit led_exit(void)
{
	tmu2_fini();

	cdev_del(&led_dev);
	unregister_chrdev_region(MKDEV(led_major, 0), LED_MAX_DEVS);
}


module_init(led_init);
module_exit(led_exit);

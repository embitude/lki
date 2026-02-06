#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include "led_ioctl.h"

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6,11,0))
static long gpio_num = 56;
static u16 led[4] = {53, 54, 55, 56};
#else
static long gpio_num = 536;
static u16 led[4] = {533, 534, 535, 536};
#endif
static char *led_name[4] = {"led1", "led2", "led3", "led4"};

static dev_t first;			// Global variable for the first device number
static struct cdev c_dev;	// Global variable for the character device structure
static struct class *cl;	// Global variable for the device class

static int gpio_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int gpio_close(struct inode *inode, struct file *file)
{
	return 0;
}

static long gpio_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	switch (cmd)
	{
		case GPIO_SELECT_LED:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6,11,0))
			gpio_num = 52 + arg;
#else
			gpio_num = 532 + arg;
#endif
			break;

		// TODO 1: Add the code for setting & getting the LED status
		default:
			return -EINVAL;
	}

	return 0;
}

static struct file_operations file_ops =
{
	.owner			= THIS_MODULE,
	.open			= gpio_open,
	.release		= gpio_close,
	.unlocked_ioctl	= gpio_ioctl
};

static int init_gpio(void)
{
	int ret, i, j;
	struct device *dev_ret;

    for (i = 0; i < 4; i++) {
        if ((ret = gpio_request_one(led[i], GPIOF_OUT_INIT_LOW, led_name[i])) != 0) {
            printk("GPIO request %d failed %d\n", led[i], ret);
            for (j = i-1; j >= 0; j--)
                gpio_free(led[j]);
            return -1;
        }
    }

	if ((ret = alloc_chrdev_region(&first, 0, 1, "gpio_drv")) < 0)
	{
		printk(KERN_ALERT "Device registration failed\n");
		return ret;
	}
	printk("Major Nr: %d\n", MAJOR(first));

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6,4,0))
	if (IS_ERR(cl = class_create(THIS_MODULE, "gpiodrv")))
#else
	if (IS_ERR(cl = class_create("gpiodrv")))
#endif
	{
		printk(KERN_ALERT "Class creation failed\n");
		unregister_chrdev_region(first, 1);
		return PTR_ERR(cl);
	}

	if (IS_ERR(dev_ret = device_create(cl, NULL, first, NULL, "gpio_drv%d", 0)))
	{
		printk(KERN_ALERT "Device creation failed\n");
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return PTR_ERR(dev_ret);
	}

	cdev_init(&c_dev, &file_ops);

	if ((ret = cdev_add(&c_dev, first, 1)) < 0)
	{
		printk(KERN_ALERT "Device addition failed\n");
		device_destroy(cl, first);
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return ret;
	}

	return 0;
}

void cleanup_gpio(void)
{
	int i;
	cdev_del(&c_dev);
	device_destroy(cl, first);
	class_destroy(cl);
	unregister_chrdev_region(first, 1);

	for (i = 0; i < 4; i++) {
        gpio_free(led[i]);
    }

	printk(KERN_INFO "Device unregistered\n");
}

module_init(init_gpio);
module_exit(cleanup_gpio);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embitude Trainings <info@embitude.in>");
MODULE_DESCRIPTION("GPIO Driver ioctl Demo");

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/list.h>

#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>


#define GPFCON_BASE_ADDRESS     0x56000050

volatile unsigned long *gpfcon = NULL;
volatile unsigned long *gpfdat = NULL;


struct class *cls;


int drv_open(struct inode *inode, struct file *file)
{
    /* init GPIO for LED. */
    
	*gpfcon &= ~((0x3<<(4*2)) | (0x3<<(5*2)) | (0x3<<(6*2)));
	*gpfcon |= ((0x1<<(4*2)) | (0x1<<(5*2)) | (0x1<<(6*2)));
	return 0;   
}

ssize_t drv_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
    unsigned char value;

    if(size != 1)
        return -1;

    copy_from_user(&value, buf, 1);
    if(value)
    {
        /* turn on leds */
        *gpfdat &= ~((1<<4) | (1<<5) | (1<<6));
    }
    else
    {
        /* shut off leds. */
        *gpfdat |= (1<<4) | (1<<5) | (1<<6);
    }
    
    return 1; 
}



static const struct file_operations firstDrv_fops = {

    .owner = THIS_MODULE,
    .open  = drv_open,
    .write = drv_write,
};


int major = 0;
int first_drv_init(void)
{
    printk("first_drv_init\n");
    
    /* 1. allocate major, and register char device. */
    major = register_chrdev(0, "first_drv", &firstDrv_fops);
    cls = class_create(THIS_MODULE, "first_drv");
    device_create(cls, NULL, MKDEV(major, 0), "firstDrv"); /* /dev/firstDrv  */

    gpfcon = (volatile unsigned long *)ioremap(GPFCON_BASE_ADDRESS, 16);
    gpfdat = gpfcon + 1;
    
    return 0;
}

void first_dev_exit(void)
{
    printk("first_dev_exit\n");

    iounmap(gpfcon);
    
    //class_device_unregister(struct class_device * class_dev)
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, "first_drv");
}

module_init(first_drv_init);
module_exit(first_dev_exit);


MODULE_LICENSE("GPL");



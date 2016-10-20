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




struct class *cls;


int drv_open(struct inode *inode, struct file *file)
{
    //printk("");
}

static const struct file_operations firstDrv_fops = {

    .owner = THIS_MODULE,
    .open  = drv_open,
    
};


int major = 0;
int first_drv_init(void)
{
    printk("first_drv_init\n");
    
    /* 1. allocate major, and register char device. */
    major = register_chrdev(0, "first_drv", &firstDrv_fops);
    cls = class_create(THIS_MODULE, "first_drv");
    device_create(cls, NULL, MKDEV(major, 0), "firstDrv");
    
    
    return 0;
}

void first_dev_exit(void)
{
    printk("first_dev_exit\n");

    //class_device_unregister(struct class_device * class_dev)
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, "first_drv");
}

module_init(first_drv_init);
module_exit(first_dev_exit);


MODULE_LICENSE("GPL");



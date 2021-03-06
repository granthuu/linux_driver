#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>


struct pin_desc{
	unsigned int pin;
	unsigned int key_val;
};


/* 键值: 按下时, 0x01, 0x02, 0x03, 0x04 */
/* 键值: 松开时, 0x81, 0x82, 0x83, 0x84 */
static unsigned char key_val;

struct pin_desc pins_desc[4] = {
	{S3C2410_GPF0,  0x01},
	{S3C2410_GPF2,  0x02},
	{S3C2410_GPG3,  0x03},
	{S3C2410_GPG11, 0x04},
};


struct class * cls;


static DECLARE_WAIT_QUEUE_HEAD(buttonQueue);
static volatile int ev_press = 0;



static irqreturn_t key_isr(int irq, void *dev_id)
{
    struct pin_desc *pinDesc = (struct pin_desc *)dev_id;
	unsigned int pinval;

	/* 1. read key value  */
	pinval = s3c2410_gpio_getpin(pinDesc->pin);
	if (pinval)
	{
		/* 松开 */
		key_val = 0x80 | pinDesc->key_val;
	}
	else
	{
		/* 按下 */
		key_val = pinDesc->key_val;
	} 

	/* 2. wake up waitQueue */
	ev_press = 1;
	wake_up_interruptible(&buttonQueue);
    
	return IRQ_RETVAL(IRQ_HANDLED);  
}


int third_drv_open(struct inode *inode, struct file *file)
{
    /* register key interrupt */
	request_irq(IRQ_EINT0,  key_isr, IRQT_BOTHEDGE, "S2", &pins_desc[0]);
	request_irq(IRQ_EINT2,  key_isr, IRQT_BOTHEDGE, "S3", &pins_desc[1]);
	request_irq(IRQ_EINT11, key_isr, IRQT_BOTHEDGE, "S4", &pins_desc[2]);
	request_irq(IRQ_EINT19, key_isr, IRQT_BOTHEDGE, "S5", &pins_desc[3]);	
	
    return 0;
}

int third_drv_close(struct inode *inode, struct file *file)
{
    printk("third_drv_close\n");
    
    free_irq(IRQ_EINT0,  &pins_desc[0]);
    free_irq(IRQ_EINT2,  &pins_desc[1]);  
    free_irq(IRQ_EINT11, &pins_desc[2]);
    free_irq(IRQ_EINT19, &pins_desc[3]);

    return 0;
}


ssize_t third_drv_read(struct file *file, char __user *buf, size_t size, loff_t *pos)
{
    if(size != 1)
    {
        return -EINVAL;
    }

    /* 等待按键按下事件发生 */
    wait_event_interruptible(buttonQueue, ev_press);

    /* 运行到这里，说明按键中断发生，读取按键值 */
    copy_to_user(buf, &key_val, 1);
    ev_press = 0;

    return 1;
}




static const struct file_operations third_fops = {

    .owner   = THIS_MODULE,
    .open    = third_drv_open,
    .read    = third_drv_read,
    .release = third_drv_close,
};


int major = 0;
int third_drv_init(void)
{
    major = register_chrdev(0, "third_drv", &third_fops);
    cls = class_create(THIS_MODULE, "third_drv");
    device_create(cls, NULL, MKDEV(major, 0), "thirdDrv");  /* /dev/thirdDrv  */
        
    return 0;
}


void third_drv_exit(void)
{
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, "third_drv");
}

module_init(third_drv_init);
module_exit(third_drv_exit);

MODULE_LICENSE("GPL");


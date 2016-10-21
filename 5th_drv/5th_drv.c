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
#include <linux/poll.h>


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

static struct fasync_struct *button_async;

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
	
	kill_fasync (&button_async, SIGIO, POLL_IN);

	return IRQ_RETVAL(IRQ_HANDLED);  
}


int fifth_drv_open(struct inode *inode, struct file *file)
{
    /* register key interrupt */
	request_irq(IRQ_EINT0,  key_isr, IRQT_BOTHEDGE, "S2", &pins_desc[0]);
	request_irq(IRQ_EINT2,  key_isr, IRQT_BOTHEDGE, "S3", &pins_desc[1]);
	request_irq(IRQ_EINT11, key_isr, IRQT_BOTHEDGE, "S4", &pins_desc[2]);
	request_irq(IRQ_EINT19, key_isr, IRQT_BOTHEDGE, "S5", &pins_desc[3]);	
	
    return 0;
}

int fifth_drv_close(struct inode *inode, struct file *file)
{
    printk("fifth_drv_close\n");
    
    free_irq(IRQ_EINT0,  &pins_desc[0]);
    free_irq(IRQ_EINT2,  &pins_desc[1]);  
    free_irq(IRQ_EINT11, &pins_desc[2]);
    free_irq(IRQ_EINT19, &pins_desc[3]);

    return 0;
}


ssize_t fifth_drv_read(struct file *file, char __user *buf, size_t size, loff_t *pos)
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

unsigned int fifth_drv_poll(struct file *file, struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	poll_wait(file, &buttonQueue, wait); // 不会立即休眠

	if (ev_press)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static int fifth_drv_fasync (int fd, struct file *filp, int on)
{
	printk("driver: fifth_drv_fasync\n");
	return fasync_helper (fd, filp, on, &button_async);
}


static const struct file_operations fifth_fops = {
    .owner   = THIS_MODULE,
    .open    = fifth_drv_open,
    .read    = fifth_drv_read,
    .release = fifth_drv_close,
    .poll    = fifth_drv_poll,
    .fasync  = fifth_drv_fasync,
};


int major = 0;
int fifth_drv_init(void)
{
    major = register_chrdev(0, "fifth_drv", &fifth_fops);
    cls = class_create(THIS_MODULE, "fifth_drv");
    device_create(cls, NULL, MKDEV(major, 0), "fifthDrv");  /* /dev/fifthDrv  */
        
    return 0;
}


void fifth_drv_exit(void)
{
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, "fifth_drv");
}

module_init(fifth_drv_init);
module_exit(fifth_drv_exit);

MODULE_LICENSE("GPL");


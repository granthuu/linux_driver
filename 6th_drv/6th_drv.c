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

/* 定义互斥锁 */
static DECLARE_MUTEX(button_lock);     


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


int sixth_drv_open(struct inode *inode, struct file *file)
{
	if (file->f_flags & O_NONBLOCK)
	{
		if (down_trylock(&button_lock))
		{
            printk("sixth_drv_open: O_NONBLOCK -> down_trylock - EBUSY\n");
			return -EBUSY;            
		}
	}
	else
	{
	    printk("sixth_drv_open: down before\n");
	    
		/* 获取信号量 
                * 如果该信号量已经被占用，
                * 则该进程一直在这里等待不返回，
                * 直到该信号量被释放
		*/
		down(&button_lock);

		printk("sixth_drv_open: down after\n");
	}

    /* register key interrupt */
	request_irq(IRQ_EINT0,  key_isr, IRQT_BOTHEDGE, "S2", &pins_desc[0]);
	request_irq(IRQ_EINT2,  key_isr, IRQT_BOTHEDGE, "S3", &pins_desc[1]);
	request_irq(IRQ_EINT11, key_isr, IRQT_BOTHEDGE, "S4", &pins_desc[2]);
	request_irq(IRQ_EINT19, key_isr, IRQT_BOTHEDGE, "S5", &pins_desc[3]);	
	
    return 0;
}

int sixth_drv_close(struct inode *inode, struct file *file)
{
    printk("sixth_drv_close\n");
    
    free_irq(IRQ_EINT0,  &pins_desc[0]);
    free_irq(IRQ_EINT2,  &pins_desc[1]);  
    free_irq(IRQ_EINT11, &pins_desc[2]);
    free_irq(IRQ_EINT19, &pins_desc[3]);

    /* 释放信号量*/
    up(&button_lock);

    return 0;
}


ssize_t sixth_drv_read(struct file *file, char __user *buf, size_t size, loff_t *pos)
{
    if(size != 1)
    {
        return -EINVAL;
    }

    
	if (file->f_flags & O_NONBLOCK)
	{
		if (!ev_press)
		{   
		    printk("sixth_drv_read: O_NONBLOCK return\n");
			return -EAGAIN;
        }
	}
	else
	{
        /* 等待按键按下事件发生 */
        wait_event_interruptible(buttonQueue, ev_press);
	}

    /* 运行到这里，说明按键中断发生，读取按键值 */
    copy_to_user(buf, &key_val, 1);
    ev_press = 0;

    return 1;   /* 返回成功读取的字节数 */
}

unsigned int sixth_drv_poll(struct file *file, struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	poll_wait(file, &buttonQueue, wait); // 不会立即休眠

	if (ev_press)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static int sixth_drv_fasync (int fd, struct file *filp, int on)
{
	printk("driver: sixth_drv_fasync\n");
	return fasync_helper (fd, filp, on, &button_async);
}


static const struct file_operations sixth_fops = {
    .owner   = THIS_MODULE,
    .open    = sixth_drv_open,
    .read    = sixth_drv_read,
    .release = sixth_drv_close,
    .poll    = sixth_drv_poll,
    .fasync  = sixth_drv_fasync,
};


int major = 0;
int sixth_drv_init(void)
{
    major = register_chrdev(0, "sixth_drv", &sixth_fops);
    cls = class_create(THIS_MODULE, "sixth_drv");
    device_create(cls, NULL, MKDEV(major, 0), "sixthDrv");  /* /dev/sixthDrv  */
        
    return 0;
}


void sixth_drv_exit(void)
{
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, "sixth_drv");
}

module_init(sixth_drv_init);
module_exit(sixth_drv_exit);

MODULE_LICENSE("GPL");


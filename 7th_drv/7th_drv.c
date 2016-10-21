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


/* 定义timer链表结构 */
static struct timer_list buttons_timer;
static struct pin_desc *irq_pd;

static irqreturn_t key_isr(int irq, void *dev_id)
{
	/* 10ms后启动定时器 */
	irq_pd = (struct pin_desc *)dev_id;
	mod_timer(&buttons_timer, jiffies + (HZ / 100));  /* 10ms按键去抖动*/
	
	return IRQ_RETVAL(IRQ_HANDLED);
}


int seventh_drv_open(struct inode *inode, struct file *file)
{
	if (file->f_flags & O_NONBLOCK)
	{
		if (down_trylock(&button_lock))
		{
            printk("seventh_drv_open: O_NONBLOCK -> down_trylock - EBUSY\n");
			return -EBUSY;            
		}
	}
	else
	{
	    printk("seventh_drv_open: down before\n");
	    
		/* 获取信号量 
                * 如果该信号量已经被占用，
                * 则该进程一直在这里等待不返回，
                * 直到该信号量被释放
		*/
		down(&button_lock);

		printk("seventh_drv_open: down after\n");
	}

    /* register key interrupt */
	request_irq(IRQ_EINT0,  key_isr, IRQT_BOTHEDGE, "S2", &pins_desc[0]);
	request_irq(IRQ_EINT2,  key_isr, IRQT_BOTHEDGE, "S3", &pins_desc[1]);
	request_irq(IRQ_EINT11, key_isr, IRQT_BOTHEDGE, "S4", &pins_desc[2]);
	request_irq(IRQ_EINT19, key_isr, IRQT_BOTHEDGE, "S5", &pins_desc[3]);	
	
    return 0;
}

int seventh_drv_close(struct inode *inode, struct file *file)
{
    printk("seventh_drv_close\n");
    
    free_irq(IRQ_EINT0,  &pins_desc[0]);
    free_irq(IRQ_EINT2,  &pins_desc[1]);  
    free_irq(IRQ_EINT11, &pins_desc[2]);
    free_irq(IRQ_EINT19, &pins_desc[3]);

    /* 释放信号量*/
    up(&button_lock);

    return 0;
}


ssize_t seventh_drv_read(struct file *file, char __user *buf, size_t size, loff_t *pos)
{
    if(size != 1)
    {
        return -EINVAL;
    }

    
	if (file->f_flags & O_NONBLOCK)
	{
		if (!ev_press)
		{   
		    printk("seventh_drv_read: O_NONBLOCK return\n");
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

unsigned int seventh_drv_poll(struct file *file, struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	poll_wait(file, &buttonQueue, wait); // 不会立即休眠

	if (ev_press)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static int seventh_drv_fasync (int fd, struct file *filp, int on)
{
	printk("driver: seventh_drv_fasync\n");
	return fasync_helper (fd, filp, on, &button_async);
}


static void buttons_timer_function(unsigned long data)
{
	struct pin_desc * pindesc = irq_pd;
	unsigned int pinval;

	if (!pindesc)
	{
	    /* 当第一次中断时，pindesc没有赋值，
	         *调用将导致系统崩溃 
	         */   
        printk("buttons_timer_function return\n");
		return;
	}

	
	pinval = s3c2410_gpio_getpin(pindesc->pin);

	if (pinval)
	{
		/* 松开 */
		key_val = 0x80 | pindesc->key_val;
	}
	else
	{
		/* 按下 */
		key_val = pindesc->key_val;
	}

    ev_press = 1;                          /* 表示中断发生了 */
    wake_up_interruptible(&buttonQueue);   /* 唤醒休眠的进程 */
	
	kill_fasync (&button_async, SIGIO, POLL_IN);
}


static const struct file_operations seventh_fops = {
    .owner   = THIS_MODULE,
    .open    = seventh_drv_open,
    .read    = seventh_drv_read,
    .release = seventh_drv_close,
    .poll    = seventh_drv_poll,
    .fasync  = seventh_drv_fasync,
};


int major = 0;
int seventh_drv_init(void)
{
    init_timer(&buttons_timer);
	buttons_timer.function = buttons_timer_function;
	buttons_timer.expires  = 0;  /* 将会导致一开始就会执行timer中断函数一次 */
	add_timer(&buttons_timer); 

    major = register_chrdev(0, "seventh_drv", &seventh_fops);
    cls = class_create(THIS_MODULE, "seventh_drv");
    device_create(cls, NULL, MKDEV(major, 0), "seventhDrv");  /* /dev/sixthDrv  */
        
    return 0;
}


void seventh_drv_exit(void)
{
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, "seventh_drv");
}

module_init(seventh_drv_init);
module_exit(seventh_drv_exit);

MODULE_LICENSE("GPL");


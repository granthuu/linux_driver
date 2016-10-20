#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>


volatile unsigned long *gpfcon = NULL;
volatile unsigned long *gpfdat = NULL;

volatile unsigned long *gpgcon = NULL;
volatile unsigned long *gpgdat = NULL;

struct class * cls;


int second_drv_open(struct inode *inode, struct file *file)
{
    /* config GPIO for KEY input mode */
    
	/* 配置GPF0,2为输入引脚 */
	*gpfcon &= ~((0x3<<(0*2)) | (0x3<<(2*2)));

	/* 配置GPG3,11为输入引脚 */
	*gpgcon &= ~((0x3<<(3*2)) | (0x3<<(11*2)));


    return 0;
}

ssize_t second_drv_read(struct file *file, char __user *buf, size_t size, loff_t *pos)
{
	/* 返回4个引脚的电平 */
	unsigned char key_vals[4];
	int regval;

	if (size != sizeof(key_vals))
		return -EINVAL;

	/* 读GPF0,2 */
	regval = *gpfdat;
	key_vals[0] = (regval & (1<<0)) ? 1 : 0;
	key_vals[1] = (regval & (1<<2)) ? 1 : 0;
	

	/* 读GPG3,11 */
	regval = *gpgdat;
	key_vals[2] = (regval & (1<<3)) ? 1 : 0;
	key_vals[3] = (regval & (1<<11)) ? 1 : 0;

	copy_to_user(buf, key_vals, sizeof(key_vals));
	
	return sizeof(key_vals);
}


static const struct file_operations second_fops = {

    .owner = THIS_MODULE,
    .open  = second_drv_open,
    .read  = second_drv_read,
};

int major = 0;

int second_drv_init(void)
{
    major = register_chrdev(0, "second_drv", &second_fops);
    cls = class_create(THIS_MODULE, "second_drv");
    device_create(cls, NULL, MKDEV(major, 0), "secondDrv");  /* /dev/secondDrv  */

	gpfcon = (volatile unsigned long *)ioremap(0x56000050, 16);
	gpfdat = gpfcon + 1;

	gpgcon = (volatile unsigned long *)ioremap(0x56000060, 16);
	gpgdat = gpgcon + 1;

    
    return 0;   
}


void second_drv_exit(void)
{
    iounmap(gpfcon);
    iounmap(gpgcon);

    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, "second_drv");
}

module_init(second_drv_init);
module_exit(second_drv_exit);


MODULE_LICENSE("GPL");




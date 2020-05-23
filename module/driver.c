#include <asm/io.h>
#include <asm/ioctl.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/version.h>

#include "ioctl.h"


int iom_open(struct inode *minode, struct file *mfile);
int iom_release(struct inode *minode, struct file *mfile);
int iom_unlocked_ioctl(struct file *flip, unsigned int cmd, unsigned long arg);

// define file_operations structure 
struct file_operations fops = {
	.owner		=	THIS_MODULE,
	.open		=	iom_open,
	.release	=	iom_release,
	.unlocked_ioctl = iom_unlocked_ioctl
};

int iom_open(struct inode *minode, struct file *mfile)
{
	/*
	if(fpga_dot_port_usage != 0) return -EBUSY;
    if(fpga_fnd_port_usage != 0) return -EBUSY;
    if(ledport_usage != 0) return -EBUSY;
    if(fpga_text_lcd_port_usage != 0) return -EBUSY;
    if(kernel_timer_usage != 0) return -EBUSY;
    
    fpga_dot_port_usage = 1;
    fpga_fnd_port_usage = 1;
    ledport_usage = 1;
    fpga_text_lcd_port_usage = 1;
    kernel_timer_usage = 1;
	*/
    return 0;
}

int iom_release(struct inode *minode, struct file *mfile)
{
	/*
	fpga_dot_port_usage = 0;
    fpga_fnd_port_usage = 0;
    ledport_usage = 0;
    fpga_text_lcd_port_usage = 0;
    kernel_timer_usage = 0;
	*/
	return 0;
}

int iom_unlocked_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
	printk("module.iom_unlocked_ioctl called\n");

	if(_IOC_TYPE(cmd) != IOCTL_MAGIC)    // 매직넘버가 틀리냐?
		return -EINVAL;

	int size = _IOC_SIZE(cmd);
	struct timer_args* args;

	switch(cmd){
	case IOCTL_WRITE_TIMER:
		printk("timer set\n");
		args = (struct timer_args *)arg;
		printk("%d %d %d\n", args->interval, args->cnt, args->init);
		break;
	case IOCTL_ON:
		printk("timer started\n");
		break;
	}	
}

int __init iom_init(void)
{	
	// init module
	printk("init module\n");

	int result = register_chrdev(MAJOR_NUMBER, DEVICE, &fops);
    if(result < 0) {
        printk("Cant register driver\n");
        return result;
    }
	printk("Register driver. name : %s, major number : %d\n", DEVICE, MAJOR_NUMBER);

	return 0;
}

void __exit iom_exit(void) 
{
	// exit module
	printk("exit module\n");
	unregister_chrdev(MAJOR_NUMBER, DEVICE);
	/*
	iounmap(iom_fpga_dot_addr);
    iounmap(iom_fpga_text_lcd_addr);
    iounmap(iom_fpga_led_addr);
    iounmap(iom_fpga_fnd_addr);
	*/
}

module_init(iom_init);
module_exit(iom_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yonguk");

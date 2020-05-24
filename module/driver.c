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
#include <linux/timer.h>
#include <linux/version.h>

#include "ioctl.h"
#include "fpga.h"

// timer variables
static struct timer_list timer;
static int timer_interval, timer_cnt, timer_init, timer_clock;

static int fpga_dot_port_usage = 0;
static int fpga_fnd_port_usage = 0;
static int ledport_usage = 0;
static int fpga_text_lcd_port_usage = 0;

static unsigned char *iom_fpga_dot_addr;
static unsigned char *iom_fpga_fnd_addr;
static unsigned char *iom_fpga_led_addr;
static unsigned char *iom_fpga_text_lcd_addr;

static int kernel_timer_usage = 0;

void set_timer();
void timer_handler();

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
    // open devices
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
    return 0;
}

int iom_release(struct inode *minode, struct file *mfile)
{
    // release devices
	fpga_dot_port_usage = 0;
    fpga_fnd_port_usage = 0;
    ledport_usage = 0;
    fpga_text_lcd_port_usage = 0;
    kernel_timer_usage = 0;
	return 0;
}



void set_timer()
{
    timer.expires = jiffies + timer_interval * 10;
    timer.data = NULL;
    timer.function = timer_handler;
    add_timer(&timer);
}

void timer_handler()
{
    printk("blink\n");
    timer_clock++;

    
    if (timer_clock < timer_cnt){
        set_timer();
    }
    else{
        printk("timeout\n");
    }
}

int iom_unlocked_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
	printk("module.iom_unlocked_ioctl called\n");

	if(_IOC_TYPE(cmd) != IOCTL_MAGIC)    // check magic number
		return -EINVAL;

	int size = _IOC_SIZE(cmd);
	struct timer_args* args;

	switch(cmd){
	case IOCTL_WRITE_TIMER:
		printk("timer set\n");
		args = (struct timer_args *)arg;
        timer_interval = args->interval;
        timer_cnt = args->cnt;
        timer_init = args->init;
		printk("%d %d %d\n", timer_interval, timer_cnt, timer_init);
		break;
	case IOCTL_ON:
		printk("timer started\n");
        set_timer();
		break;
	}	
}

int __init iom_init(void)
{	
	// init module
	printk("init module\n");

    // register device driver
	int result = register_chrdev(MAJOR_NUMBER, DEVICE, &fops);
    if(result < 0) {
        printk("Cant register driver\n");
        return result;
    }
	printk("Register driver. name : %s, major number : %d\n", DEVICE, MAJOR_NUMBER);

    // init timer
    init_timer(&timer);

	return 0;
}

void __exit iom_exit(void) 
{
	// unregister device driver
	printk("exit module\n");
	unregister_chrdev(MAJOR_NUMBER, DEVICE);

	// unmap devices
	iounmap(iom_fpga_dot_addr);
    iounmap(iom_fpga_text_lcd_addr);
    iounmap(iom_fpga_led_addr);
    iounmap(iom_fpga_fnd_addr);

    // delete timer
    del_timer_sync(&timer);
}

module_init(iom_init);
module_exit(iom_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yonguk");

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

// devcies variables
static int fpga_dot_port_usage = 0;
static int fpga_fnd_port_usage = 0;
static int ledport_usage = 0;
static int fpga_text_lcd_port_usage = 0;

static unsigned char *iom_fpga_dot_addr;
static unsigned char *iom_fpga_fnd_addr;
static unsigned char *iom_fpga_led_addr;
static unsigned char *iom_fpga_text_lcd_addr;

static int kernel_timer_usage = 0;

// fnd states
int fnd_pos;
int fnd_val;

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
	if(fpga_dot_port_usage 
        || fpga_fnd_port_usage 
        || ledport_usage 
        || fpga_text_lcd_port_usage 
        || kernel_timer_usage)
        return -EBUSY;
    
    timer_clock = 0;
    fpga_dot_port_usage = 1;
    fpga_fnd_port_usage = 1;
    ledport_usage = 1;
    fpga_text_lcd_port_usage = 1;
    kernel_timer_usage = 1;
    return 0;
}

int iom_release(struct inode *minode, struct file *mfile)
{
    timer_clock = 0;

    // release devices
	fpga_dot_port_usage = 0;
    fpga_fnd_port_usage = 0;
    ledport_usage = 0;
    fpga_text_lcd_port_usage = 0;
    kernel_timer_usage = 0;
	return 0;
}

void fnd_write(){
    unsigned int value[4] = {0, 0, 0, 0};
    value[fnd_pos] = fnd_val;
    unsigned short int value_short = 0;

    value_short = value[0] << 12 | value[1] << 8 |value[2] << 4 |value[3];
    outw(value_short,(unsigned int)iom_fpga_fnd_addr);
    
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

    // write devices
    fnd_write();


    // update states
    fnd_val = ((fnd_val - 1 + 1) % 8) + 1;
    if (timer_clock != 0 && timer_clock % 8  == 0)
        fnd_pos = (fnd_pos + 1) % 4;
    

    // check timeout 
    if (timer_clock < timer_cnt){
        set_timer();
    }
    else{
        printk("timeout\n");
    }

    // increase clock
    timer_clock++;
}

int iom_unlocked_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
	printk("module.iom_unlocked_ioctl called\n");

	if(_IOC_TYPE(cmd) != IOCTL_MAGIC)    // check magic number
		return -EINVAL;


	struct timer_args* args;
    char init_buf[10];
    int i=0;

	switch(cmd){
	case IOCTL_WRITE_TIMER:
		printk("timer set\n");
		args = (struct timer_args *)arg;
        timer_interval = args->interval;
        timer_cnt = args->cnt;
        timer_init = args->init;
		printk("%d %d %d\n", timer_interval, timer_cnt, timer_init);

        sprintf(init_buf, "%04d", timer_init);
        for(i = 0; i < 4; i++)
            if(init_buf[i] != '0')
                break;
        fnd_pos = i;
        fnd_val = init_buf[i] - '0';
        printk("pos : %d, val : %d\n", fnd_pos, fnd_val);
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

    //map devices
    iom_fpga_led_addr = ioremap(IOM_LED_ADDRESS, 0x1);
    iom_fpga_fnd_addr = ioremap(IOM_FND_ADDRESS, 0x4);
    iom_fpga_dot_addr = ioremap(IOM_FPGA_DOT_ADDRESS, 0x10);
    iom_fpga_text_lcd_addr = ioremap(IOM_FPGA_TEXT_LCD_ADDRESS, 0x32);

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

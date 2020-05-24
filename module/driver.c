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

// lcd states
char name[] = "Yonguk Lee";
char id[] = "20171667";
int name_period = 12;
int id_period = 16;

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

void lcd_write()
{
    int space_name, space_id;
    if (timer_clock % name_period <= 6)
        space_name = timer_clock % name_period;
    else
        space_name = 12 - (timer_clock % name_period);
    
    if (timer_clock % id_period <= 8)
        space_id = timer_clock % id_period;
    else
        space_id = 16 - (timer_clock % id_period);

    unsigned char lcd_buf[33] = "                                "; // 32 blanks
    int i;

    for(i = 0; i < 10; i++){
        lcd_buf[i + space_name] = name[i];
    }
    for(i = 0; i < 8; i++){
        lcd_buf[i + space_id + 16] = id[i];
    }


    unsigned short int _s_value = 0;
    for(i=0; i<33; i++)
    {
        _s_value = (lcd_buf[i] & 0xFF) << 8 | lcd_buf[i + 1] & 0xFF;
        outw(_s_value,(unsigned int)iom_fpga_text_lcd_addr+i);
        i++;
    }
}

void dot_write() {
    // display fnd_val on dat display
    unsigned char *value = dot_matix_numbers[fnd_val];
    int i;    
    for(i=0;i<10;i++)
    {
        outw(value[i] & 0x7F, (unsigned int)iom_fpga_dot_addr + i*2);
    }
}

void fnd_write(){
    // write fnd_val at fnd_pos of fnd display
    unsigned int value[4] = {0, 0, 0, 0};
    value[fnd_pos] = fnd_val;
    unsigned short int value_short = 0;

    value_short = value[0] << 12 | value[1] << 8 |value[2] << 4 |value[3];
    outw(value_short,(unsigned int)iom_fpga_fnd_addr);
}

void led_write(){
    // turn on the led at fnd_val
    unsigned short value = 1 << (8 - fnd_val);
    outw(value, (unsigned int)iom_fpga_led_addr);
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
    // check timeout 
    if (timer_clock < timer_cnt){
        // write devices
        fnd_write();
        led_write();
        dot_write();
        lcd_write();

        set_timer();
    }
    else{
        printk("timeout\n");
        
        // reset devices
        // turn off fnd
        fnd_val = 0;
        fnd_write();
        // turn off led
        outw(0, (unsigned int)iom_fpga_led_addr);
        // turn off dot display
        int i;    
        for(i=0;i<10;i++)
        {
            outw(0, (unsigned int)iom_fpga_dot_addr + i*2);
        }
        // turn off lcd
        unsigned char lcd_buf[33] = "                                "; // 32 blanks
        unsigned short int _s_value = 0;
        for(i=0; i<33; i++)
        {
            _s_value = (lcd_buf[i] & 0xFF) << 8 | lcd_buf[i + 1] & 0xFF;
            outw(_s_value,(unsigned int)iom_fpga_text_lcd_addr+i);
            i++;
        }
    }

    // increase clock
    timer_clock++;

    // update states
    fnd_val = ((fnd_val - 1 + 1) % 8) + 1;
    if (timer_clock != 0 && timer_clock % 8  == 0)
        fnd_pos = (fnd_pos + 1) % 4;
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

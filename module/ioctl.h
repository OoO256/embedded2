#ifndef __IOCTL_H__

#define MAJOR_NUMBER 242
#define DEVICE "/dev/dev_driver"
#define SET_OPTION 0x9999
#define COMMAND 0x10000

struct timer_args {
    // timer 에게 전달할 인자들
    int interval;
    int cnt;
    int init;
};

#define IOCTL_MAGIC      MAJOR_NUMBER
#define IOCTL_ON         _IO(IOCTL_MAGIC, 0)
#define IOCTL_OFF        _IO(IOCTL_MAGIC, 1)
#define IOCTL_READ       _IOR(IOCTL_MAGIC, 2, int)
#define IOCTL_WRITE      _IOW(IOCTL_MAGIC, 3, int)
#define IOCTL_RDWR       _IOWR(IOCTL_MAGIC, 4, int)
#define IOCTL_WRITE_TIMER      _IOW(IOCTL_MAGIC, 5, void*)

#define IOCTL_MAX        5

#define __IOCTL_H__
#endif
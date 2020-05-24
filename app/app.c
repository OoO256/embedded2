#include <asm/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "../module/ioctl.h"

int main(int argc, char** argv){

    if (argc != 4){
        // arg 갯수가 다른 경우
        printf("wrong number of args : %d\n", argc);
        return 0;
    }

    // 인자 입력
    struct timer_args args = {
        .interval = atoi(argv[1]),
        .cnt = atoi(argv[2]),
        .init = atoi(argv[3]),
    };
    printf("timer args : %d %d %d\n", args.interval, args.cnt, args.init);
    
    // Device driver open
    int dev = open(DEVICE, O_RDWR);
    if (dev < 0){
        printf("Device open error : %s\n", DEVICE);
        exit(1);
    }
    
    // send timer args
    ioctl(dev, IOCTL_WRITE_TIMER, &args);
    // start timer
    ioctl(dev, IOCTL_ON);

    // release devices
    close(dev);
    return 0;
}
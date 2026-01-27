#include <stdio.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <linux/i2c.h>

#define LED0_ON_L 0x0006
#define LED3_OFF_H 0x0015
#define LED15_OFF_H 0x0045

int convert_to_hex(int duty_cycle){
    int val = duty_cycle*4096/100;
    return val;
}

int main(){
    
    int file;
    char filename[20];
    int adapter_nr = 1;
    // Check whether the adapter is working or not
    snprintf(filename, 19, "/dev/i2c-%d", adapter_nr);
    file = open(filename, O_RDWR);
    if (file<0){
        perror("I2C bus adapter failed");
        exit(1);
    }

    return 0;
}
#include <stdio.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/i2c.h>

#define PCAADDR 0x40
#define MODE1 0x00
#define MODE1_RUN 0x20
#define MODE1_SLEEP 0x10
#define PRE_SCALE 0xFE
#define MODE2 0x01
#define MODE2_FAST 0x0C
#define MODE2_SAFE 0x04
#define ALLCALLADR 0x70
#define LED0_ON_L 0x06
#define LED0_ON_H 0x07
#define LED0_OFF_L 0x08
#define LED0_OFF_H 0x09

int main() {
    int file;  //Will contain contents of the sensor
    char filename[20]; //Will contain sensor name
    int adapter_nr = 1; //I2C adapter number
    //Define empty buffers to read and write to registers
    __uint8_t buf[2];
    __uint8_t reg, val;

    //Open the adapter
    snprintf(filename, 19, "/dev/i2c-%d", adapter_nr);
    file = open(filename, O_RDWR);
    if(file<0){
        return 1;
    }
    printf("Opened I2C bus: %s\n", filename);

    //Try communicating with PCA9685
    if (ioctl(file, I2C_SLAVE, PCAADDR) < 0) {
        perror("Failed to select I2C slave");
        close(file);
        return 1;
    }
    printf("PCA9685 detected at address 0x%X\n", PCAADDR);

    //Change PRESCALE value for 50Hz PWM frequency
    buf[0] = MODE1;
    buf[1] = MODE1_SLEEP;
    write(file, buf, 2); //Enter sleep (stop oscillator)
    buf[0] = PRE_SCALE;
    buf[1] = 0x79; //50Hz PWM frequency
    write(file, buf, 2); //Change frequency
    //Verify whether the write to prescale was successful
    reg = PRE_SCALE;
    write(file, &reg, 1);
    read(file, &val, 1);
    if(val == 0x79){
        printf("PRESCALE write successful: 0x%02X\n", val);
    }
    else{
        printf("PRESCALE mismatch: 0x%02X\n", val);
    }

    //Set MODE1 to initialise PCA9685. Wake up (Start Oscillator)
    buf[0] = MODE1; //MODE1 register address
    buf[1] = MODE1_RUN; //Value to be writen onto MODE1 reg (0b00100000 -- AI=1)
    write(file, buf, 2);
    usleep(500); //Wait for the oscillator to stabilise according to the datasheet
    //Verify whether the write to mode1 was successful
    reg = MODE1;
    write(file, &reg, 1);
    read(file, &val, 1);
    if(val == MODE1_RUN){
        printf("MODE1 write successful: 0x%02X\n", val);
    }
    else{
        printf("MODE1 mismatch: 0x%02X\n", val);
    }

    //Set MODE2
    buf[0] = MODE2; //MODE2 register address
    buf[1] = MODE2_FAST; //Value to be writen onto MODE2 reg (0b00001100 -- OUTDRV=1)
    write(file, buf, 2);
    //Verify whether the write to mode2 was successful
    reg = MODE2;
    write(file, &reg, 1);
    read(file, &val, 1);
    if(val == MODE2_FAST){
        printf("MODE2 write successful: 0x%02X\n", val);
    }
    else{
        printf("MODE2 mismatch: 0x%02X\n", val);
    }

    // Turn the servo motor to ~90 degrees
    __uint8_t buf_1[5];
    buf_1[0] = LED0_ON_L;
    buf_1[1] = 0x00;   // ON_L
    buf_1[2] = 0x00;   // ON_H
    buf_1[3] = 0xCD;   // OFF_L (307 counts)
    buf_1[4] = 0x00;   // OFF_H
    write(file, buf_1, 5);
    if (write(file, buf_1, 5) != 5) {
        perror("Servo write failed");
    }
    else{
        printf("PWM Write successful");
    }
    
    return 0;
}

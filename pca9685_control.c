#include <stdio.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
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
#define SERVO_1 0x06
#define SERVO_2 0x0A
#define SERVO_3 0x0E
#define MOTOR 0x12

//We take zero delay and thus the start time is always zero
//Initialize empty 12-bit char arrays to store stop time of pulse. Define a structure to hold both values
typedef struct {
    unsigned char LED_OFF_L;
    unsigned char LED_OFF_H;
} DutyCycleValues;

//Function to convert pulse stop time to a 12-bit array and compute LED_OFF_L & LED_OFF_H
DutyCycleValues duty_cycle(double duty_cycle) {
    DutyCycleValues values;
    // Compute the 12-bit value
    int dec = (int)round(4095 * duty_cycle/100);
    // Assign the lower and upper bits
    values.LED_OFF_L = dec & 0xFF;
    values.LED_OFF_H = (dec >> 8) & 0x0F;
    return values;
}

int main(){
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

    //Range of servo motor is 180 degrees, from 1.6 to 15.5 duty cycle.
    //The servo rotates continuously at duty cycle = 1.55 and 16
    //Initialise all the servo motors at duty cycle = 8.5
    DutyCycleValues low_time = duty_cycle(10.5);
    __uint8_t buf_1[5];
    //SERVO1
    buf_1[0] = SERVO_1;
    buf_1[1] = 0x00;   // ON_L
    buf_1[2] = 0x00;   // ON_H
    buf_1[3] = low_time.LED_OFF_L;   // OFF_L
    buf_1[4] = low_time.LED_OFF_H;   // OFF_H
    if (write(file, buf_1, 5) != 5) {
        perror("Servo write failed");
    }
    else{
        printf("PWM Write successful");
    }
    //SERVO2
    buf_1[0] = SERVO_2;
    if (write(file, buf_1, 5) != 5) {
        perror("Servo write failed");
    }
    else{
        printf("PWM Write successful");
    }
    //SERVO3
    buf_1[0] = SERVO_3;
    if (write(file, buf_1, 5) != 5) {
        perror("Servo write failed");
    }
    else{
        printf("PWM Write successful");
    }
    
    //Arm the BLDC motor
    DutyCycleValues low_time_motor = duty_cycle(10);
    __uint8_t buf_2[5];
    buf_2[0] = MOTOR;
    buf_2[1] = 0x00;   // ON_L
    buf_2[2] = 0x00;   // ON_H
    buf_2[3] = low_time_motor.LED_OFF_L;   // OFF_L
    buf_2[4] = low_time_motor.LED_OFF_H;   // OFF_H
    if (write(file, buf_2, 5) != 5) {
        perror("Servo write failed");
    }
    else{
        printf("PWM Write successful");
    }
    usleep(1000000);
    usleep(1000000);
    if (write(file, buf_2, 5) != 5) {
        perror("Servo write failed");
    }
    else{
        printf("PWM Write successful");
    }
    usleep(1000000);
    usleep(1000000);
    low_time_motor = duty_cycle(7.5);
    buf_2[3] = low_time_motor.LED_OFF_L;
    buf_2[4] = low_time_motor.LED_OFF_H;
    if (write(file, buf_2, 5) != 5) {
        perror("Servo write failed");
    }
    else{
        printf("PWM Write successful");
    }
    usleep(1000000);
    usleep(1000000);
    low_time_motor = duty_cycle(0);
    buf_2[3] = low_time_motor.LED_OFF_L;
    buf_2[4] = low_time_motor.LED_OFF_H;
    if (write(file, buf_2, 5) != 5) {
        perror("Servo write failed");
    }
    else{
        printf("PWM Write successful");
    }

    return 0;
}
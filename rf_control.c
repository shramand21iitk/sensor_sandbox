#include <stdio.h> 
#include<stdlib.h> 
#include <lgpio.h> 
#include <unistd.h> 
#include <signal.h> 
#include <stdint.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
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

volatile int running = 1; 
void handle_sigint(int sig) {running = 0;} 
volatile uint64_t pulse_width = 0, old_pulse_width=0, rising_tick = 0;
typedef struct {
    unsigned char LED_OFF_L;
    unsigned char LED_OFF_H;
} DutyCycleValues;

void alert_callback(int num_alerts, lgGpioAlert_p alerts, void *userdata){
    for (int i = 0; i < num_alerts; i++){
        if(alerts[i].report.level == 1){
            rising_tick = alerts[i].report.timestamp;
        }
        else if(alerts[i].report.level == 0){
            if(rising_tick > 0){
                pulse_width = (alerts[i].report.timestamp - rising_tick)/1000;
                rising_tick = 0;
            }
        }
    }
}
DutyCycleValues duty_cycle(double duty_cycle) {
    DutyCycleValues values;
    int dec = (int)round(4095 * duty_cycle/100);
    values.LED_OFF_L = dec & 0xFF;
    values.LED_OFF_H = (dec >> 8) & 0x0F;
    return values;
}

int main() {
    static int userdata = 123;
    signal(SIGINT, handle_sigint);
    int chip_number = 4; 
    int h = lgGpiochipOpen(chip_number);
    if (h < 0) {
        printf("Failed to open GPIO chip %d. Error code: %d\n", chip_number, h);
        return 1;
    }
    printf("Successfully opened chip! Handle: %d\n", h);
    lgChipInfo_t cInfo;
    int status = lgGpioGetChipInfo(h, &cInfo);
    if (status == LG_OKAY){
        printf("lines=%d  name=%s  label=%s \n", cInfo.lines, cInfo.name ,cInfo.label);
    }
    int gpio = 17;
    lgLineInfo_t lInfo;
    status = lgGpioGetLineInfo(h, gpio, &lInfo);
    if (status == LG_OKAY){
        printf("lFlags=%d  name=%s  user=%s \n", lInfo.lFlags, lInfo.name, lInfo.user);
    }
    status = lgGpioClaimAlert(h, 0, LG_BOTH_EDGES, gpio, -1);
    if(status<0){
        printf("Failed to claim alert on pin %d. Error: %d\n", gpio, status);
        lgGpiochipClose(h);
        return 1;
    }
    printf("Alert claim successful on GPIO %d\n", gpio);
    int alert = lgGpioSetAlertsFunc(h, gpio, alert_callback, &userdata);
    if (alert < 0) {
        printf("Failed to set alert function. Error: %d\n", alert);
    }

    int file; 
    char filename[20]; 
    int adapter_nr = 1;
    __uint8_t buf[2];
    __uint8_t reg, val;
    snprintf(filename, 19, "/dev/i2c-%d", adapter_nr);
    file = open(filename, O_RDWR);
    if(file<0){return 1;}
    printf("Opened I2C bus: %s\n", filename);
    if (ioctl(file, I2C_SLAVE, PCAADDR) < 0) {
        perror("Failed to select I2C slave");
        close(file);
        return 1;
    }
    printf("PCA9685 detected at address 0x%X\n", PCAADDR);
    buf[0] = MODE1;
    buf[1] = MODE1_SLEEP;
    write(file, buf, 2); 
    buf[0] = PRE_SCALE;
    buf[1] = 0x79; 
    write(file, buf, 2); 
    reg = PRE_SCALE;
    write(file, &reg, 1);
    read(file, &val, 1);
    if(val == 0x79){printf("PRESCALE write successful: 0x%02X\n", val);}
    else{printf("PRESCALE mismatch: 0x%02X\n", val);}
    buf[0] = MODE1; 
    buf[1] = MODE1_RUN; 
    write(file, buf, 2);
    usleep(500); 
    reg = MODE1;
    write(file, &reg, 1);
    read(file, &val, 1);
    if(val == MODE1_RUN){printf("MODE1 write successful: 0x%02X\n", val);}
    else{printf("MODE1 mismatch: 0x%02X\n", val);}
    buf[0] = MODE2; 
    buf[1] = MODE2_FAST;
    write(file, buf, 2);
    reg = MODE2;
    write(file, &reg, 1);
    read(file, &val, 1);
    if(val == MODE2_FAST){printf("MODE2 write successful: 0x%02X\n", val);}
    else{printf("MODE2 mismatch: 0x%02X\n", val);}
    
    while(running){
        if(pulse_width > 0 && abs(pulse_width-old_pulse_width)>10){
            double duty = (pulse_width / 200.0);
            // Map the 5.0-10.0 range to the 1.6-15.5 range
            // Formula: output = (input - in_min) * (out_max - out_min) / (in_max - in_min) + out_min
            double target_duty = (duty - 5.0) * (15.5 - 1.6) / (10.0 - 5.0) + 1.6;
            DutyCycleValues low_time = duty_cycle(target_duty);
            __uint8_t buf_1[5];
            buf_1[0] = SERVO_1;
            buf_1[1] = 0x00;  
            buf_1[2] = 0x00;  
            buf_1[3] = low_time.LED_OFF_L;  
            buf_1[4] = low_time.LED_OFF_H;  
            if (write(file, buf_1, 5) != 5) {perror("Servo write failed");}
            else{printf("Pulse: %lu us | Duty: %.2f%%\n", pulse_width, target_duty);}
            old_pulse_width = pulse_width;
            pulse_width = 0;
        }
        usleep(1000);
    }

    lgGpiochipClose(h);
    return 0;
}
#include <stdio.h> //Standard i/o functions library
#include<stdlib.h> //For abs() function
#include <lgpio.h> //GPIO library
#include <unistd.h> // Required for usleep()
#include <signal.h> // Required for catching Ctrl+C
#include <stdint.h> //Required for uint64_t

volatile int running = 1; // Global flag to control the main loop
void handle_sigint(int sig) {running = 0;} // Signal handler to catch Ctrl+C and exit gracefully
volatile uint64_t pulse_width = 0, old_pulse_width=0, rising_tick = 0; //global variable storing pulse_width and leading edge time

void alert_callback(int num_alerts, lgGpioAlert_p alerts, void *userdata){
    for (int i = 0; i < num_alerts; i++){
        if(alerts[i].report.level == 1){
            rising_tick = alerts[i].report.timestamp;
        }
        else if(alerts[i].report.level == 0){
            if(rising_tick > 0){
                pulse_width = (alerts[i].report.timestamp - rising_tick)/1000; //Difference in nanoseconds and we need answer in microseconds
                rising_tick = 0;
            }
        }
    }
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
    
    while(running){
        //Send pulse width to PCA9685 via I2C
        if(pulse_width > 0 && abs(pulse_width-old_pulse_width)>7){
            printf("Pulse width : %lu\n", pulse_width);
            old_pulse_width = pulse_width;
            pulse_width = 0;
        }
        usleep(1000);
    }

    lgGpiochipClose(h);
    return 0;
}
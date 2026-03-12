#include <stdio.h>
#include <lgpio.h>
#include <unistd.h> // Required for usleep()
#include <signal.h> // Required for catching Ctrl+C

volatile int running = 1; // Global flag to control the main loop
void handle_sigint(int sig) {running = 0;} // Signal handler to catch Ctrl+C and exit gracefully

int main() {
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

    int gpioin = lgGpioClaimInput(h, 0, gpio);
    if (gpioin < 0){
        printf("Failed to claim pin %d for input. Error code: %d\n", gpio, gpioin);
        lgGpiochipClose(h); 
        return 1; 
    }
    printf("Input claim successful.\n");
    printf("Reading GPIO %d. Press Ctrl+C to exit...\n", gpio);

    while(running){
        int read = lgGpioRead(h, gpio);
        if(read < 0){
            printf("Unable to read GPIO. Error code: %d\n", read);
            break; 
        }
        printf("Level = %d \n", read);
        usleep(500); // Sleep for 500 microseconds to prevent CPU hogging
    }
    
    printf("\nCleaning up and closing GPIO chip...\n");
    lgGpiochipClose(h);
    
    return 0;
}
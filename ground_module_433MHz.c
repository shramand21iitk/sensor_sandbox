// C library headers
#include <stdio.h>
#include <string.h>
// Linux headers
#include <fcntl.h>   // Contains file controls like O_RDWR
#include <errno.h>   // Error integer and sterror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>  // write(), read(), close()

int main(){
    int fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
    if (fd == -1){
        perror("Failed to open serial port to GPS");
        return 1;
    }
    else{
        printf("Connection to GPS port opened fd = %d \n", fd);
    }
    return 0;
}
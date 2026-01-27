#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

int main(){
    int fd = open("/dev/ttyAMA10", O_RDWR | O_NOCTTY);
    if (fd == -1){
        perror("Failed to open serial port to GPS");
        return 1;
    }
    else{
        printf("Connection to GPS port opened fd = %d \n", fd);
    }
    close(fd);
}
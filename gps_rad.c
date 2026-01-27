/*
struct termios{
    tcflag_t c_iflag;  //input mode flags
    tcflag_t c_oflag;  //output mode flags
    tcflag_t c_cflag;  //control mode flags
    tcflag_t c_lflag;  //local mode flags
    cc_t c_line;       //line discipline
    cc_t c_cc[NCCS];   //control characters
};
*/

// To solve header/configuration issue
#define _DEFAULT_SOURCE
// C library headers
#include <stdio.h>
#include <string.h>
// Linux headers
#include <fcntl.h>   // Contains file controls like O_RDWR
#include <errno.h>   // Error integer and sterror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>  // write(), read(), close()

int main(){
    struct termios gps_port_settings;
    char gps_read_buffer[100] = {};
    char gps_read_line[1024] = {};
    int fd = open("/dev/ttyAMA10", O_RDWR | O_NOCTTY);
    
    if (fd == -1){
        perror("Failed to open serial port to GPS");
        return 1;
    }
    else{
        printf("Connection to GPS port opened fd = %d \n", fd);
    }
    
    tcgetattr(fd, &gps_port_settings);
    
    gps_port_settings.c_cflag &= ~PARENB;  // Clear parity bit (disabling parity)
    gps_port_settings.c_cflag &= ~CSTOPB;  // Only 1 stop bit (Clear stop field)
    gps_port_settings.c_cflag |= CS8;      // 8 data bits per byte
    gps_port_settings.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control
    gps_port_settings.c_cflag |= CREAD | CLOCAL;
    
    gps_port_settings.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); //Disable any special handling of received bytes
    gps_port_settings.c_iflag &= ~(IXON|IXOFF|IXANY); //Turn off software flow control

    //Check the following 3 lines during testing
    gps_port_settings.c_lflag &= ~ECHO;    // Disable echo
    gps_port_settings.c_lflag &= ~ECHOE;   // Disable erasure
    gps_port_settings.c_lflag &= ~ECHONL;  // Disable new-line echo
    gps_port_settings.c_lflag &= ~ISIG;    // Disable interpretation of INTR, QUIT and SUSP
    gps_port_settings.c_lflag &= ~ICANON;  //Disable canonical mode

    gps_port_settings.c_oflag &= ~OPOST;   // Disable specal interpretation of output bytes (e.g:- newline char)
    gps_port_settings.c_oflag &= ~ONLCR;   // Disable conversion of newline to carriage return/line feed
    
    gps_port_settings.c_cc[VTIME] = 1;     // read() returns after 
    gps_port_settings.c_cc[VMIN] = 0;      // non-blocking return read()

    //Get the current Baud rate and configure accordingly
    speed_t input_baudrate = cfgetispeed(&gps_port_settings); //get input baud rate
    cfsetispeed(&gps_port_settings, input_baudrate); //set the baudrate
    cfsetispeed(&gps_port_settings, B9600);
    cfsetospeed(&gps_port_settings, B9600);
    
    tcsetattr(fd, TCSANOW, &gps_port_settings);

    //clear out garbage values
    if (tcflush(fd, TCIOFLUSH) != 0){
        perror("tcflush");
    }

    //continuous loop to keep reading the data from GPS
    while(1){
        int received_bytes = read(fd, &gps_read_buffer, sizeof(gps_read_buffer)-1);
        if (received_bytes > 0){
            gps_read_buffer[received_bytes] = '\0';
            printf("%s", gps_read_buffer);
            fflush(stdout);
        }
        memset(gps_read_buffer, 0, sizeof(gps_read_buffer));
    }

    close(fd);
}
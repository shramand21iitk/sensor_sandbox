#include <stdio.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <linux/i2c.h>

// reg we need for the code (burst reads)
#define mpuaddr 0x68
#define bmpaddr 0x76
#define magaddr 0x0D
#define HX_L 0x00
#define ASAX 0x10
#define ACCEL_XOUT_H 0x3B
#define WHO_AM_I 0x75
#define dig_T1 0x88
#define P_MSB 0xF7
#define PWR_MGMT_1 0x6B
#define PWR_MGMT_2 0x6C
#define CTRL_MEAS 0xF4
#define CONFIG 0xF5
#define SMPLRT_DIV 0x19
#define CONFIG_MPU 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define ACCEL_CONFIG_2 0x1D
#define MAG_STATUS 0x06
#define MAG_CTRL1 0x09
#define MAG_CTRL2 0X0A

int main(){

    int file;
    char filename[20];
    int adapter_nr = 1;
    snprintf(filename, 19, "/dev/i2c-%d", adapter_nr);
    file = open(filename, O_RDWR);
    if (file<0){
        perror("I2C bus adapter failed");
        exit(1);
    }

    //Configure MPU (accel and gyro)
    unsigned char mpu_wai_reg = WHO_AM_I;
    unsigned char wake_buf[2];
    wake_buf[0] = PWR_MGMT_1;
    wake_buf[1] = 0x00;
    unsigned char smplrt[2];
    smplrt[0] = SMPLRT_DIV;
    smplrt[1] = 0x13;
    unsigned char config_mpu[2];
    config_mpu[0] = CONFIG_MPU;
    config_mpu[1] = 0x04;
    unsigned char gyro_config[2];
    gyro_config[0] = GYRO_CONFIG;
    gyro_config[1] = 0x08;
    unsigned char accel_config1[2];
    accel_config1[0] = ACCEL_CONFIG;
    accel_config1[1] = 0x08;
    unsigned char accel_config2[2];
    accel_config2[0] = ACCEL_CONFIG_2;
    accel_config2[1] = 0x04;
    unsigned char start[2];
    start[0] = PWR_MGMT_2;
    start[1] = 0x00;
    unsigned char mpu_conf_buf[1];
    if (ioctl(file, I2C_SLAVE, mpuaddr) < 0) {
        perror("Failed to acquire bus access and/or talk to MPU9250");
        close(file);
        exit(1);
    }
    else {
        // Write register address we want to read
        if (write(file, &mpu_wai_reg, 1) != 1) {
            perror("Failed to write to the I2C bus (MPU9250)");
        } 
        else {
            // Read one byte from WHO_AM_I
            if (read(file, mpu_conf_buf, 1) != 1) {
                perror("Failed to read from the I2C bus (MPU9250)");
            } 
            else {
                printf("MPU9250 WHO_AM_I = 0x%02X\n", mpu_conf_buf[0]);
                if (write(file, wake_buf, 2) != 2) {perror("Failed to write wake-up command to MPU9250");} 
                else {
                    printf("MPU9250 is awake and ready!\n");
                    write(file, start, 2);
                    write(file, smplrt, 2);
                    write(file, config_mpu, 2);
                    write(file, gyro_config, 2);
                    write(file, accel_config1, 2);
                    write(file, accel_config2, 2);
                }
            }
        }
    }

    //Configure BMP280
    unsigned char ctrl[2], config[2];
    ctrl[0] = CTRL_MEAS;
    ctrl[1] = 0x57;
    config[0] = CONFIG;
    config[1] = 0x10;
    if (ioctl(file, I2C_SLAVE, bmpaddr) < 0) {
        perror("Failed to acquire bus access and/or talk to BMP280");
        close(file);
        exit(1);
    }
    else {
        printf("BMP280 communication setup OK\n");
        write(file, ctrl, 2);
        write(file, config, 2);
    }

    //Configure QMC5883L (magnetometer)
    unsigned char mag_status[2];
    mag_status[0] = MAG_STATUS;
    unsigned char mag_ctrl1[2];
    mag_ctrl1[0] = MAG_CTRL1;
    mag_ctrl1[1] = 0x15;
    unsigned char mag_ctrl2[2];
    mag_ctrl2[0] = MAG_CTRL2;
    mag_ctrl2[1] = 0x80;
    if (ioctl(file, I2C_SLAVE, magaddr) < 0){
        perror("Failed to acquire bus access and/or talk to QMC5883L");
        close(file);
        exit(1);
    }
    else {
        printf("BMP280 communication setup OK\n");
        write(file, mag_ctrl2, 2);
        usleep(50000);
        write(file, mag_ctrl1, 2);
    }

    // Store magnetometer values
    unsigned char mag_reg = HX_L;      
    unsigned char mag_data[6]; 
    struct i2c_rdwr_ioctl_data mag_packets;
    struct i2c_msg mag_messages[2];
    mag_messages[0].addr  = magaddr;
    mag_messages[0].flags = 0;        
    mag_messages[0].len   = 1;
    mag_messages[0].buf   = &mag_reg;
    mag_messages[1].addr  = magaddr;
    mag_messages[1].flags = I2C_M_RD;  
    mag_messages[1].len   = 6; 
    mag_messages[1].buf   = mag_data;
    mag_packets.msgs = mag_messages;
    mag_packets.nmsgs = 2;
    // Store accelerometer and gyroscope values
    unsigned char mpu_reg = ACCEL_XOUT_H;      
    unsigned char mpu_data[14];         
    struct i2c_rdwr_ioctl_data mpu_packets;
    struct i2c_msg mpu_messages[2];
    mpu_messages[0].addr  = mpuaddr;
    mpu_messages[0].flags = 0;        
    mpu_messages[0].len   = 1;
    mpu_messages[0].buf   = &mpu_reg;
    mpu_messages[1].addr  = mpuaddr;
    mpu_messages[1].flags = I2C_M_RD;  
    mpu_messages[1].len   = 14; 
    mpu_messages[1].buf   = mpu_data;
    mpu_packets.msgs = mpu_messages;
    mpu_packets.nmsgs = 2;
    // Store barometer values
    unsigned char bmp_reg = dig_T1;
    unsigned char bmp_data[24];
    struct i2c_rdwr_ioctl_data bmp_packets;
    struct i2c_msg bmp_messages[2];
    bmp_messages[0].addr  = bmpaddr;
    bmp_messages[0].flags = 0;        
    bmp_messages[0].len   = 1;
    bmp_messages[0].buf   = &bmp_reg;
    bmp_messages[1].addr  = bmpaddr;
    bmp_messages[1].flags = I2C_M_RD;  
    bmp_messages[1].len   = 24; 
    bmp_messages[1].buf   = bmp_data;
    bmp_packets.msgs = bmp_messages;
    bmp_packets.nmsgs = 2;
    // ADC vals
    unsigned char bmp_adc_reg = P_MSB;
    unsigned char bmp_adc_data[6];
    struct i2c_rdwr_ioctl_data bmp_adc_packets;
    struct i2c_msg bmp_adc_messages[2];
    bmp_adc_messages[0].addr  = bmpaddr;
    bmp_adc_messages[0].flags = 0;        
    bmp_adc_messages[0].len   = 1;
    bmp_adc_messages[0].buf   = &bmp_adc_reg;
    bmp_adc_messages[1].addr  = bmpaddr;
    bmp_adc_messages[1].flags = I2C_M_RD;  
    bmp_adc_messages[1].len   = 6; 
    bmp_adc_messages[1].buf   = bmp_adc_data;
    bmp_adc_packets.msgs = bmp_adc_messages;
    bmp_adc_packets.nmsgs = 2;
    //Declare variables
    double p1, p2, p3, p4, p5, p6, p7, p8, p9, t1, t2, t3, t, t_fine, p, adc_p, adc_t, var1, var2;
    float data[10]; //Data buffer

    // Burst read bmp calib data
    if (ioctl(file, I2C_RDWR, &bmp_packets) < 0) {
        perror("Unable to send data");
        close(file);
        exit(1);
    }
    // Do something with the data in bmp_data
    t1 = (bmp_data[1] << 8) | bmp_data[0];
    t2 = (double)((__int16_t)((bmp_data[3] << 8) | bmp_data[2]));
    t3 = (double)((__int16_t)((bmp_data[5] << 8) | bmp_data[4]));
    p1 = (bmp_data[7] << 8) | bmp_data[6];
    p2 = (double)((__int16_t)((bmp_data[9] << 8) | bmp_data[8]));
    p3 = (double)((__int16_t)((bmp_data[11] << 8) | bmp_data[10]));
    p4 = (double)((__int16_t)((bmp_data[13] << 8) | bmp_data[12]));
    p5 = (double)((__int16_t)((bmp_data[15] << 8) | bmp_data[14]));
    p6 = (double)((__int16_t)((bmp_data[17] << 8) | bmp_data[16]));
    p7 = (double)((__int16_t)((bmp_data[19] << 8) | bmp_data[18]));
    p8 = (double)((__int16_t)((bmp_data[21] << 8) | bmp_data[20]));
    p9 = (double)((__int16_t)((bmp_data[23] << 8) | bmp_data[22]));

    //Loop to read data
    while(1){
        // Burst read magnetometer data
        if (ioctl(file, I2C_RDWR, &mag_packets) < 0) {
            perror("Unable to send data (Magnetometer)");
            continue;
        }
        // Do something with the data in mag_data now
        data[0] = (float)((__int16_t)((mag_data[1] << 8) | mag_data[0]));
        data[1] = (float)((__int16_t)((mag_data[3] << 8) | mag_data[2]));
        data[2] = (float)((__int16_t)((mag_data[5] << 8) | mag_data[4]));
        // Burst read mpu data
        if (ioctl(file, I2C_RDWR, &mpu_packets) < 0) {
            perror("Unable to send data (MPU6050)");
            continue;
        }
        // Do something with the data in mpu_data now
        data[3] = (float)((__int16_t)((mpu_data[0] << 8) | mpu_data[1])/8192.0f); 
        data[4] = (float)((__int16_t)((mpu_data[2] << 8) | mpu_data[3])/8192.0f);
        data[5] = (float)((__int16_t)((mpu_data[4] << 8) | mpu_data[5])/8192.0f);
        data[6] = (float)((__int16_t)((mpu_data[8] << 8) | mpu_data[9])/65.5f);
        data[7] = (float)((__int16_t)((mpu_data[10] << 8) | mpu_data[11])/65.5f);
        data[8] = (float)((__int16_t)((mpu_data[12] << 8) | mpu_data[13])/65.5f);
        // Burst read bmp adc data
        if (ioctl(file, I2C_RDWR, &bmp_adc_packets) < 0) {
            perror("Unable to send data (BMP280)");
            continue;
        }
        // Do something with the adc data in bmp_data
        adc_p = (bmp_adc_data[0] << 12) | (bmp_adc_data[1] << 4) | (bmp_adc_data[2] >> 4);
        adc_t = (bmp_adc_data[3] << 12) | (bmp_adc_data[4] << 4) | (bmp_adc_data[5] >> 4);
        // Temperature values calibration
        var1 = (adc_t/16384.0 - t1/1024.0)*t2;
        var2 = (adc_t/31072.0-t1/8192.0)*(adc_t/31072-t1/8192)*(t3);
        t_fine = var1+var2;
        t = (var1+var2)/5120.0;
        // Pressure values calibration
        var1 = t_fine/2.0 - 64000.0;
        var2 = var1*var1*p6/32768.0;
        var2 = var2*var1*p5*2.0;
        var2 = var2/4.0 + p4*65536.0;
        var1 = p3*var1*var1/524288.0 + p2*var1/524288.0;
        var1 = (1.0 + var1/32768.0)*p1;
        p = 1048576.0-adc_p;
        p = (p-var2/4096.0)*6250.0/var1;
        var1 = p9*p*p/2147483648.0;
        var2 = p*p8/32768.0;
        p = p+(var1+var2+p7)/16.0;
        data[9] = 8434.42536*log(pow(101325/p, 0.0341/(t+273.15)));
        for (int i = 0; i < 10; i++) {
            printf("%f ", data[i]);
        }
        printf("\n");
        usleep(20000);
    }
    close(file);

    return 0;
}
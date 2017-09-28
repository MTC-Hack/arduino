#include <TroykaRTC.h>
#include <Wire.h>

#include <stdint.h>

void mpu6050_init();

void mpu6050_get_all(int16_t data[6]);

float gps_get_lat();

float gps_get_long();

float gps_get_spd();

void rtc_config();

uint32_t rtc_get_time();

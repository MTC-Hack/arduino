#include <TroykaRTC.h>
#include <Wire.h>
#include <SD.h>
#include "SX1272.h"
//#include <SPI.h>

#include <stdint.h>

#define CS_PIN 7
#define FUEL_PIN A0

void mpu6050_init();

void mpu6050_get_all(int16_t data[6]);

float gps_get_lat();

float gps_get_long();

float gps_get_spd();

void rtc_config();

uint32_t rtc_get_time();

void sd_init();

void sd_write_to_file(char *filename, char *data);

void fuelmeter_init();

uint16_t fuelmeter_get();

void wifi_init();

void wifi_send(char *buf);

void InitLORA();

bool lora_receive(uint8_t receive_buffer[]);

bool get_ACK();

void send_data(char *buf);

void print_lora_sent(char *packet);

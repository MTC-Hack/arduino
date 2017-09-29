#include "util.h"

const int MPU_addr = 0x68; // I2C address of the MPU-6050
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

void mpu6050_init() {
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
}

void mpu6050_get_all(int16_t data[6]) {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 14, true); // request a total of 14 registers
  data[0] = Wire.read() << 8 | Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L) AcX
  data[1] = Wire.read() << 8 | Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L) AcY
  data[2] = Wire.read() << 8 | Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L) AcZ
  Wire.read() << 8 | Wire.read(); // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  data[3] = Wire.read() << 8 | Wire.read(); // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L) GyX
  data[4] = Wire.read() << 8 | Wire.read(); // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L) GyY
  data[5] = Wire.read() << 8 | Wire.read(); // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L) GyZ
}

float gps_get_lat() {
  return 55.761354;
}

float gps_get_long() {
  return 37.574172;
}

float gps_get_spd() {
  return 65.34;
}


RTC clock;
void rtc_config() {
  Serial.println("entering rtc_config");
  // инициализация часов
  clock.begin();
  // метод установки времени и даты в модуль вручную
  // clock.set(10,25,45,27,07,2005,THURSDAY);
  // метод установки времени и даты автоматически при компиляции
  clock.set(__TIMESTAMP__);
  // что бы время менялось при прошивки или сбросе питания
  // закоментируйте оба метода clock.set();
  Serial.println("leaving rtc_config");
}

uint32_t rtc_get_time() {
  return clock.getUnixTime();
}

void sd_init() {
  Serial.print("Initializing SD card...");
  pinMode(CS_PIN, OUTPUT);
  // see if the card is present and can be initialized:
  if (!SD.begin(CS_PIN)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
}

void sd_write_to_file(char *filename, char *data) {
  File dataFile = SD.open(filename, FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(data);
    dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening file");
  }
}

void fuelmeter_init() {
  pinMode(FUEL_PIN, OUTPUT);
}

uint16_t fuelmeter_get() {
  return analogRead(FUEL_PIN);
}

void wifi_init() {
  Serial1.begin(115200);
}

void wifi_send(char *buf) {
  Serial.print("Sending packet via wi-fi: ");
  Serial.println(buf);
  Serial1.println(buf);
}



void InitLORA() {
  int e ;
  // Power ON the module
  e = sx1272.ON();
  Serial.print(F("Setting power ON: state "));
  Serial.println(e, DEC);

  // Set transmission mode and print the result
  e = sx1272.setMode(1);
  Serial.print(F("Setting Mode: state "));
  Serial.println(e, DEC);

  // Set header
  e = sx1272.setHeaderON();
  Serial.print(F("Setting Header ON: state "));
  Serial.println(e, DEC);

  // Select frequency channel
  e = sx1272.setChannel(CH_10_868);
  Serial.print(F("Setting Channel: state "));
  Serial.println(e, DEC);

  // Set CRC
  e = sx1272.setCRC_ON();
  Serial.print(F("Setting CRC ON: state "));
  Serial.println(e, DEC);

  // Select output power (Max, High or Low)
  e = sx1272.setPower('x');
  Serial.print(F("Setting Power: state "));
  Serial.println(e, DEC);

  // Set the node address and print the result
  e = sx1272.setNodeAddress(2);
  Serial.print(F("Setting node address: state "));
  Serial.println(e, DEC);

  // Print a success message
  Serial.println(F("SX1272 successfully configured"));
  Serial.println();
}

bool lora_receive(uint8_t receive_buffer[]) {
  bool received = false;
  int e;
  uint16_t w_timer = 10000;
  e = sx1272.receivePacketTimeout(w_timer);
  // if packet was received?
  if (!e) {
    uint8_t tmp_length;
    sx1272.getSNR();
    sx1272.getRSSIpacket();
    tmp_length = sx1272._payloadlength;
    for (int i = 0; i < tmp_length; i++)
      receive_buffer[i] = sx1272.packet_received.data[i];
#ifdef DEBUG
    if (tmp_length) {
      Serial.print("Received:");
      for (int i = 0; i < tmp_length; i++)
        Serial.print(((uint8_t)sx1272.packet_received.data[i]));
      Serial.print(" ");
    }
    Serial.println();
#endif
    received = true;
  }
  return received;
}

bool get_ACK() {
  uint8_t receive_buffer[10];
  lora_receive(receive_buffer);
  if (receive_buffer[0] == 'O' && receive_buffer[1] == 'K'){
    //Serial.println("LORA ACK");
    return true;
  }
  else
    return false;
}

void send_data(char *buf) {
  int e ;
  e = sx1272.sendPacketTimeout(3, buf);
  if (!get_ACK())
    wifi_send(buf);
  else
    print_lora_sent(buf);
}


void print_lora_sent(char *packet) {
  Serial.print("Packet: ");
  Serial.print(packet);
  Serial.println(" sended by LoRa!");
}


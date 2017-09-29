#include <SoftwareSerial.h>
#define FLOAT_STR_LEN 12
#include<Wire.h>

#include <stdlib.h>
#include <math.h>
#include "SX1272.h"
#include "util.h"

#define READ_BUFFER_SIZE 100
#define MAX_RETRIES 3
#define OBD_CMD_RETRIES 3
#define GRXPIN 12
#define GTXPIN 13
boolean isVinSet = false;
char globalVin[18];
int e;
int retries = MAX_RETRIES;
boolean bt_error_flag;
boolean obd_error_flag;

SoftwareSerial BTSerial(GRXPIN, GTXPIN);

void ClearBuffer() {
  while (BTSerial.available() > 0) {
    delay(10);
    BTSerial.read();
  }
}

void send_OBD_query(char *obd_query) {
  ClearBuffer();
  BTSerial.write(obd_query);
  BTSerial.write('\r');
}

void send_OBD_cmd(char *obd_cmd) {
  Serial.println("send_OBD_cmd");
  char recvChar;
  boolean prompt;
  int retries, i;
  char bufin[100];
  prompt = false;
  retries = 0;
  ClearBuffer();
  if (!(obd_error_flag)) {
   // Serial.println("obd_error_flag=false");
    prompt = false;
    retries = 0;
    while ((!prompt)  && (retries < OBD_CMD_RETRIES)) {
      ClearBuffer();
      BTSerial.print(obd_cmd);
      BTSerial.print("\r");
      i = 0;
      Serial.println("before while");
      while (BTSerial.available() <= 0);
      Serial.println("after while");
     // Serial.println(BTSerial.available());
      while ((BTSerial.available() > 0) && (!prompt)) {
        recvChar = BTSerial.read();
        bufin[i] = recvChar;
       // Serial.print("recvchar=");
       // Serial.println(recvChar);
        i++;
        if (recvChar == 62) {
          prompt = true;
          bufin[i] = '\0';
       }
      }

      retries = retries + 1;
      delay(2000);
    }
    if (retries >= OBD_CMD_RETRIES) {
      obd_error_flag = true;
      Serial.println("Troubles with OBD-II connection!");
    }
  }
 /* else
    Serial.println("obd_error_flag==true");*/
  Serial.println(bufin);
}

void InitLORA() {
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

void InitOBD() {
  Serial.println("initob");
  if (bt_error_flag == false) {
    Serial.println("initob if");
    obd_error_flag = false;
    send_OBD_cmd("ATZ");
    delay(250);
    send_OBD_cmd("ATE0");
    delay(250);
    send_OBD_cmd("ATL0");
    delay(250);
    send_OBD_cmd("ATH0");
    delay(250);
    send_OBD_cmd("ATSP0");
    delay(250);

  }
}

unsigned int hexToDec(String hexString) {
  unsigned int decValue = 0;
  int nextInt;
  for (int i = 0; i < hexString.length(); i++) {
    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    decValue = (decValue * 16) + nextInt;
  }
  return decValue;
}

int hex_to_int(char c) {
  if (c >= 97)
    c = c - 32;
  int first = c / 16 - 3;
  int second = c % 16;
  int result = first * 10 + second;
  if (result > 9) result--;
  return result;
}

int hex_to_ascii(char c, char d) {
  int high = hex_to_int(c) * 16;
  int low = hex_to_int(d);
  return high + low;
}

int speed_calc() {
  boolean prompt;
  char recvChar;
  char bufin[7];
  int i;
  int spd;
  int retries;
  if (!(obd_error_flag)) {                                       //if no OBD connection error
    prompt = false;
    retries = 0;
    while ((!prompt) && (retries < OBD_CMD_RETRIES)) {
      ClearBuffer();
      send_OBD_query("010D1");
      while (BTSerial.available() <= 0);

      i = 0;
      while ((BTSerial.available() > 0) && (!prompt)) {
        recvChar = BTSerial.read();
        if (!(recvChar == 32)) {
          bufin[i] = recvChar;
          i = i + 1;
        }
        if (recvChar == 62) {
          prompt = true;
          bufin[i] = '\0';
        }
      }

      retries = retries + 1;                                      //increase retries
      delay(2000);
    }
    if (retries >= OBD_CMD_RETRIES) {                             // if OBD cmd retries reached
      obd_error_flag = true;
      return -1;
    } else {
      String tempHex(bufin[4]);
      String tempHex2(bufin[5]);
      String tempHexTotal = tempHex + tempHex2;
      spd = hexToDec(tempHexTotal);
      return spd;
    }
  }
  return -1;
}

int rpm_calc() {
  boolean prompt;
  char recvChar;
  char bufin[9];
  int i;
  int rpm;
  int retries;
  if (!(obd_error_flag)) {                                       //if no OBD connection error
    prompt = false;
    retries = 0;
    while ((!prompt) && (retries < OBD_CMD_RETRIES)) {
      ClearBuffer();
      send_OBD_query("010C1");
      while (BTSerial.available() <= 0);

      i = 0;
      while ((BTSerial.available() > 0) && (!prompt)) {
        recvChar = BTSerial.read();
        if (!(recvChar == 32)) {
          bufin[i] = recvChar;
          i = i + 1;
        }
        if (recvChar == 62) {
          prompt = true;
          bufin[i] = '\0';
        }
      }

      retries = retries + 1;                                      //increase retries
      delay(2000);
    }
    if (retries >= OBD_CMD_RETRIES) {                             // if OBD cmd retries reached
      obd_error_flag = true;
      return -1;
    } else {
      String tempHex(bufin[4]);
      String tempHex2(bufin[5]);
      String tempHex3(bufin[6]);
      String tempHex4(bufin[7]);
      String tempHexTotal = tempHex + tempHex2 + tempHex3 + tempHex4;
      rpm = 0;
      rpm = hexToDec(tempHexTotal);
      rpm = rpm / 4;
      return rpm;
    }
  }
  return -1;
}

int maf_calc() {
  boolean prompt;
  char recvChar;
  char bufin[9];
  int i;
  int rpm;
  int retries;
  if (!(obd_error_flag)) {                                       //if no OBD connection error
    prompt = false;
    retries = 0;
    while ((!prompt) && (retries < OBD_CMD_RETRIES)) {
      ClearBuffer();
      send_OBD_query("01101");
      while (BTSerial.available() <= 0);

      i = 0;
      while ((BTSerial.available() > 0) && (!prompt)) {
        recvChar = BTSerial.read();
        if (!(recvChar == 32)) {
          bufin[i] = recvChar;
          i = i + 1;
        }
        if (recvChar == 62) {
          prompt = true;
          bufin[i] = '\0';
        }
      }

      retries = retries + 1;                                      //increase retries
      delay(2000);
    }
    if (retries >= OBD_CMD_RETRIES) {                             // if OBD cmd retries reached
      obd_error_flag = true;
      return -1;
    } else {
      String tempHex(bufin[4]);
      String tempHex2(bufin[5]);
      String tempHex3(bufin[6]);
      String tempHex4(bufin[7]);
      String tempHexTotal = tempHex + tempHex2 + tempHex3 + tempHex4;
      rpm = 0;
      rpm = hexToDec(tempHexTotal);
      rpm = rpm / 100;
      return rpm;
    }
  }
  return -1;
}

int temp_calc() {
  boolean prompt;
  char recvChar;
  char bufin[7];
  int i;
  int temp;
  int retries;
  if (!(obd_error_flag)) {                                       //if no OBD connection error
    prompt = false;
    retries = 0;
    while ((!prompt) && (retries < OBD_CMD_RETRIES)) {
      ClearBuffer();
      send_OBD_query("01051");
      while (BTSerial.available() <= 0);

      i = 0;
      while ((BTSerial.available() > 0) && (!prompt)) {
        recvChar = BTSerial.read();
        if (!(recvChar == 32)) {
          bufin[i] = recvChar;
          i = i + 1;
        }
        if (recvChar == 62) {
          prompt = true;
          bufin[i] = '\0';
        }
      }

      retries = retries + 1;                                      //increase retries
      delay(2000);
    }
    ClearBuffer();
    if (retries >= OBD_CMD_RETRIES) {                             // if OBD cmd retries reached
      obd_error_flag = true;
      return -1;
    } else {
      String tempHex(bufin[4]);
      String tempHex2(bufin[5]);
      String tempHexTotal = tempHex + tempHex2;
      temp = hexToDec(tempHexTotal);
      temp -= 40;
      return temp;
    }
  }
}

String getDTC() {
  boolean prompt, chet;
  char recvChar;
  char bufin[100];
  int i, j;
  String DTCS;
  int retries;
  if (!(obd_error_flag)) {                                       //if no OBD connection error
    prompt = false;
    retries = 0;
    while ((!prompt) && (retries < OBD_CMD_RETRIES)) {
      ClearBuffer();
      send_OBD_query("03");
      i = 0; j = 0;
      boolean newline = true;
      chet = false;
      while (newline) {
        j++;
        while (BTSerial.available() <= 0);
        while ((BTSerial.available() > 0) && (!prompt)) {
          recvChar = BTSerial.read();
          if ((i < 13 * j) && (!(recvChar == 32))) {
            bufin[i] = recvChar;
            i = i + 1;
          }
          if (recvChar == 62) {
            prompt = true;
            newline = false;
            bufin[i] = '\0';
            if (i == 12 * j) {
              chet = true;
            } else {
              chet = false;
            }
            break;
          }
        }
      }

      retries = retries + 1;                                      //increase retries
      delay(2000);
    }
    ClearBuffer();
    if (retries >= OBD_CMD_RETRIES) {                             // if OBD cmd retries reached
      obd_error_flag = true;
      String ans("ERR");
      return ans;
    } else {
      String nc(bufin[3]);
      String divider("_");
      String pref;
      String pref1;
      int numCodeRows = 0;
      numCodeRows = hexToDec(nc);
      for (int i = 0; i < numCodeRows; i++) {
        String char1(bufin[4 + 13 * i]);
        if (bufin[4 + 13 * i] == '0' || bufin[4 + 13 * i] == '1' || bufin[4 + 13 * i] == '2' || bufin[4 + 13 * i] == '3') pref = "P";
        if (bufin[4 + 13 * i] == '4' || bufin[4 + 13 * i] == '5' || bufin[4 + 13 * i] == '6' || bufin[4 + 13 * i] == '7') pref = "C";
        if (bufin[4 + 13 * i] == '8' || bufin[4 + 13 * i] == '9' || bufin[4 + 13 * i] == 'A' || bufin[4 + 13 * i] == 'B') pref = "B";
        if (bufin[4 + 13 * i] == 'C' || bufin[4 + 13 * i] == 'D' || bufin[4 + 13 * i] == 'E' || bufin[4 + 13 * i] == 'F') pref = "U";

        if (bufin[8 + 13 * i] == '0' || bufin[8 + 13 * i] == '1' || bufin[8 + 13 * i] == '2' || bufin[8 + 13 * i] == '3') pref1 = "P";
        if (bufin[8 + 13 * i] == '4' || bufin[8 + 13 * i] == '5' || bufin[8 + 13 * i] == '6' || bufin[8 + 13 * i] == '7') pref1 = "C";
        if (bufin[8 + 13 * i] == '8' || bufin[8 + 13 * i] == '9' || bufin[8 + 13 * i] == 'A' || bufin[8 + 13 * i] == 'B') pref1 = "B";
        if (bufin[8 + 13 * i] == 'C' || bufin[8 + 13 * i] == 'D' || bufin[8 + 13 * i] == 'E' || bufin[8 + 13 * i] == 'F') pref1 = "U";
        String char2(bufin[5 + 13 * i]);
        String char3(bufin[6 + 13 * i]);
        String char4(bufin[7 + 13 * i]);
        String char5(bufin[8 + 13 * i]);
        String char6(bufin[9 + 13 * i]);
        String char7(bufin[10 + 13 * i]);
        String char8(bufin[11 + 13 * i]);
        DTCS += pref;
        DTCS += char1;
        DTCS += char2;
        DTCS += char3;
        DTCS += char4;
        DTCS += divider;

        if ((i == numCodeRows - 1) && (!chet)) break;
        DTCS += pref1;
        DTCS += char5;
        DTCS += char6;
        DTCS += char7;
        DTCS += char8;
        DTCS += divider;
      }
      return DTCS;
    }
  }
}

String getVIN() {
  boolean prompt, check;
  char recvChar;
  char bufin[100];
  int i, j;
  check = true;
  int retries;
  if (!(obd_error_flag)) {
    prompt = false;
    retries = 0;
    while ((!prompt) && (retries < OBD_CMD_RETRIES)) {
      ClearBuffer();
      send_OBD_query("0902");
      i = 0;
      j = 0;
      boolean newline = true;
      while (newline) {
        j++;
        while (BTSerial.available() <= 0);
        while ((BTSerial.available() > 0) && (!prompt)) {
          recvChar = BTSerial.read();
          if (i < 17 * j) {
            bufin[i] = recvChar;
            i = i + 1;
          }
          if (recvChar == 62) {
            prompt = true;
            newline = false;
            bufin[i] = '\0';
            break;
          }
        }
      }
      retries = retries + 1;
      delay(2000);
    }
    ClearBuffer();
    if (retries >= OBD_CMD_RETRIES) {
      obd_error_flag = true;
      String ans("ERR");
      return ans;
    } else {
      if ((bufin[0] == '1') && (bufin[1] == '0') && (bufin[2] == '1') && (bufin[3] == '4') &&
          (bufin[4] == '4') && (bufin[5] == '9') && (bufin[6] == '0') && (bufin[7] == '2') && (bufin[8] == '0') && (bufin[9] == '1')) {
        char buf;
        int j = 0;
        char vin[18];
        for (int i = 10; i < 16; i++) {
          if (i % 2 != 0) {
            vin[j] = hex_to_ascii(buf, bufin[i]);
            if (!((((vin[j] >= 65) && (vin[j] <= 90)) || ((vin[j] >= 48) && (vin[j] <= 57))) && check)) check = false; // РїСЂРѕРІРµСЂРєР° РЅР° РїСЂР°РІРёР»СЊРЅРѕСЃС‚СЊ VIN РєРѕРґР° A-Z0-9
            j++;
          } else {
            buf = bufin[i];
          }
        }
        for (int i = 19; i < 33; i++) {
          if (i % 2 == 0) {
            vin[j] = hex_to_ascii(buf, bufin[i]);
            if (!((((vin[j] >= 65) && (vin[j] <= 90)) || ((vin[j] >= 48) && (vin[j] <= 57))) && check)) check = false; // РїСЂРѕРІРµСЂРєР° РЅР° РїСЂР°РІРёР»СЊРЅРѕСЃС‚СЊ VIN РєРѕРґР° A-Z0-9
            j++;
          } else {
            buf = bufin[i];
          }
        }
        for (int i = 36; i < 50; i++) {
          if (i % 2 != 0) {
            vin[j] = hex_to_ascii(buf, bufin[i]);
            if (!((((vin[j] >= 65) && (vin[j] <= 90)) || ((vin[j] >= 48) && (vin[j] <= 57))) && check)) check = false; // РїСЂРѕРІРµСЂРєР° РЅР° РїСЂР°РІРёР»СЊРЅРѕСЃС‚СЊ VIN РєРѕРґР° A-Z0-9
            j++;
          } else {
            buf = bufin[i];
          }
        }
        vin[17] = '\0';

        String VIN(vin);
        return VIN;
      }
    }
  }
}

void setup() {
  BTSerial.begin(38400);
  bt_error_flag = false;
  Serial.begin(9600);
  InitLORA();
  InitOBD();
  mpu6050_init();
  delay(3000);
 // rtc_config();
 // sd_init();
  fuelmeter_init();
}

int i = 0;

void print_lora_sent(char *packet) {
  Serial.print("Packet: ");
  Serial.print(packet);
  Serial.println(" sended by LoRa!");
}

void loop() {
  char buf[100];
  if (!(obd_error_flag)) {
    if (!isVinSet) {
      char vin[18];
      String vinCode = getVIN();
      strcpy(vin, vinCode.c_str());

      if (retries != 0) {
        if (retries == MAX_RETRIES) {
          strcpy(globalVin, vin);
          retries--;
        } else {
          Serial.print("Comparing ");
          Serial.print(vin);
          Serial.print(" and ");
          Serial.println(globalVin);
          if (!strcmp(vin, globalVin)) {
            retries--;
          } else {
            retries = MAX_RETRIES;
          }
        }
      } else {
        isVinSet = true;
      }
    } else {
      String errors = getDTC();
      //1-пакет: ид, номер пакета, время (пока не работает), ошибки
     // sd_write_to_file("datalog.txt", "***START OF DATA TRANSMISSION");
      snprintf(buf, sizeof(buf), "%s_1_%d_%s", globalVin, 0, errors.c_str());
      e = sx1272.sendPacketTimeout(3, buf);
     // sd_write_to_file("datalog.txt", buf);
      print_lora_sent(buf);
      //2-пакет: ид, номер пакета, время (пока не работает), скорость, maf, обороты/мин
      snprintf(buf, sizeof(buf), "%s_2_%d_%s_%s_%s", globalVin, 0, String(speed_calc()).c_str(),  String(maf_calc()).c_str(),  String(rpm_calc()).c_str());
      e = sx1272.sendPacketTimeout(3, buf);
      //sd_write_to_file("datalog.txt", buf);
      print_lora_sent(buf);
      //3-пакет: ид, номер пакета, время (пока не работает), скорость, широта, долгота
      snprintf(buf, sizeof(buf), "%s_3_%d_%s_%s_%s", globalVin, 0, String(gps_get_spd()).c_str(), String(gps_get_lat()).c_str(), String(gps_get_long()).c_str());
      e = sx1272.sendPacketTimeout(3, buf);
     // sd_write_to_file("datalog.txt", buf);
      print_lora_sent(buf);
      //4-пакет: ид, номер пакета, время (пока не работает), уровень топлива, 3 параметра акселерометра, 3 параметра гироскопа
      int16_t data[6];
      mpu6050_get_all(data);
      snprintf(buf, sizeof(buf), "%s_4_%d_%d_%d_%d_%d_%d_%d_%d", globalVin, 0, fuelmeter_get(), data[0], data[1], data[2], data[3], data[4], data[5]);
      e = sx1272.sendPacketTimeout(3, buf);
     // sd_write_to_file("datalog.txt", buf);
      print_lora_sent(buf);
     // sd_write_to_file("datalog.txt", "***END OF DATA TRANSMISSION");
    }
  }
  if (obd_error_flag) {
    isVinSet = false;
    InitOBD();
  }
}


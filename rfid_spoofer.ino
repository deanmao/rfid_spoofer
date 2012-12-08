#include <SoftwareSerial.h>
#include <EEPROM.h>

int led = 0;
int coil_pin = 1;
int serial_rx = 2;
int reader_pin = 3;

byte finalParity[4];
bool readingRfid = false;
byte dataIndex = 0;

SoftwareSerial rfid = SoftwareSerial(serial_rx, 4);
void setup() {
  pinMode(coil_pin, OUTPUT);
  pinMode(reader_pin, OUTPUT);
  pinMode(led, OUTPUT);

  digitalWrite(coil_pin, HIGH);
  digitalWrite(reader_pin, HIGH);
  analogWrite(led, 0);
  rfid.begin(9600);
}

void setPinManchester(int clock_half, int signal) {
  if((clock_half ^ signal) == 1) {
    digitalWrite(coil_pin, LOW);
  } else {
    digitalWrite(coil_pin, HIGH);
  }
}

void sendBit(byte b) {
  setPinManchester(0, b & 0x1);
  delayMicroseconds(256); 

  setPinManchester(1, b & 0x1);
  delayMicroseconds(256); 
}

void sendByte(byte b) {
  byte parity = 0;
  for(int i=0; i<4; i++) {
    byte x = (b >> (3 - i)) & 0x1;
    sendBit(x);
    parity = parity ^ x;
    finalParity[i] = finalParity[i] ^ x;
  }
  sendBit(parity);
}

void sendStartCode() {
  byte i;
  for(i=0; i<4; i++) {
    finalParity[i] = 0;
  }
  for(i=0; i<9; i++) {
    sendBit(1);
  }
}

void sendStopCode() {
  byte z[10] = {0x1, 0x1, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
  for(int i=0; i<4; i++) {
    sendBit(finalParity[i]);
  }
  sendBit(0);
}

void sendCode() {
  sendStartCode();
  for(byte i=0; i<10; i++) {
    sendByte(EEPROM.read(i));
  }
  sendStopCode();
}

void loop() {
  if (millis() < 3000) {
    while(rfid.available()) {
      byte readByte = rfid.read();
      if (readByte == 2) readingRfid = true;
      if( readingRfid ){
        if (readByte != 2 && readByte != 3 && dataIndex < 16) {
          byte b = readByte - 48;
          if (b > 9) {
            b = b - 7;
          }
          EEPROM.write(dataIndex, b);
          dataIndex++;
        }
        else if (readByte == 3) {
          readingRfid = false;
          dataIndex = 0;
          for(byte i=0; i<5; i++) {
            analogWrite(led, 255);
            delay(50);
            analogWrite(led, 0);
            delay(50);
          }
        }
      }
    }
  } else {
    digitalWrite(reader_pin, LOW);
    digitalWrite(coil_pin, LOW);
    analogWrite(led, 128);
    sendCode();
    analogWrite(led, 32);
    delay(500);
  }
}


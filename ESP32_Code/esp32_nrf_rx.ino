#include <nRF24L01.h>
#include <RF24.h>
#include <Arduino.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <math.h>

#define SDA_PIN 21
#define SCL_PIN 22

RF24 rf(4, 5);

MPU9250_asukiaaa imu(0x68);

Servo SV;
Servo ESC;

float previousTime, currentTime, dt;
float gain = 0.95;
float angleZ = 0.0;
float alpha = 0.992;
float prevCorr = 90;
float smoothedCorrection = 90;

float minAngle = 70;
float maxAngle = 126;

#pragma pack(push, 1)
typedef struct Packet {
  uint8_t sof;
  uint8_t len;
  int32_t steer;
  int32_t accel;
  uint8_t eof;
  uint8_t checksum;
} Packet;
#pragma pack(pop)
Packet P;


volatile uint32_t pkt_count = 0;
const int togglePin = 2;



void setup() {
  Serial.begin(230400);
  pinMode(togglePin, OUTPUT);
  digitalWrite(togglePin, LOW);

  delay(1000);
  rf.begin();
  rf.setDataRate(RF24_2MBPS);
  rf.setPALevel(RF24_PA_HIGH);
  rf.setAutoAck(false);
  rf.setChannel(5);
  rf.setPayloadSize(sizeof(Packet));
  rf.setCRCLength(RF24_CRC_DISABLED);
  rf.openReadingPipe(1, 0xAABBCCDDEE);
  rf.startListening();

  SV.attach(32);
  ESC.attach(33, 1000, 2000);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);  //100 kHz
  imu.setWire(&Wire);
  imu.beginGyro(GYRO_FULL_SCALE_2000_DPS);

  ESC.write(90);
  delay(50);

  P.steer = 98;
  P.accel = 90;
  previousTime = millis();

  ESC.write(90);
  delay(1500);
}


unsigned long lastPrint = 0;
uint32_t lastCount = 0;



void loop() {

  digitalWrite(2, LOW);

  while (rf.available()) {
    rf.read(&P, sizeof(P));
  }
  if (isPacketValid(P)) {
    /*
      Serial.print("Steer : ");
      Serial.print(P.steer);
      Serial.print(", Throttle : ");
      Serial.println(P.accel);
*/
    // debug

    /*
      imu.gyroUpdate();
      currentTime = millis();
      dt = (currentTime - previousTime) / 1000;
      previousTime = currentTime;

      float gyroZ = imu.gyroZ();
      if (abs(gyroZ) < 100) gyroZ = 0;
      angleZ += gyroZ * dt;  // yaw angle

      float ngyr = abs(gyroZ);
      float power = ngyr / 500;
      float sharpness = 1.5;  // augmente = plus agressif
      float curve = (exp(power * sharpness) - 1) / (exp(sharpness) - 1);
      float ngain = gain * curve;


      float correction = P.steer - gyroZ * ngain;
      correction = constrain(correction, minAngle, maxAngle);

      smoothedCorrection = alpha * prevCorr + (1 - alpha) * correction;
      prevCorr = smoothedCorrection;
*/
    // Serial.println(gyroZ);
    if (P.accel >= 88 && P.accel <= 92) {
      digitalWrite(2, HIGH);
      P.accel = 90;
    } else {
      digitalWrite(2, LOW);
    }

    ESC.write(P.accel);
    SV.write(P.steer);
  }
}


uint8_t checksumCalc(const Packet& P) {
  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&P);
  uint8_t sum = 0;

  for (size_t i = 0; i < offsetof(Packet, checksum); ++i) {
    sum ^= ptr[i];
  }
  return sum;
}


bool isPacketValid(const Packet& P) {
  if (P.sof != 0xFF) return false;
  if (P.eof != 0xFE) return false;
  if (P.len != 8) return false;

  return checksumCalc(P) == P.checksum;
}

#include <nRF24L01.h>
#include <RF24.h>


RF24 rf(4, 5);

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


void setup() {
  delay(2000);
  pinMode(2, OUTPUT);
  Serial.begin(230400);
  Serial.setTimeout(1);

  rf.begin();
  rf.setDataRate(RF24_2MBPS);
  rf.setPALevel(RF24_PA_HIGH);
  rf.setAutoAck(false);
  rf.setChannel(5);
  rf.setPayloadSize(sizeof(Packet));
  rf.setCRCLength(RF24_CRC_DISABLED);
  rf.stopListening();
  rf.openWritingPipe(0xAABBCCDDEE);

  rf.txDelay = 0;
  
  P.sof = 0xFF;
  P.len = 8;
  P.steer = 98;
  P.accel = 90;
  P.eof = 0xFE;
  P.checksum = checksumCalc(P);
}



void loop() {
  if (readLatestPacket(P) && isPacketValid(P)) {
    if (P.accel >= 88 && P.accel <= 92) {
      digitalWrite(2, HIGH);
      P.accel = 90;
    } else {
      digitalWrite(2, LOW);
    }
    rf.writeFast(&P, sizeof(P));
    //rf.write(&P, sizeof(P), false);
  }

  static uint8_t counter = 0;
  if (++counter >= 10) {
    rf.txStandBy(); // Assure que tout est envoyé
    counter = 0;
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

bool readLatestPacket(Packet& P) {
  bool gotOne = false;
  while (Serial.available() >= sizeof(Packet)) {
    Serial.readBytes((uint8_t*)&P, sizeof(Packet));
    gotOne = true;
  }
  return gotOne;
}


bool readPacket(Packet& P) {
  static uint8_t buffer[sizeof(Packet)];
  static size_t index = 0;

  while (Serial.available()) {
    uint8_t b = Serial.read();
    if (index == 0 && b != 0xFF) {
      continue;
    }

    buffer[index++] = b;

    if (index == sizeof(Packet)) {
      memcpy(&P, buffer, sizeof(Packet));
      index = 0;
      return true;
    }
  }
  return false;
}

bool isPacketValid(const Packet& P) {
  if (P.sof != 0xFF) return false;
  if (P.eof != 0xFE) return false;
  if (P.len != 8) return false;

  return checksumCalc(P) == P.checksum;
}

#include <cox.h>
#include "PMS3003.hpp"
#include "RHT03.hpp"
#include "SparkFunCCS811.hpp"
#include "LoRaMacKR920.hpp"

PMS3003 pms3003 = PMS3003(Serial2, D3, D4);
CCS811 mysensor(Wire, 0x5B);
RHT03 rht;

#include "LoRaMacKR920.hpp"
LoRaMacKR920 LoRaWAN = LoRaMacKR920(SX1276, 10);

Timer timerSend;

int32_t latestPM2_5 = -1;
int32_t latestPM10_0 = -1;

int CO2;
int TVOC;

float latestHumidity;
float latestTempC;
float latestTempF;

#define OVER_THE_AIR_ACTIVATION 1

#if (OVER_THE_AIR_ACTIVATION == 1)
static uint8_t devEui[]="\x14\x0C\x5B\xFF\xFF\x00\x05\x4A";
static uint8_t appKey[]="\x4e\x59\x36\xca\x46\x05\x32\xec\xe8\x50\xeb\xd0\x0b\x75\xef\x25";
static const uint8_t appEui[] = "\x00\x00\x00\x00\x00\x00\x00\x00";

char keyBuf[128];
Timer timerKeyInput;
#else

static const uint8_t NwkSKey[] = "\xa4\x88\x55\xad\xe9\xf8\xf4\x6f\xa0\x94\xb1\x98\x36\xc3\xc0\x86";
static const uint8_t AppSKey[] = "\x7a\x56\x2a\x75\xd7\xa3\xbd\x89\xa3\xde\x53\xe1\xcf\x7f\x1c\xc7";
static uint32_t DevAddr = 0x06e632e8;
#endif //OVER_THE_AIR_ACTIVATION

static void eventSensorMeasuredDone(
  int32_t pm1_0_CF1,
  int32_t pm2_5_CF1,
  int32_t pm10_0_CF1,
  int32_t pm1_0_Atmosphere,
  int32_t pm2_5_Atmosphere,
  int32_t pm10_0_Atmosphere
) {
  latestPM2_5 = pm2_5_Atmosphere;
  latestPM10_0 = pm10_0_Atmosphere;

  printf("PM1.0:%ld, PM2.5:%ld, PM10:%ld\n", pm1_0_Atmosphere, pm2_5_Atmosphere, pm10_0_Atmosphere);
}

static void taskPeriodicSend(void *) {
  digitalWrite(D5, HIGH);

  LoRaMacFrame *f = new LoRaMacFrame(242);
  if (!f) {
    printf("* Out of memory\n");
    digitalWrite(D5, LOW);
    return;
  }

  f->port = 1;
  f->type = LoRaMacFrame::CONFIRMED;

  uint8_t i = 0;

  int updateRet = rht.update();
  if (updateRet == 1){
    float h = rht.humidity();
    float t = rht.tempC();
    i += sprintf(
      (char *)(f->buf + i), "\"Hum\":%u.%02u,\"Temp\":%u.%02u,",
      (uint16_t) h, ((uint16_t) round(h * 100)) % 100,
      (uint16_t) t, ((uint16_t) round(t * 100)) % 100
    );
  } else {
    printf("* Read RHT error:%d\n", updateRet);
  }

  mysensor.readAlgorithmResults();
  i += sprintf(
    (char *) (f->buf + i), "\"CO2e\":%u,\"TVOC\":%u",
    mysensor.getCO2e(), mysensor.getTVOC()
  );

  if (latestPM2_5 >= 0) {
    i += sprintf((char *) (f->buf + i), ",\"PM2.5\":%ld", latestPM2_5);
  }

  if (latestPM10_0 >= 0) {
    i += sprintf((char *) (f->buf + i), ",\"PM10\":%ld", latestPM10_0);
  }

  f->len = i;
  error_t err = LoRaWAN.send(f);
  printf("* Sending periodic report (%s (%u byte)): %d\n", f->buf, f->len, err);
  if (err != ERROR_SUCCESS) {
    delete f;
    timerSend.startOneShot(10000);
    digitalWrite(D5, LOW);
  }
}

#if (OVER_THE_AIR_ACTIVATION == 1)
static void eventLoRaWANJoin(
  LoRaMac &lw,
  bool joined,
  const uint8_t *joinedDevEui,
  const uint8_t *joinedAppEui,
  const uint8_t *joinedAppKey,
  const uint8_t *joinedNwkSKey,
  const uint8_t *joinedAppSKey,
  uint32_t joinedDevAddr,
  const RadioPacket &,
  uint32_t airTime
) {
  if (joined) {
    Serial.println("* Joining done!");
    postTask(taskPeriodicSend, NULL);
  } else {
    Serial.println("* Joining failed. Retry to join.");
    lw.beginJoining(devEui, appEui, appKey);
  }
}
#endif //OVER_THE_AIR_ACTIVATION

static void eventLoRaWANSendDone(LoRaMac &lw, LoRaMacFrame *frame) {
  digitalWrite(D5, LOW);

  Serial.printf(
    "* Send done(%d): destined for port:%u, fCnt:0x%08lX, Freq:%lu Hz, "
    "Power:%d dBm, # of Tx:%u, ",
    frame->result,
    frame->port,
    frame->fCnt,
    frame->freq,
    frame->power,
    frame->numTrials
  );

  if (frame->modulation == Radio::MOD_LORA) {
    const char *strBW[] = {
      "Unknown", "125kHz", "250kHz", "500kHz", "Unexpected value"
    };
    if (frame->meta.LoRa.bw > 3) {
      frame->meta.LoRa.bw = (Radio::LoRaBW_t) 4;
    }
    Serial.printf(
      "LoRa, SF:%u, BW:%s, ", frame->meta.LoRa.sf, strBW[frame->meta.LoRa.bw]
    );
  } else if (frame->modulation == Radio::MOD_FSK) {
    Serial.printf("FSK, ");
  } else {
    Serial.printf("Unkndown modulation, ");
  }
  if (frame->type == LoRaMacFrame::UNCONFIRMED) {
    Serial.printf("UNCONFIRMED");
  } else if (frame->type == LoRaMacFrame::CONFIRMED) {
    Serial.printf("CONFIRMED");
  } else if (frame->type == LoRaMacFrame::MULTICAST) {
    Serial.printf("MULTICAST (error)");
  } else if (frame->type == LoRaMacFrame::PROPRIETARY) {
    Serial.printf("PROPRIETARY");
  } else {
    Serial.printf("unknown type");
  }
  Serial.printf(" frame\n");

  for (uint8_t t = 0; t < 8; t++) {
    const char *strTxResult[] = {
      "not started",
      "success",
      "no ack",
      "air busy",
      "Tx timeout",
    };
    Serial.printf("- [%u] %s\n", t, strTxResult[min(frame->txResult[t], 4)]);
  }
  delete frame;

  timerSend.startOneShot(60000);
}

static void eventLoRaWANReceive(LoRaMac &lw, const LoRaMacFrame *frame) {
  static uint32_t fCntDownPrev = 0;

  Serial.print("* Received a frame. Destined for port:");
  Serial.print(frame->port);
  Serial.print(", fCnt:0x");
  Serial.print(frame->fCnt, HEX);
  if (fCntDownPrev != 0 && fCntDownPrev == frame->fCnt) {
    Serial.print(" (duplicate)");
  }
  fCntDownPrev = frame->fCnt;
  Serial.print(", Freq:");
  Serial.print(frame->freq);
  Serial.print(" Hz, RSSI:");
  Serial.print(frame->power);
  Serial.print(" dB");

  if (frame->modulation == Radio::MOD_LORA) {
    const char *strBW[] = {
      "Unknown", "125kHz", "250kHz", "500kHz", "Unexpected value"
    };
    Serial.printf(
      ", LoRa, SF:%u, BW:%s",
      frame->meta.LoRa.sf, strBW[min(frame->meta.LoRa.bw, 4)]
    );
  } else if (frame->modulation == Radio::MOD_FSK) {
    Serial.printf(", FSK");
  } else {
    Serial.printf(", Unkndown modulation");
  }
  if (frame->type == LoRaMacFrame::UNCONFIRMED) {
    Serial.printf(", Type:UNCONFIRMED");
  } else if (frame->type == LoRaMacFrame::CONFIRMED) {
    Serial.printf(", Type:CONFIRMED");

    if (LoRaWAN.getNumPendingSendFrames() == 0) {
      // If there is no pending send frames, send an empty frame to ack.
      LoRaMacFrame *ackFrame = new LoRaMacFrame(0);
      if (ackFrame) {
        error_t err = LoRaWAN.send(ackFrame);
        if (err != ERROR_SUCCESS) {
          delete ackFrame;
        }
      }
    }

  } else if (frame->type == LoRaMacFrame::MULTICAST) {
    Serial.printf(", Type:MULTICAST");
  } else if (frame->type == LoRaMacFrame::PROPRIETARY) {
    Serial.printf(", Type:PROPRIETARY");
  } else {
    Serial.printf(", unknown type");
  }

  if (frame->len > 0) {
    Serial.println(", ");
    for (uint8_t i = 0; i < frame->len; i++) {
      Serial.printf(" %02X", frame->buf[i]);
    }
  }
  Serial.println();
}

#if (OVER_THE_AIR_ACTIVATION == 1)
static void taskBeginJoin(void *) {
  Serial.stopListening();
  Serial.println("* Let's start join!");
  LoRaWAN.setCurrentDatarateIndex(1); //SF8
  LoRaWAN.beginJoining(devEui, appEui, appKey);
}

static void eventAppKeyInput(SerialPort &) {
  uint8_t numOctets = strlen(keyBuf);
  if (numOctets == 32) {
    numOctets /= 2;
    char strOctet[3];

    for (uint8_t j = 0; j < numOctets; j++) {
      strOctet[0] = keyBuf[2 * j];
      strOctet[1] = keyBuf[2 * j + 1];
      strOctet[2] = '\0';

      appKey[j] = strtoul(strOctet, NULL, 16);
    }

    printf("* New AppKey:");
    for (uint8_t j = 0; j < numOctets; j++) {
      printf(" %02X", appKey[j]);
    }
    printf(" (%u byte)\n", numOctets);
    ConfigMemory.write(appKey, 0, numOctets);
    postTask(taskBeginJoin, NULL);
    return;
  } else {
    printf("* HEX string length MUST be 32-byte.");
  }

  Serial.inputKeyboard(keyBuf, sizeof(keyBuf) - 1);
}

static void eventKeyInput(SerialPort &) {
  timerKeyInput.stop();
  while (Serial.available() > 0) {
    Serial.read();
  }

  Serial.onReceive(eventAppKeyInput);
  Serial.inputKeyboard(keyBuf, sizeof(keyBuf) - 1);
  Serial.println(
    "* Enter a new appKey as a hexadecimal string "
    "[ex. 00112233445566778899aabbccddeeff]"
  );
}
#endif //OVER_THE_AIR_ACTIVATION

void setup() {
  Serial.begin(115200);
  Serial.printf("\n*** [Nol.Board] LoRaWAN Class A&C Example ***\n");

  mysensor.begin();
  pms3003.onReadDone(eventSensorMeasuredDone);
  pms3003.begin();

  rht.begin(D13);

  pinMode(D5, OUTPUT);
  digitalWrite(D5, LOW);

  System.setTimeDiff(9 * 60);  // KST

  timerSend.onFired(taskPeriodicSend, NULL);

  LoRaWAN.begin();

  LoRaWAN.onSendDone(eventLoRaWANSendDone);
  LoRaWAN.onReceive(eventLoRaWANReceive);
  LoRaWAN.onJoin(eventLoRaWANJoin);

  LoRaWAN.setPublicNetwork(false);

#if (OVER_THE_AIR_ACTIVATION == 0)
  printf("ABP!\n");
  LoRaWAN.setABP(NwkSKey, AppSKey, DevAddr);
  LoRaWAN.setNetworkJoined(true);

  postTask(taskPeriodicSend, NULL);
#else
  // System.getEUI(devEui);
  Serial.printf(
    "* DevEUI: %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n",
    devEui[0], devEui[1], devEui[2], devEui[3],
    devEui[4], devEui[5], devEui[6], devEui[7]
  );

  // ConfigMemory.read(appKey, 0, 16);
  Serial.printf(
    "* AppKey: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
    appKey[0], appKey[1], appKey[2], appKey[3],
    appKey[4], appKey[5], appKey[6], appKey[7],
    appKey[8], appKey[9], appKey[10], appKey[11],
    appKey[12], appKey[13], appKey[14], appKey[15]
  );

  if (memcmp(appKey, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 16) == 0) {
    /* The appKey is required to be entered by user.*/
    Serial.onReceive(eventAppKeyInput);
    Serial.inputKeyboard(keyBuf, sizeof(keyBuf) - 1);
    Serial.println(
      "* Enter a new appKey as a hexadecimal string "
      "[ex. 00112233445566778899aabbccddeeff]"
    );
  } else {
    Serial.println("* Press any key to enter a new appKey in 3 seconds...");
    timerKeyInput.onFired(taskBeginJoin, NULL);
    timerKeyInput.startOneShot(3000);
    Serial.onReceive(eventKeyInput);
  }

  Serial.listen();

#endif //OVER_THE_AIR_ACTIVATION
}

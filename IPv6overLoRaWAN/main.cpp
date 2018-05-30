#include <cox.h>
#include <IPv6.hpp>
#include <IPv6LoRaWAN.hpp>
#include <UDP.hpp>

#define LORAWAN_SKT 0
#define OVER_THE_AIR_ACTIVATION 1

#if (LORAWAN_SKT == 1)
#include "LoRaMacKR920SKT.hpp"
LoRaMacKR920SKT LoRaWAN = LoRaMacKR920SKT(SX1276, 12);
#else
#include "LoRaMacKR920.hpp"
LoRaMacKR920 LoRaWAN = LoRaMacKR920(SX1276, 12);
#endif

Timer timerSend;

static uint8_t devEui[8];
static const uint8_t appEui[] = "\x00\x00\x00\x00\x00\x00\x00\x00";

#if (OVER_THE_AIR_ACTIVATION == 1)
static uint8_t appKey[16];

char keyBuf[128];
Timer timerKeyInput;
#else

static const uint8_t NwkSKey[] = "\xa4\x88\x55\xad\xe9\xf8\xf4\x6f\xa0\x94\xb1\x98\x36\xc3\xc0\x86";
static const uint8_t AppSKey[] = "\x7a\x56\x2a\x75\xd7\xa3\xbd\x89\xa3\xde\x53\xe1\xcf\x7f\x1c\xc7";
static uint32_t DevAddr = 0x06e632e8;
#endif //OVER_THE_AIR_ACTIVATION

IPv6 ipv6;
IPv6LoRaWAN ipv6LoRaWAN(LoRaWAN);
uint16_t txCnt = 0;

static void taskPeriodicSend(void *) {
  IPv6Address server;
  server.s6_addr[0] = 0xfe;
  server.s6_addr[1] = 0x80;
  memcpy(&server.s6_addr[8], appEui, 8);
  char message[10];

  printf("Let's send an UDP datagram to ");
  server.printTo(Serial);
  printf(".\n");

  uint16_t len = sprintf(message, "Hi %u\n", txCnt++);
  error_t err = Udp.sendto(&ipv6LoRaWAN, NULL, 40000, &server, 40000, message, len);
  printf("err:%d\n", err);
}

#if (OVER_THE_AIR_ACTIVATION == 1)

#if (LORAWAN_SKT == 1)
static void eventLoRaWANJoin(
  LoRaMac &,
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
  Serial.printf("* Tx time of JoinRequest: %lu usec.\n", airTime);

  if (joinedNwkSKey && joinedAppSKey) {
    /* RealAppKey Joining */
    if (joined) {
      Serial.println("* RealAppKey joining done!");
      timerSend.startPeriodic(10000);
    } else {
      Serial.println("* RealAppKey joining failed. Retry to join.");
      LoRaWAN.beginJoining(NULL, NULL, NULL);
    }
  } else {
    /* PseudoAppKey Joining */
    if (joined) {
      Serial.println("* PseudoAppKey joining done! Let's do RealAppKey joining!");
      LoRaWAN.beginJoining(NULL, NULL, NULL);
    } else {
      Serial.println("* PseudoAppKey joining failed. Retry to join.");
      LoRaWAN.beginJoining(devEui, appEui, appKey);
    }
  }
}
#else
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
  Serial.printf("* Tx time of JoinRequest: %lu usec.\n", airTime);

  if (joined) {
    Serial.println("* Joining done!");
    timerSend.startPeriodic(10000);
  } else {
    Serial.println("* Joining failed. Retry to join.");
    lw.beginJoining(devEui, appEui, appKey);
  }
}
#endif //LORAWAN_SKT
#endif //OVER_THE_AIR_ACTIVATION

static void eventLoRaWANJoinRequested(
  LoRaMac &, uint32_t frequencyHz, const LoRaMac::DatarateParams_t &dr
) {
  printf("* JoinRequested(Frequency: %lu Hz, Modulation: ", frequencyHz);
  if (dr.mod == Radio::MOD_FSK) {
    printf("FSK\n");
  } else if (dr.mod == Radio::MOD_LORA) {
    const char *strLoRaBW[] = { "UNKNOWN", "125kHz", "250kHz", "500kHz" };
    printf("LoRa, SF:%u, BW:%s)\n", dr.param.LoRa.sf, strLoRaBW[dr.param.LoRa.bw]);
  }
}

static void eventLoRaWANLinkADRReqReceived(LoRaMac &l, const uint8_t *payload) {
  printf("* LoRaWAN LinkADRReq received: [");
  for (uint8_t i = 0; i < 4; i++) {
    printf(" %02X", payload[i]);
  }
  printf(" ]\n");
}

static void printChannelInformation(LoRaMac &lw) {
  for (uint8_t i = 0; i < lw.MaxNumChannels; i++) {
    const LoRaMac::ChannelParams_t *p = lw.getChannel(i);
    if (p) {
      printf(" - [%u] Frequency:%lu Hz\n", i, p->Frequency);
    } else {
      printf(" - [%u] disabled\n", i);
    }
  }

  const LoRaMac::DatarateParams_t *dr = lw.getDatarate(lw.getCurrentDatarateIndex());
  printf(" - Default DR%u:", lw.getCurrentDatarateIndex());
  if (dr->mod == Radio::MOD_LORA) {
    const char *strBW[] = {
      "Unknown", "125kHz", "250kHz", "500kHz", "Unexpected value"
    };
    printf(
      "LoRa(SF%u BW:%s)\n",
      dr->param.LoRa.sf,
      strBW[min(dr->param.LoRa.bw, 4)]
    );
  } else if (dr->mod == Radio::MOD_FSK) {
    printf("FSK\n");
  } else {
    printf("Unknown modulation\n");
  }

  int8_t power = lw.getTxPower(lw.getCurrentTxPowerIndex());
  printf(" - Default Tx: ");
  if (power == -127) {
    printf("unexpected value\n");
  } else {
    printf("%d dBm\n", power);
  }

  printf(
    " - # of repetitions of unconfirmed uplink frames: %u\n",
    lw.getNumRepetitions()
  );
}

static void eventLoRaWANLinkADRAnsSent(LoRaMac &lw, uint8_t status) {
  printf("* LoRaWAN LinkADRAns sent with status 0x%02X.\n", status);
  printChannelInformation(lw);
}

static void eventLoRaWANDutyCycleReqReceived(
  LoRaMac &lw, const uint8_t *payload
) {
  printf("* LoRaWAN DutyCycleReq received: [");
  for (uint8_t i = 0; i < 1; i++) {
    printf(" %02X", payload[i]);
  }
  printf(" ]\n");
}

static void eventLoRaWANDutyCycleAnsSent(LoRaMac &lw) {
  printf(
    "* LoRaWAN DutyCycleAns sent. Current MaxDCycle is %u.\n",
    lw.getMaxDutyCycle()
  );
}

static void eventLoRaWANRxParamSetupReqReceived(
  LoRaMac &lw,
  const uint8_t *payload
) {
  printf("* LoRaWAN RxParamSetupReq received: [");
  for (uint8_t i = 0; i < 4; i++) {
    printf(" %02X", payload[i]);
  }
  printf(" ]\n");
}

static void eventLoRaWANRxParamSetupAnsSent(LoRaMac &lw, uint8_t status) {
  printf(
    "* LoRaWAN RxParamSetupAns sent with status 0x%02X. "
    "Current Rx1Offset is %u, and Rx2 channel is (DR%u, %lu Hz).\n",
    status,
    lw.getRx1DrOffset(),
    lw.getRx2Datarate(),
    lw.getRx2Frequency()
  );
}

static void eventLoRaWANDevStatusReqReceived(LoRaMac &lw) {
  printf("* LoRaWAN DevStatusReq received.\n");
}

static void eventLoRaWANDevStatusAnsSent(
  LoRaMac &lw,
  uint8_t bat,
  uint8_t margin
) {
  printf("* LoRaWAN DevStatusAns sent. (");
  if (bat == 0) {
    printf("Powered by external power source. ");
  } else if (bat == 255) {
    printf("Battery level cannot be measured. ");
  } else {
    printf("Battery: %lu %%. ", map(bat, 1, 254, 0, 100));
  }

  if (bitRead(margin, 5) == 1) {
    margin |= bit(7) | bit(6);
  }

  printf(" SNR: %d)\n", (int8_t) margin);
}

static void eventLoRaWANNewChannelReqReceived(
  LoRaMac &lw, const uint8_t *payload
) {
  printf("* LoRaWAN NewChannelReq received [");
  for (uint8_t i = 0; i < 5; i++) {
    printf(" %02X", payload[i]);
  }
  printf(" ]\n");
}

static void eventLoRaWANNewChannelAnsSent(LoRaMac &lw, uint8_t status) {
  printf(
    "* LoRaWAN NewChannelAns sent with "
    "(Datarate range %s and channel frequency %s).\n",
    (bitRead(status, 1) == 1) ? "OK" : "NOT OK",
    (bitRead(status, 0) == 1) ? "OK" : "NOT OK"
  );

  for (uint8_t i = 0; i < lw.MaxNumChannels; i++) {
    const LoRaMac::ChannelParams_t *p = lw.getChannel(i);
    if (p) {
      printf(" - [%u] Frequency:%lu Hz\n", i, p->Frequency);
    } else {
      printf(" - [%u] disabled\n", i);
    }
  }
}

static void eventLoRaWANRxTimingSetupReqReceived(
  LoRaMac &lw,
  const uint8_t *payload
) {
  printf("* LoRaWAN RxTimingSetupReq received:  [");
  for (uint8_t i = 0; i < 1; i++) {
    printf(" %02X", payload[i]);
  }
  printf(" ]\n");
}

static void eventLoRaWANRxTimingSetupAnsSent(LoRaMac &lw) {
  printf(
    "* LoRaWAN RxTimingSetupAns sent. "
    "Current Rx1 delay is %u msec, and Rx2 delay is %u msec.\n",
    lw.getRx1Delay(),
    lw.getRx2Delay()
  );
}

static void eventLoRaWANLinkChecked(
  LoRaMac &lw,
  uint8_t demodMargin,
  uint8_t numGateways
) {
  if (numGateways > 0) {
    printf(
      "* LoRaWAN link checked. Demodulation margin: %u dB, # of gateways: %u\n",
      demodMargin, numGateways
    );
  } else {
    printf("* LoRaWAN link check failed.\n");
  }
}

static void eventLoRaWANDeviceTimeAnswered(
  LoRaMac &lw,
  bool success,
  uint32_t tSeconds,
  uint8_t tFracSeconds
) {
  if (success) {
    struct tm tLocal, tUtc;
    System.getDateTime(tLocal);
    System.getUTC(tUtc);
    printf(
      "* LoRaWAN DeviceTime answered: (%lu + %u/256) GPS epoch.\n"
      "- Adjusted local time: %u-%u-%u %02u:%02u:%02u\n"
      "- Adjusted UTC time: %u-%u-%u %02u:%02u:%02u\n",
      tSeconds, tFracSeconds,
      tLocal.tm_year + 1900, tLocal.tm_mon + 1, tLocal.tm_mday,
      tLocal.tm_hour, tLocal.tm_min, tLocal.tm_sec,
      tUtc.tm_year + 1900, tUtc.tm_mon + 1, tUtc.tm_mday,
      tUtc.tm_hour, tUtc.tm_min, tUtc.tm_sec
    );
  } else {
    printf("* LoRaWAN DeviceTime not answered\n");
  }
}

#if (OVER_THE_AIR_ACTIVATION == 1)

#if (LORAWAN_SKT == 1)
static void taskBeginJoin(void *) {
  Serial.stopListening();

#if 0
  Serial.println("* Let's start PseudoAppKey join!");
  //! [SKT PseudoAppKey joining]
  LoRaWAN.setNetworkJoined(false);
  LoRaWAN.beginJoining(devEui, appEui, appKey);
  //! [SKT PseudoAppKey joining]
#else
  Serial.println("* Let's start RealAppKey join!");
  //! [SKT RealAppKey joining]
  LoRaWAN.setNetworkJoined(true);
  LoRaWAN.beginJoining(devEui, appEui, appKey);
  //! [SKT RealAppKey joining]
#endif
}
#else
static void taskBeginJoin(void *) {
  Serial.stopListening();
  Serial.println("* Let's start join!");
  LoRaWAN.beginJoining(devEui, appEui, appKey);
}
#endif

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
  Serial.printf("\n*** [Nol.Board] IPv6 over LoRaWAN ***\n");

  System.setTimeDiff(9 * 60);  // KST

  timerSend.onFired(taskPeriodicSend, NULL);
  System.getEUI(devEui);

  LoRaWAN.begin();

#if (OVER_THE_AIR_ACTIVATION == 1)
  LoRaWAN.onJoin(eventLoRaWANJoin);
#endif

  LoRaWAN.onJoinRequested(eventLoRaWANJoinRequested);
  LoRaWAN.onLinkChecked(eventLoRaWANLinkChecked);
  LoRaWAN.onDeviceTimeAnswered(eventLoRaWANDeviceTimeAnswered, &System);
  LoRaWAN.onLinkADRReqReceived(eventLoRaWANLinkADRReqReceived);
  LoRaWAN.onLinkADRAnsSent(eventLoRaWANLinkADRAnsSent);
  LoRaWAN.onDutyCycleReqReceived(eventLoRaWANDutyCycleReqReceived);
  LoRaWAN.onDutyCycleAnsSent(eventLoRaWANDutyCycleAnsSent);
  LoRaWAN.onRxParamSetupReqReceived(eventLoRaWANRxParamSetupReqReceived);
  LoRaWAN.onRxParamSetupAnsSent(eventLoRaWANRxParamSetupAnsSent);
  LoRaWAN.onDevStatusReqReceived(eventLoRaWANDevStatusReqReceived);
  LoRaWAN.onDevStatusAnsSent(eventLoRaWANDevStatusAnsSent);
  LoRaWAN.onNewChannelReqReceived(eventLoRaWANNewChannelReqReceived);
  LoRaWAN.onNewChannelAnsSent(eventLoRaWANNewChannelAnsSent);
  LoRaWAN.onRxTimingSetupReqReceived(eventLoRaWANRxTimingSetupReqReceived);
  LoRaWAN.onRxTimingSetupAnsSent(eventLoRaWANRxTimingSetupAnsSent);

  LoRaWAN.setPublicNetwork(false);

  printChannelInformation(LoRaWAN);

#if (OVER_THE_AIR_ACTIVATION == 0)
  printf("ABP!\n");
  LoRaWAN.setABP(NwkSKey, AppSKey, DevAddr);
  LoRaWAN.setNetworkJoined(true);

  timerSend.startPeriodic(10000);
#else
  Serial.printf(
    "* DevEUI: %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n",
    devEui[0], devEui[1], devEui[2], devEui[3],
    devEui[4], devEui[5], devEui[6], devEui[7]
  );

  ConfigMemory.read(appKey, 0, 16);
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

  pinMode(D4, OUTPUT);
  digitalWrite(D4, LOW);

  ipv6LoRaWAN.begin(devEui, appEui);
  ipv6.begin();
}

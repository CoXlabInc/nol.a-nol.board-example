#include <cox.h>
#include <RHT03.hpp>

#define REPORT_TO_NOLITER 1

RHT03 rht;
Timer timerMeasure;
LPPMac *Lpp;

static void taskMeasure(void *) {
  // Call rht.update() to get new humidity and temperature values from the sensor.
  int updateRet = rht.update();

  // If successful, the update() function will return 1.
  // If update fails, it will return a value <0
  if (updateRet == 1) {
    // The humidity(), tempC(), and tempF() functions can be called -- after
    // a successful update() -- to get the last humidity and temperature
    // value
    float latestHumidity = rht.humidity();
    float latestTempC = rht.tempC();
    float latestTempF = rht.tempF();

    // Now print the values:
    printf("* Results\n");
    printf("- Humidity: %u.%02u %%\n", (uint16_t) latestHumidity, (uint16_t) round(latestHumidity * 100) % 100);
    printf("- Temp (F): %u.%02u deg F\n", (uint16_t) latestTempF, (uint16_t) round(latestTempF * 100) % 100);
    printf("- Temp (C): %u.%02u deg C\n", (uint16_t) latestTempC, (uint16_t) round(latestTempC * 100) % 100);

    IEEE802_15_4Frame *f = new IEEE802_15_4Frame(100);
    if (f == NULL) {
      printf("Not enough memory\n");
      return;
    }

    f->dstAddr.len = 2;
    f->dstAddr.id.s16 = 1;
    f->dstAddr.pan.len = 2;
    f->dstAddr.pan.id = 0x1234;

    uint8_t *payload = (uint8_t *) f->getPayloadPointer();

    uint16_t payloadLen = snprintf((char *) payload, 125,
                                    "\"Hum\":\"%u.%02u\",\"Temp\":\"%u.%02u\"",
                                    (uint16_t) latestHumidity, (uint16_t) round(latestHumidity * 100) % 100,
                                    (uint16_t) latestTempC, (uint16_t) round(latestTempC * 100) % 100);
    f->setPayloadLength(payloadLen);
    printf("* Report: %s (%u byte %p)\n", (const char *) payload, payloadLen, payload);
    Lpp->send(f);
  } else {
    printf("* updateRet:%d\n", updateRet);
  }
}

static void eventSendDone(IEEE802_15_4Mac &radio, IEEE802_15_4Frame *f, error_t result) {
  printf("* Send done: ");

  if (result == ERROR_SUCCESS) {
    printf("SUCCESS");
  } else {
    printf("FAIL");
  }

  const uint8_t *payload = (const uint8_t *) f->getPayloadPointer();
  printf(" (%02X %02X..) t: %u, RSSI:%d\n",
         payload[0],
         payload[1],
         f->txCount,
         f->power);
  delete f;
}

void setup() {
  Serial.begin(115200);
  Serial.printf("\n*** [Nol.Board] RHT03 Temperature & Humidity Sensor Test ***\n");

  rht.begin(D2);

  timerMeasure.onFired(taskMeasure, NULL);
  timerMeasure.startPeriodic(30000);

  SX1276.begin();
  SX1276.setDataRate(Radio::SF7);
  SX1276.setCodingRate(Radio::CR_4_5);
  SX1276.setTxPower(14);
  SX1276.setChannel(921900000);

  Lpp = LPPMac::Create();
  Lpp->begin(SX1276, 0x1234, 0x0002, NULL);
  Lpp->setProbePeriod(3000);
  Lpp->setListenTimeout(3300);
  Lpp->setTxTimeout(632);
  Lpp->setRxTimeout(465);
  Lpp->setRxWaitTimeout(30);
  Lpp->setUseSITFirst(true);

  Lpp->onSendDone(eventSendDone);
}

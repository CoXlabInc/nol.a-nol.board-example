#include <cox.h>

LoRaMac *LoRaWAN;
Timer timerSend;

#define OVER_THE_AIR_ACTIVATION 1

#if (OVER_THE_AIR_ACTIVATION == 1)
static const uint8_t devEui[] = "\xA7\x05\x00\xFF\xFF\x5B\x0C\x14";
static const uint8_t appEui[] = "\x00\x00\x00\x00\x00\x00\x00\x00";
static const uint8_t appKey[] = "\x0b\xf2\x80\x34\xed\xcb\x14\xe0\x9e\x1f\x94\xea\x73\xe8\xef\x0e";
#else

static const uint8_t NwkSKey[] = "\xa4\x88\x55\xad\xe9\xf8\xf4\x6f\xa0\x94\xb1\x98\x36\xc3\xc0\x86";
static const uint8_t AppSKey[] = "\x7a\x56\x2a\x75\xd7\xa3\xbd\x89\xa3\xde\x53\xe1\xcf\x7f\x1c\xc7";
static uint32_t DevAddr = 0x06e632e8;
#endif //OVER_THE_AIR_ACTIVATION

static void taskPeriodicSend(void *) {
  LoRaMacFrame *f = new LoRaMacFrame(8);
  if (!f) {
    printf("* Out of memory\n");
    return NULL;
  }

  f->port = 1;
  f->type = LoRaMacFrame::CONFIRMED;
  memcpy(f->buf, "\"test\":1", 8);

  error_t err = LoRaWAN->send(f);
  printf("* Sending periodic report (%p): %d\n", f, err);
  if (err != ERROR_SUCCESS) {
    delete f;
    timerSend.startOneShot(10000);
  }
}

static void eventLoRaWANJoin( LoRaMac &,
                              bool joined,
                              const uint8_t *joinedDevEui,
                              const uint8_t *joinedAppEui,
                              const uint8_t *joinedAppKey,
                              const uint8_t *joinedNwkSKey,
                              const uint8_t *joinedAppSKey,
                              uint32_t joinedDevAddr) {
#if (OVER_THE_AIR_ACTIVATION == 1)
  if (joined) {
    // Status is OK, node has joined the network
    printf("* Joined to the network!\n");
    timerSend.onFired(taskPeriodicSend, NULL);
    timerSend.startOneShot(10000);
  } else {
    printf("* Join failed. Retry to join\n");
    LoRaWAN->beginJoining(devEui, appEui, appKey);
  }
#endif
}

static void eventLoRaWANSendDone(LoRaMac &, LoRaMacFrame *frame, error_t result) {
  printf("* Send done(%d): [%p] destined for port[%u] ", result, frame, frame->port);
  if (frame->type == LoRaMacFrame::UNCONFIRMED) {
    printf("UNCONFIRMED");
  } else if (frame->type == LoRaMacFrame::CONFIRMED) {
    printf("CONFIRMED");
  } else if (frame->type == LoRaMacFrame::MULTICAST) {
    printf("MULTICAST");
  } else if (frame->type == LoRaMacFrame::PROPRIETARY) {
    printf("PROPRIETARY");
  } else {
    printf("unknown type");
  }
  printf(" frame\n");
  delete frame;

  //timerSend.startOneShot(10000);
  postTask(taskPeriodicSend, NULL);
}

static void eventLoRaWANReceive(LoRaMac &, const LoRaMacFrame *frame) {
  printf("* Received: destined for port[%u] ", frame->port);
  if (frame->type == LoRaMacFrame::UNCONFIRMED) {
    printf("UNCONFIRMED");
  } else if (frame->type == LoRaMacFrame::CONFIRMED) {
    printf("CONFIRMED");
  } else if (frame->type == LoRaMacFrame::MULTICAST) {
    printf("MULTICAST");
  } else if (frame->type == LoRaMacFrame::PROPRIETARY) {
    printf("PROPRIETARY");
  } else {
    printf("unknown type");
  }
  printf(" frame\n");
}

void setup() {
  Serial.begin(115200);
  Serial.printf("\n*** [Nol.Board] LoRaWAN Class A Example ***\n");

  LoRaWAN = LoRaMac::CreateForKR917();

  LoRaWAN->begin(SX1276);
  LoRaWAN->onSendDone(eventLoRaWANSendDone);
  LoRaWAN->onReceive(eventLoRaWANReceive);
  LoRaWAN->onJoin(eventLoRaWANJoin);
  LoRaWAN->setPublicNetwork(false);

#if (OVER_THE_AIR_ACTIVATION == 0)
  printf("ABP!\n");
  LoRaWAN->setABP(NwkSKey, AppSKey, DevAddr);
  LoRaWAN->setNetworkJoined(true);

  timerSend.onFired(taskPeriodicSend, NULL);
  timerSend.startPeriodic(10000);
#else
  printf("Trying to join\n");
  LoRaWAN->setNetworkJoined(true);  /* true: RealAppKey join, false: PseudoAppKey join */
  LoRaWAN->beginJoining(devEui, appEui, appKey);
#endif
}

#include <cox.h>
#include "LoRaMacKR920.hpp"

LoRaMacKR920 *LoRaWAN;
Timer timerSend;

#define OVER_THE_AIR_ACTIVATION 1

#if (OVER_THE_AIR_ACTIVATION == 1)
static const uint8_t devEui[] = "\x14\x0C\x5B\xFF\xFF\x00\x05\xA7";
static const uint8_t appEui[] = "\x01\x00\x00\x00\x00\x00\x00\x00";
static const uint8_t appKey[] = "\x0b\xf2\x80\x34\xed\xcb\x14\xe0\x9e\x1f\x94\xea\x73\xe8\xef\x0e";
#else

static const uint8_t NwkSKey[] = "\xa4\x88\x55\xad\xe9\xf8\xf4\x6f\xa0\x94\xb1\x98\x36\xc3\xc0\x86";
static const uint8_t AppSKey[] = "\x7a\x56\x2a\x75\xd7\xa3\xbd\x89\xa3\xde\x53\xe1\xcf\x7f\x1c\xc7";
static uint32_t DevAddr = 0x06e632e8;
#endif //OVER_THE_AIR_ACTIVATION

//! [How to send]
static void taskPeriodicSend(void *) {
  LoRaMacFrame *f = new LoRaMacFrame(255);
  if (!f) {
    printf("* Out of memory\n");
    return NULL;
  }

  f->port = 1;
  f->type = LoRaMacFrame::CONFIRMED;
  strcpy((char *) f->buf, "Test");
  f->len = strlen((char *) f->buf);

  /* Uncomment below lines to specify parameters manually. */
  // f->freq = 922500000;
  // f->modulation = Radio::MOD_LORA;
  // f->meta.LoRa.bw = Radio::BW_125kHz;
  // f->meta.LoRa.sf = Radio::SF7;
  // f->power = 10;
  // f->numTrials = 5;

  error_t err = LoRaWAN->send(f);
  printf("* Sending periodic report (%p:%s (%u byte)): %d\n", f, f->buf, f->len, err);
  if (err != ERROR_SUCCESS) {
    delete f;
    timerSend.startOneShot(10000);
  }
}
//! [How to send]

//! [How to use onJoin callback for SKT]
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
    if (joinedNwkSKey && joinedAppSKey) {
      // Status is OK, node has joined the network
      printf("* Joined to the network!\n");
      postTask(taskPeriodicSend, NULL);
    } else {
      printf("* PseudoAppKey joining done!\n");
    }
  } else {
    printf("* Join failed. Retry to join\n");
    LoRaWAN->beginJoining(devEui, appEui, appKey);
  }
#endif
}
//! [How to use onJoin callback for SKT]

//! [How to use onSendDone callback]
static void eventLoRaWANSendDone(LoRaMac &, LoRaMacFrame *frame, error_t result) {
  printf("* Send done(%d): [%p] destined for port[%u], Freq:%lu Hz, Power:%d dBm, # of Tx:%u, ", result, frame, frame->port, frame->freq, frame->power, frame->numTrials);
  if (frame->modulation == Radio::MOD_LORA) {
    const char *strBW[] = { "Unknown", "125kHz", "250kHz", "500kHz", "Unexpected value" };
    if (frame->meta.LoRa.bw > 3) {
      frame->meta.LoRa.bw = (Radio::LoRaBW_t) 4;
    }
    printf("LoRa, SF:%u, BW:%s, ", frame->meta.LoRa.sf, strBW[frame->meta.LoRa.bw]);
  } else if (frame->modulation == Radio::MOD_FSK) {
    printf("FSK, ");
  } else {
    printf("Unkndown modulation, ");
  }
  if (frame->type == LoRaMacFrame::UNCONFIRMED) {
    printf("UNCONFIRMED");
  } else if (frame->type == LoRaMacFrame::CONFIRMED) {
    printf("CONFIRMED");
  } else if (frame->type == LoRaMacFrame::MULTICAST) {
    printf("MULTICAST (error)");
  } else if (frame->type == LoRaMacFrame::PROPRIETARY) {
    printf("PROPRIETARY");
  } else {
    printf("unknown type");
  }
  printf(" frame\n");
  delete frame;

  timerSend.startOneShot(10000);
}
//! [How to use onSendDone callback]

//! [How to use onReceive callback]
static void eventLoRaWANReceive(LoRaMac &, const LoRaMacFrame *frame) {
  printf("* Received: destined for port[%u], Freq:%lu Hz, RSSI:%d dB", frame->port, frame->freq, frame->power);
  if (frame->modulation == Radio::MOD_LORA) {
    const char *strBW[] = { "Unknown", "125kHz", "250kHz", "500kHz", "Unexpected value" };
    printf(", LoRa, SF:%u, BW:%s", frame->meta.LoRa.sf, strBW[min(frame->meta.LoRa.bw, 4)]);
  } else if (frame->modulation == Radio::MOD_FSK) {
    printf(", FSK");
  } else {
    printf("Unkndown modulation");
  }
  if (frame->type == LoRaMacFrame::UNCONFIRMED) {
    printf(", Type:UNCONFIRMED,");
  } else if (frame->type == LoRaMacFrame::CONFIRMED) {
    printf(", Type:CONFIRMED,");
  } else if (frame->type == LoRaMacFrame::MULTICAST) {
    printf(", Type:MULTICAST,");
  } else if (frame->type == LoRaMacFrame::PROPRIETARY) {
    printf(", Type:PROPRIETARY,");
  } else {
    printf(", unknown type,");
  }

  for (uint8_t i = 0; i < frame->len; i++) {
    printf(" %02X", frame->buf[i]);
  }
  printf("\n");
}
//! [How to use onReceive callback]

//! [How to use onJoinRequested callback]
static void eventLoRaWANJoinRequested(LoRaMac &, uint32_t frequencyHz, const LoRaMac::DatarateParams_t &dr) {
  printf("* JoinRequested(Frequency: %lu Hz, Modulation: ", frequencyHz);
  if (dr.mod == Radio::MOD_FSK) {
    printf("FSK\n");
  } else if (dr.mod == Radio::MOD_LORA) {
    const char *strLoRaBW[] = { "UNKNOWN", "125kHz", "250kHz", "500kHz" };
    printf("LoRa, SF:%u, BW:%s\n", dr.param.LoRa.sf, strLoRaBW[dr.param.LoRa.bw]);
  }
}
//! [How to use onJoinRequested callback]

void setup() {
  Serial.begin(115200);
  Serial.printf("\n*** [Nol.Board] LoRaWAN Class A Example ***\n");

  LoRaWAN = new LoRaMacKR920();

  LoRaWAN->begin(SX1276);

  //! [How to set onSendDone callback]
  LoRaWAN->onSendDone(eventLoRaWANSendDone);
  //! [How to set onSendDone callback]

  //! [How to set onReceive callback]
  LoRaWAN->onReceive(eventLoRaWANReceive);
  //! [How to set onReceive callback]

  LoRaWAN->onJoin(eventLoRaWANJoin);

  //! [How to set onJoinRequested callback]
  LoRaWAN->onJoinRequested(eventLoRaWANJoinRequested);
  //! [How to set onJoinRequested callback]

  LoRaWAN->setPublicNetwork(false);

#if (OVER_THE_AIR_ACTIVATION == 0)
  printf("ABP!\n");
  LoRaWAN->setABP(NwkSKey, AppSKey, DevAddr);
  LoRaWAN->setNetworkJoined(true);

  postTask(taskPeriodicSend, NULL);
#else
  printf("Trying to join\n");

#if 0
  //! [SKT PseudoAppKey joining]
  LoRaWAN->setNetworkJoined(false);
  LoRaWAN->beginJoining(devEui, appEui, appKey);
  //! [SKT PseudoAppKey joining]
#else
  //! [SKT RealAppKey joining]
  LoRaWAN->setNetworkJoined(true);
  LoRaWAN->beginJoining(devEui, appEui, appKey);
  //! [SKT RealAppKey joining]
#endif
#endif
}

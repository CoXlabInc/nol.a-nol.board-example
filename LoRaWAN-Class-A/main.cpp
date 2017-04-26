#include <cox.h>
#include "LoRaMacKR920.hpp"

LoRaMacKR920 *LoRaWAN;
Timer timerSend;

#define OVER_THE_AIR_ACTIVATION 0

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
  f->type = LoRaMacFrame::UNCONFIRMED;
  strcpy((char *) f->buf, "Test");
  f->len = strlen((char *) f->buf);

  /* Uncomment below lines to specify parameters manually. */
  // f->freq = 922500000;
  // f->modulation = Radio::MOD_LORA;
  // f->meta.LoRa.bw = Radio::BW_125kHz;
  // f->meta.LoRa.sf = Radio::SF7;
  // f->power = 10;
  f->numTrials = 5;

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

//! [eventLoRaWANLinkADRReqReceived]
static void eventLoRaWANLinkADRReqReceived(LoRaMac &l, const uint8_t *payload) {
  printf("* LoRaWAN LinkADRReq received: [");
  for (uint8_t i = 0; i < 4; i++) {
    printf(" %02X", payload[i]);
  }
  printf(" ]\n");
}
//! [eventLoRaWANLinkADRReqReceived]

//! [eventLoRaWANLinkADRAnsSent]
static void printChannelInformation(LoRaMac &lw) {
  //! [getChannel]
  for (uint8_t i = 0; i < lw.MaxNumChannels; i++) {
    const LoRaMac::ChannelParams_t *p = lw.getChannel(i);
    if (p) {
      printf(" - [%u] Frequency:%lu Hz\n", i, p->Frequency);
    } else {
      printf(" - [%u] disabled\n", i);
    }
  }
  //! [getChannel]

  //! [getDatarate]
  const LoRaMac::DatarateParams_t *dr = lw.getDatarate(lw.getDefDatarate());
  printf(" - Default DR%u:", lw.getDefDatarate());
  if (dr->mod == Radio::MOD_LORA) {
    printf("LoRa SF%u ", dr->param.LoRa.sf);
    const char *strBW[] = { "Unknown", "125kHz", "250kHz", "500kHz", "Unexpected value" };
    printf("LoRa, SF%u BW:%s", dr->param.LoRa.sf, strBW[min(dr->param.LoRa.bw, 4)]);
  } else if (dr->mod == Radio::MOD_FSK) {
    printf("FSK\n");
  } else {
    printf("Unknown modulation\n");
  }
  //! [getDatarate]

  printf(" - # of repetitions of unconfirmed uplink frames: %u\n", lw.getNumRepetitions());
}

static void eventLoRaWANLinkADRAnsSent(LoRaMac &l, uint8_t status) {
  printf("* LoRaWAN LinkADRAns sent with status 0x%02X.\n", status);
  printChannelInformation(l);
}
//! [eventLoRaWANLinkADRAnsSent]

//! [eventLoRaWANDutyCycleReqReceived]
static void eventLoRaWANDutyCycleReqReceived(LoRaMac &lw, const uint8_t *payload) {
  printf("* LoRaWAN DutyCycleReq received: [");
  for (uint8_t i = 0; i < 1; i++) {
    printf(" %02X", payload[i]);
  }
  printf(" ]\n");
}
//! [eventLoRaWANDutyCycleReqReceived]

//! [eventLoRaWANDutyCycleAnsSent]
static void eventLoRaWANDutyCycleAnsSent(LoRaMac &lw) {
  printf("* LoRaWAN DutyCycleAns sent. Current MaxDCycle is %u.\n", lw.getMaxDutyCycle());
}
//! [eventLoRaWANDutyCycleAnsSent]

//! [eventLoRaWANRxParamSetupReqReceived]
static void eventLoRaWANRxParamSetupReqReceived(LoRaMac &lw, const uint8_t *payload) {
  printf("* LoRaWAN RxParamSetupReq received: [");
  for (uint8_t i = 0; i < 4; i++) {
    printf(" %02X", payload[i]);
  }
  printf(" ]\n");
}
//! [eventLoRaWANRxParamSetupReqReceived]

//! [eventLoRaWANRxParamSetupAnsSent]
static void eventLoRaWANRxParamSetupAnsSent(LoRaMac &lw, uint8_t status) {
  printf("* LoRaWAN RxParamSetupAns sent with status 0x%02X. Current Rx1Offset is %u, and Rx2 channel is (DR%u, %lu Hz).\n", status, lw.getRx1DrOffset(), lw.getRx2Datarate(), lw.getRx2Frequency());
}
//! [eventLoRaWANRxParamSetupAnsSent]

static void eventLoRaWANDevStatusReqReceived(LoRaMac &lw) {
  printf("* LoRaWAN DevStatusReq received.\n");
}

//! [eventLoRaWANDevStatusAnsSent]
static void eventLoRaWANDevStatusAnsSent(LoRaMac &lw, uint8_t bat, uint8_t margin) {
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
//! [eventLoRaWANDevStatusAnsSent]

//! [eventLoRaWANNewChannelReqReceived]
static void eventLoRaWANNewChannelReqReceived(LoRaMac &lw, const uint8_t *payload) {
  printf("* LoRaWAN NewChannelReq received [");
  for (uint8_t i = 0; i < 5; i++) {
    printf(" %02X", payload[i]);
  }
  printf(" ]\n");
}
//! [eventLoRaWANNewChannelReqReceived]

//! [eventLoRaWANNewChannelAnsSent]
static void eventLoRaWANNewChannelAnsSent(LoRaMac &lw, uint8_t status) {
  printf("* LoRaWAN NewChannelAns sent with (Datarate range %s and channel frequency %s).\n", (bitRead(status, 1) == 1) ? "OK" : "NOT OK", (bitRead(status, 0) == 1) ? "OK" : "NOT OK");

  for (uint8_t i = 0; i < lw.MaxNumChannels; i++) {
    const LoRaMac::ChannelParams_t *p = lw.getChannel(i);
    if (p) {
      printf(" - [%u] Frequency:%lu Hz\n", i, p->Frequency);
    } else {
      printf(" - [%u] disabled\n", i);
    }
  }
}
//! [eventLoRaWANNewChannelAnsSent]

//! [eventLoRaWANRxTimingSetupReqReceived]
static void eventLoRaWANRxTimingSetupReqReceived(LoRaMac &lw, const uint8_t *payload) {
  printf("* LoRaWAN RxTimingSetupReq received:  [");
  for (uint8_t i = 0; i < 1; i++) {
    printf(" %02X", payload[i]);
  }
  printf(" ]\n");
}
//! [eventLoRaWANRxTimingSetupReqReceived]

//! [eventLoRaWANRxTimingSetupAnsSent]
static void eventLoRaWANRxTimingSetupAnsSent(LoRaMac &lw) {
  printf("* LoRaWAN RxTimingSetupAns sent. Current Rx1 delay is %u msec, and Rx2 delay is %u msec.\n", lw.getRx1Delay(), lw.getRx2Delay());
}
//! [eventLoRaWANRxTimingSetupAnsSent]

void setup() {
  Serial.begin(115200);
  Serial.printf("\n*** [Nol.Board] LoRaWAN Class A Example ***\n");

  timerSend.onFired(taskPeriodicSend, NULL);

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

  LoRaWAN->onLinkADRReqReceived(eventLoRaWANLinkADRReqReceived);
  LoRaWAN->onLinkADRAnsSent(eventLoRaWANLinkADRAnsSent);
  LoRaWAN->onDutyCycleReqReceived(eventLoRaWANDutyCycleReqReceived);
  LoRaWAN->onDutyCycleAnsSent(eventLoRaWANDutyCycleAnsSent);
  LoRaWAN->onRxParamSetupReqReceived(eventLoRaWANRxParamSetupReqReceived);
  LoRaWAN->onRxParamSetupAnsSent(eventLoRaWANRxParamSetupAnsSent);
  LoRaWAN->onDevStatusReqReceived(eventLoRaWANDevStatusReqReceived);
  LoRaWAN->onDevStatusAnsSent(eventLoRaWANDevStatusAnsSent);
  LoRaWAN->onNewChannelReqReceived(eventLoRaWANNewChannelReqReceived);
  LoRaWAN->onNewChannelAnsSent(eventLoRaWANNewChannelAnsSent);
  LoRaWAN->onRxTimingSetupReqReceived(eventLoRaWANRxTimingSetupReqReceived);
  LoRaWAN->onRxTimingSetupAnsSent(eventLoRaWANRxTimingSetupAnsSent);

  LoRaWAN->setPublicNetwork(false);

  printChannelInformation(*LoRaWAN);

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

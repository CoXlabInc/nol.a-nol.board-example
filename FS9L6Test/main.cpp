#include <cox.h>
#include "FS9L6.hpp"

Timer timerHello;
FS9L6 seq;

static void taskHello(void *) {
  timerHello.stop();

  seq.Maintain();

}

void setup() {
  Serial.begin(115200);
  seq.begin();
  timerHello.onFired(taskHello, NULL);
  timerHello.startPeriodic(1000);

}

#include <cox.h>

Timer t;
float in = 0;

static void taskAnalogWrite(void *) {
  float out = sin(in) * 127.5 + 127.5;

  printf("* Duty cycle: %u.%02u\n", (uint8_t) out, (uint8_t) (out * 100) % 100);
  analogWrite(D2, (uint8_t) round(out));

  in += 0.01;
  if (in >= 6.283)
    in = 0;
}

void setup() {
  Serial.begin(115200);
  printf("\n[Nol.Board] PWM Test\n");

  t.onFired(taskAnalogWrite, NULL);
  t.startPeriodic(10);

  pinMode(D2, OUTPUT);
}

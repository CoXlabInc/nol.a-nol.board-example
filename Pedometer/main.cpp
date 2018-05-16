#include <cox.h>
#include <MMA8452Q.hpp>

MMA8452Q gyro;
Timer timerPrintXYZ, timerLEDOff;

static void taskPrintXYZ(void *) {
  int16_t x, y, z;
  if (gyro.isActive()) {
    gyro.readXYZ(&x, &y, &z);
    printf("* X:%5d, Y:%5d, Z:%5d\n", x, y, z);
  }
}

static void eventTransientDetected(MMA8452Q &) {
  digitalWrite(D3, LOW);
  timerLEDOff.onFired([](void *) {
    digitalWrite(D3, HIGH);
  }, NULL);
  timerLEDOff.startOneShot(100);

  printf("* Transient detected.\n");
}

void setup() {
  Serial.begin(115200);
  printf("\n*** [Nol.Board] Pedometer Test ***\n");

  Wire.begin();

  pinMode(D2, INPUT);
  gyro.begin(Wire, 0x1D, D2);
  gyro.setStandby();
  gyro.setODR(MMA8452Q::ODR_12p5Hz);
  gyro.setMode(MMA8452Q::MODE_LOW_NOISE_LOW_POWER);
  gyro.onDetectTransient(eventTransientDetected);
  gyro.setTransientDetection(true, true, true, 500, 100000);
  gyro.setActive();
  printf("* MMA8452Q setup done.\n");

  timerPrintXYZ.onFired(taskPrintXYZ, NULL);
  timerPrintXYZ.startPeriodic(100);

  pinMode(D3, OUTPUT);
  digitalWrite(D3, HIGH);
}

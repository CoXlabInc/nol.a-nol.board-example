// -*- mode:c++; indent-tabs-mode:nil; -*-

#include <cox.h>

Timer ledTimer;
Timer printTimer;
Timer pulseTimer;

static void ledOffTask(void *args);

static void ledOnTask(void *args) {
  ledTimer.onFired(ledOffTask, NULL);
  ledTimer.startOneShot(10);
  digitalWrite(D2, HIGH);
  digitalToggle(D3);
}

static void ledOffTask(void *args) {
  ledTimer.onFired(ledOnTask, NULL);
  ledTimer.startOneShot(990);
  digitalWrite(D2, LOW);
}

static void togglePulse(void *args) {
  digitalToggle(D1);
}

//![weekday strings]
static const char *weekday[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
//![weekday strings]

static void printTask(void *args) {
  printf("[%lu usec] Timer works!\n", micros());

  //![How to use getDateTime]
  struct tm t;
  System.getDateTime(t);

  struct timeval now;
  gettimeofday(&now, NULL);
  printf(
    "Now: %u-%u-%u %s %02u:%02u:%02u (%ld.%06ld)\n",
    t.tm_year + 1900,
    t.tm_mon + 1,
    t.tm_mday,
    weekday[t.tm_wday],
    t.tm_hour,
    t.tm_min,
    t.tm_sec,
    now.tv_sec,
    now.tv_usec
  );
  //![How to use getDateTime]
  printf("* Supply voltage: %ld mV\n", System.getSupplyVoltage());
  printf("* Random number:%lu\n", random());

  Serial2.printf("[%lu usec] Timer works!\n", micros());
  Serial2.write("012345\r\n");
}

static void eventDateTimeAlarm() {
  struct tm t;
  System.getDateTime(t);
  printf(
    "* Alarm! Now: %u-%u-%u %s %02u:%02u:%02u\n",
    t.tm_year + 1900,
    t.tm_mon + 1,
    t.tm_mday,
    weekday[t.tm_wday],
    t.tm_hour,
    t.tm_min,
    t.tm_sec
  );
}

static void eventSerialRx(SerialPort &p) {
  while (p.available() > 0) {
    char c = p.read();
    if (c == 'q') {
      reboot();
    } else {
      p.write(c);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.onReceive(eventSerialRx);
  Serial.listen();

  Serial2.begin(115200);
  // Serial2.onReceive(eventSerialRx);
  // Serial2.listen();
  Serial.printf("\n*** [Nol.Board] Basic Functions ***\n");
  Serial.printf("* Nol.A-SDK Version: %u.%u.%u\n", NOLA_VER_MAJOR, NOLA_VER_MINOR, NOLA_VER_PATCH);

  uint8_t eui[8];
  System.getEUI(eui);
  Serial.printf(
    "* EUI-64: %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n",
    eui[0], eui[1], eui[2], eui[3], eui[4], eui[5], eui[6], eui[7]
  );

  //![How to use setTimeDiff]
  System.setTimeDiff(9 * 60); //KST: +9*60 minutes
  //![How to use setTimeDiff]

  struct tm t;
  t.tm_year = 2017 - 1900;  // 2017
  t.tm_mon = 8 - 1;         // August
  t.tm_mday = 22;
  t.tm_hour = 17;
  t.tm_min = 29;
  t.tm_sec = 50;

  System.setDateTime(t);

  System.onDateTimeAlarm(eventDateTimeAlarm);

  System.setTimeAlarm(17, 30);

  ledTimer.onFired(ledOffTask, NULL);
  ledTimer.startOneShot(1000);

  printTimer.onFired(printTask, NULL);
  printTimer.startPeriodic(1000);

  pinMode(D2, OUTPUT);
  digitalWrite(D2, HIGH);
  pinMode(D3, OUTPUT);
  digitalWrite(D3, HIGH);
}

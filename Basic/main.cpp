// -*- mode:c++; indent-tabs-mode:nil; -*-

#include <cox.h>

Timer ledTimer;
Timer printTimer;

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

static const char *weekday[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
static void printTask(void *args) {
  printf("[%lu usec] Timer works!\n", micros());

  uint16_t year;
  uint8_t month, day, hour, minute, second;
  RTCCalendar::WeekDay_t dayOfWeek;
  System.getDateTime(&year, &month, &day, &hour, &minute, &second, &dayOfWeek);
  printf("Now: %u-%u-%u %s %02u:%02u:%02u\n", year, month, day, weekday[dayOfWeek], hour, minute, second);
  printf("Supply voltage: %ld mV\n", System.getSupplyVoltage());

  Serial2.printf("[%lu usec] Timer works!\n", micros());
}

static void eventDateTimeAlarm() {
  uint16_t year;
  uint8_t month, day, hour, minute, second;
  RTCCalendar::WeekDay_t dayOfWeek;
  System.getDateTime(&year, &month, &day, &hour, &minute, &second, &dayOfWeek);
  printf("* Alarm! Now: %u-%u-%u %s %02u:%02u:%02u\n", year, month, day, weekday[dayOfWeek], hour, minute, second);
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

  Serial2.begin(57600);
  Serial2.onReceive(eventSerialRx);
  Serial2.listen();

  printf("\n*** [Nol.Board] Basic Functions ***\n");
  System.setDateTime(2016, 8, 22, 10, 0, 0, RTCCalendar::MONDAY);

  System.onDateTimeAlarm(eventDateTimeAlarm);

  // Set alarm to be occurred every hour when the minute value is changed to 1.
  System.setDateTimeAlarm(0xFFFF,                     // not supported
                          0xFF,                       // not supported
                          0xFF,                       // don't care
                          0xFF,                       // don't care
                          1,                          // care
                          0xFF,                       // not supported
                          RTCCalendar::UNKNOWNDAY);   // don't care

  ledTimer.onFired(ledOffTask, NULL);
  ledTimer.startOneShot(1000);

  printTimer.onFired(printTask, NULL);
  printTimer.startPeriodic(1000);

  pinMode(D2, OUTPUT);
  digitalWrite(D2, HIGH);
  pinMode(D3, OUTPUT);
  digitalWrite(D3, HIGH);
}

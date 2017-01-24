#include <cox.h>
#include <RHT03.hpp>

RHT03 rht;
Timer timerMeasure;

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
    char buf[33];
    printf("* Results\n");
    printf("- Humidity: %s %%\n", gcvtf(latestHumidity, 4, buf));
    printf("- Temp (F): %s deg F\n", gcvtf(latestTempF, 4, buf));
    printf("- Temp (C): %s deg C\n", gcvtf(latestTempC, 4, buf));
  } else {
    printf("* updateRet:%d\n", updateRet);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.printf("\n*** [Nol.Board] RHT03 Temperature & Humidity Sensor Test ***\n");

  rht.begin(D2);

  timerMeasure.onFired(taskMeasure, NULL);
  timerMeasure.startPeriodic(1000);
}

#include <cox.h>
#include <PMS3003.hpp>

PMS3003 pms3003 = PMS3003(Serial2, D2, D3);

static void eventSensorMeasuredDone(int32_t pm1_0_CF1,
                                    int32_t pm2_5_CF1,
                                    int32_t pm10_0_CF1,
                                    int32_t pm1_0_Atmosphere,
                                    int32_t pm2_5_Atmosphere,
                                    int32_t pm10_0_Atmosphere) {
  printf("* (CF=1) PM1.0:%ld, PM2.5:%ld, PM10:%ld (unit: ug/m^3)\n", pm1_0_CF1, pm2_5_CF1, pm10_0_CF1);
  printf("* (Atmosphere) PM1.0:%ld, PM2.5:%ld, PM10:%ld (unit: ug/m^3)\n", pm1_0_Atmosphere, pm2_5_Atmosphere, pm10_0_Atmosphere);
}

void setup() {
  Serial.begin(115200);
  printf("\n*** [Nol.Board] PMS3003 Particle sensor test ***\n");
  pms3003.begin();
  pms3003.onReadDone(eventSensorMeasuredDone);
}

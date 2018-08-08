#include <cox.h>

#define dataNum 100

class FS9L6 {
public:

  FS9L6();
  void begin();
  void Maintain();


private:

  float DataValue = 0;
  float VoltageValue = 0;
  float StandardVoltage = 0;
  float currentData = 0;
  float maxData = 0;
  float StandardMaxCurrent = 0;
  float StandardMinCurrent = 0;
  float StandardCurrent = 0;
  float voltageRMS = 0.0;
  float voltageP = 0.0;
  float Current = 0;
  float tolerance = 0.2;
  void CurrentChange();
  static void eventKeyInput(SerialPort &);
};

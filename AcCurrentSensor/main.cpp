#include <cox.h>
#include "AcCurrentSensor.hpp"

Timer timerHello;
AcCurrentSensor seq(14, 2.5, 4095.0, 330, 600);
//AcCurrentSensor name(DigitalPinNumInput, ADC_AmaxInput, ADC_DmaxInput, ResistorInput, TransformerWindingInput);

  static void taskHello(void *) {
    timerHello.stop();

    while(1){
      seq.Sense(); // Measure and Caculate AC
      int VrmsDataH = seq.voltageRMS;
      int VrmsDataL = seq.voltageRMS * 1000;
      int CurrentDataH = seq.Current;
      int CurrentDataL = seq.Current * 1000;

      printf("Current : %d.%d, Vrms : %d.%d\n",VrmsDataH,VrmsDataL,CurrentDataH,CurrentDataL);
    }

  }

void setup() {
  Serial.begin(115200);
  Serial.printf("\n*** [Nol.Board] AC Current Sensor Example ***\n");
  timerHello.onFired(taskHello, NULL);
  timerHello.startPeriodic(1000);
}

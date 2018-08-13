#include "FS9L6.hpp"
#include "cox.h"

FS9L6::FS9L6(){
  printf("*********** FS9L6 ************\n");
}

// void FS9L6::begin(){
//   this->VoltageValue = 0;
//
//   printf("Find Standard Current...\n");
//   for (int i=0 ; i<3000; i++) {
//     this->DataValue = analogRead(A0);
//     if (this->DataValue > this->VoltageValue) this->VoltageValue = this->DataValue;
//   }
//
//   this->StandardVoltage = this->VoltageValue / 1.414 / 4095.0 * 2.5; // when the voltage 2.5, it is 4095.0 on the Nol.Board and rms is Vp / sqr(2)
//   this->StandardCurrent = this->StandardVoltage / 330 * 600; // Resister is 330ohms and FS9L6s winding ratio is 600 : 1
//
//   printf("Current is ");
//   Serial.print(this->StandardCurrent, 3);
//   printf("A.\n");
//
//   // Ref voltage
//   printf("Voltage is ");
//   Serial.print(this->StandardVoltage, 3);
//   printf("Vrms.\n");
//
//   this->StandardMaxCurrent = this->StandardCurrent * (1+tolerance);
//   this->StandardMinCurrent = this->StandardCurrent * (1-tolerance); // safety margin
//   this->Current = this->StandardCurrent;
//
// }

// Save VpData
void FS9L6::VpData(){
  VpDataH = this->voltageRMS;
  VpDataL = this->voltageRMS * 1000;
}
// !Save VpData

// Save CurrentData
void FS9L6::CurrentData(){
  CurrentDataH = this->Current;
  CurrentDataL = this->Current * 1000;
}
// !Save CurrentData

// detect change standard current
// void FS9L6::CurrentChange(){
//   printf("Current is change.\n");
//   printf("Apply New StandardCurrent\n");
//   this->begin();
// }
// !detect change standard current

// Measure and Caculate AC
void FS9L6::Maintain(){
    this->VoltageValue = 0;

    for (int i=0 ; i<dataNum; i++) {
      this->DataValue = analogRead(A0);
      if (this->DataValue > this->VoltageValue) this->VoltageValue = this->DataValue;
    }
    //calculate data
    this->voltagePeak = this->VoltageValue / 4095.0 * 2.5;
    this->voltageRMS = this->VoltageValue / 1.414 / 4095.0 * 2.5;
    this->Current = this->VoltageValue / 1.414 / 4095.0 / 330 * 2.5 * 600;

    //show data (serial port)
    /*
    printf("Standard Current = ");
    Serial.print(this->StandardCurrent, 3);
    printf(".\n");

    Serial.print("* Vp = ");
    Serial.print(this->voltagePeak, 3);
    Serial.print(", * Vrms = ");
    Serial.print(this->voltageRMS, 3);
    Serial.print(", * Current = ");
    Serial.print(this->Current, 3);
    printf("\n");
    */
    //!show data (serial port)

    this->VpData();
    this->CurrentData();

    // if(this->Current > this->StandardMaxCurrent || this->StandardMinCurrent > this->Current ){
    //   this->CurrentChange();
    // }

}
// !Measure and Caculate AC

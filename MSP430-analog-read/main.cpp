#include <cox.h>
#include <msp430.h>

void setup() {
  Serial.begin(115200);

  ADC12CTL0 = 0;
  ADC12CTL1 = 0;
  ADC12CTL0 |= ADC12SHT0_0;
  ADC12CTL1 |= ADC12SSEL_1;
  ADC12CTL1 |= ADC12SHP;

  P6SEL |= (1 << 4);
  P6DIR &= ~(1 << 4);
  P6REN &= ~(1 << 4);

  ADC12CTL0 |= ADC12ON;
  ADC12MCTL0 = (4 << 0);

  REFCTL0 = (REFMSTR | REFVSEL_2 | REFON);
  ADC12MCTL0 |= ADC12SREF_1;

  ADC12IFG &= ~ADC12IFG0;
  ADC12CTL0 |= (ADC12SC | ADC12ENC);

  while ((ADC12IFG & ADC12IFG0) == 0);

  ADC12CTL0 &= ~ADC12ENC;
  ADC12IFG &= ~ADC12IFG0;

  uint16_t val = ADC12MEM0;
  ADC12CTL0 &= ~ADC12ON;
  REFCTL0 &= ~REFON;

  printf("Voltage value of A0: %ld mV\n", map(val, 0, 4095, 0, 2500));
}

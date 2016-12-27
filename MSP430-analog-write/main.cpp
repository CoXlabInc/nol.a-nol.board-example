
#include <cox.h>
#include <msp430.h>

void setup() {
  P2SEL |= (1 << 3);
  P2DIR |= (1 << 3);
  P2REN &= ~(1 << 3);

  TA1CTL = (MC__STOP);
  TA1CCR0 = 0xFF;
  TA1CCR2 = 0xFF - 10;
  TA1CCTL2 = OUTMOD_3;
  TA1CTL = TASSEL__ACLK | MC__UP;
}

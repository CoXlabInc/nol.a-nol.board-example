#include <cox.h>
#include <msp430.h>

void setup() {
  P2SEL &= ~(1 << 3);
  P2DIR |= (1 << 3);
  P2REN &= ~(1 << 3);

  P2OUT &= ~(1 << 3);
}

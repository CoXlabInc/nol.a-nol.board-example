#include <cox.h>
#include <msp430.h>

void setup() {
  Serial.begin(115200);

  P2SEL &= ~(1 << 3);
  P2DIR &= ~(1 << 3);
  P2REN &= ~(1 << 3);

  int x = (P2IN >> 3) & 1;

  printf("State of D2: %d\n", x);
}

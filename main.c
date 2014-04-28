#include "msp430g2553.h"
#include "MotorOutput.h"
/*
 * main.c
 */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    forward(0.3);
	
	return 0;
}

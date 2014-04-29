#include "msp430g2553.h"
#include "MotorOutput.h"
/*
 * main.c
 */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    BCSCTL1 = CALBC1_8MHZ; // 8Mhz calibration for clock
    DCOCTL = CALDCO_8MHZ;

    init_motors();
	
    _bis_SR_register(GIE+LPM0_bits); //enable interrupts (not used) and stop CPU
}

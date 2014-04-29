#include "msp430g2553.h"
#include "MotorOutput.h"
/*
 * main.c
 */

void init_wdt();

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
<<<<<<< HEAD
    BCSCTL1 = CALBC1_8MHZ;    // 1Mhz calibration for clock
    DCOCTL  = CALDCO_8MHZ;

    init_sensors();
    init_wdt();

    _bis_SR_register(GIE+LPM0_bits);	//enable general interrupts and power down CPU
=======

    forward(0.3);
    straight();
	
	return 0;
>>>>>>> 8e9fda374d565dd2dc722fdef250a865e2bafca5
}

//Initializes WDT
void init_wdt(){ // setup WDT
	  WDTCTL =(WDTPW + // (bits 15-8) password
	                   // bit 7=0 => watchdog timer on
	                   // bit 6=0 => NMI on rising edge (not used here)
	                   // bit 5=0 => RST/NMI pin does a reset (not used here)
	           WDTTMSEL + // (bit 4) select interval timer mode
	           WDTCNTCL +  // (bit 3) clear watchdog timer counter
	  		          0 // bit 2=0 => SMCLK is the source
	  		          +1 // bits 1-0 = 01 => source/8K, gives an interrupt every ~1ms
	  		   );
	  IE1 |= WDTIE;		// enable the WDT interrupt (in the system interrupt register IE1)
}

interrupt void WDT_interval_handler(){
	ADC10CTL0 |= ADC10SC;  // trigger a conversion
}

ISR_VECTOR(WDT_interval_handler, ".int10")

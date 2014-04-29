#include "msp430g2553.h"
#include "MotorOutput.h"
#include "SensorCollect.h"
/*
 * main.c
 */

#define SENSOR_LOOPS 50
int sensor_conversions = SENSOR_LOOPS;

void init_wdt();

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    BCSCTL1 = CALBC1_8MHZ;    // 8Mhz calibration for clock
    DCOCTL  = CALDCO_8MHZ;

    init_sensors();
    init_wdt();
    init_motors();

    _bis_SR_register(GIE+LPM0_bits);	//enable general interrupts and power down CPU
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

//called every 1 ms
interrupt void WDT_interval_handler(){
	//code here
	sensor_conversions--;
	if (sensor_conversions == 0) {
		sensor_conversions = SENSOR_LOOPS;
		//ADC10CTL0 |= ADC10SC;  // trigger a conversion
		make_front_measurement();
	}
}

ISR_VECTOR(WDT_interval_handler, ".int10")

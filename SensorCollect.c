#include "msp430g2553.h"

#define ULTRASONIC_LEFT 0x8 //P1.3
#define ULTRASONIC_RIGHT 0x10 //P1.4
#define ADC_INCH_LEFT INCH_3
#define ADC_INCH_RIGHT INCH_4

volatile int latest_left;

void init_sensors(void);	// routine to setup the sensors
void init_sensor_adc(void);	// routine to setup ADC


void interrupt adc_handler(){
	 latest_left=ADC10MEM;   // store the ADC result
}
ISR_VECTOR(adc_handler, ".int05")

void init_sensors() {
	 init_sensor_adc();
}

 //Initialize the ADC
void init_sensor_adc(){
  ADC10CTL1= ADC_INCH_LEFT	//input channel 4
 			  +SHS_0 //use ADC10SC bit to trigger sampling
 			  +ADC10DIV_4 // ADC10 clock/5
 			  +ADC10SSEL_0 // Clock Source=ADC10OSC
 			  +CONSEQ_0; // single channel, single conversion
 			  ;
  ADC10AE0=ULTRASONIC_LEFT; // enable A4 analog input
  ADC10CTL0= SREF_0	//reference voltages are Vss and Vcc
 	          +ADC10SHT_3 //64 ADC10 Clocks for sample and hold time (slowest)
 	          +ADC10ON	//turn on ADC10
 	          +ENC		//enable (but not yet start) conversions
 	          +ADC10IE  //enable interrupts
 	          ;
}


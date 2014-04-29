#include "msp430g2553.h"

#define ULTRASONIC_LEFT 0x8 //P1.3
#define ULTRASONIC_RIGHT 0x1 //P1.0
#define ADC_INCH_LEFT INCH_3
#define ADC_INCH_RIGHT INCH_0
#define ULTRASONIC_FRONT 0x1

unsigned int ADC[4];  // Array to hold ADC values
volatile int latest_left;
volatile int latest_right;

void init_sensors(void);	// routine to setup the sensors
void init_sensor_adc(void);	// routine to setup ADC
void init_ultrasonic_timer(void);

int front_state = 0;
int front_initial = 0;
int front_final = 0;
int front_dist = 0;
void interrupt adc_handler(){
	latest_right = ADC[3];  // Notice the reverse in index
	latest_left = ADC[0];

	ADC10CTL0 &= ~ADC10IFG;  // clear interrupt flag

	ADC10SA = (short)&ADC[0]; // ADC10 data transfer starting address.
}
ISR_VECTOR(adc_handler, ".int05")

int get_latest_left() {
	//TODO make these return cm instead of raw value from 0-1024
	return latest_left;
}

int get_latest_right() {
	//TODO make these return cm instead of raw value from 0-1024
	return latest_right;
}

void init_sensors() {
	 init_sensor_adc();
	 init_ultrasonic_timer();
}

 //Initialize the ADC
void init_sensor_adc(){
	P1SEL &= ~(ULTRASONIC_LEFT & ULTRASONIC_RIGHT);
	P1SEL2 &= ~(ULTRASONIC_LEFT & ULTRASONIC_RIGHT);
	P1REN = 0x0;  // No resistors enabled for Port 1
	P1DIR &= ~(ULTRASONIC_LEFT & ULTRASONIC_RIGHT);

	ADC10CTL1= ADC_INCH_LEFT	//input channel 4
 			  +SHS_0 //use ADC10SC bit to trigger sampling
 			  +ADC10DIV_4 // ADC10 clock/5
 			  +ADC10SSEL_0 // Clock Source=ADC10OSC
 			  +CONSEQ_1;
 			  ;
 	ADC10AE0=ULTRASONIC_LEFT & ULTRASONIC_RIGHT; // enable A4 analog input

 	ADC10CTL0= SREF_0	//reference voltages are Vss and Vcc
 	          +ADC10SHT_3 //64 ADC10 Clocks for sample and hold time (slowest)
 	          +ADC10ON	//turn on ADC10
 	          +MSC
 	          +ENC
 	          +ADC10IE  //enable interrupts
 	          ;
	ADC10DTC1 = 4;			       // Four conversions.
	ADC10SA = (short)&ADC[0];           // ADC10 data transfer starting address.
}

void init_ultrasonic_timer() {
	TA1CTL |= TACLR;              // reset clock
	TA1CTL = TASSEL_2+ID_3+MC_2;  // clock source = SMCLK
	                            // clock divider=1
	                            // UP mode
	                            // timer A interrupt off
	TA1CCTL0 = CM_1 + SCS + CAP + OUTMOD_0 + CCIE; // compare mode, output 0, no interrupt enabled
	P2DIR &= ~ULTRASONIC_FRONT;
	P2SEL |= ULTRASONIC_FRONT;
}

void make_front_measurement() {

	TA1CCTL0 &= ~CCIE;
	P2DIR |= ULTRASONIC_FRONT;
	TA1CCTL0 &= ~OUT;
	__delay_cycles(50);
	TA1CCTL0 |= OUT; // compare mode, output 0, no interrupt enabled
	//front_initial = TA1R;
	__delay_cycles(50);
	TA1CCTL0 &= ~OUT;
	TA1CCTL0 |= CCIE;
	P2DIR &= ~ULTRASONIC_FRONT;
}


interrupt void ultrasonic_timer_handler() {
	if (front_state == 0) {
		front_state = 1;
		TA1CCTL0 &= ~CM_1;
		TA1CCTL0 |= CM_2;
		front_initial = TA1CCR0;
	} else {
		front_state = 0;
		TA1CCTL0 &= ~CM_2;
		TA1CCTL0 |= CM_1;
		front_final = TA1CCR0;
	}
	front_dist = ((front_final - front_initial))/29/2;
	TA1CCTL0 &= ~CCIFG;

}
ISR_VECTOR(ultrasonic_timer_handler, ".int13")

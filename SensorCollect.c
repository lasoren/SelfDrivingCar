#include "msp430g2553.h"
#include "MotorOutput.h"
#include "SensorCollect.h"


//Pin definitions
#define ULTRASONIC_LEFT 0x8 //P1.3
#define ULTRASONIC_RIGHT 0x1 //P1.0
#define ADC_INCH_LEFT INCH_3
#define ADC_INCH_RIGHT INCH_0
#define ULTRASONIC_FRONT 0x1 //P2.0
#define ULTRASONIC_FRONT_TRIGGER 0x2 //P2.1

#define SCALE_FACTOR_FRONT 148 //to get half-inches

#define CAMERA_TRIGGER_PIN 0x4 //P2.2
#define CAMERA_POSEDGE_LEN 40 //in milliseconds

unsigned int ADC[4];  // Array to hold ADC values in multi-channel conversion

//The latest sensor measurements, in half-inches
volatile int latest_left = 0;
volatile int latest_right = 0;
volatile int latest_front = 20; //Init to non-zero so robot starts forward

int front_state = 0;
unsigned int front_initial = 0;
unsigned int front_final = 0;
unsigned int taie_overflow = 0;

int camera_timeout = 0; //Tracks when we can take another picture

//Single conversion, multi channel 10-bit ADC
void interrupt adc_handler(){
	latest_right = ADC[3];  // Notice the reverse in index
	latest_left = ADC[0];

	ADC10CTL0 &= ~ADC10IFG;  // clear interrupt flag

	ADC10SA = (short)&ADC[0]; // ADC10 data transfer starting address.
}
ISR_VECTOR(adc_handler, ".int05")

int get_latest_left() {
	//in half inches
	return (int) (latest_left);
}

int get_latest_right() {
	//in half inches
	return (int) (latest_right);
}

int get_latest_front() {
	//in half inches
	return latest_front;
}

//Initializes the sensors -- called by main
void init_sensors() {
	 init_sensor_adc();
	 init_ultrasonic_timer();
	 init_camera();
}

void init_camera() {
	P2DIR |= CAMERA_TRIGGER_PIN; //output trigger for camera
}

//Initialize ADC for reading left and right distances as linearly-scaled voltages
void init_sensor_adc(){
	P1SEL &= ~(ULTRASONIC_LEFT & ULTRASONIC_RIGHT); //Reset pins to generic inputs
	P1SEL2 &= ~(ULTRASONIC_LEFT & ULTRASONIC_RIGHT);
	P1REN = 0x0;  // No resistors enabled for Port 1
	P1DIR &= ~(ULTRASONIC_LEFT & ULTRASONIC_RIGHT); //Reset pins to generic inputs

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
 	          +MSC  //Multi-channel
 	          +ENC
 	          +ADC10IE  //enable interrupts
 	          ;
	ADC10DTC1 = 4;			       // Four conversions
	ADC10SA = (short)&ADC[0];           // ADC10 data transfer starting address.
}


//Ultrasonic front sensor differs fron side sensors--must measure width of pulse after trigger
void init_ultrasonic_timer() {
    //Use TA1 since TA0 is already in use by motors
	TA1CTL |= TACLR;              // reset clock
	TA1CTL = TASSEL_2+ID_3+MC_2+TAIE;  // clock source = SMCLK
	                            // clock divider=1
	                            // UP mode
	                            // timer A interrupt off
	TA1CCTL0 = CM_3 + SCS + CAP + OUTMOD_0 + CCIE; // compare mode, output 0, no interrupt enabled
	P2DIR &= ~ULTRASONIC_FRONT;
	P2SEL |= ULTRASONIC_FRONT;
}

//Initiates a reading from the front ultrasonic sensor by sending a trigger pulse
void make_front_measurement() {
	P2DIR |= ULTRASONIC_FRONT_TRIGGER;
	P2OUT &= ~ULTRASONIC_FRONT_TRIGGER;
	__delay_cycles(10);
	P2OUT |= 0x2; // compare mode, output 0, no interrupt enabled
	__delay_cycles(10);
	P2OUT &= ~ULTRASONIC_FRONT_TRIGGER;
}

//Measures length of pulse from front ultrasonic measurement
interrupt void ultrasonic_timer_handler() {
	front_state++;
    //Two states determine which edge we look for
	if (front_state == 1) {
		front_initial = TA1CCR0;
		taie_overflow = 0;
	} else if (front_state == 2) {
		front_state = 0;
		front_final = TA1CCR0;
        //taie_overflow is incremented when TA1R overflows; if an overflow occurs, disregard the bogus data
		if (taie_overflow == 0) {
			latest_front = ((front_final - front_initial))/(SCALE_FACTOR_FRONT)*2;
		}
	}
}
ISR_VECTOR(ultrasonic_timer_handler, ".int13")

//Interrupt routine for detecting overflows
interrupt void ultrasonic_timer_taie() {
	if (TA1IV == 0x0A) { //0x0A is an overflow (distinguishes from other possibilities)
		taie_overflow++;
	}
	TA1CTL &= ~TAIFG;
}
ISR_VECTOR(ultrasonic_timer_taie, ".int12")

//Helper function for setting camera trigger pin
void set_camera_gpio(bool high) {
	if (high) {
		P2OUT |= CAMERA_TRIGGER_PIN;
	} else {
		P2OUT &= ~CAMERA_TRIGGER_PIN;
	}
}

//Takes a picture if the camera is ready
bool take_picture() {
	if (camera_timeout == 0) {
		set_camera_gpio(true);
		camera_timeout = CAMERA_POSEDGE_LEN;
		return true;
	} else {
		return false;
	}
}

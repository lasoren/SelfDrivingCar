#include "msp430g2553.h"
#include "MotorOutput.h"
#include "SensorCollect.h"
/*
 * main.c
 */

#define SENSOR_LOOPS_SIDE 20
#define SENSOR_LOOPS_FRONT 100
//if the sensor on the right and left differ by this amount or more, lets turn
#define TURN_THRESHOLD 20
//down to the end of the hall
#define BIG_THRESHOLD 750
//turn
#define FRONT_TURN 10
//backup
#define FRONT_THRESHOLD 12

double defaultPWM = 0.62;
int sensor_conversions_side = SENSOR_LOOPS_SIDE;
int sensor_conversions_front = SENSOR_LOOPS_FRONT;

extern int camera_timeout; //from SensorCollect.c

//possible previous states
#define STOPPED 0
#define F_STRAIGHT 1
#define F_RIGHT 2
#define F_LEFT 3
#define R_STRAIGHT 4
#define R_RIGHT 5
#define R_LEFT 6
volatile char ps = STOPPED;

volatile char last_turn;

#define TURNBACK_LENGTH 200
volatile char turningBack = 0;
volatile int turnBackCounter = 0;


#define TURNAROUND_LENGTH 1550
volatile char turningAround = 0; //1 is turning around
volatile int turnAroundCounter = 0;

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
	sensor_conversions_side--;
	sensor_conversions_front--;
	if (camera_timeout == 1) {
		camera_timeout = 0;
		set_camera_gpio(false);
	} else if (camera_timeout > 0) {
		camera_timeout--;
	}
	if (sensor_conversions_side == 0) {
		sensor_conversions_side = SENSOR_LOOPS_SIDE;
		ADC10CTL0 |= ADC10SC;  // trigger a conversion
	}
	if (sensor_conversions_front == 0) {
		sensor_conversions_front = SENSOR_LOOPS_FRONT;
		make_front_measurement();
	}

	//automated driving logic
	int leftDist = get_latest_left();
	int rightDist = get_latest_right();
//	double avgDist = (leftDist+rightDist)/2.0;
//	double metric = avgDist/60.0;
	int frontDist = get_latest_front();
	switch (ps) {
	case STOPPED:
		//start moving, just go for it
		forward(defaultPWM);
		straight();
		ps = F_STRAIGHT;
		break;
	case F_STRAIGHT:
		if (frontDist <= FRONT_THRESHOLD) {
			reverse(defaultPWM);
			straight();
			ps = R_STRAIGHT;
		}
		else if (leftDist > BIG_THRESHOLD) {
			left();
			ps = F_LEFT;
		}
		else if (rightDist > BIG_THRESHOLD) {
			right();
			ps = F_RIGHT;
		}
		else if (leftDist - rightDist > TURN_THRESHOLD) {
			left();
			ps = F_LEFT;
		}
		else if (rightDist - leftDist > TURN_THRESHOLD) {
			right();
			ps = F_RIGHT;
		}
//		else if (frontDist < FRONT_TURN) {
//			if (leftDist >= rightDist) {
//				left();
//				ps = F_LEFT;
//			}
//			else {
//				right();
//				ps = F_RIGHT;
//			}
//		}
		break;
	case F_LEFT:
		last_turn = F_LEFT;
		if (turningAround == 1) {
			turnAroundCounter--;
			if (turnAroundCounter <= 0) {
				forward(defaultPWM);
				straight();
				turnAroundCounter = 0;
				ps = F_STRAIGHT;
				turningAround = 0;
			}
		}
		else if (turningBack == 1) {
			turnBackCounter--;
			if (turnBackCounter <= 0) {
				straight();
				ps = F_STRAIGHT;
				turnBackCounter = 0;
				turningBack = 0;
			}
		}
		else if (rightDist >= leftDist) {
			right();
			ps = F_RIGHT;
			turningBack = 1;
			turnBackCounter = TURNBACK_LENGTH;
		}
		break;
	case F_RIGHT:
		last_turn = F_RIGHT;
		if (turningAround == 1) {
			turnAroundCounter--;
			if (turnAroundCounter <= 0) {
				forward(defaultPWM);
				straight();
				turnAroundCounter = 0;
				ps = F_STRAIGHT;
				turningAround = 0;
			}
		}
		else if (turningBack == 1) {
			turnBackCounter--;
			if (turnBackCounter <= 0) {
				straight();
				ps = F_STRAIGHT;
				turnBackCounter = 0;
				turningBack = 0;
			}
		}
		else if (leftDist >= rightDist) {
			left();
			ps = F_LEFT;
			turningBack = 1;
			turnBackCounter = TURNBACK_LENGTH;
		}
		break;
	case R_STRAIGHT:
		if (frontDist >= 3*FRONT_THRESHOLD) {
			turningAround = 1;
			turnAroundCounter = TURNAROUND_LENGTH;
			if (last_turn == F_RIGHT) {
				ps = R_RIGHT;
				right();
			}
			else {
				ps = R_LEFT;
				left();
			}
		}
		break;
	case R_RIGHT:
		if (turningAround == 1) {
			turnAroundCounter--;
			if (turnAroundCounter <= 0) {
				forward(defaultPWM);
				left();
				turnAroundCounter = TURNAROUND_LENGTH;
				ps = F_LEFT;
			}
		}
		break;
	case R_LEFT:
		if (turningAround == 1) {
			turnAroundCounter--;
			if (turnAroundCounter <= 0) {
				forward(defaultPWM);
				right();
				turnAroundCounter = TURNAROUND_LENGTH;
				ps = F_RIGHT;
			}
		}
		break;
	default:
		stop();
		straight();
		ps = STOPPED;
		break;
	}

}
ISR_VECTOR(WDT_interval_handler, ".int10")

#include "msp430g2553.h"
#include "stdlib.h"
#include "MotorOutput.h"
#include "SensorCollect.h"
/*
 * SIM-V (Self-Driving Indoor Mapping Vehicle)
 *
 *Built and Coded by Luke Sorenson and John Moore
 *
 *
 *main.c by Luke Sorenson
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
//take pictures once a second
#define PICTURE_TIMER 1000
volatile int pictureTimer = 1000;

double defaultPWM = 0.58;
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

#define TURNBACK_LENGTH 30
volatile char turningBack = 0;
volatile int turnBackCounter = 0;

#define ARRAY_SIZE 18
#define NUM_SENSORS 3
int lastData[ARRAY_SIZE][NUM_SENSORS];
volatile char idx = 0;
#define STUCK_COUNT 200
char stuckCounter = STUCK_COUNT;



#define TURNAROUND_LENGTH 1450
volatile char turningAround = 0; //1 is turning around
volatile int turnAroundCounter = 0;

void init_wdt();
void init_lastData();

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    BCSCTL1 = CALBC1_8MHZ;    // 8Mhz calibration for clock
    DCOCTL  = CALDCO_8MHZ;

    init_sensors();
    init_wdt();
    init_motors();
    init_lastData();

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

void init_lastData() {
	char i = 0;
	for (i = 0; i < ARRAY_SIZE; i++) {
		lastData[i][0] = 300; //right dist
		lastData[i][1] = 300; //left dist
		lastData[i][2] = 300; //front dist
	}
}

bool amIStuck() {
	unsigned int sum = 0;
	char i = 0;
	for (i = 0; i < ARRAY_SIZE; i++) {
		sum += lastData[i][0] + lastData[i][1] + lastData[i][2];
	}
	if (sum/(3*ARRAY_SIZE) < 7) {
		return true;
	}
	return false;
}

//called every 1 ms
interrupt void WDT_interval_handler(){
	int leftDist = get_latest_left();
	int rightDist = get_latest_right();
	int frontDist = get_latest_front();
	bool iAmStuck = false;

	sensor_conversions_side--;
	sensor_conversions_front--;
	stuckCounter--;
	if (camera_timeout == 1) {
		camera_timeout = 0;
		set_camera_gpio(false);
	} else if (camera_timeout > 0) {
		camera_timeout--;
	}
	//trigger the front sensor every 100ms
	if (sensor_conversions_side == 0) {
		sensor_conversions_side = SENSOR_LOOPS_SIDE;
		ADC10CTL0 |= ADC10SC;  // trigger a conversion
	}
	//trigger an ADC conversion on left and right sensors every 20ms
	if (sensor_conversions_front == 0) {
		sensor_conversions_front = SENSOR_LOOPS_FRONT;
		make_front_measurement();
	}
	//keep track of the last 18 sensor measurements
	if (stuckCounter == 0) {
		stuckCounter = STUCK_COUNT;
		//keep track of the current sensor values
		char current = idx%20;
		char prev = (idx-1)%20;
		lastData[current][0] = abs(lastData[prev][0] - rightDist);
		lastData[current][1] = abs(lastData[prev][1] - leftDist);
		lastData[current][2] = abs(lastData[prev][2] - frontDist);
		idx++;

		//if the car hasnt moved in the last 18 sensor measurements
		iAmStuck = amIStuck();
	}

	//if the car isnt stopped, take a picture every 1000ms
	if (ps != STOPPED) {
		if (pictureTimer <= 0) {
			pictureTimer = PICTURE_TIMER;
			bool success = take_picture();
		}
	}

	//automated driving logic
	pictureTimer--;
	switch (ps) {
	case STOPPED:
		//start moving, just go for it
		forward(defaultPWM);
		straight();
		ps = F_STRAIGHT;
		break;
	case F_STRAIGHT:
		//stuck, car hasnt moved in 3 seconds
		if (iAmStuck) {
			reverse(defaultPWM);
			straight();
			ps = R_STRAIGHT;
			turningAround = 1;
			turnAroundCounter = TURNAROUND_LENGTH;
			init_lastData();
			iAmStuck = false;
			break;
		}
		//up against a wall
		if (frontDist <= FRONT_THRESHOLD) {
			reverse(defaultPWM);
			straight();
			ps = R_STRAIGHT;
			turningAround = 1;
			turnAroundCounter = TURNAROUND_LENGTH;
		}
		//looking down and open hallway
		else if (leftDist > BIG_THRESHOLD) {
			left();
			ps = F_LEFT;
		}
		else if (rightDist > BIG_THRESHOLD) {
			right();
			ps = F_RIGHT;
		}
		//off center in a hallway
		else if (leftDist - rightDist > TURN_THRESHOLD) {
			left();
			ps = F_LEFT;
		}
		else if (rightDist - leftDist > TURN_THRESHOLD) {
			right();
			ps = F_RIGHT;
		}
		break;
	case F_LEFT:
		last_turn = F_LEFT;
		if (iAmStuck) {
			reverse(defaultPWM);
			straight();
			ps = R_STRAIGHT;
			turningAround = 1;
			turnAroundCounter = TURNAROUND_LENGTH;
			init_lastData();
			iAmStuck = false;
			break;
		}
		//in the process of turning around
		if (turningAround == 1) {
			turnAroundCounter--;
			if (turnAroundCounter <= TURNAROUND_LENGTH/2) {
				forward(defaultPWM);
				straight();
				turnAroundCounter = 0;
				ps = F_STRAIGHT;
				turningAround = 0;
			}
		}
		//turning back to center wheels
		else if (turningBack == 1) {
			turnBackCounter--;
			if (turnBackCounter <= 0) {
				straight();
				ps = F_STRAIGHT;
				turnBackCounter = 0;
				turningBack = 0;
			}
		}
		//stop turning, we are now centered in hallway
		else if (rightDist >= leftDist) {
			right();
			ps = F_RIGHT;
			turningBack = 1;
			turnBackCounter = TURNBACK_LENGTH;
		}
		break;
	case F_RIGHT:
		last_turn = F_RIGHT;
		if (iAmStuck) {
			reverse(defaultPWM);
			straight();
			ps = R_STRAIGHT;
			turningAround = 1;
			turnAroundCounter = TURNAROUND_LENGTH;
			init_lastData();
			iAmStuck = false;
			break;
		}
		if (turningAround == 1) {
			turnAroundCounter--;
			if (turnAroundCounter <= TURNAROUND_LENGTH/2) {
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
		turnAroundCounter--;
		if (frontDist >= 3*FRONT_THRESHOLD || turnAroundCounter <= 0) {
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

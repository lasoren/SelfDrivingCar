#include "msp430g2553.h"
#include "MotorOutput.h"

//1 BIT1 P1.0
//2 BIT2 P1.1
//4 BIT3 P1.2
//8 BIT4 P1.3
//16 BIT5 P1.4
//32 BIT6 P1.5
//64 BIT7 P1.6
//128 BIT8 P1.7

//driving
#define PWMA_F 4 //port1.2, controls motors PWM
#define AIN1_F 8 //port2, controls the front and back motors
#define AIN2_F 16

//steering
#define PWMA_S 32 //port1, controls the steering
#define AIN1_S 64
#define AIN2_S 128

// define direction register, output register, and select registers
#define TA_DIR P1DIR
#define TA_OUT P1OUT
#define TA_SEL P1SEL
// define the bit mask (within the port) corresponding to output TA1
#define TA1_BIT 4

void init_PWM_timer(double PWM) {
    
//    BCSCTL1 = CALBC1_8MHZ;			// 8Mhz calibration for clock
//    DCOCTL  = CALDCO_8MHZ;
    
    TACTL = TACLR;					// reset clock
    TACTL = TASSEL_2+ID_3;			// clock source = SMCLK
                                    // clock divider=8
                                    // (clock still off)
                                    
    TACCTL0=0;						// Nothing on CCR0
    TACCTL1=OUTMOD_7;				// reset/set mode
    TACCR0 = 999;					// period-1 in CCR0
    TACCR1 = (int) PWM*1000;                  // duty cycle in CCR1
    TA_SEL|=TA1_BIT;				// connect timer 1 output to pin 2
    TA_DIR|=TA1_BIT;

    TACTL |= MC_1;					// timer on in up mode
}

void init_motors() {
    //set the output directions as neccessary TODO
    P1DIR |= (PWMA_S+AIN1_S+AIN2_S);
    P2DIR |= (PWMA_F+AIN1_F+AIN2_F+PWMB_B+BIN1_B+BIN2_B);
}

//input - PWM 0.0 - 1.0 specifying the PWM duty cycle
void forward(double PWM) {
    init_PWM_timer(PWM);
    P1OUT |= (AIN1_F);
    P1OUT &= !(AIN2_F);
}

//input - PWM 0.0 - 1.0 specifying the PWM duty cycle
void reverse(double PWM) {
    init_PWM_timer(PWM);
    P1OUT |= (AIN2_F);
    P1OUT &= !(AIN1_F);
}

void straight() {
    P1OUT &= !(PWMA_S+AIN1_S+AIN2_S);
}

void left() {
    P1OUT |= (PWMA_S+AIN1_S);
    P1OUT &= !(AIN2_S);
}

void right() {
    P1OUT |= (PWMA_S+AIN2_S);
    P1OUT &= !(AIN1_S);
}


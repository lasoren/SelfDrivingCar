#include "msp430g2553.h"
//header file for MotorOutput.c

#ifndef MOTOR_OUTPUT
#define MOTOR_OUTPUT

void init_PWM_timer(double);
void init_motors();
void forward(double);
void reverse(double);
void stop();
void straight();
void left();
void right();

#endif

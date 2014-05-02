#ifndef SENSORCOLLECT_H_
#define SENSORCOLLECT_H_

//boolean support
typedef int bool;
#define true 1
#define false 0

void init_sensors(void);	// routine to setup the sensors
void init_sensor_adc(void);	// routine to setup ADC
void init_ultrasonic_timer(void);
void init_camera(void);
void make_front_measurement();
int get_latest_right();
int get_latest_left();
int get_latest_front();
void set_camera_gpio(bool high);
bool take_picture();

#endif /* SENSORCOLLECT_H_ */

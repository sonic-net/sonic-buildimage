#ifndef _DFD_FAN_DRIVER_H_
#define _DFD_FAN_DRIVER_H_

ssize_t dfd_get_fan_speed(unsigned int fan_index, unsigned int motor_index,unsigned int *speed);

int dfd_set_fan_pwm(unsigned int fan_index, unsigned int motor_index, int pwm);

int dfd_get_fan_pwm(unsigned int fan_index, unsigned int motor_index, int *pwm);

int dfd_get_fan_present_status(unsigned int fan_index);

int dfd_get_fan_roll_status(unsigned int fan_index, unsigned int motor_index);

int dfd_get_fan_speed_level(unsigned int fan_index, unsigned int motor_index, int *level);

int dfd_set_fan_speed_level(unsigned int fan_index, unsigned int motor_index, int level);

#endif /* _DFD_FAN_DRIVER_H_ */

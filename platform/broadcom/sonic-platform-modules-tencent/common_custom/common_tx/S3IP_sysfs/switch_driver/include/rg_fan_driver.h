#ifndef _RG_FAN_DRIVER_H_
#define _RG_FAN_DRIVER_H_

ssize_t dfd_get_fan_status_str(unsigned int fan_index, char *buf, size_t count);

ssize_t dfd_get_fan_info(unsigned int fan_index, uint8_t cmd, char *buf, size_t count);

int dfd_get_fan_speed(unsigned int fan_index, unsigned int motor_index,unsigned int *speed);

ssize_t dfd_get_fan_speed_str(unsigned int fan_index, unsigned int motor_index,
            char *buf, size_t count);

int dfd_set_fan_pwm(unsigned int fan_index, int pwm);

int dfd_get_fan_pwm(unsigned int fan_index, int *pwm);

ssize_t dfd_get_fan_pwm_str(unsigned int fan_index, char *buf, size_t count);

ssize_t dfd_get_fan_motor_speed_tolerance_str(unsigned int fan_index, unsigned int motor_index,
            char *buf, size_t count);

int dfd_get_fan_speed_target(unsigned int fan_index, unsigned int motor_index, int *value);

ssize_t dfd_get_fan_motor_speed_target_str(unsigned int fan_index, unsigned int motor_index,
            char *buf, size_t count);

ssize_t dfd_get_fan_direction_str(unsigned int fan_index, char *buf, size_t count);

ssize_t dfd_get_fan_motor_speed_max_str(unsigned int fan_index, unsigned int motor_index,
            char *buf, size_t count);

ssize_t dfd_get_fan_motor_speed_min_str(unsigned int fan_index, unsigned int motor_index,
            char *buf, size_t count);

#endif /* _RG_FAN_DRIVER_H_ */

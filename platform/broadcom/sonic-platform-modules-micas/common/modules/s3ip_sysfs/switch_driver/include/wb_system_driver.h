#ifndef _SYSTEM_DRIVER_H_
#define _SYSTEM_DRIVER_H_

typedef enum module_pwr_status_e {
    MODULE_POWER_OFF = 0,
    MODULE_POWER_ON,
} module_pwr_status_t;

ssize_t dfd_system_get_system_value(unsigned int type, int *value);
ssize_t dfd_system_set_system_value(unsigned int type, int value);

#endif /* _SYSTEM_DRIVER_H_ */

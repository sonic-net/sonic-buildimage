#ifndef _SYSTEM_DRIVER_H_
#define _SYSTEM_DRIVER_H_

#define BMC_VIEW_FILE         "/sys/bus/platform/devices/wb_aspeed_common/bmc_view"
#define BMC_SWITCH_FILE       "/sys/bus/platform/devices/wb_aspeed_common/bmc_switch"
typedef enum module_pwr_status_e {
    MODULE_POWER_OFF = 0,
    MODULE_POWER_ON,
} module_pwr_status_t;

ssize_t dfd_system_get_system_value(unsigned int type, int *value);
ssize_t dfd_system_set_system_value(unsigned int type, int value);
ssize_t dfd_system_get_port_power_status(unsigned int type, char *buf, size_t count);
ssize_t dfd_system_get_bmc_view(char *buf, size_t count);
ssize_t dfd_system_set_bmc_switch(const char* buf, size_t count);

#endif /* _SYSTEM_DRIVER_H_ */

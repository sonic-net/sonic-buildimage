#ifndef _RG_WATCHDOG_DRIVER_H_
#define _RG_WATCHDOG_DRIVER_H_

ssize_t dfd_get_watchdog_info(uint8_t type, char *buf, size_t count);

ssize_t dfd_watchdog_get_status_str(char *buf, size_t count);

#endif /* _RG_WATCHDOG_DRIVER_H_ */

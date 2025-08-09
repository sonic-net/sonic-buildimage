#ifndef _WB_WATCHDOG_DRIVER_H_
#define _WB_WATCHDOG_DRIVER_H_

/**
 * dfd_get_watchdog_info - Get watchdog information
 * @type: Watchdog information type
 * @buf: Receive buf
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_watchdog_info(uint8_t type, char *buf, size_t count);

ssize_t dfd_watchdog_get_status_str(char *buf, size_t count);
ssize_t dfd_watchdog_get_status(char *buf, size_t count);
ssize_t dfd_watchdog_set_status(int value);

#endif /* _WB_WATCHDOG_DRIVER_H_ */

#ifndef _WB_SLOT_DRIVER_H_
#define _WB_SLOT_DRIVER_H_

/**
 * dfd_get_slot_present_str - Gets the subcard present
 * @slot_index: Number of the sub-card, starting from 1
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_slot_present_str(unsigned int slot_index, char *buf, size_t count);

/**
 * dfd_get_slot_status_str - Gets the subcard status
 * @slot_index: Number of the sub-card, starting from 1
 * @buf: Receive buf
 * @count: Receive buf length
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_slot_status_str(unsigned int slot_index, char *buf, size_t count);

/**
 * dfd_get_slot_info - Obtain the subcard information
 * @slot_index: Number of the sub-card, starting from 1
 * @cmd: Card information type, sub-card name :2, sub-card serial number :3, sub-card hardware version number :5
 * @buf: Receive buf
 * @count: Receive buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_slot_info(unsigned int slot_index, uint8_t cmd, char *buf, size_t count);

/**
 * dfd_get_slot_power_ctrl_status_str - get power control status of slot
 * @slot_index: Number of the sub-card, starting from 1
 * @buf: Receive buf
 * @count: Receive buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_slot_power_ctrl_status_str(unsigned int slot_index, char *buf, size_t count);

/**
 * dfd_set_slot_power_ctrl_status_str - set power control status of slot
 * @slot_index: Number of the sub-card, starting from 1
 * @value: Power status of slot
 * return: Success: 0 is returned
 *       : Failed: A negative value is returned
 */
int dfd_set_slot_power_ctrl_status_str(unsigned int slot_index, int value);

#endif /* _WB_SLOT_DRIVER_H_ */

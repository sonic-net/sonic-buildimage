#ifndef _WB_LED_DRIVER_H_
#define _WB_LED_DRIVER_H_

/**
 * dfd_get_led_status - Get LED and other status
 * @led_id: led lamp type
 * @led_index: led light offset
 * @buf: LED light status receives buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_led_status(uint16_t led_id, uint8_t led_index, char *buf, size_t count);

/**
 * dfd_set_led_status - Set LED light status
 * @led_id: led lamp type
 * @led_index: led light offset
 * @value: LED light status value
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_set_led_status(uint16_t led_id, uint8_t led_index, int value);

#endif /* _WB_LED_DRIVER_H_ */

#ifndef _RG_LED_DRIVER_H_
#define _RG_LED_DRIVER_H_

ssize_t dfd_get_led_status(uint16_t led_id, uint8_t led_index, char *buf, size_t count);

ssize_t dfd_set_led_status(uint16_t led_id, uint8_t led_index, int value);

#endif /* _RG_LED_DRIVER_H_ */

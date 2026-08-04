#ifndef _DFD_SENSORS_DRIVER_H_
#define _DFD_SENSORS_DRIVER_H_

ssize_t dfd_get_temp_info(uint8_t main_dev_id, uint8_t dev_index,
            uint8_t temp_index, uint8_t temp_attr, char *buf);

ssize_t dfd_get_voltage_info(uint8_t main_dev_id, uint8_t dev_index,
            uint8_t in_index, uint8_t in_attr, char *buf);

#endif /* _DFD_SENSORS_DRIVER_H_ */

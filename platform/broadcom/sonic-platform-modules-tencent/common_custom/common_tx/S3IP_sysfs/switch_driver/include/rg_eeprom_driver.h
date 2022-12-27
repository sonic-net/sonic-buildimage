#ifndef _RG_EEPROM_DRIVER_H_
#define _RG_EEPROM_DRIVER_H_

int dfd_get_eeprom_size(int e2_type, int index);

ssize_t dfd_read_eeprom_data(int e2_type, int index, char *buf, loff_t offset, size_t count);

ssize_t dfd_write_eeprom_data(int e2_type, int index, char *buf, loff_t offset, size_t count);
#endif /* _RG_EEPROM_DRIVER_H_ */

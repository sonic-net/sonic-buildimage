#ifndef _WB_EEPROM_DRIVER_H_
#define _WB_EEPROM_DRIVER_H_

#define SFF_E2_PATH             "/sys/bus/i2c/devices/%d-0050/eeprom"
#define E2_PATH_SIZE            (64)
#define SFF_E2_DEFAULT_SIZE     (0x8180)

/**
 * dfd_get_eeprom_size - Gets the data size of the eeprom
 * @e2_type: This section describes the E2 type, including system, PSU, fan, and module E2
 * @index: E2 number
 * return: Succeeded: The data size of the eeprom is returned
 *       : Failed: A negative value is returned
 */
int dfd_get_eeprom_size(int e2_type, int index);

/**
 * dfd_read_eeprom_data - Read eeprom data
 * @e2_type: This section describes the E2 type, including system, PSU, fan, and module E2
 * @index: E2 number
 * @buf: eeprom data received buf
 * @offset: The offset address of the read
 * @count: Read length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_read_eeprom_data(int e2_type, int index, char *buf, loff_t offset, size_t count);

/**
 * dfd_write_eeprom_data - Write eeprom data
 * @e2_type: This section describes the E2 type, including system, PSU, fan, and module E2
 * @index: E2 number
 * @buf: eeprom data buf
 * @offset: The offset address of the write
 * @count: Write length
 * return: Success: The length of the written data is returned
 *       : Failed: A negative value is returned
 */
ssize_t dfd_write_eeprom_data(int e2_type, int index, char *buf, loff_t offset, size_t count);
ssize_t dfd_get_eeprom_alias(unsigned int e2_type, unsigned int e2_index, char *buf, size_t count);
ssize_t dfd_get_eeprom_tag(unsigned int e2_type, unsigned int e2_index, char *buf, size_t count);
ssize_t dfd_get_eeprom_type(unsigned int e2_type, unsigned int e2_index, char *buf, size_t count);
ssize_t dfd_get_eeprom_bus(unsigned int e2_type, unsigned int e2_index, char *buf, size_t count);
ssize_t dfd_get_eeprom_addr(unsigned int e2_type, unsigned int e2_index, char *buf, size_t count);
int dfd_set_eeprom_write_protection(unsigned int e2_type, unsigned int e2_index, void *val, unsigned int len);
ssize_t dfd_get_eeprom_wp_reg_str(unsigned int e2_type, unsigned int e2_index,
            char *buf, size_t count);

extern dfd_sysfs_func_map_t eeprom_func_table[DFD_EEPROM_MAX_E];

#endif /* _WB_EEPROM_DRIVER_H_ */

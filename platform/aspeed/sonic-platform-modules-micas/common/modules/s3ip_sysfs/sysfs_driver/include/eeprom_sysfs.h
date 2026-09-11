#ifndef _EEPROM_SYSFS_H_
#define _EEPROM_SYSFS_H_

struct s3ip_sysfs_eeprom_drivers_s {
    int (*get_eeprom_number)(void);
    int (*set_eeprom_attr)(unsigned int eeprom_index, unsigned int type, unsigned int value);
    ssize_t (*get_eeprom_attr)(unsigned int eeprom_index, unsigned int type, char *buf, size_t count);
    int (*get_eeprom_size)(unsigned int e2_index);
    ssize_t (*read_eeprom_data)(unsigned int e2_index, char *buf, loff_t offset, size_t count);
    ssize_t (*write_eeprom_data)(unsigned int e2_index, char *buf, loff_t offset, size_t count);
};

extern int s3ip_sysfs_eeprom_drivers_register(struct s3ip_sysfs_eeprom_drivers_s *drv);
extern void s3ip_sysfs_eeprom_drivers_unregister(void);
#endif /*_EEPROM_SYSFS_H_ */

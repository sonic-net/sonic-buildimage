#ifndef _CABLETRAY_SYSFS_H_
#define _CABLETRAY_SYSFS_H_

struct s3ip_sysfs_cabletray_drivers_s {
    int (*get_cabletray_number)(void);
    int (*get_cabletray_eeprom_size)(unsigned int cabletray_index);
    ssize_t (*get_cabletray_name)(unsigned int cabletray_index, char *buf, size_t count);
    ssize_t (*get_cabletray_alias)(unsigned int cabletray_index, char *buf, size_t count);
    ssize_t (*get_cabletray_manufacturer)(unsigned int cabletray_index, char *buf, size_t count);
    ssize_t (*get_cabletray_serial_number)(unsigned int cabletray_index, char *buf, size_t count);
    ssize_t (*get_cabletray_part_number)(unsigned int cabletray_index, char *buf, size_t count);
    ssize_t (*get_cabletray_version)(unsigned int cabletray_index, char *buf, size_t count);
    ssize_t (*get_cabletray_slotid)(unsigned int cabletray_index, char *buf, size_t count);
    ssize_t (*read_cabletray_eeprom_data)(unsigned int cabletray_index, char *buf, loff_t offset, size_t count);
};

extern int s3ip_sysfs_cabletray_drivers_register(struct s3ip_sysfs_cabletray_drivers_s *drv);
extern void s3ip_sysfs_cabletray_drivers_unregister(void);
#define CABLETRAY_NAME_LEN                       16
#endif /*_CABLETRAY_SYSFS_H_ */

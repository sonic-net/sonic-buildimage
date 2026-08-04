#ifndef _CPLD_SYSFS_H_
#define _CPLD_SYSFS_H_

struct s3ip_sysfs_cpld_drivers_s {
    int (*get_main_board_cpld_number)(void);
    ssize_t (*get_main_board_cpld_alias)(unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_main_board_cpld_type)(unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_main_board_cpld_firmware_version)(unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_main_board_cpld_board_version)(unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_main_board_cpld_test_reg)(unsigned int cpld_index, char *buf, size_t count);
    int (*set_main_board_cpld_test_reg)(unsigned int cpld_index, unsigned int value);
};

extern int s3ip_sysfs_cpld_drivers_register(struct s3ip_sysfs_cpld_drivers_s *drv);
extern void s3ip_sysfs_cpld_drivers_unregister(void);
#endif /*_CPLD_SYSFS_H_ */

#ifndef _MISC_FW_SYSFS_H_
#define _MISC_FW_SYSFS_H_

struct s3ip_sysfs_misc_fw_drivers_s {
    int (*get_main_board_misc_fw_number)(void);
    int (*set_main_board_misc_fw_attr)(unsigned int misc_fw_index, unsigned int type, unsigned int value);
    ssize_t (*get_main_board_misc_fw_attr)(unsigned int misc_fw_index, unsigned int type, char *buf, size_t count);
};

extern int s3ip_sysfs_misc_fw_drivers_register(struct s3ip_sysfs_misc_fw_drivers_s *drv);
extern void s3ip_sysfs_misc_fw_drivers_unregister(void);
#endif /*_MISC_FW_SYSFS_H_ */

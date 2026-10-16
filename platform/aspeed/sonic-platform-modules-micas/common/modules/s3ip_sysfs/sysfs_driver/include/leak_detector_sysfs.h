#ifndef _LEAK_DETECTOR_SYSFS_H_
#define _LEAK_DETECTOR_SYSFS_H_

struct s3ip_sysfs_leak_detector_drivers_s {
    int (*get_main_board_leak_number)(void);
    int (*set_main_board_leak_attr)(unsigned int index, unsigned int type, unsigned int value);
    ssize_t (*get_main_board_leak_attr)(unsigned int index, unsigned int type, char *buf, size_t count);
};

extern int s3ip_sysfs_leak_detector_drivers_register(struct s3ip_sysfs_leak_detector_drivers_s *drv);
extern void s3ip_sysfs_leak_detector_drivers_unregister(void);
#endif /*_LEAK_DETECTOR_SYSFS_H_ */
#ifndef _DPU_SYSFS_H_
#define _DPU_SYSFS_H_

struct s3ip_sysfs_dpu_drivers_s {
    int (*get_dpu_number)(void);
    int (*get_dpu_fw_number)(void);
    int (*get_dpu_temp_number)(void);
    int (*set_dpu_attr)(unsigned int cpld_index, unsigned int type, unsigned int value);
    ssize_t (*get_dpu_attr)(unsigned int dpu_index, unsigned int type, char *buf, size_t count);
    ssize_t (*get_dpu_fw_attr)(unsigned int dpu_index, unsigned int fw_index, unsigned int type, char *buf, size_t count);
    ssize_t (*get_dpu_temp_attr)(unsigned int dpu_index, unsigned int temp_index, unsigned int type, char *buf, size_t count);
};

extern int s3ip_sysfs_dpu_drivers_register(struct s3ip_sysfs_dpu_drivers_s *drv);
extern void s3ip_sysfs_dpu_drivers_unregister(void);
#endif /*_DPU_SYSFS_H_ */

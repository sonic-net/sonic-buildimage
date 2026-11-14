#ifndef __GCU_SYSFS_H__
#define __GCU_SYSFS_H__

struct s3ip_sysfs_gcu_drivers_s {
    int (*get_gcu_number)(void);
    int (*get_gcu_temp_number)(void);
    int (*get_gcu_vol_number)(void);
    int (*get_gcu_power_number)(void);
    ssize_t (*get_main_board_gcu_attr)(unsigned int index, unsigned int type, char *buf, size_t count);
    ssize_t (*get_gcu_temp_attr)(unsigned int gcu_index, unsigned int temp_index, unsigned int type, char *buf, size_t count);
    ssize_t (*get_gcu_vol_attr)(unsigned int gcu_index, unsigned int vol_index, unsigned int type, char *buf, size_t count);
    ssize_t (*get_gcu_power_attr)(unsigned int gcu_index, unsigned int vol_index, unsigned int type, char *buf, size_t count);
};

extern int s3ip_sysfs_gcu_drivers_register(struct s3ip_sysfs_gcu_drivers_s *drv);
extern void s3ip_sysfs_gcu_drivers_unregister(void);

#endif /* __GCU_SYSFS_H__ */
#ifndef __PLL_SYSFS_H__
#define __PLL_SYSFS_H__

struct s3ip_sysfs_pll_drivers_s {
    int (*get_pll_number)(void);
    ssize_t (*get_pll_attr)(unsigned int index, unsigned int type, char *buf, size_t count);
};

extern int s3ip_sysfs_pll_drivers_register(struct s3ip_sysfs_pll_drivers_s *drv);
extern void s3ip_sysfs_pll_drivers_unregister(void);

#endif /* __PLL_SYSFS_H__ */
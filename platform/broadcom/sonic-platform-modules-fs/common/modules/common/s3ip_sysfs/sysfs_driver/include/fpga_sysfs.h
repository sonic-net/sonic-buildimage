#ifndef _FPGA_SYSFS_H_
#define _FPGA_SYSFS_H_

struct s3ip_sysfs_fpga_drivers_s {
    int (*get_main_board_fpga_number)(void);
    int (*set_main_board_fpga_attr)(unsigned int fpga_index, unsigned int type, unsigned int value);
    ssize_t (*get_main_board_fpga_attr)(unsigned int fpga_index, unsigned int type, char *buf, size_t count);
};

extern int s3ip_sysfs_fpga_drivers_register(struct s3ip_sysfs_fpga_drivers_s *drv);
extern void s3ip_sysfs_fpga_drivers_unregister(void);
#endif /*_FPGA_SYSFS_H_ */

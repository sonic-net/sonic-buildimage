#ifndef _FPGA_SYSFS_H_
#define _FPGA_SYSFS_H_

struct s3ip_sysfs_fpga_drivers_s {
    int (*get_main_board_fpga_number)(void);
    ssize_t (*get_main_board_fpga_alias)(unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_type)(unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_firmware_version)(unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_board_version)(unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_test_reg)(unsigned int fpga_index, char *buf, size_t count);
    int (*set_main_board_fpga_test_reg)(unsigned int fpga_index, unsigned int value);
};

extern int s3ip_sysfs_fpga_drivers_register(struct s3ip_sysfs_fpga_drivers_s *drv);
extern void s3ip_sysfs_fpga_drivers_unregister(void);
#endif /*_FPGA_SYSFS_H_ */

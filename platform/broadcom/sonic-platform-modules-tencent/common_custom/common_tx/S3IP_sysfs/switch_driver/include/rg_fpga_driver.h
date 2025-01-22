#ifndef _RG_FPGA_DRIVER_H_
#define _RG_FPGA_DRIVER_H_

ssize_t dfd_get_fpga_name(uint8_t main_dev_id, unsigned int fpga_index, char *buf, size_t count);

ssize_t dfd_get_fpga_type(uint8_t main_dev_id, unsigned int fpga_index, char *buf, size_t count);

ssize_t dfd_get_fpga_fw_version(uint8_t main_dev_id, unsigned int fpga_index, char *buf, size_t count);

ssize_t dfd_get_fpga_hw_version(uint8_t main_dev_id, unsigned int fpga_index, char *buf, size_t count);

int dfd_set_fpga_testreg(uint8_t main_dev_id, unsigned int fpga_index, int value);

int dfd_get_fpga_testreg(uint8_t main_dev_id, unsigned int fpga_index, int *value);

ssize_t dfd_get_fpga_testreg_str(uint8_t main_dev_id, unsigned int fpga_index,
            char *buf, size_t count);

#endif /* _RG_FPGA_DRIVER_H_ */

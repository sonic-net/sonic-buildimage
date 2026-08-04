#ifndef _RG_CPLD_DRIVER_H_
#define _RG_CPLD_DRIVER_H_

ssize_t dfd_get_cpld_name(uint8_t main_dev_id, unsigned int cpld_index, char *buf, size_t count);

ssize_t dfd_get_cpld_type(uint8_t main_dev_id, unsigned int cpld_index, char *buf, size_t count);

ssize_t dfd_get_cpld_fw_version(uint8_t main_dev_id, unsigned int cpld_index, char *buf, size_t count);

ssize_t dfd_get_cpld_hw_version(uint8_t main_dev_id, unsigned int cpld_index, char *buf, size_t count);

int dfd_set_cpld_testreg(uint8_t main_dev_id, unsigned int cpld_index, int value);

int dfd_get_cpld_testreg(uint8_t main_dev_id, unsigned int cpld_index, int *value);

ssize_t dfd_get_cpld_testreg_str(uint8_t main_dev_id, unsigned int cpld_index,
            char *buf, size_t count);

#endif /* _RG_CPLD_DRIVER_H_ */

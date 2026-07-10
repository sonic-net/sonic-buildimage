#ifndef _RG_PSU_DRIVER_H_
#define _RG_PSU_DRIVER_H_

ssize_t dfd_get_psu_info(unsigned int psu_index, uint8_t cmd, char *buf, size_t count);

ssize_t dfd_get_psu_present_status_str(unsigned int psu_index, char *buf, size_t count);

ssize_t dfd_get_psu_out_status_str(unsigned int psu_index, char *buf, size_t count);

ssize_t dfd_get_psu_in_status_str(unsigned int psu_index, char *buf, size_t count);

ssize_t dfd_get_psu_input_type(unsigned int psu_index, char *buf, size_t count);

#endif /* _RG_PSU_DRIVER_H_ */

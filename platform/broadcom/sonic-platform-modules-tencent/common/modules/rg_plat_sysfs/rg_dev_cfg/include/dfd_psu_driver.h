#ifndef _DFD_PSU_DRIVER_H_
#define _DFD_PSU_DRIVER_H_

ssize_t dfd_get_psu_info(unsigned int psu_index, uint8_t cmd, char *buf);

ssize_t dfd_get_psu_status_str(unsigned int psu_index, char *buf);

int dfd_get_psu_present_status(unsigned int psu_index);

int dfd_get_psu_output_status(unsigned int psu_index);

int dfd_get_psu_alert_status(unsigned int psu_index);

#endif /* _DFD_PSU_DRIVER_H_ */

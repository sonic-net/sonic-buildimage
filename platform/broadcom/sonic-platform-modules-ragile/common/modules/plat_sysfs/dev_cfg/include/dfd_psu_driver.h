#ifndef _DFD_PSU_DRIVER_H_
#define _DFD_PSU_DRIVER_H_

int dfd_get_psu_present_status(unsigned int psu_index);

int dfd_get_psu_output_status(unsigned int psu_index);

int dfd_get_psu_alert_status(unsigned int psu_index);

#endif /* _DFD_PSU_DRIVER_H_ */

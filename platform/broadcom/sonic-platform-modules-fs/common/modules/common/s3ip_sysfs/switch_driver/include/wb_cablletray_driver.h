#ifndef _WB_CABLETRAY_DRIVER_H_
#define _WB_CABLETRAY_DRIVER_H_

ssize_t dfd_get_cabletray_info(unsigned int cabletray_index, uint8_t cmd, char *buf, size_t count);
ssize_t dfd_get_cabletray_slotid_info(unsigned int cabletray_index, char *buf, size_t count);

#endif /* _WB_CABLETRAY_DRIVER_H_ */

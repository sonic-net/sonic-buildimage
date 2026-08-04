#ifndef _DFD_SLOT_DRIVER_H_
#define _DFD_SLOT_DRIVER_H_

int dfd_get_slot_status(unsigned int slot_index);

ssize_t dfd_get_slot_info(unsigned int slot_index, uint8_t cmd, char *buf);

ssize_t dfd_get_slot_status_str(unsigned int slot_index, char *buf);

#endif /* _DFD_SLOT_DRIVER_H_ */

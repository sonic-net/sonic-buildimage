#ifndef _RG_SLOT_DRIVER_H_
#define _RG_SLOT_DRIVER_H_

ssize_t dfd_get_slot_status_str(unsigned int slot_index, char *buf, size_t count);

ssize_t dfd_get_slot_info(unsigned int slot_index, uint8_t cmd, char *buf, size_t count);

#endif /* _RG_SLOT_DRIVER_H_ */

#ifndef _WB_DISK_DRIVER_H_
#define _WB_DISK_DRIVER_H_

ssize_t dfd_get_disk_info(unsigned int dev_index, unsigned int type,  char *buf, size_t count);
ssize_t dfd_disk_temp_status(unsigned int dev_index, unsigned int type,  char *buf, size_t count);
ssize_t dfd_get_disk_link_alarm(unsigned int dev_index, unsigned int type,  char *buf, size_t count);
ssize_t dfd_get_disk_wear_status(unsigned int dev_index, unsigned int type,  char *buf, size_t count);
ssize_t dfd_get_disk_link_status(unsigned int dev_index, unsigned int type,  char *buf, size_t count);
ssize_t dfd_get_disk_speed_status(unsigned int dev_index, unsigned int type,  char *buf, size_t count);
ssize_t dfd_get_disk_correctable_error(unsigned int dev_index, unsigned int type,  char *buf, size_t count);
ssize_t dfd_get_disk_uncorrectable_error(unsigned int dev_index, unsigned int type,  char *buf, size_t count);

extern dfd_sysfs_func_map_t disk_func_table[DFD_DISK_MAX_E];
extern dfd_debug_data_key_map_t disk_dbg_key_table[DFD_DISK_MAX_E];

#endif /* _WB_DISK_DRIVER_H_ */
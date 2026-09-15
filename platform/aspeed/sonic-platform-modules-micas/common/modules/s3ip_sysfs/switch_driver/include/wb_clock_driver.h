#ifndef _WB_CLOCK_DRIVER_H_
#define _WB_CLOCK_DRIVER_H_

#define DFD_CLOCK_FAULT     (1)

ssize_t dfd_clock_get_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);

ssize_t dfd_clock_get_alias(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count);

extern dfd_sysfs_func_map_t clock_func_table[DFD_CLOCK_MAX_E];

#endif /* _WB_CLOCK_DRIVER_H_ */
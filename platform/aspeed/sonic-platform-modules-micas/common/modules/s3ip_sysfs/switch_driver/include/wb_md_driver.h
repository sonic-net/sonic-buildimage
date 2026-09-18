#ifndef _WB_MD_DRIVER_H_
#define _WB_MD_DRIVER_H_

ssize_t dfd_get_md_status(unsigned int dev_index, unsigned int type,  char *buf, size_t count);
ssize_t dfd_get_md_dev_path(unsigned int dev_index, unsigned int type, char *buf, size_t count);

extern dfd_sysfs_func_map_t md_func_table[DFD_MD_MAX_E];
extern dfd_debug_data_key_map_t md_dbg_key_table[DFD_MD_MAX_E];
#endif /* _WB_MD_DRIVER_H_ */
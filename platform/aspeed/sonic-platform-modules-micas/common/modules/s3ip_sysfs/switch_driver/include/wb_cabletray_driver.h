#ifndef _WB_CABLETRAY_DRIVER_H_
#define _WB_CABLETRAY_DRIVER_H_

ssize_t dfd_get_cabletray_alias(unsigned int main_dev_id, unsigned int cabletray_index, char *buf, size_t count);
ssize_t dfd_get_cabletray_name(unsigned int main_dev_id, unsigned int cabletray_index, char *buf, size_t count);
ssize_t dfd_get_cabletray_manufacturer(unsigned int main_dev_id, unsigned int cabletray_index, char *buf, size_t count);
ssize_t dfd_get_cabletray_serial_number(unsigned int main_dev_id, unsigned int cabletray_index, char *buf, size_t count);
ssize_t dfd_get_cabletray_part_number(unsigned int main_dev_id, unsigned int cabletray_index, char *buf, size_t count);
ssize_t dfd_get_cabletray_version(unsigned int main_dev_id, unsigned int cabletray_index, char *buf, size_t count);
ssize_t dfd_get_cabletray_slotid(unsigned int main_dev_id, unsigned int cabletray_index, char *buf, size_t count);
ssize_t dfd_get_cabletray_rack_sn(unsigned int main_dev_id, unsigned int cabletray_index, char *buf, size_t count);
ssize_t dfd_get_cabletray_uid(unsigned int main_dev_id, unsigned int cabletray_index, char *buf, size_t count);
ssize_t dfd_get_cabletray_h_location(unsigned int main_dev_id, unsigned int cabletray_index, char *buf, size_t count);
ssize_t dfd_get_cabletray_v_location(unsigned int main_dev_id, unsigned int cabletray_index, char *buf, size_t count);

extern dfd_sysfs_func_map_t cabletray_func_table[DFD_CABLETRAY_MAX_E];

#endif /* _WB_CABLETRAY_DRIVER_H_ */

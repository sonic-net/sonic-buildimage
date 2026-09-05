#ifndef _DFD_SFF_DRIVER_H_
#define _DFD_SFF_DRIVER_H_

int dfd_get_sff_id(unsigned int sff_index);

int dfd_set_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, int value);

ssize_t dfd_get_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, char *buf, int len);

ssize_t dfd_get_sff_eeprom_info(unsigned int sff_index, const char *attr_name, char *buf, int buf_len);

ssize_t dfd_get_sff_dir_name(unsigned int sff_index, char *buf, int buf_len);

int dfd_get_sff_polling_size(void);

ssize_t dfd_get_sff_polling_data(unsigned int sff_index, char *buf, loff_t offset, size_t count);

#endif /* _DFD_SFF_DRIVER_H_ */

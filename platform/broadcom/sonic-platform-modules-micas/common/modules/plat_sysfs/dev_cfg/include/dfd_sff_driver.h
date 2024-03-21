#ifndef _DFD_SFF_DRIVER_H_
#define _DFD_SFF_DRIVER_H_

ssize_t dfd_get_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, char *buf, int len);

ssize_t dfd_get_sff_dir_name(unsigned int sff_index, char *buf, int buf_len);

#endif /* _DFD_SFF_DRIVER_H_ */

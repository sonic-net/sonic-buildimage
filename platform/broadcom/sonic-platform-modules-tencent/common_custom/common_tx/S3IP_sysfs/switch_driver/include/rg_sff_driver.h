#ifndef _RG_SFF_DRIVER_H_
#define _RG_SFF_DRIVER_H_

int dfd_set_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, int value);

ssize_t dfd_get_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, char *buf, size_t count);

#endif /* _RG_SFF_DRIVER_H_ */

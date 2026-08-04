#ifndef _DFD_CPLD_DRIVER_H_
#define _DFD_CPLD_DRIVER_H_

ssize_t dfd_get_cpld_name(unsigned int cpld_index, char *buf);

ssize_t dfd_get_cpld_type(unsigned int cpld_index, char *buf);

ssize_t dfd_get_cpld_version(unsigned int cpld_index, char *buf);

int dfd_set_cpld_testreg(unsigned int cpld_index, int value);

int dfd_get_cpld_testreg(unsigned int cpld_index, int *value);

#endif /* _DFD_CPLD_DRIVER_H_ */

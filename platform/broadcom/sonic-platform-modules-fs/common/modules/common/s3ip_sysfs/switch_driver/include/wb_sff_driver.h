#ifndef _WB_SFF_DRIVER_H_
#define _WB_SFF_DRIVER_H_

/**
 * dfd_set_sff_cpld_info - Example Set the CPLD register status of the optical module
 * @sff_index: Optical module number, starting from 1
 * @cpld_reg_type: Optical module CPLD register type
 * @value: Writes the value to the register
 * return: Success :0
 *       : Failed: A negative value is returned
 */
int dfd_set_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, int value);

/**
 * dfd_get_sff_cpld_info - Obtain the CPLD register status of the optical module
 * @sff_index: Optical module number, starting from 1
 * @cpld_reg_type: Optical module CPLD register type
 * @buf: Optical module E2 receives information from buf
 * @count: buf length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, char *buf, size_t count);

/**
 * dfd_get_single_eth_optoe_type - get sff optoe type
 * @sff_index: Optical module number, starting from 1
 * @optoe_type: Optical module type
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_get_single_eth_optoe_type(unsigned int sff_index, int *optoe_type);

/**
 * dfd_set_single_eth_optoe_type - set sff optoe type
 * @sff_index: Optical module number, starting from 1
 * @optoe_type: Optical module type
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_set_single_eth_optoe_type(unsigned int sff_index, int optoe_type);

#endif /* _WB_SFF_DRIVER_H_ */

#ifndef _WB_SFF_DRIVER_H_
#define _WB_SFF_DRIVER_H_

#define SFF_OPTOE_TYPE_PATH             "/sys/bus/i2c/devices/%d-0050/dev_class"
#define OPTOE_TYPE_PATH_SIZE            (64)
#define OPTOE_TYPE_RW_LEN               (1)

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

/**
 * dfd_get_single_eth_i2c_bus - get sff i2c_bus
 * @sff_index: Optical module number, starting from 1
 * @buf: Optical module E2 receives information from buf
 * @count: buf length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_get_single_eth_i2c_bus(unsigned int sff_index, char *buf, size_t count);

/**
 * dfd_get_single_eth_power_group - get sff port power group
 * @sff_index: Optical module number, starting from 1
 * power_group: power group id
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_get_single_eth_power_group(unsigned int sff_index, int *power_group);

/**
 * dfd_set_sff_power_group_state - Set the power state of an SFP power group.
 * @group: Power group number, starting from 1.
 * @group_state: Power state to set for the group (0 for off, 1 for on).
 *
 * Returns: 0 on success, or a negative error code on failure.
 */
int dfd_set_sff_power_group_state(int group, int group_state);

/**
 * dfd_get_sff_power_group_state - Obtain the power group state.
 * @group: Power group number, starting from 1.
 * @buf: Buffer to store the power group state information.
 * @count: buf length.
 * return: Success: Returns the length of fill buf.
 *        Failed: A negative value is returned.
 */
ssize_t dfd_get_sff_power_group_state(int group, char *buf, size_t count);
#endif /* _WB_SFF_DRIVER_H_ */

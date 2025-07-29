#ifndef _WB_CPLD_DRIVER_H_
#define _WB_CPLD_DRIVER_H_

/**
 * dfd_get_cpld_name - Obtain the CPLD name
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_name(unsigned int main_dev_id, unsigned int cpld_index, char *buf, size_t count);

/**
 * dfd_get_cpld_type - Obtain the CPLD model
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_type(unsigned int main_dev_id, unsigned int cpld_index, char *buf, size_t count);

/**
 * dfd_get_cpld_vendor - Obtain the CPLD vendor
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_vendor(unsigned int main_dev_id, unsigned int cpld_index, char *buf, size_t count);

/**
 * dfd_get_cpld_fw_version - Obtain the CPLD firmware version
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_fw_version(unsigned int main_dev_id, unsigned int cpld_index, char *buf, size_t count);

/**
 * dfd_get_cpld_hw_version - Obtain the hardware version of the CPLD
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_hw_version(unsigned int main_dev_id, unsigned int cpld_index, char *buf, size_t count);

/**
 * dfd_set_cpld_testreg - Set the CPLD test register value
 * @main_dev_id: Motherboard :0 Subcard :5
 * @cpld_index:The number of the CPLD starts from 0
 * @value: Writes the value of the test register
 * return: Success:0
 *       :Failed: A negative value is returned
 */
int dfd_set_cpld_testreg(unsigned int main_dev_id, unsigned int cpld_index, void *val, unsigned int len);

/**
 * dfd_get_cpld_testreg - Read the CPLD test register value
 * @main_dev_id: Motherboard :0 Subcard :5
 * @cpld_index:The number of the CPLD starts from 0
 * @value: Read the test register value
 * return: Success:0
 *       :Failed: A negative value is returned
 */
int dfd_get_cpld_testreg(unsigned int main_dev_id, unsigned int cpld_index, int *value);

/**
 * dfd_get_cpld_testreg_str - Read the CPLD test register value
 * @main_dev_id: Motherboard :0 Subcard :5
 * @cpld_index: The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_testreg_str(unsigned int main_dev_id, unsigned int cpld_index,
            char *buf, size_t count);

/**
 * dfd_get_cpld_support_upgrade - Obtain the CPLD support upgrade
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_support_upgrade(unsigned int main_dev_id, unsigned int cpld_index, char *buf, size_t count);

/**
 * dfd_get_cpld_upgrade_active_type - Obtain the CPLD upgrade active type
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_upgrade_active_type(unsigned int main_dev_id, unsigned int cpld_index, char *buf, size_t count);


extern dfd_sysfs_func_map_t cpld_func_table[DFD_CPLD_MAX_E];

#endif /* _WB_CPLD_DRIVER_H_ */

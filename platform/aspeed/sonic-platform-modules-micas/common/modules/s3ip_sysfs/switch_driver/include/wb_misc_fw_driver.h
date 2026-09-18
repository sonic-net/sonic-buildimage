#ifndef _WB_MISC_FW_DRIVER_H_
#define _WB_MISC_FW_DRIVER_H_

/**
 * dfd_get_misc_fw_name - Obtain the MISC_FW name
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the MISC_FW starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_misc_fw_name(unsigned int main_dev_id, unsigned int misc_fw_index, char *buf, size_t count);

/**
 * dfd_get_misc_fw_type - Obtain the MISC_FW model
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the MISC_FW starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_misc_fw_type(unsigned int main_dev_id, unsigned int misc_fw_index, char *buf, size_t count);

/**
 * dfd_get_misc_fw_vendor - Obtain the MISC_FW vendor
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the MISC_FW starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_misc_fw_vendor(unsigned int main_dev_id, unsigned int misc_fw_index, char *buf, size_t count);

/**
 * dfd_get_misc_fw_fw_version - Obtain the MISC_FW firmware version
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the MISC_FW starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_misc_fw_fw_version(unsigned int main_dev_id, unsigned int misc_fw_index, char *buf, size_t count);

/**
 * dfd_get_misc_fw_hw_version - Obtain the hardware version of the MISC_FW
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the MISC_FW starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_misc_fw_hw_version(unsigned int main_dev_id, unsigned int misc_fw_index, char *buf, size_t count);

/**
 * dfd_get_misc_fw_support_upgrade - Obtain the MISC_FW support upgrade
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the MISC_FW starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_misc_fw_support_upgrade(unsigned int main_dev_id, unsigned int misc_fw_index, char *buf, size_t count);

/**
 * dfd_get_misc_fw_upgrade_active_type - Obtain the MISC_FW upgrade active type
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the MISC_FW starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_misc_fw_upgrade_active_type(unsigned int main_dev_id, unsigned int misc_fw_index, char *buf, size_t count);


extern dfd_sysfs_func_map_t misc_fw_func_table[DFD_MISC_FW_MAX_E];

#endif /* _WB_MISC_FW_DRIVER_H_ */

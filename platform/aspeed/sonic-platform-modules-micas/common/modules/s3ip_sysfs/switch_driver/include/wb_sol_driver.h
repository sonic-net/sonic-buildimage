#ifndef _WB_SOL_DRIVER_H_
#define _WB_SOL_DRIVER_H_

/**
 * dfd_get_sol_name - get sol info
 * @main_dev_id: not use
 * @sub_dev_id: sol index
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_sol_name(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);


/**
 * dfd_get_sol_device - get sol info
 * @main_dev_id: not use
 * @sub_dev_id: sol index
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_sol_device(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);

/**
 * dfd_get_sol_active - get sol info
 * @main_dev_id: not use
 * @sub_dev_id: sol index
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_sol_active(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);

/**
 * dfd_set_sol_active - Set sol status
 * @sub_dev_id:the index of sol
 * @main_dev_id: not use
 * @val: sol value
 * return: Success: Returns the length of buf
 *       : Faisol: A negative value is returned
 */
int dfd_set_sol_active(unsigned int main_dev_id, unsigned int sub_dev_id, void *val, unsigned int len);

extern dfd_sysfs_func_map_t sol_func_table[DFD_SOL_MAX_E];
#endif /* _WB_SOL_DRIVER_H_ */

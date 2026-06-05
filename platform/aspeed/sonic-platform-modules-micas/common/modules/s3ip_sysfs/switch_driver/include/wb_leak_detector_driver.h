#ifndef _WB_LEAK_DETECTOR_DRIVER_H_
#define _WB_LEAK_DETECTOR_DRIVER_H_

#define LEAK_DETECTOR_SIMULATE_DISABLE      (-1)

typedef enum leak_detector_status_e {
    LEAK_DETECTOR_STATUS_ABSENT = 0,        /* leak detector absent */
    LEAK_DETECTOR_STATUS_OK     = 1,        /* leak detector ok */
    LEAK_DETECTOR_STATUS_NOT_OK = 2,        /* leak detector not ok */
} leak_detector_status_t;

/**
 * dfd_get_leak_detector_status - Get leak_detector and other status
 * @leak_detector_id: leak_detector lamp type
 * @leak_detector_index: leak_detector light offset
 * @buf: leak_detector light status receives buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Faileak_detector: A negative value is returned
 */
ssize_t dfd_get_leak_name(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
ssize_t dfd_get_leak_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
ssize_t dfd_get_leak_simulate_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
int dfd_set_leak_simulate_status(unsigned int main_dev_id, unsigned int sub_dev_id, void *val, unsigned int len);
ssize_t dfd_get_leak_present(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
extern dfd_sysfs_func_map_t leak_func_table[DFD_LEAK_MAX_E];

#endif /* _WB_LEAK_DETECTOR_DRIVER_H_ */

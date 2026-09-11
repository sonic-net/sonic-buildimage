#ifndef __WB_DUMMY_DEV_H__
#define __WB_DUMMY_DEV_H__
#include <wb_logic_dev_common.h>

typedef struct dummy_dev_device_s {
    char dev_name[MAX_NAME_SIZE];
    char dev_alias[MAX_NAME_SIZE];
    char logic_dev_name[MAX_NAME_SIZE];
    uint32_t data_bus_width;
    uint32_t dummy_len;
    uint32_t offset;
    uint32_t logic_func_mode;
    int device_flag;
    uint32_t log_num;                           /* The number of write record registers is 64 at most */
    uint32_t log_index[BSP_KEY_DEVICE_NUM_MAX]; /* Write record register address = (base_addr & 0xFFFF) | (len << 16) */
    device_status_check_t status_check;
} dummy_dev_device_t;

int dummy_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count);
int dummy_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count);

#endif /* __WB_DUMMY_DEV_H__ */

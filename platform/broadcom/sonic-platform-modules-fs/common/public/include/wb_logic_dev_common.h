#ifndef __WB_LOGIC_DEV_COMMON_H__
#define __WB_LOGIC_DEV_COMMON_H__
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/kprobes.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/time.h>

#define mem_clear(data, size) memset((data), 0, (size))

#define SYMBOL_I2C_DEV_MODE          (1)
#define FILE_MODE                    (2)
#define SYMBOL_PCIE_DEV_MODE         (3)
#define SYMBOL_IO_DEV_MODE           (4)
#define SYMBOL_SPI_DEV_MODE          (5)
#define SYMBOL_INDIRECT_DEV_MODE     (6)
#define SYMBOL_PLATFORM_I2C_DEV_MODE (7)
#define SYMBOL_SPI_DEV_ATOMIC_MODE   (8) /* spi atomic mode, use for irq mode access spi dev */

#define WIDTH_1Byte               (1)
#define WIDTH_2Byte               (2)
#define WIDTH_4Byte               (4)
#define MAX_DEV_NUM               (256)
#define MAX_RW_LEN                (256)
#define MAX_NAME_SIZE             (32)

#define CACHE_FILE_PATH           "/tmp/.%s_cache"
#define MASK_FILE_PATH            "/tmp/.%s_mask"

#define LOGIC_DEV_RETRY_TIME      (3)
#define LOGIC_DEV_RETRY_DELAY_MS  (10)
#define LOGIC_DEV_STATUS_OK       (1)
#define LOGIC_DEV_STATUS_NOT_OK   (0)

#define TEST_REG_MAX_NUM          (8)
#define INIT_TEST_DATA            (0x5a)

typedef enum dev_status_check_type_s {
    NOT_SUPPORT_CHECK_STATUS = 0,     /* Not support status check */
    READ_BACK_CHECK = 1,              /* Readback verification */
    READ_BACK_NAGATIVE_CHECK = 2,     /* Readback anti-verification */
} dev_status_check_type_t;

#define BSP_KEY_DEVICE_LOG_SIZE             (256)
#define BSP_KEY_DEVICE_NUM_MAX              (64)
#define BSP_KEY_DEV_INDEX_TO_ADDR(index)    ((index) & 0xffff)
#define BSP_KEY_DEV_INDEX_TO_SIZE(index)    (((index) >> 16) & 0xffff)
#define BSP_LOG_DIR                         "/var/log/bsp_tech/"
#define BSP_LOG_TS_BUFF_SIZE                (32)
#define WB_BSP_LOG_MAX                      (1 * 1024 * 1024)
#define BSP_LOG_DEV_NAME_MAX_LEN            (64)

typedef struct {
    uint32_t log_num;
    uint32_t log_index[BSP_KEY_DEVICE_NUM_MAX];
    struct mutex file_lock;
} wb_bsp_key_device_log_node_t;

#define DEBUG_BUF_MAX_LEN                   (256)

extern struct kobject *logic_dev_kobj;
extern unsigned long (*kallsyms_lookup_name_fun)(const char *name);

typedef int (*device_func_write)(const char *, uint32_t, uint8_t *, size_t);
typedef int (*device_func_read)(const char *, uint32_t, uint8_t *, size_t );

int find_intf_addr(unsigned long *write_intf_addr, unsigned long *read_intf_addr, uint32_t mode);
int cache_value_read(const char *mask_file_path, const char *cache_file_path, uint32_t offset, uint8_t *value, uint32_t width);
int cache_value_write(const char *mask_file_path, const char *cache_file_path, uint32_t offset, uint8_t *value, uint32_t width);
int dev_rw_check(uint8_t *rd_buf, uint8_t *wr_buf, uint32_t len, uint32_t type);
int wb_bsp_key_device_log(char *dev_name, char *log_name, int log_size,
        wb_bsp_key_device_log_node_t *log_node, uint32_t offset, uint8_t *buf, size_t size);
void logic_dev_dump_data(const char *dev_name, uint32_t offset, u8 *val, size_t count, bool read_flag);
#endif

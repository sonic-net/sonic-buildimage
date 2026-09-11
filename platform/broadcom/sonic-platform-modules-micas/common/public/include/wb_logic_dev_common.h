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
#include <wb_bsp_kernel_debug.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/device.h>

#ifndef PRIu64
#define PRIu64 "llu"
#endif

#ifndef PRIX64
#define PRIX64 "llx"
#endif

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
#define MAX_DATA_WIDTH            (8)
#define MAX_DEV_NUM               (64)
#define MAX_RW_LEN                (256)
#define MAX_NAME_SIZE             (32)
#define LOGIC_DEV_DUMP_NAME_LEN   (64)
#define LOGIC_DEV_DUMP_MAX_NUM    (8)
#define LOGIC_DEV_DUMP_REG_END_DEFAULT (0xff)

#define CACHE_FILE_PATH           "/tmp/.%s_cache"
#define MASK_FILE_PATH            "/tmp/.%s_mask"

#define LOGIC_DEV_RETRY_TIME      (3)
#define LOGIC_DEV_RETRY_TIMEOUT   (3000)
#define LOGIC_DEV_RETRY_DELAY_MS  (10)
#define LOGIC_DEV_STATUS_OK       (0)
#define LOGIC_DEV_STATUS_NOT_OK   (1)

static inline int wb_logic_get_retry_time(int retry_time)
{
    return retry_time > 0 ? retry_time : 1;
}

#define TEST_REG_MAX_NUM          (8)
#define INIT_TEST_DATA            (0x5a)
#define MAX_REG_DATA_LEN          (MAX_DATA_WIDTH * TEST_REG_MAX_NUM)

/* Device read/write enable flag */
#define WB_DEV_AVAILABLE_FLAG       (1)
#define WB_DEV_UNAVAILABLE_FLAG     (0)

typedef enum dev_status_check_type_s {
    NOT_SUPPORT_CHECK_STATUS = 0,       /* Not support status check */
    READ_BACK_CHECK = 1,                /* Readback verification */
    READ_BACK_NAGATIVE_CHECK = 2,       /* Readback anti-verification */
    READ_ONLY_CHECK = 3,                /* Read only */
    SUPPORT_CHECK_MAX,
} dev_status_check_type_t;

typedef enum {
    WB_SPIN_LOCK_MODE = 1,
    WB_MUTEX_LOCK_MODE = 2,
} cs_lock_mode_t;

#define BSP_KEY_DEVICE_LOG_SIZE             (256)
#define BSP_KEY_DEVICE_NUM_MAX              (64)
#define BSP_KEY_DEV_INDEX_TO_ADDR(index)    ((index) & 0xffff)
#define BSP_KEY_DEV_INDEX_TO_SIZE(index)    (((index) >> 16) & 0xffff)
#define BSP_LOG_DIR                         "/var/log/bsp_tech/"
#define BSP_LOG_TS_BUFF_SIZE                (32)
#define WB_BSP_LOG_MAX                      (1 * 1024 * 1024)
#define BSP_LOG_DEV_NAME_MAX_LEN            (64)

/* CRAM */
#define CRAM_REG_OFFSET                 (14)
#define CRAM_REG_BYTE_INDEX             (1)
#define CRAM_REG_BYTE_OFFSET            (6)
#define CRAM_REG_MASK                   (0x3)
#define CRAM_STATUS(value)              ((value >> CRAM_REG_OFFSET) & CRAM_REG_MASK)
#define CRAM_STATUS_FROM_BYTE(value)    ((value[CRAM_REG_BYTE_INDEX] >> CRAM_REG_BYTE_OFFSET) & CRAM_REG_MASK)
#define CRAM_CE_STATUS                  (0x1)
#define CRAM_UE_STATUS                  (0x2)
#define CRAM_STATUS_OK                  (0)
#define CRAM_ALL_BITS_SET               (~0)
#define CRAM_CONFIG_MASK_MIN_COUNT      (1)
#define CRAM_ERR_ADDR_DATA_OFFSET       (2)
#define CRAM_ERR_ADDR_NUM_DEF           (1)
#define CRAM_ERR_ADDR_REG_MASK_DEF      (0xfc)
#define CRAM_STATUS_SINGLE_BIT_ERROR          (0x1)
#define CRAM_STATUS_MULTI_BIT_ERROR           (0x2)
#define CRAM_STATUS_SINGLE_MULTI_BIT_ERROR    (0x3)
#define CRAM_DETAIL_SINGLE_BIT_ERROR    "SingleBit_Error"
#define CRAM_DETAIL_MULTI_BIT_ERROR     "MultiBit_Error"
#define CRAM_DETAIL_LOGICDEV_ERROR      "LogicDevRead_Error"


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
typedef int (*get_pci_dev_func)(const char *, struct pci_dev **);
typedef int (*wb_cs_func)(uint32_t cs_index);

typedef struct logic_dev_dump_cfg_s {
    char logic_dev_name[LOGIC_DEV_DUMP_NAME_LEN];
    int logic_dev_func_mode;
    int logic_dev_reg_start;
    int logic_dev_reg_end;
} logic_dev_dump_cfg_t;

typedef struct logic_dev_dump_s {
    const char *logic_dev_name;
    uint32_t logic_dev_func_mode;
    uint32_t logic_dev_reg_start;
    uint32_t logic_dev_reg_end;
    unsigned long logic_dev_write_intf_addr;
    unsigned long logic_dev_read_intf_addr;
} logic_dev_dump_t;

typedef struct logic_dev_dump_cfg_info_s {
    uint32_t dump_logic_dev_num;
    logic_dev_dump_cfg_t dump_logic_dev_cfg[LOGIC_DEV_DUMP_MAX_NUM];
} logic_dev_dump_cfg_info_t;

typedef struct logic_dev_dump_info_s {
    uint32_t dump_logic_dev_num;
    logic_dev_dump_t dump_logic_dev_cfg[LOGIC_DEV_DUMP_MAX_NUM];
} logic_dev_dump_info_t;

int find_intf_addr(unsigned long *write_intf_addr, unsigned long *read_intf_addr, uint32_t mode);
int cache_value_read(const char *mask_file_path, const char *cache_file_path, uint32_t offset, uint8_t *value, uint32_t width);
int cache_value_write(const char *mask_file_path, const char *cache_file_path, uint32_t offset, uint8_t *value, uint32_t width);
int dev_rw_check(uint8_t *rd_buf, uint8_t *wr_buf, uint32_t len, uint32_t type, uint32_t check_mode);
int wb_bsp_key_device_log(char *dev_name, char *log_name, int log_size,
        wb_bsp_key_device_log_node_t *log_node, uint32_t offset, uint8_t *buf, size_t size);
void logic_dev_dump_data(const char *dev_name, uint32_t offset, u8 *val, size_t count, bool read_flag);
int find_cs_intf_addr(unsigned long *cs_enable_intf_addr, unsigned long *cs_disable_intf_addr);
int logic_dev_dump_of_node_init(logic_dev_dump_info_t *dump_info, struct device *dev);
int logic_dev_dump_cfg_info_init(logic_dev_dump_info_t *dump_info, struct device *dev,
    const logic_dev_dump_cfg_info_t *dump_logic_dev_cfg_info);
void logic_dev_dump_regs(logic_dev_dump_info_t *dump_info, struct device *dev, const char *log_tag);

typedef enum logic_dev_status_check_type_e {
    STATUS_CHECK_SEU_E = 0,
    STATUS_CHECK_SELFTEST_E = 1,
    STATUS_CHECK_SCRATCH_E = 2,
    STATUS_CHECK_CRAM_E = 3,
    STATUS_CHECK_TYPE_MAX,
} logic_dev_status_check_type_t;

typedef enum dev_status_check_mode_e {
    WB_LOGIC_NORMAL = 0,
    WB_LOGIC_NAGATIVE_MODE = 1,                /* anti-verification */
    WB_LOGIC_SUPPORT_CHECK_MODE_MAX,
} dev_status_check_mode_t;

typedef enum logic_dev_cram_check_type_e {
    CRAM_CHECK_CE_E = 0,
    CRAM_CHECK_UE_E = 1,
    CRAM_TYPE_MAX,
} logic_dev_cram_check_type_t;

/* status check function */
#define STATUS_CHECK_BITS(type)         (1UL << type)
#define STATUS_CHECK_SEU                STATUS_CHECK_BITS(STATUS_CHECK_SEU_E)
#define STATUS_CHECK_SELFTEST           STATUS_CHECK_BITS(STATUS_CHECK_SELFTEST_E)
#define STATUS_CHECK_SCRATCH            STATUS_CHECK_BITS(STATUS_CHECK_SCRATCH_E)
#define STATUS_CHECK_CRAM               STATUS_CHECK_BITS(STATUS_CHECK_CRAM_E)

typedef struct reg_check {
    u32 reg;
    uint8_t value[MAX_DATA_WIDTH];
    uint8_t mask[MAX_DATA_WIDTH];
} reg_check_t;

struct seu_reg_check {
    u32 reg;
    uint8_t value[MAX_DATA_WIDTH];
    uint8_t mask[MAX_DATA_WIDTH];
};

struct selftest_check {
    u32 reg;
    uint8_t write_value[MAX_DATA_WIDTH];
    u32 check_mode;                         /* write mode */
};

struct scratch_check {
    u32 reg;
    uint8_t value[MAX_DATA_WIDTH];
    bool initialized;
    u32 check_mode;                         /* write mode */
};

struct cram_check {
    u32 reg;
    u32 cram_err_addr_reg_num;
    u32 cram_err_addr_reg[TEST_REG_MAX_NUM];
    u32 cram_err_addr_reg_mask[TEST_REG_MAX_NUM];
};

struct device_bus_ops {
    device_func_read read;
    device_func_write write;
};

typedef struct device_status_check {
    u32 status_check_type_bmp;
    char dev_name[MAX_NAME_SIZE];

    u32 seu_check_num;
    struct seu_reg_check seu_checks[TEST_REG_MAX_NUM];

    u32 selftest_check_num;
    struct selftest_check selftest_checks[TEST_REG_MAX_NUM];

    u32 scratch_num;
    struct scratch_check scratch_checks[TEST_REG_MAX_NUM];

    struct cram_check cram_checks;

    struct device_bus_ops dev_bus_ops;
    unsigned long last_jiffies;             /* The number of jiffies when the device status was last obtained */
    u32 dev_status;                         /* Total status of check type */
    unsigned long type_last_jiffies[STATUS_CHECK_TYPE_MAX];             /* last jiffies by check type */
    u32 type_dev_status[STATUS_CHECK_TYPE_MAX];                         /* status by check type */
} device_status_check_t;

int wb_logic_check_status_hw_init(struct device_status_check *status_check,
                                     uint32_t data_byte_len);

int wb_logic_dev_get_status(struct device_status_check *status_check,
                                     uint32_t data_byte_len,
                                     char *buf,
                                     uint32_t buf_len);

uint32_t wb_logic_status_type_get_number(uint32_t type);
int wb_logic_dev_get_status_with_type(struct device_status_check *status_check,
                                     uint32_t data_byte_len,
                                     char *buf,
                                     uint32_t buf_len,
                                     uint32_t type);

int of_dev_status_check_config_init(struct device *dev,
                                                struct device_status_check *status_check,
                                                uint32_t data_byte_len,
                                                device_func_write read,
                                                device_func_write write,
                                                const char *dev_name);

int platform_dev_status_check_config_init(struct device *dev,
                                                        struct device_status_check *status_check,
                                                        uint32_t data_byte_len,
                                                        struct device_status_check *status_check_data,
                                                        device_func_write read,
                                                        device_func_write write,
                                                        const char *dev_name);

int wb_logic_get_cache_status_with_type(struct device_status_check *status_check,
                                                                uint32_t check_type,
                                                                uint32_t status_cache_ms,
                                                                char *buf,
                                                                uint32_t buf_len);

int wb_logic_clear_status(struct device_status_check *status_check,
                                     uint32_t data_byte_len,
                                     uint32_t type_bmp);

/* end status check function */
void sleep_by_lock_mode(int sleep_time, uint32_t lock_mode);

int wb_logic_lock_mode_check(uint32_t lock_mode, uint32_t func_mode);

#endif

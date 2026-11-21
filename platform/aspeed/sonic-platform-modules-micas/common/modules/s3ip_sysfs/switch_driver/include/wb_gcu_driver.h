#ifndef _WB_GCU_DRIVER_H_
#define _WB_GCU_DRIVER_H_

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"

#define GCU_MAX_NUMBER                              (64)
#define INFO_FPATH_MAX_LEN                          (128)
#define L600_900_GCU_HEALTH_WARNING                 (0x4000)
#define L600_900_GCU_HEALTH_CRITICAL                (0x8000)
#define ORDER_DIRECTION_MASK                        (0x00C0)
#define L600_MAPPING_COUNT                          (2)
#define L600_HEALTH_RULES_COUNT                     (3)
#define L900_MAPPING_COUNT                          (2)
#define L900_HEALTH_RULES_COUNT                     (3)

/**
 * @brief Generate GCU key1 by combining gcu_type and gcu_id
 * @param gcu_type GCU type (16-bit value)
 * @param gcu_id GCU identifier (8-bit value)
 * @return 32-bit key where:
 *         - Bits [31:16] = gcu_type
 *         - Bits [15:0]  = gcu_id
 */
#define DFD_GET_GCU_KEY1(gcu_type, gcu_id) \
    ((((gcu_type) & 0xffff) << 16) | ((gcu_id) & 0xff))

/**
 * @brief Generate GCU key2 by combining function and sensor index
 * @param func Function code (8-bit value)
 * @param sensor_index Sensor index (8-bit value)
 * @return 16-bit key where:
 *         - Bits [15:8] = func
 *         - Bits [7:0]  = sensor_index
 */
#define DFD_GET_GCU_KEY2(func, sensor_index) \
    ((((func) & 0xff) << 8) | ((sensor_index) & 0xff))

/**
 * @brief Generate common GCU key2 by combining gcu_id and function
 * @param gcu_id GCU identifier (8-bit value)
 * @param func Function code (8-bit value)
 * @return 16-bit key where:
 *         - Bits [15:8] = gcu_id
 *         - Bits [7:0]  = func
 */
#define DFD_GET_GCU_COMMON_KEY2(gcu_id, func) \
        ((((gcu_id) & 0xff) << 8) | ((func) & 0xff))

/**
 * @brief Extract GCU ID from main device ID
 * @param main_dev_id Main device identifier (32-bit value)
 * @return 16-bit GCU ID extracted from bits [31:16] of main_dev_id
 */
#define DFD_GET_SENSOR_GCUID(main_dev_id)  \
        (((main_dev_id) >> 16) & 0xffff)

/**
 * @brief Extract SENSOR ID from main device ID
 * @param main_dev_id Main device identifier (32-bit value)
 * @return 16-bit SENSOR ID extracted from bits [15:0] of main_dev_id
 */
#define DFD_GET_GCU_SENSOR_ID(main_dev_id)  \
        ((main_dev_id) & 0xffff)

#define DFD_COM_GCU_SENSOR_MAIN_DEV_ID(gcu_index, sensor_index)  \
        ((gcu_index) << 16 | (sensor_index))

#define DFD_GCU_TYPE_IS_VALID(gcu_type)  \
        (((gcu_type) < WB_MAIN_GCU_DEV_MAX) && ((gcu_type) >= 0))

#define CHECK_BITS_ALL(val, mask) ({ \
    bool __ret = true;               \
    size_t __i;                      \
    uint8_t __target;                \
    for (__i = 0; __i < sizeof(mask); __i++) { \
        __target = (mask >> (__i * 8)) & 0xFF; \
        if ((((uint8_t*)(val))[__i] & __target) != __target) { \
            __ret = false;           \
            break;                   \
        } \
    } \
    __ret;                           \
})

#define IS_TARGET_SENSOR_STATIC(x) \
    ((x) == DFD_BMC_MANAGED_SENSOR_ALIAS_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_MAX_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_MIN_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_HIGH_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_LOW_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_NOTICE_HIGH_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_NOTICE_LOW_E)

typedef struct {
    uint8_t reg_value;
    const char *str;
} gcu_para_map_t;

typedef struct {
    const char *name;
    int offset;
    int size;
    unsigned int base;
    const char *map_name;
} gcu_reg_attr_t;

typedef struct {
    char *name;
    const gcu_para_map_t *map;
    size_t map_size;
} gcu_map_t;

typedef enum {
    PCIE_RATE_MAP_SIZE = 6,
    PCIE_WIDTH_MAP_SIZE = 6,
} pcie_para_num_t;

static const gcu_para_map_t default_pcie_rate_map[PCIE_RATE_MAP_SIZE] = {
    {0x00,  "0"},
    {0x01,  "Gen1"},
    {0x02,  "Gen2"},
    {0x03,  "Gen3"},
    {0x04,  "Gen4"},
    {0x05,  "Gen5"},
};

static const gcu_para_map_t default_pcie_width_map[PCIE_WIDTH_MAP_SIZE] = {
    {0x0,   "0"},
    {0x01,  "x1"},
    {0x02,  "x2"},
    {0x04,  "x4"},
    {0x08,  "x8"},
    {0x10,  "x16"},
};

typedef enum {
    HEALTH_GOOD            = (0),
    HEALTH_PERI_WARNING    = (1 << 0),
    HEALTH_PCIE_WARNING    = (1 << 1),
    HEALTH_HBM_WARNING     = (1 << 2),
    HEALTH_PERI_CRITICAL   = (1 << 3),
    HEALTH_PCIE_CRITICAL   = (1 << 4),
    HEALTH_HBM_CRITICAL    = (1 << 5),
} health_status_mask_t;

typedef enum {
    BINARY_BASE         = (1 << 1),
    OCTAL_BASE          = (1 << 2),
    DECIMAL_BASE        = (1 << 3),
    HEX_BASE            = (1 << 4),
    ASCII_BASE          = (1 << 5),
    FORWARD_BASE        = (1 << 6),
    REVERSE_BASE        = (1 << 7),
    MAPPING_BASE        = (1 << 8),
    X1000_BASE          = (1 << 9),
    X1000000_BASE       = (1 << 10),
} gcu_base_type;

typedef enum {
    GCU_KEY_MODE_SENSOR,
    GCU_KEY_MODE_COMMON
} gcu_key_mode_t;

typedef struct {
    gcu_device_type         error_type;         /* Error category */
    unsigned int            warning_bitmask;    /* Bitmask for warning level errors */
    unsigned int            critical_bitmask;   /* Bitmask for critical level errors */
    health_status_mask_t    warning_status;     /* Status flag for warning level */
    health_status_mask_t    critical_status;    /* Status flag for critical level */
} health_check_rule_t;

typedef struct {
    const char          *name;
    uint16_t            gcu_main_id;
    uint8_t             ven_addr;
    uint8_t             dev_addr;
    unsigned int        size;
    uint16_t            vendor_id;
    uint16_t            device_id;
    gcu_reg_attr_t     *attr_table;
    gcu_reg_attr_t     *temp_attr_table;
    unsigned int        temp_number;
    gcu_reg_attr_t     *vol_attr_table;
    unsigned int        vol_number;
    gcu_reg_attr_t     *power_attr_table;
    unsigned int        power_number;
    health_check_rule_t *health_rules;
    unsigned int        health_rule_number;
    const gcu_map_t     *gcu_map;
    unsigned int        gcu_map_number;
    dfd_sysfs_func_map_t    *gcu_attr_funcs;
    unsigned int            gcu_attr_funcs_size;
    dfd_sysfs_func_map_t    *gcu_temp_funcs;
    unsigned int            gcu_temp_funcs_size;
    dfd_sysfs_func_map_t    *gcu_vol_funcs;
    unsigned int            gcu_vol_funcs_size;
    dfd_sysfs_func_map_t    *gcu_power_funcs;
    unsigned int            gcu_power_funcs_size;
} gcu_list_t;

typedef struct {
    int gcu_type;
    int monitor_last_flag;
    int monitor_curr_flag;
} gcu_id_info_t;

typedef struct {
    char fpath[INFO_FPATH_MAX_LEN];
} gcu_monitor_info_t;

ssize_t dfd_get_gcu_alias(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
ssize_t dfd_get_gcu_attr_common(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count, char *map_table_name);
int find_gpu_type_via_single_i2c(unsigned int slot);
int data_to_buf(char *buf, ssize_t buf_size, uint8_t *val, int valid_size, unsigned int gcu_type, unsigned int mode, char *map_name);
int get_gcu_para_val(unsigned int main_dev_id, unsigned int sub_dev_id,
                          unsigned int sensor_index, int gcu_type,
                          char *val, char *buf, gcu_reg_attr_t func_attr, unsigned int count);
int get_gcu_temp_para_val(unsigned int main_dev_id, unsigned int sub_dev_id,
                               unsigned int sensor_index, int gcu_type,
                               char *val, char *buf, gcu_reg_attr_t func_attr, unsigned int count);
int get_gcu_vol_para_val(unsigned int main_dev_id, unsigned int sub_dev_id,
                               unsigned int sensor_index, int gcu_type,
                               char *val, char *buf, gcu_reg_attr_t func_attr, unsigned int count);
int get_gcu_power_para_val(unsigned int main_dev_id, unsigned int sub_dev_id,
                               unsigned int sensor_index, int gcu_type,
                               char *val, char *buf, gcu_reg_attr_t func_attr, unsigned int count);
int dfd_check_gcu_type_is_valid(unsigned int main_dev_id, char *buf);

extern gcu_id_info_t gcu_id_info_table[GCU_MAX_NUMBER];
extern gcu_list_t gcu_list[WB_MAIN_GCU_DEV_MAX];
extern int g_dfd_gcu_dbg_level;
#endif /* _WB_GCU_DRIVER_H_ */
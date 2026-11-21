#ifndef __WB_CHIP_SELECT_H__
#define __WB_CHIP_SELECT_H__

#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <wb_logic_dev_common.h>

#define MAX_CS_SIZE          (64)
#define WAIT_STATUS_DELAY    (10000)   /* us */

/* Chip select operation types */
typedef enum {
    CS_OP_TYPE_GPIO,
    CS_OP_TYPE_LOGIC_DEV,
} cs_op_type_t;

/* GPIO-based chip select configuration */
typedef struct cs_gpio_config_s {
    uint32_t gpio_index;
    const char *controller_name;
    uint32_t gpio_offset;
    int en_level;
    int dis_level;
} cs_gpio_config_t;

/* Logic device-based chip select configuration */
typedef struct cs_logic_dev_config_s {
    uint32_t dev_mode;
    const char *dev_name;
    uint32_t reg_addr;
    uint32_t en_value;
    uint32_t dis_value;
    uint32_t mask;
    uint32_t width;
    unsigned long write_intf_addr;
    unsigned long read_intf_addr;
} cs_logic_dev_config_t;

/* Generic chip select device structure */
typedef struct chip_select_device_s {
    uint32_t cs_index;
    cs_op_type_t op_type;
    uint32_t lock_mode;
    union {
        cs_gpio_config_t gpio_config;
        cs_logic_dev_config_t logic_dev_config;
    } attr;
    int device_flag;
} chip_select_device_t;

typedef struct wb_chip_select_cfg_info_s {
    uint32_t cs_index;
    uint32_t op_type;
    uint32_t lock_mode;
    void *cs_lock;
    unsigned long lock_flags;
    union {
        cs_gpio_config_t gpio_config;
        cs_logic_dev_config_t logic_dev_config;
    } attr;
} wb_chip_select_cfg_info_t;

struct cs_lock_pool {
    union {
        spinlock_t spin_lock;
        struct mutex mutex_lock;
    } lock;
    cs_op_type_t op_type;
    uint32_t lock_mode;
    uint32_t user_count;
    union {
        struct {
            uint32_t gpio_offset;
        } gpio;
        struct {
            char dev_name[MAX_NAME_SIZE];
            uint8_t reg_addr;
        } logic;
    } key;
    struct list_head list;
};

int chip_select_enable(uint32_t cs_index);
int chip_select_disable(uint32_t cs_index);

#endif /* __WB_CHIP_SELECT_H__ */

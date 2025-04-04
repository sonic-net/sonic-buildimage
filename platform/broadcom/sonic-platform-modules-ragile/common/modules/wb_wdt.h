#ifndef __WB_WDT_H__
#define __WB_WDT_H__

#include <linux/of_gpio.h>

#define SYSFS_NO_CFG         (0xff)

typedef struct gpio_wdt_info_s {
    int       gpio;
    enum of_gpio_flags  flags;
    bool      active_low;
    bool      state;
}gpio_wdt_info_t;

typedef struct logic_wdt_info_s {
    const char  *feed_dev_name;
    uint8_t     logic_func_mode;
    uint32_t    feed_reg;
    uint8_t     active_val;
    uint8_t     state_val;
}logic_wdt_info_t;

typedef struct wb_wdt_device_s {
    int         device_flag;
    const char  *config_dev_name;
    uint8_t     config_mode;
    const char  *hw_algo;
    uint8_t     enable_val;
    uint8_t     disable_val;
    uint8_t     enable_mask;
    uint8_t     priv_func_mode;
    uint8_t     feed_wdt_type;
    uint32_t    enable_reg;
    uint32_t    timeout_cfg_reg;
    uint32_t    timeleft_cfg_reg;
    uint32_t    hw_margin;
    uint32_t    feed_time;
    uint32_t    timer_accuracy;
    union {
        gpio_wdt_info_t gpio_wdt;
        logic_wdt_info_t logic_wdt;
    } wdt_config_mode;
    uint8_t sysfs_index;
} wb_wdt_device_t;

#endif

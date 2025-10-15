#ifndef __WB_WDT_H__
#define __WB_WDT_H__

#include <linux/of_gpio.h>

#define INVALID_REG_ADDR     (0xffffffff)

#define GPIO_FEED_WDT_MODE   (1)
#define LOGIC_FEED_WDT_MODE  (2)

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
    unsigned long write_intf_addr;
    unsigned long read_intf_addr;
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
    uint8_t     timer_accuracy_reg_flag;
    uint32_t    timer_accuracy_reg;
    uint8_t     timer_accuracy_reg_val;
    uint32_t    timer_accuracy;
    uint8_t     timer_update_reg_flag;
    uint32_t    timer_update_reg;
    uint8_t     timer_update_reg_val;
    union {
        gpio_wdt_info_t gpio_wdt;
        logic_wdt_info_t logic_wdt;
    } wdt_config_mode;
    uint32_t wdt_ready;                     /* 0: invalid; 1: ready; 2: no ready */
} wb_wdt_device_t;

typedef enum {
    PARENT_WDT       = 0,
    SECOND_LOGIC_WDT = 1,
} wb_wdt_type_t;

enum timer_debug_s {
    GPIO_PING_DEBUG = 0,
    NMI_PING_DEBUG,
    HRTIMER_PING_DEBUG,
    THREAD_PING_DEBUG,
};

#endif

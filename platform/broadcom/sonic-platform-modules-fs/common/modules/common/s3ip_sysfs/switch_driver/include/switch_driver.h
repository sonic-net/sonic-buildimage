#ifndef _SWITCH_DRIVER_H_
#define _SWITCH_DRIVER_H_

#define SWITCH_DEV_NO_SUPPORT         "NA"
#define SWITCH_DEV_ERROR              "ACCESS FAILED"
#define SWITCH_DEV_NO_CFG             "NO_CFG"

/* Used in the bitmap buff */
#define SWITCH_BIT_DEV_NO_SUPPORT         "N"
#define SWITCH_BIT_DEV_ERROR              "E"
#define SWITCH_BIT_NOT_CFG                "-"

#define WB_SYSFS_RV_UNSUPPORT         (999)

typedef enum dbg_level_e {
    DBG_VERBOSE = 0x01,
    DBG_WARN    = 0x02,
    DBG_ERROR   = 0x04,
} dbg_level_t;

typedef enum fan_status_e {
    FAN_STATUS_ABSENT  = 0,
    FAN_STATUS_OK      = 1,
    FAN_STATUS_NOT_OK  = 2,
} fan_status_t;

typedef enum led_status_e {
    LED_STATUS_DARK         = 0,
    LED_STATUS_GREEN        = 1,
    LED_STATUS_YELLOW       = 2,
    LED_STATUS_RED          = 3,
    LED_STATUS_BLUE         = 4,
    LED_STATUS_GREEN_FLASH  = 5,
    LED_STATUS_YELLOW_FLASH = 6,
    LED_STATUS_RED_FLASH    = 7,
} led_status_t;

typedef enum air_flow_direction_e {
    F2B = 0, /* air enters from the front of the cabinet, and exhausts from the back */
    B2F = 1, /* air enters from the back of the cabinet, and exhausts from the front */
} air_flow_direction_t;

typedef enum dev_status_e {
    DEV_ABSENT  = 0, /* dev absent */
    DEV_PRESENT = 1, /* dev present */
} dev_status_t;

typedef enum value_equal_to_extra1_e {
    DFD_CONFIG_NOT_EQUAL_TO_EXTRA1   = 0,
    DFD_CONFIG_EQUAL_TO_EXTRA1       = 1,
    DFD_CONFIG_IGNORE_TO_EXTRA1      = 2,
} value_equal_to_extra1_t;

extern int g_switch_dbg_level;

#define SWITCH_DEBUG(level, fmt, arg...) do { \
    if (g_switch_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#endif  /* _SWITCH_DRIVER_H_ */

#ifndef _SYSLED_SYSFS_H_
#define _SYSLED_SYSFS_H_

#define BMC_SYS_LED_STATUS_MAX   8
#define HOST_SYS_LED_COUNT_CHECK  10
#define WB_MAX_S3IP_NODE_STR_LEN 128
/* the followings are the color value of s3ip. */
typedef enum s3ip_led_status_e {
    S3IP_LED_STATUS_DARK         = 0,
    S3IP_LED_STATUS_GREEN        = 1,
    S3IP_LED_STATUS_YELLOW       = 2,
    S3IP_LED_STATUS_RED          = 3,
    S3IP_LED_STATUS_BLUE         = 4,
    S3IP_LED_STATUS_GREEN_FLASH  = 5,
    S3IP_LED_STATUS_YELLOW_FLASH = 6,
    S3IP_LED_STATUS_RED_FLASH    = 7,
} s3ip_led_status_t;

typedef enum {
    SYS_LED_PRIORITY_MIN = 0, /* high priority */
    SYS_LED_PRIORITY_GREEN_4HZ_FLASH = 1,
    SYS_LED_PRIORITY_RED = 2,
    SYS_LED_PRIORITY_YELLOW = 3,
    SYS_LED_PRIORITY_DARK = 4,
    SYS_LED_PRIORITY_GREEN = 5,
    SYS_LED_PRIORITY_MAX = 6, /* lower priority */
} sys_led_priority_t;

typedef struct {
    int sys_led_value;
    int sys_led_priority;
} sys_led_info_t;

struct s3ip_sysfs_sysled_drivers_s {
    ssize_t (*get_sys_led_status)(char *buf, size_t count);
    int (*set_sys_led_status)(int status);
    ssize_t (*get_bmc_led_status)(char *buf, size_t count);
    int (*set_bmc_led_status)(int status);
    ssize_t (*get_sys_fan_led_status)(char *buf, size_t count);
    int (*set_sys_fan_led_status)(int status);
    ssize_t (*get_sys_psu_led_status)(char *buf, size_t count);
    int (*set_sys_psu_led_status)(int status);
    ssize_t (*get_id_led_status)(char *buf, size_t count);
    int (*set_id_led_status)(int status);
    ssize_t (*get_bmc_host_sysled)(char *buf, size_t count);
    int (*set_bmc_host_sysled_attr)(int status);
};

extern int s3ip_sysfs_sysled_drivers_register(struct s3ip_sysfs_sysled_drivers_s *drv);
extern void s3ip_sysfs_sysled_drivers_unregister(void);
#endif /*_SYSLED_SYSFS_H_ */

#ifndef _SYSTEM_SYSFS_H_
#define _SYSTEM_SYSFS_H_

#define MEM_ISOLATION_ENABLE    1
#define MEM_ISOLATION_DISABLE   0

struct s3ip_sysfs_system_drivers_s {
    ssize_t (*get_system_value)(unsigned int type, char *buf, size_t count);
    ssize_t (*get_system_value_decode_string)(unsigned int type, char *buf, size_t count);
    ssize_t (*set_system_value)(unsigned int type, int value);
    ssize_t (*get_system_port_power_status)(unsigned int type, char *buf, size_t count);
    ssize_t (*get_bmc_view)(char *buf, size_t count);
    ssize_t (*set_bmc_switch)(const char* buf, size_t count);
    ssize_t (*get_bmc_dualboot_wdt_status)(char *buf, size_t count);
    ssize_t (*set_bmc_dualboot_wdt)(const char* buf, size_t count);
    ssize_t (*get_my_slot_id)(char *buf, size_t count);
    ssize_t (*get_bmc_status)(int bmc_sw_status,char *buf, size_t count);
    ssize_t (*get_system_serial_number)(char *buf, size_t count);
    ssize_t (*get_mem_isolation_value)(unsigned int type, char *buf, size_t count);
    ssize_t (*set_mem_isolation_value)(unsigned int type, int value);
};

extern int s3ip_sysfs_system_drivers_register(struct s3ip_sysfs_system_drivers_s *drv);
extern void s3ip_sysfs_system_drivers_unregister(void);

typedef enum module_bmc_status_e {
    BMC_ABSENT    = 0,
    BMC_OK        = 1,
    BMC_UNKNOWN   = 2,
    BMC_NOTICE    = 3,
    BMC_WARN      = 4,
    BMC_ERROR     = 5,
    BMC_OTHER_STATUS,
} module_bmc_status_t;

typedef enum module_bmc_sw_status_e {
    BMC_SW_UNKNOWN        = 0,
    BMC_SW_OK            = 1,
    BMC_SW_NOTICE        = 2,
    BMC_SW_WARN          = 3,
    BMC_SW_ERROR         = 4,
    BMC_SW_OTHER_STATUS,
} module_bmc_sw_status_t;

#endif /*_SYSTEM_SYSFS_H_ */

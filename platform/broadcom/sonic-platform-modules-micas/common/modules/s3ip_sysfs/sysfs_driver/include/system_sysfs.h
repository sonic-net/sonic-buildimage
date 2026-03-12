#ifndef _SYSTEM_SYSFS_H_
#define _SYSTEM_SYSFS_H_

struct s3ip_sysfs_system_drivers_s {
    ssize_t (*get_system_value)(unsigned int type, int *value);
    ssize_t (*set_system_value)(unsigned int type, int value);
};

extern int s3ip_sysfs_system_drivers_register(struct s3ip_sysfs_system_drivers_s *drv);
extern void s3ip_sysfs_system_drivers_unregister(void);

/* system api type */
typedef enum wb_plat_system_type_e {
    WB_SYSTEM_BMC_READY         = 0x0000,  /* bmc ready         */
    WB_SYSTEM_SOL_ACTIVE        = 0x0100,  /* sol active        */
    WB_SYSTEM_PSU_RESET         = 0x0200,  /* psu reset         */
    WB_SYSTEM_CPU_BOARD_CTRL    = 0x0300,  /* cpu board control */
    WB_SYSTEM_CPU_BOARD_STATUS  = 0x0400,  /* cpu board status  */
    WB_SYSTEM_BIOS_UPGRADE      = 0x0500,  /* bios upgrade      */
    WB_SYSTEM_BIOS_SWITCH       = 0x0600,  /* bios switch       */
    WB_SYSTEM_BIOS_VIEW         = 0x0700,  /* bios flash view   */
    WB_SYSTEM_BIOS_BOOT_OK      = 0x0800,  /* bios boot status  */
    WB_SYSTEM_BIOS_FAIL_RECORD  = 0x0900,  /* bios startup failure record */
    WB_SYSTEM_BMC_RESET         = 0x0a00,  /* bmc reset         */
} wb_plat_system_type_t;

#endif /*_SYSTEM_SYSFS_H_ */

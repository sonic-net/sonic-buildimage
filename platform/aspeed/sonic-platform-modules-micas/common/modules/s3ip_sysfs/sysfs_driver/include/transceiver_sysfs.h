#ifndef _TRANSCEIVER_SYSFS_H_
#define _TRANSCEIVER_SYSFS_H_

#include <wb_platform_common.h>

struct s3ip_sysfs_transceiver_drivers_s {
    int (*get_eth_number)(void);
    ssize_t (*get_transceiver_power_on_status)(char *buf, size_t count);
    int (*set_transceiver_power_on_status)(int status);
    ssize_t (*get_sff_power_status_mix_value)(char *buf, size_t count);
    ssize_t (*get_transceiver_present_status)(char *buf, size_t count);
    ssize_t (*get_eth_power_on_status)(unsigned int eth_index, char *buf, size_t count);
    int (*set_eth_power_on_status)(unsigned int eth_index, int status);
    ssize_t (*get_eth_present_status)(unsigned int eth_index, char *buf, size_t count);
    ssize_t (*get_eth_attr)(unsigned int eth_index, unsigned int type, char *buf, size_t count);
    int (*set_eth_attr)(unsigned int eth_index, unsigned int type, int status);
    int (*get_eth_eeprom_size)(unsigned int eth_index);
    ssize_t (*read_eth_eeprom_data)(unsigned int eth_index, char *buf, loff_t offset, size_t count, int upgrade_flag);
    ssize_t (*write_eth_eeprom_data)(unsigned int eth_index, char *buf, loff_t offset, size_t count, int upgrade_flag);
    ssize_t (*get_eth_optoe_type)(unsigned int sff_index, int *optoe_type, char *buf, size_t count);
    ssize_t (*set_eth_optoe_type)(unsigned int sff_index, int optoe_type);
    ssize_t (*get_eth_dev_available)(unsigned int eth_index, char *buf, size_t count);
    ssize_t (*set_eth_dev_available)(unsigned int eth_index, int status);
    ssize_t (*get_eth_i2c_bus)(unsigned int eth_index, char *buf, size_t count);
    ssize_t (*get_eth_cage_type)(unsigned int eth_index, char *buf, size_t count);
    ssize_t (*get_eth_power_group)(unsigned int eth_index, int *power_group);
    ssize_t (*get_eth_e2_low_page)(unsigned int eth_index, char *buf, size_t count);
    ssize_t (*get_eth_temp)(unsigned int eth_index, char *buf, size_t count);
    ssize_t (*get_eth_port_led_status)(unsigned int eth_index, char *buf, size_t count);
    ssize_t (*set_eth_port_led_status)(unsigned int eth_index, int data);
    ssize_t (*get_sw_init_enable)(unsigned int type, char *buf, size_t count);
    ssize_t (*set_sw_init_enable)(unsigned int type,  int value);
    ssize_t (*set_main_board_debug_sff_attr)(unsigned int type, unsigned int sff_index, const char *value);
    ssize_t (*get_eth_port_bus_status)(unsigned int eth_index, char *buf, size_t count);
};

extern int s3ip_sysfs_sff_drivers_register(struct s3ip_sysfs_transceiver_drivers_s *drv);
extern void s3ip_sysfs_sff_drivers_unregister(void);
extern ssize_t eth_debug_present_get(char *buf);
extern ssize_t eth_debug_present_set(const char* buf, size_t count);
#define SINGLE_TRANSCEIVER_PRESENT_DEBUG_FILE       WB_HOST_MISC_DIR ".present_eth_%d"

#define MODULE_STATUS_ABSENT       (0)

#define SFP_TEMP_OFFSET             (256 + 96)


#define QSFP_TEMP_OFFSET            (22)
#define QSFP_TX_FAULT_OFFSET        (4)
#define QSFP_TX_DISABLE_OFFSET      (86)
#define QSFP_RX_LOS_OFFSET          (3)
#define QSFP_LP_MODE_OFFSET         (93)


#define CMIS_TEMP_OFFSET            (14)
#define CMIS_LP_MODE_OFFSET         (26)
#define CMIS_TX_FAULT_OFFSET        (17*128 + 135)
#define CMIS_TX_DISABLE_OFFSET      (16*128 + 130)
#define CMIS_RX_LOS_OFFSET          (17*128 + 147)

#define CMIS_TX_FLAG_SUP_OFFSET     (1*128 + 157)
#define CMIS_RX_FLAG_SUP_OFFSET     (1*128 + 158)
#define CMIS_SNR_SUP_OFFSET         (19*128 + 130)
#define CMIS_LANE_SPECIFIC_OFFSET   (1*128 + 160)

#define CMIS_TX_LOS_OFFSET          (17*128 + 136)
#define CMIS_TX_CDR_LOL_OFFSET      (17*128 + 137)
#define CMIS_RX_CDR_LOL_OFFSET      (17*128 + 148)
#define CMIS_MODULE_STATUS_OFFSET   (3)
#define CMIS_DATAPATH_STATUS_OFFSET (17*128 + 128)
#define CMIS_SNR_SELECT_OFFSET      (20*128 + 128)
#define CMIS_HOST_SNR_OFFSET        (20*128 + 208)
#define CMIS_MEDIA_SNR_OFFSET       (20*128 + 240)
#define CMIS_TEMP_OFFSET            (14)
#define CMIS_VOLTAGE_OFFSET         (16)
#define CMIS_TX_BIAS_OFFSET         (17*128 + 170)
#define CMIS_TX_POWER_OFFSET        (17*128 + 154)
#define CMIS_RX_POWER_OFFSET        (17*128 + 186)


/* SFP/SFP+/SFP28 and later */
#define SFF_Identifier_SFP       (0x3)
/* QSFP+ or later with SFF-8636 or SFF-8436 management interface (SFF-8436, SFF-8635, SFF-8665,SFF-8685 et al.) */
#define SFF_Identifier_QSFP_PLUS (0x0D)
/* QSFP28 or later with SFF-8636 management interface (SFF-8665 et al.) */
#define SFF_Identifier_QSFP28    (0x11)
/* QSFP-DD Double Density 8X Pluggable Transceiver (INF-8628) */
#define SFF_Identifier_QSFPDD    (0x18)
/* OSFP 8X Pluggable Transceiver */
#define SFF_Identifier_OSFP      (0x19)
/* DSFP Dual Small Form Factor Pluggable Transceiver */
#define SFF_Identifier_DSFP      (0x1B)
/* QSFP+ or later with Common Management Interface Specification (CMIS) */
#define SFF_Identifier_QSFP_CMIS (0x1E)


#define WB_PORT_POWER_GROUP_MAX       (256)   /* Max power groups */
#define WB_PORT_IN_GROUP_MAX          (256)   /* Max ports per power group */

#define SFF_E2_IDENTIFIER_REG         (0)

/* protocol type. */
enum wb_port_protocol_type {
    SFF_PROTOCOL_SFP  = 1,
    SFF_PROTOCOL_QSFP = 2,
    SFF_PROTOCOL_CMIS = 3,
};

/* power groups set flag. */
enum wb_port_power_cfg {
    WB_PORT_POWER_GROUP_NOT_CFG = 0,    /* port power group is not configured */
    WB_PORT_POWER_GROUP_CFG = 1         /* configured */
};

#endif /*_TRANSCEIVER_SYSFS_H_ */

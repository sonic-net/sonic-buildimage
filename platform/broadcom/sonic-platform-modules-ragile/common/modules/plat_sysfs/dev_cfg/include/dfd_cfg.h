#ifndef __DFD_CFG_H__
#define __DFD_CFG_H__

#include <linux/list.h>

#define DFD_KO_CFG_FILE_NAME     "/etc/plat_sysfs_cfg/cfg_file_name"
#define DFD_KO_CFG_FILE_DIR      "/etc/plat_sysfs_cfg/"
#define DFD_PUB_CARDTYPE_FILE    "/sys/module/platform_common/parameters/dfd_my_type"

#define DFD_CFG_CMDLINE_MAX_LEN (256)
#define DFD_CFG_NAME_MAX_LEN    (256)
#define DFD_CFG_VALUE_MAX_LEN   (256)
#define DFD_CFG_STR_MAX_LEN     (64)
#define DFD_CFG_CPLD_NUM_MAX    (16)
#define DFD_PRODUCT_ID_LENGTH   (8)
#define DFD_PID_BUF_LEN         (32)
#define DFD_TEMP_NAME_BUF_LEN   (32)

#define DFD_CFG_EMPTY_VALUE     (-1)
#define DFD_CFG_INVALID_VALUE   (0)

#define DFD_CFG_KEY(item, index1, index2) \
    ((((item) & 0xff) << 24) | (((index1) & 0xffff) << 8) | ((index2) & 0xff))
#define DFD_CFG_ITEM_ID(key)    (((key) >> 24) & 0xff)
#define DFD_CFG_INDEX1(key)     (((key) >> 8) & 0xffff)
#define DFD_CFG_INDEX2(key)     ((key)& 0xff)

#define INDEX_NOT_EXIST     (-1)
#define INDEX1_MAX           (0xffff)
#define INDEX2_MAX           (0xff)

#define DFD_CFG_ITEM_ALL \
    DFD_CFG_ITEM(DFD_CFG_ITEM_NONE, "none", INDEX_NOT_EXIST, INDEX_NOT_EXIST)                       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_DEV_NUM, "dev_num", INDEX1_MAX, INDEX2_MAX)                           \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CPLD_LPC_DEV, "cpld_lpc_dev", INDEX1_MAX, DFD_CFG_CPLD_NUM_MAX)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_INT_END, "end_int", INDEX_NOT_EXIST, INDEX_NOT_EXIST)                 \
                                                                                                    \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CPLD_MODE, "mode_cpld", INDEX1_MAX, DFD_CFG_CPLD_NUM_MAX)             \
    DFD_CFG_ITEM(DFD_CFG_ITEM_SFF_DIR_NAME, "sff_dir_name", INDEX1_MAX, INDEX_NOT_EXIST)            \
    DFD_CFG_ITEM(DFD_CFG_ITEM_STRING_END, "end_string", INDEX_NOT_EXIST, INDEX_NOT_EXIST)           \
                                                                                                    \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CPLD_I2C_DEV, "cpld_i2c_dev", INDEX1_MAX, INDEX2_MAX)                 \
    DFD_CFG_ITEM(DFD_CFG_ITEM_OTHER_I2C_DEV, "other_i2c_dev", INDEX1_MAX, INDEX2_MAX)               \
    DFD_CFG_ITEM(DFD_CFG_ITEM_I2C_DEV_END, "end_i2c_dev", INDEX_NOT_EXIST, INDEX_NOT_EXIST)         \
                                                                                                    \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FAN_ROLL_STATUS, "fan_roll_status", INDEX1_MAX, INDEX2_MAX)           \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FAN_SPEED, "fan_speed", INDEX1_MAX, INDEX2_MAX)                       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FAN_RATIO, "fan_ratio", INDEX1_MAX, INDEX2_MAX)                       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_DEV_PRESENT_STATUS, "dev_present_status", INDEX1_MAX, INDEX2_MAX)     \
    DFD_CFG_ITEM(DFD_CFG_ITEM_PSU_STATUS, "psu_status", INDEX1_MAX, INDEX2_MAX)                     \
    DFD_CFG_ITEM(DFD_CFG_ITEM_HWMON_TEMP, "hwmon_temp", INDEX1_MAX, INDEX2_MAX)                     \
    DFD_CFG_ITEM(DFD_CFG_ITEM_HWMON_IN, "hwmon_in", INDEX1_MAX, INDEX2_MAX)                         \
    DFD_CFG_ITEM(DFD_CFG_ITEM_SFF_CPLD_REG, "sff_cpld_reg", INDEX1_MAX, INDEX2_MAX)                 \
    DFD_CFG_ITEM(DFD_CFG_ITEM_INFO_CTRL_END, "end_info_ctrl", INDEX_NOT_EXIST, INDEX_NOT_EXIST)     \

#ifdef DFD_CFG_ITEM
#undef DFD_CFG_ITEM
#endif
#define DFD_CFG_ITEM(_id, _name, _index_min, _index_max)    _id,
typedef enum dfd_cfg_item_id_s {
    DFD_CFG_ITEM_ALL
} dfd_cfg_item_id_t;

#define DFD_CFG_ITEM_IS_INT(item_id) \
    (((item_id) > DFD_CFG_ITEM_NONE) && ((item_id) < DFD_CFG_ITEM_INT_END))

#define DFD_CFG_ITEM_IS_STRING(item_id) \
    (((item_id) > DFD_CFG_ITEM_INT_END) && ((item_id) < DFD_CFG_ITEM_STRING_END))

#define DFD_CFG_ITEM_IS_I2C_DEV(item_id) \
    (((item_id) > DFD_CFG_ITEM_STRING_END) && ((item_id) < DFD_CFG_ITEM_I2C_DEV_END))

#define DFD_CFG_ITEM_IS_INFO_CTRL(item_id) \
    (((item_id) > DFD_CFG_ITEM_I2C_DEV_END) && ((item_id) < DFD_CFG_ITEM_INFO_CTRL_END))

typedef struct index_range_s {
    int index1_max;
    int index2_max;
} index_range_t;

typedef struct val_convert_node_s {
    struct list_head lst;
    int int_val;
    char str_val[DFD_CFG_STR_MAX_LEN];
    int index1;
    int index2;
} val_convert_node_t;

void *dfd_ko_cfg_get_item(int key);

void dfd_ko_cfg_show_item(int key);

int32_t dfd_dev_cfg_init(void);

void dfd_dev_cfg_exit(void);

#endif /* __DFD_CFG_H__ */

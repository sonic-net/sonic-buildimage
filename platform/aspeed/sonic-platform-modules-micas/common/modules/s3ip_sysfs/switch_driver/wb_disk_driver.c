/*
 * disk_device_driver.c
 *
 * This module realize /sys/s3ip/disk attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2026-01-07                  S3IP sysfs
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/fcntl.h>
#include <linux/string.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"
#include "wb_disk_driver.h"

#define DISK_LINK_ALARM_OK          0
#define DISK_LINK_ALARM_LINKDOWN    1
#define DISK_LINK_ALARM_SLOWDOWN    2
#define DISK_LINK_ALARM_CE          4
#define DISK_LINK_ALARM_UE          8

#define DISK_NORMAL                 0
#define DISK_ABNORMAL               1

#define STAT_DET_STATUS_OK          3

#define DISK_PXSSTS_FILE_FORMAT "/sys/kernel/ahci_reg/%s/pxssts"
#define DISK_PXSERR_FILE_FORMAT "/sys/kernel/ahci_reg/%s/pxserr"
#define DISK_TEMP_STATUS_FILE_FORMAT "/etc/sonic/disk/disk%d_temp_status"
#define DISK_WEAR_STATUS_FILE_FORMAT "/etc/sonic/disk/disk%d_wear_status"

int g_dfd_disk_dbg_level = 0;
module_param(g_dfd_disk_dbg_level, int, S_IRUGO | S_IWUSR);

static const dfd_status_desc_map_t dfd_disk_status_map[] = {
    {DISK_STATUS_LINKDOWN, "link_down"},
    {DISK_STATUS_SLOWDOWN, "link_speed_degrade"},
    {DISK_STATUS_CE, "link_correctable_error"},
    {DISK_STATUS_UE, "link_uncorrectable_error"},
};

dfd_sysfs_func_map_t disk_func_table[DFD_DISK_MAX_E] = {
    [DFD_DISK_ATA_PORT_E] = {dfd_get_disk_info, NULL},
    [DFD_DISK_TYPE_E] = {dfd_get_disk_info, NULL},
    [DFD_DISK_TEMP_MAX_E] = {dfd_get_disk_info, NULL},
    [DFD_DISK_TEMP_MIN_E] = {dfd_get_disk_info, NULL},
    [DFD_DISK_TEMP_HIGH_E] = {dfd_get_disk_info, NULL},
    [DFD_DISK_TEMP_LOW_E] = {dfd_get_disk_info, NULL},
    [DFD_DISK_TARGET_LINK_SPEED_E] = {dfd_get_disk_info, NULL},
    [DFD_DISK_REMAINING_LIFE_THRESHOLD_E] = {dfd_get_disk_info, NULL},
    [DFD_DISK_TEMP_STATUS_E] = {dfd_disk_temp_status, NULL},
    [DFD_DISK_LINK_ALARM_E] = {dfd_get_disk_link_alarm, NULL},
    [DFD_DISK_WEAR_STATUS_E] = {dfd_get_disk_wear_status, NULL},
    [DFD_DISK_LINK_STATUS_E] = {dfd_get_disk_link_status, NULL},
    [DFD_DISK_SPEED_STATUS_E] = {dfd_get_disk_speed_status, NULL},
    [DFD_DISK_CE_E] = {dfd_get_disk_correctable_error, NULL},
    [DFD_DISK_UE_E] = {dfd_get_disk_uncorrectable_error, NULL},
};

dfd_debug_data_key_map_t disk_dbg_key_table[DFD_DISK_MAX_E] = {
    [DFD_DISK_ATA_PORT_E] = {DFD_CFG_ITEM_DISK_INFO, CFG_INDEX1_INDEX2_CMB_1, CFG_STR_DATA },
    [DFD_DISK_TYPE_E] = {DFD_CFG_ITEM_DISK_INFO, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [DFD_DISK_TEMP_MAX_E] = {DFD_CFG_ITEM_DISK_INFO, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [DFD_DISK_TEMP_MIN_E] = {DFD_CFG_ITEM_DISK_INFO, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [DFD_DISK_TEMP_HIGH_E] = {DFD_CFG_ITEM_DISK_INFO, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [DFD_DISK_TEMP_LOW_E] = {DFD_CFG_ITEM_DISK_INFO, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [DFD_DISK_TARGET_LINK_SPEED_E] = {DFD_CFG_ITEM_DISK_INFO, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [DFD_DISK_REMAINING_LIFE_THRESHOLD_E] = {DFD_CFG_ITEM_DISK_INFO, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [DFD_DISK_TEMP_STATUS_E] = {DFD_CFG_ITEM_DISK_INFO, CFG_2INDEXES_2, CFG_STR_DATA},
    [DFD_DISK_LINK_ALARM_E] = {DFD_CFG_ITEM_DISK_INFO, CFG_2INDEXES_2, CFG_STR_DATA},
    [DFD_DISK_WEAR_STATUS_E] = {DFD_CFG_ITEM_DISK_INFO, CFG_2INDEXES_2, CFG_STR_DATA},
    [DFD_DISK_LINK_STATUS_E] = {DFD_CFG_ITEM_DISK_INFO, CFG_2INDEXES_2, CFG_STR_DATA},
    [DFD_DISK_SPEED_STATUS_E] = {DFD_CFG_ITEM_DISK_INFO, CFG_2INDEXES_2, CFG_STR_DATA},
    [DFD_DISK_CE_E] = {DFD_CFG_ITEM_DISK_INFO, CFG_2INDEXES_2, CFG_INT_DATA},
    [DFD_DISK_UE_E] = {DFD_CFG_ITEM_DISK_INFO, CFG_2INDEXES_2, CFG_INT_DATA},
}; 

ssize_t dfd_get_disk_info(unsigned int dev_index, unsigned int type,  char *buf, size_t count)
{
    uint64_t key;
    int ret;

    if (buf == NULL) {
        DBG_DISK_DEBUG(DBG_ERROR, "param error, buf is NULL. dev_index: %u, type: %u\n",
            dev_index, type);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_DISK_DEBUG(DBG_ERROR, "buf size error, count: %zu, dev_index: %u, type: %u\n",
            count, dev_index, type);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DISK_INFO, dev_index, type);
    ret = dfd_get_value_from_info(key, buf, count);
    if (ret < 0) {
        DBG_DISK_DEBUG(DBG_ERROR, "get DISK disk info error, key_name: %s, ret: %d\n",
            key_to_name(DFD_CFG_ITEM_DISK_INFO), ret);
    } else {
        DBG_DISK_DEBUG(DBG_VERBOSE, "get DISK disk info success, value: %s\n", buf);
    }
  
    return ret;
}

static int read_u32_from_file(const char *path, u32 *val)
{
    char buf[32];
    ssize_t ret;

    if (path == NULL || val == NULL) {
        DBG_DISK_DEBUG(DBG_ERROR, "param error, path or val is NULL\n");
        return -DFD_RV_INVALID_VALUE;
    }
    
    mem_clear(buf, sizeof(buf));
    ret = dfd_ko_read_file((char *)path, 0, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        DBG_DISK_DEBUG(DBG_ERROR, "fail to read file: %s, ret=%ld\n", path, ret);
        return -DFD_RV_INVALID_VALUE;
    }

    buf[ret] = '\0';

    ret = kstrtou32(buf, 0, val);
    if (ret < 0) {
        DBG_DISK_DEBUG(DBG_ERROR, "fail to convert %s, ret=%ld, buf:%s\n", path, (long)ret, buf);
        return -DFD_RV_INVALID_VALUE;
    }
    return 0;
}

ssize_t dfd_get_disk_link_alarm(unsigned int dev_index, unsigned int type, char *buf, size_t count)
{
    char filename[256];
    u32 pxssts = 0, pxserr = 0, target_link_speed = 0;
    uint64_t key;
    int ret, len;
    int link_alarm;

    if (buf == NULL) {
        DBG_DISK_DEBUG(DBG_ERROR, "param error, buf is NULL. dev_index: %u, type: %u\n",
            dev_index, type);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_DISK_DEBUG(DBG_ERROR, "buf size error, count: %zu, dev_index: %u, type: %u\n",
            count, dev_index, type);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);

    ret = dfd_get_dev_debug_config_data(DFD_CFG_ITEM_DISK_INFO, dev_index, type, buf, count);
    if (ret >= 0) {
        DBG_DISK_DEBUG(DBG_VERBOSE, "get link status from debug node, value: %s\n", buf);
        return ret;
    } else {
        DBG_DISK_DEBUG(DBG_VERBOSE, "failed to get link status from debug node, dev_index: %u, type: %u, ret=%d\n",
            dev_index, type, ret);
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DISK_INFO, dev_index, DFD_DISK_ATA_PORT_E);
    ret = dfd_get_value_from_info(key, buf, count);
    if (ret < 0) {
        DBG_DISK_DEBUG(DBG_ERROR, "get DISK port error, key_name: %s, ret: %d\n",
            key_to_name(DFD_CFG_ITEM_DISK_INFO), ret);
        return -DFD_RV_INVALID_VALUE;
    }

    len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    }

    /* Read DISK PXSSTS (Port x Serial ATA Status) */
    snprintf(filename, sizeof(filename), DISK_PXSSTS_FILE_FORMAT, buf);
    ret = read_u32_from_file(filename, &pxssts);
    if (ret < 0) {
        DBG_DISK_DEBUG(DBG_ERROR, "get DISK pxssts error, ret: %d\n", ret);
        return -DFD_RV_INVALID_VALUE;
    }

    /* Read DISK PXSERR (Port x Serial ATA Error) */
    snprintf(filename, sizeof(filename), DISK_PXSERR_FILE_FORMAT, buf);
    ret = read_u32_from_file(filename, &pxserr);
    if (ret < 0) {
        DBG_DISK_DEBUG(DBG_ERROR, "get DISK pxserr error, ret: %d\n", ret);
        return -DFD_RV_INVALID_VALUE;
    }

    link_alarm = 0;
    if ( (pxssts & 0xf) == STAT_DET_STATUS_OK ) {
        mem_clear(buf, count);
        key = DFD_CFG_KEY(DFD_CFG_ITEM_DISK_INFO, dev_index, DFD_DISK_TARGET_LINK_SPEED_E);
        ret = dfd_get_value_from_info(key, buf, count);
        if (ret < 0) {
            DBG_DISK_DEBUG(DBG_ERROR, "get DISK target_link_speed error, key_name: %s, ret: %d\n",
                key_to_name(DFD_CFG_ITEM_DISK_INFO), ret);
            return -DFD_RV_INVALID_VALUE;
        } else {
            ret = kstrtou32(buf, 0, &target_link_speed);
            if (ret < 0) {
                DBG_DISK_DEBUG(DBG_ERROR, "Invalid target_link_speed, ret: %d, buf: %s\n", ret, buf);
                return -DFD_RV_INVALID_VALUE;
            }
        }

        if ( ((pxssts & 0xf0) >> 4) != target_link_speed ) {
            DBG_DISK_DEBUG(DBG_VERBOSE, "DISK %d link slowdown, pxssts: %x\n", dev_index, pxssts);
            link_alarm |= DISK_LINK_ALARM_SLOWDOWN;
        }
    } else {
        DBG_DISK_DEBUG(DBG_VERBOSE, "DISK %d link down, pxssts: %x\n", dev_index, pxssts);
        link_alarm |= DISK_LINK_ALARM_LINKDOWN;
    }

    if (pxserr != 0) {
        if ( (pxserr & 0xf00) != 0 ) {
            DBG_DISK_DEBUG(DBG_VERBOSE, "DISK %d occur uncorrectable error, pxserr: %x\n", dev_index, pxserr);
            link_alarm |= DISK_LINK_ALARM_UE;
        }
        if ( (pxserr & 0xfffff0ff) != 0 ) {
            DBG_DISK_DEBUG(DBG_VERBOSE, "DISK %d occur correctable error, pxserr: %x\n", dev_index, pxserr);
            link_alarm |= DISK_LINK_ALARM_CE;
        }
    }

    mem_clear(buf, count);
    return snprintf(buf, count, "0x%x\n", link_alarm);
}

ssize_t dfd_disk_temp_status(unsigned int dev_index, unsigned int type,  char *buf, size_t count)
{
    char filename[256];

    if (buf == NULL) {
        DBG_DISK_DEBUG(DBG_ERROR, "param error, buf is NULL. dev_index: %u, type: %u\n",
            dev_index, type);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_DISK_DEBUG(DBG_ERROR, "buf size error, count: %zu, dev_index: %u, type: %u\n",
            count, dev_index, type);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    snprintf(filename, sizeof(filename), DISK_TEMP_STATUS_FILE_FORMAT, dev_index);
    return dfd_ko_read_file(filename, 0, buf, count);
}

ssize_t dfd_get_disk_wear_status(unsigned int dev_index, unsigned int type,  char *buf, size_t count)
{
    char filename[256];
    int ret;

    if (buf == NULL) {
        DBG_DISK_DEBUG(DBG_ERROR, "param error, buf is NULL. dev_index: %u, type: %u\n",
            dev_index, type);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_DISK_DEBUG(DBG_ERROR, "buf size error, count: %zu, dev_index: %u, type: %u\n",
            count, dev_index, type);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_dev_debug_config_data(DFD_CFG_ITEM_DISK_INFO, dev_index, type, buf, count);
    if (ret >= 0) {
        DBG_DISK_DEBUG(DBG_VERBOSE, "get wear status from debug node, value: %s\n", buf);
        return ret;
    } else {
        DBG_DISK_DEBUG(DBG_VERBOSE, "failed to get wear status from debug node, dev_index: %u, type: %u, ret=%d\n",
            dev_index, type, ret);
    }

    mem_clear(buf, count);
    snprintf(filename, sizeof(filename), DISK_WEAR_STATUS_FILE_FORMAT, dev_index);
    return dfd_ko_read_file(filename, 0, buf, count);
}

static ssize_t dfd_get_pxssts_and_pxserr_details(unsigned int dev_index, unsigned int type, char *buf, size_t count)
{
    char filename[256];
    u32 pxssts = 0, pxserr = 0;
    uint64_t key;
    int ret, len;

    if (buf == NULL) {
        DBG_DISK_DEBUG(DBG_ERROR, "param error, buf is NULL. dev_index: %u, type: %u\n",
            dev_index, type);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count == 0) {
        DBG_DISK_DEBUG(DBG_ERROR, "buf size error, count: %zu, dev_index: %u, type: %u\n",
            count, dev_index, type);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);

    ret = dfd_get_dev_debug_config_data(DFD_CFG_ITEM_DISK_INFO, dev_index, type, buf, count);
    if (ret >= 0) {
        DBG_DISK_DEBUG(DBG_VERBOSE, "get link status from debug node, value: %s\n", buf);
        return ret;
    } else {
        DBG_DISK_DEBUG(DBG_VERBOSE, "failed to get link status from debug node, dev_index: %u, type: %u, ret=%d\n",
            dev_index, type, ret);
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DISK_INFO, dev_index, DFD_DISK_ATA_PORT_E);
    ret = dfd_get_value_from_info(key, buf, count);
    if (ret < 0) {
        DBG_DISK_DEBUG(DBG_ERROR, "get DISK port error, key_name: %s, ret: %d\n",
            key_to_name(DFD_CFG_ITEM_DISK_INFO), ret);
        return -DFD_RV_INVALID_VALUE;
    }

    len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    }

    /* Read DISK PXSSTS (Port x Serial ATA Status) */
    snprintf(filename, sizeof(filename), DISK_PXSSTS_FILE_FORMAT, buf);
    ret = read_u32_from_file(filename, &pxssts);
    if (ret < 0) {
        DBG_DISK_DEBUG(DBG_ERROR, "get DISK pxssts error, ret: %d\n", ret);
        return -DFD_RV_INVALID_VALUE;
    }

    /* Read DISK PXSERR (Port x Serial ATA Error) */
    snprintf(filename, sizeof(filename), DISK_PXSERR_FILE_FORMAT, buf);
    ret = read_u32_from_file(filename, &pxserr);
    if (ret < 0) {
        DBG_DISK_DEBUG(DBG_ERROR, "get DISK pxserr error, ret: %d\n", ret);
        return -DFD_RV_INVALID_VALUE;
    }

    return snprintf(buf, count, "pxssts: 0x%x, pxserr: 0x%x", pxssts, pxserr);
}

static ssize_t dfd_get_disk_status(unsigned int dev_index, unsigned int type,  char *buf, size_t count, int error_mask)
{
    int link_alarm, status, error_type;
    char detail[64];
    size_t detail_len;
    char reg_value[64];
    dfd_status_detail_t detail_cfg;
    int ret;

    if (buf == NULL) {
        DBG_DISK_DEBUG(DBG_ERROR, "param error, buf is NULL. dev_index: %u, type: %u\n",
            dev_index, type);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count == 0) {
        DBG_DISK_DEBUG(DBG_ERROR, "buf size error, count: %zu, dev_index: %u, type: %u\n",
            count, dev_index, type);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_disk_link_alarm(dev_index, DFD_DISK_LINK_ALARM_E, buf, count);
    if (ret < 0) {
        DBG_DISK_DEBUG(DBG_ERROR, "failed to get disk link alarm, dev_index: %u, type: %u, ret=%d\n",
            dev_index, DFD_DISK_LINK_ALARM_E, ret);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = kstrtoint(strim(buf), 0, &link_alarm);
    if (ret < 0) {
        DBG_DISK_DEBUG(DBG_ERROR, "invalid link alarm value, dev_index: %u, type: %u, value: %s\n",
            dev_index, type, buf);
        return -DFD_RV_INVALID_VALUE;
    }

    error_type = link_alarm & error_mask;
    if (error_type) {
        status = DISK_ABNORMAL;
    } else {
        status = DISK_NORMAL;
    }

    mem_clear(detail, sizeof(detail));
    DFD_STATUS_DETAIL_CFG_SET(detail_cfg, error_type, DISK_STATUS_OK, dfd_disk_status_map, false);
    detail_len = dfd_status_get_detail(&detail_cfg, detail, sizeof(detail));
    if (detail_len > 0) {
        ret = dfd_get_pxssts_and_pxserr_details(dev_index, type, reg_value, sizeof(reg_value));
        if (ret < 0) {
            DBG_DISK_DEBUG(DBG_ERROR, "failed to get pxssts and pxserr detail, dev_index: %u, type: %u, ret=%d\n",
                dev_index, type, ret);
            return -DFD_RV_INVALID_VALUE;
        }
        return (ssize_t)snprintf(buf, count, "%d %s(%s)\n", status, detail, reg_value);
    } else {
        return (ssize_t)snprintf(buf, count, "%d\n", status);
    }
}

ssize_t dfd_get_disk_link_status(unsigned int dev_index, unsigned int type,  char *buf, size_t count)
{
    return dfd_get_disk_status(dev_index, type, buf, count, DISK_LINK_ALARM_LINKDOWN);
}

ssize_t dfd_get_disk_speed_status(unsigned int dev_index, unsigned int type,  char *buf, size_t count)
{
    return dfd_get_disk_status(dev_index, type, buf, count, DISK_LINK_ALARM_SLOWDOWN);
}

ssize_t dfd_get_disk_correctable_error(unsigned int dev_index, unsigned int type,  char *buf, size_t count)
{
    return dfd_get_disk_status(dev_index, type, buf, count, DISK_LINK_ALARM_CE);
}

ssize_t dfd_get_disk_uncorrectable_error(unsigned int dev_index, unsigned int type,  char *buf, size_t count)
{
    return dfd_get_disk_status(dev_index, type, buf, count, DISK_LINK_ALARM_UE);
}

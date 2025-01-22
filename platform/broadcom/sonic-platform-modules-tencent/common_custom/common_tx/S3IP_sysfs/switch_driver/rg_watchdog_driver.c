/*
 * Copyright(C) 2001-2022 Ruijie Network. All rights reserved.
 */
/*
 * rg_watchdog_driver.c
 * Original Author: sonic_rd@ruijie.com.cn 2020-02-17
 *
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@ruijie.com.cn         2021-09-05          S3IP sysfs Initial version
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "rg_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"

#define WDT_FILE_NAME_LEN                   (64)
#define WDT_ABSOLUTE_PATH_NAME_LEN          (256)
#define WDT_SYSFS_FILE_DIR                  ("/sys/class/watchdog/watchdog%d/")

typedef enum rg_wdt_enable_status_e {
    RG_WDT_DISENABLE        = 0,     /* close watchdog */
    RG_WDT_ENABLE           = 1,     /* open watchdog */
} rg_wdt_enable_status_t;

struct wdt_file_enable_status_s {
    rg_wdt_enable_status_t value;
    char state[WDT_FILE_NAME_LEN];
};

struct wdt_file_enable_status_s wdt_file_enable_status_match[] = {
    {RG_WDT_DISENABLE,       "inactive"},
    {RG_WDT_ENABLE,      "active"},
};

int g_dfd_watchdog_dbg_level = 0;
module_param(g_dfd_watchdog_dbg_level, int, S_IRUGO | S_IWUSR);

static int watchdog_file_read(char *fpath, char *buf, int size)
{
    int ret;
    struct file *filp;
    loff_t pos;

    filp = filp_open(fpath, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        DFD_WDT_DEBUG(DBG_ERROR, "watchdog can't open %s.\n", fpath);
        filp = NULL;
        ret = -ENOENT;
        return ret;
    }

    memset(buf, 0, size);
    pos = 0;
    ret = kernel_read(filp, buf, size - 1, &pos);
    if (ret < 0) {
        DFD_WDT_DEBUG(DBG_ERROR, "kernel_read failed, path=%s, addr=0, size=%d, ret=%d\n",
            fpath, size -1, ret);
    }

    filp_close(filp, NULL);
    filp = NULL;
    return ret;
}

static int dfd_get_watchdog_id(unsigned int wdt_index)
{
    int key;
    int *p_watchdog_id;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_WATCHDOG_ID, wdt_index, 0);
    p_watchdog_id = dfd_ko_cfg_get_item(key);
    if (p_watchdog_id == NULL) {
        DFD_WDT_DEBUG(DBG_ERROR, "get watchdog id error, key: 0x%x\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    DFD_WDT_DEBUG(DBG_VERBOSE, "get watchdog id ok, watchdog index:%u, id:0x%x.\n",
        wdt_index, *p_watchdog_id);
    return *p_watchdog_id;
}

static int watchdog_get_file_name(unsigned int wdt_index, rg_wdt_type_t type, char *buf, int len)
{
    int key;
    char *watchdog_name;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_WATCHDOG_NAME, wdt_index, type);
    watchdog_name = dfd_ko_cfg_get_item(key);
    if (watchdog_name == NULL) {
        DFD_WDT_DEBUG(DBG_ERROR, "watchdog name config error, key: 0x%08x\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DFD_WDT_DEBUG(DBG_VERBOSE, "get watchdog%u %s\n", wdt_index, watchdog_name);
    snprintf(buf, len, "%s", watchdog_name);
    return DFD_RV_OK;
}

ssize_t dfd_get_watchdog_info(uint8_t type, char *buf, size_t count)
{
    char fpath[WDT_ABSOLUTE_PATH_NAME_LEN];
    int watchdog_id, len, ret;

    watchdog_id = dfd_get_watchdog_id(0);
    memset(fpath, 0, WDT_ABSOLUTE_PATH_NAME_LEN);
    snprintf(fpath, WDT_ABSOLUTE_PATH_NAME_LEN - 1, WDT_SYSFS_FILE_DIR, watchdog_id);
    len = strlen(fpath);
    ret = watchdog_get_file_name(watchdog_id, type, &fpath[len], WDT_ABSOLUTE_PATH_NAME_LEN - len);
    if (ret < 0) {
        DFD_WDT_DEBUG(DBG_WARN, "watchdog type[%u] don't support to get sysfs name.\n", type);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = watchdog_file_read(fpath, buf, count - 1);
    if (ret < 0) {
        DFD_WDT_DEBUG(DBG_ERROR, "watchdog read file %s error, ret: %d\n", fpath, ret);
    }

    return ret;
}

ssize_t dfd_watchdog_get_status_str(char *buf, size_t count)
{
    int ret, i;
    int enable_status;

    ret = dfd_get_watchdog_info(RG_WDT_TYPE_STATE, buf, count);
    if (ret < 0) {
        DFD_WDT_DEBUG(DBG_ERROR, "watchdog type[%d] get sysfs name failed.\n", RG_WDT_TYPE_STATE);
        return -DFD_RV_DEV_FAIL;
    }

    enable_status = -1;
    for (i = 0; i < ARRAY_SIZE(wdt_file_enable_status_match); i++) {
        if (strncmp(wdt_file_enable_status_match[i].state, buf, \
                 strlen(wdt_file_enable_status_match[i].state)) == 0) {
            enable_status = wdt_file_enable_status_match[i].value;
            DFD_WDT_DEBUG(DBG_VERBOSE, "watchdog read state file %s match enable status[%d].\n",
                buf, enable_status);
            break;
        }
    }

    if (enable_status < 0) {
        DFD_WDT_DEBUG(DBG_ERROR, "watchdog read state file %s don't match enable status\n", buf);
        return -DFD_RV_DEV_FAIL;
    }

    return (ssize_t)snprintf(buf, count, "%d\n", enable_status);
}

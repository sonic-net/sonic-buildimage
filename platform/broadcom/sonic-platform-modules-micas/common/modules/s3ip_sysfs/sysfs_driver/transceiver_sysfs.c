/*
 * transceiver_sysfs.c
 *
 * This module create eth kobjects and attributes in /sys/s3ip/transceiver
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "./include/switch.h"
#include "./include/transceiver_sysfs.h"

static int g_sff_loglevel = 0;

#define WB_QSFP_TX_FAULT_OFFSET       (4)
#define WB_QSFPDD_TX_FAULT_OFFSET     (17*128 + 135)
#define WB_QSFP_TX_DISABLE_OFFSET     (86)
#define WB_QSFPDD_TX_DISABLE_OFFSET   (16*128 + 130)
#define WB_QSFP_RX_LOS_OFFSET         (3)
#define WB_QSFPDD_RX_LOS_OFFSET       (17*128 + 147)
#define WB_QSFP_LP_MODE_OFFSET        (93)
#define WB_QSFPDD_LP_MODE_OFFSET      (26)

#define SFF_INFO(fmt, args...) do {                                        \
    if (g_sff_loglevel & INFO) { \
        printk(KERN_INFO "[SFF_SYSFS][func:%s line:%d]"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SFF_ERR(fmt, args...) do {                                        \
    if (g_sff_loglevel & ERR) { \
        printk(KERN_ERR "[SFF_SYSFS][func:%s line:%d]"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SFF_DBG(fmt, args...) do {                                        \
    if (g_sff_loglevel & DBG) { \
        printk(KERN_DEBUG "[SFF_SYSFS][func:%s line:%d]"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct sff_obj_s {
    struct switch_obj *sff_obj;
    struct bin_attribute bin;
    int sff_creat_bin_flag;
};

struct sff_s {
    unsigned int sff_number;
    struct sff_obj_s *sff;
};

static struct sff_s g_sff;
static struct switch_obj *g_sff_obj = NULL;
static struct s3ip_sysfs_transceiver_drivers_s *g_sff_drv = NULL;

static void sysfs_del_lf_cr(char *str)
{
    int i, len;

    len = strlen(str);
    for (i = 0; i < len; i++) {
        if (str[i] == '\r' || str[i] == '\n') {
            str[i] = '\0';
        }
    }
}

static ssize_t transceiver_power_on_show(struct switch_obj *obj, struct switch_attribute *attr,
        char *buf)
{
    unsigned int eth_index, eth_num;
    int ret, len, left_len;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_power_on_status);

    eth_num = g_sff.sff_number;
    memset(buf, 0, PAGE_SIZE);
    len = 0;
    left_len = PAGE_SIZE - 1;

    for (eth_index = 1; eth_index <= eth_num; eth_index++) {
        SFF_DBG("eth index: %u\n", eth_index);
        if (left_len > 0) {
            ret = g_sff_drv->get_eth_power_on_status(eth_index, buf + len, left_len);
            if (ret < 0) {
                SFF_ERR("get eth%u power status failed, ret: %d\n", eth_index, ret);
                break;
            }
        } else {
            SFF_ERR("error: transceiver_power_on_show are not enough buffers.\n");
            ret = -EINVAL;
            break;
        }
        sysfs_del_lf_cr(buf);  /* del '\n' */
        len = strlen(buf);
        left_len = PAGE_SIZE - len - 2; /* Reserve end to add '\n' and '\0' */
    }

    if (ret < 0) {
        memset(buf, 0, PAGE_SIZE);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    len = strlen(buf);
    buf[len] = '\n';
    ret = strlen(buf);
    SFF_DBG("transceiver_power_on_show ok. sff num:%d, len:%d\n", eth_num, ret);
    return ret;
}

static ssize_t transceiver_power_on_store(struct switch_obj *obj, struct switch_attribute *attr,
        const char* buf, size_t count)
{
    unsigned int eth_index, eth_num;
    int ret, value;

    check_p(g_sff_drv);
    check_p(g_sff_drv->set_eth_power_on_status);

    sscanf(buf, "%d", &value);
    if (value < 0 || value > 1) {
        SFF_ERR("invalid value: %d, can't set power on status.\n", value);
        return -EINVAL;
    }

    eth_num = g_sff.sff_number;
    for (eth_index = 1; eth_index <= eth_num; eth_index++) {
        SFF_DBG("eth index: %u\n", eth_index);
        ret = g_sff_drv->set_eth_power_on_status(eth_index, value);
        if (ret < 0) {
            SFF_ERR("set eth%u power status failed, ret: %d\n", eth_index, ret);
            break;
        }
    }
    SFF_DBG("transceiver_power_on_store ok. sff num:%d, len:%d\n", eth_num, ret);
    return count;
}

static ssize_t transceiver_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_sff.sff_number);
}

static ssize_t transceiver_present_show(struct switch_obj *obj, struct switch_attribute *attr,
                                        char *buf)
{
    unsigned int eth_index, eth_num;
    int ret, len, left_len;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_present_status);

    eth_num = g_sff.sff_number;
    memset(buf, 0, PAGE_SIZE);
    len = 0;
    left_len = PAGE_SIZE - 1;

    for (eth_index = 1; eth_index <= eth_num; eth_index++) {
        SFF_DBG("eth index: %u\n", eth_index);
        if (left_len > 0) {
            ret = g_sff_drv->get_eth_present_status(eth_index, buf + len, left_len);
            if (ret < 0) {
                SFF_ERR("get eth%u present status failed, ret: %d\n", eth_index, ret);
                break;
            }
        } else {
            SFF_ERR("error: all_sff_cpld_info_show are not enough buffers.\n");
            ret = -EINVAL;
            break;
        }
        sysfs_del_lf_cr(buf);  /* del '\n' */
        len = strlen(buf);
        left_len = PAGE_SIZE - len - 2; /* Reserve end to add '\n' and '\0' */
    }

    if (ret < 0) {
        memset(buf, 0, PAGE_SIZE);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    len = strlen(buf);
    buf[len] = '\n';
    ret = strlen(buf);
    SFF_DBG("sff_cpld_info_show ok. sff num:%d, len:%d\n", eth_num, ret);
    return ret;
}

static ssize_t eth_power_on_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;
    int ret;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_power_on_status);

    eth_index = obj->index;
    SFF_DBG("eth index: %u\n", eth_index);
    ret = g_sff_drv->get_eth_power_on_status(eth_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SFF_ERR("get eth%u power on status failed, ret: %d\n", eth_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t eth_power_on_store(struct switch_obj *obj, struct switch_attribute *attr,
                                  const char* buf, size_t count)
{
    unsigned int eth_index;
    int ret, value;

    check_p(g_sff_drv);
    check_p(g_sff_drv->set_eth_power_on_status);

    sscanf(buf, "%d", &value);
    eth_index = obj->index;
    if (value < 0 || value > 1) {
        SFF_ERR("invalid value: %d, can't set eth%u power on status.\n", value, eth_index);
        return -EINVAL;
    }

    ret = g_sff_drv->set_eth_power_on_status(eth_index, value);
    if (ret < 0) {
        SFF_ERR("set eth%u power on status %d failed, ret: %d\n", eth_index, value, ret);
        return -EIO;
    }
    SFF_DBG("set eth%u power on status %d success\n", eth_index, value);
    return count;
}

static ssize_t eth_tx_fault_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;
    int ret;
    char module_type[1], value[1];
    loff_t offset;
    char mask;

    check_p(g_sff_drv);
    check_p(g_sff_drv->read_eth_eeprom_data);
    check_p(g_sff_drv->get_eth_tx_fault_status);

    eth_index = obj->index;
    SFF_DBG("eth index: %u\n", eth_index);
    memset(module_type, 0, sizeof(module_type));
    memset(value, 0, sizeof(value));
    ret = g_sff_drv->read_eth_eeprom_data(eth_index, module_type, 0, 1);
    if (ret < 0) {
        SFF_ERR("get eth%u module type failed, ret: %d\n", eth_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }

    if (module_type[0] == 0x03) {
        SFF_DBG("get eth%u module type is SFP\n", eth_index);
        ret = g_sff_drv->get_eth_tx_fault_status(eth_index, buf, PAGE_SIZE);
        if (ret < 0) {
            SFF_ERR("get eth%u tx fault status failed, ret: %d\n", eth_index, ret);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
        }
    } else {
        if ((module_type[0] == 0x11) || (module_type[0] == 0x0D)) {
            SFF_DBG("get eth%u module type is QSFP\n", eth_index);
            offset = WB_QSFP_TX_FAULT_OFFSET;
            mask = 0xf;
        } else if ((module_type[0] == 0x18) || (module_type[0] == 0x1e)) {
            SFF_DBG("get eth%u module type is QSFP-DD\n", eth_index);
            offset = WB_QSFPDD_TX_FAULT_OFFSET;
            mask = 0xff;
        } else {
            SFF_ERR("eth%u module is unknown, module_type:%d\n", eth_index, module_type[0]);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
        }

        ret = g_sff_drv->read_eth_eeprom_data(eth_index, value, offset, 1);
        if (ret < 0) {
            SFF_ERR("get eth%u module tx fault value failed, ret: %d\n", eth_index, ret);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
        }

        if ((value[0] & mask) != 0) {
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 1);
        } else {
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 0);
        }
    }

    return ret;
}

static ssize_t eth_tx_disable_show(struct switch_obj *obj, struct switch_attribute *attr,
                                   char *buf)
{
    unsigned int eth_index;
    int ret;
    char module_type[1], value[1];
    loff_t offset;
    char mask;

    check_p(g_sff_drv);
    check_p(g_sff_drv->read_eth_eeprom_data);
    check_p(g_sff_drv->get_eth_tx_disable_status);

    eth_index = obj->index;
    SFF_DBG("eth index: %u\n", eth_index);
    memset(module_type, 0, sizeof(module_type));
    memset(value, 0, sizeof(value));
    ret = g_sff_drv->read_eth_eeprom_data(eth_index, module_type, 0, 1);
    if (ret < 0) {
        SFF_ERR("get eth%u module type failed, ret: %d\n", eth_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }

    if (module_type[0] == 0x03) {
        SFF_DBG("get eth%u module type is SFP\n", eth_index);
        ret = g_sff_drv->get_eth_tx_disable_status(eth_index, buf, PAGE_SIZE);
        if (ret < 0) {
            SFF_ERR("get eth%u tx disable status failed, ret: %d\n", eth_index, ret);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
        }
    } else {
        if ((module_type[0] == 0x11) || (module_type[0] == 0x0D)) {
            SFF_DBG("get eth%u module type is QSFP\n", eth_index);
            offset = WB_QSFP_TX_DISABLE_OFFSET;
            mask = 0xf;
        } else if ((module_type[0] == 0x18) || (module_type[0] == 0x1e)) {
            SFF_DBG("get eth%u module type is QSFP-DD\n", eth_index);
            offset = WB_QSFPDD_TX_DISABLE_OFFSET;
            mask = 0xff;
        } else {
            SFF_ERR("eth%u module is unknown, module_type:%d\n", eth_index, module_type[0]);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
        }

        ret = g_sff_drv->read_eth_eeprom_data(eth_index, value, offset, 1);
        if (ret < 0) {
            SFF_ERR("get eth%u module tx disable value failed, ret: %d\n", eth_index, ret);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
        }

        if ((value[0] & mask) != 0) {
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 1);
        } else {
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 0);
        }
    }

    return ret;
}

static ssize_t eth_tx_disable_store(struct switch_obj *obj, struct switch_attribute *attr,
                                    const char* buf, size_t count)
{
    unsigned int eth_index;
    int ret, value;
    char module_type[1], write_buf[1];
    loff_t offset;

    check_p(g_sff_drv);
    check_p(g_sff_drv->read_eth_eeprom_data);
    check_p(g_sff_drv->write_eth_eeprom_data);
    check_p(g_sff_drv->set_eth_tx_disable_status);

    sscanf(buf, "%d", &value);
    eth_index = obj->index;
    SFF_DBG("eth index: %u, tx_disable:0x%x\n", eth_index, value);

    if (value < 0 || value > 1) {
        SFF_ERR("invalid value: %d, can't set eth%u tx disable status.\n", value, eth_index);
    }

    write_buf[0] = 0;
    memset(module_type, 0, sizeof(module_type));
    ret = g_sff_drv->read_eth_eeprom_data(eth_index, module_type, 0, 1);
    if (ret < 0) {
        SFF_ERR("get eth%u module type failed, ret: %d\n", eth_index, ret);
        return -EIO;
    }

    if (module_type[0] == 0x03) {
        SFF_DBG("get eth%u module type is SFP\n", eth_index);

        ret = g_sff_drv->set_eth_tx_disable_status(eth_index, value);
        if (ret < 0) {
            SFF_ERR("set eth%u tx disable status %d failed, ret: %d\n", eth_index, value, ret);
            return -EIO;
        }
    } else {
        if ((module_type[0] == 0x11) || (module_type[0] == 0x0D)) {
            SFF_DBG("get eth%u module type is QSFP\n", eth_index);
            offset = WB_QSFP_TX_DISABLE_OFFSET;
            if (value != 0) {
                write_buf[0] = 0xf;
            }
        } else if ((module_type[0] == 0x18) || (module_type[0] == 0x1e)) {
            SFF_DBG("get eth%u module type is QSFP-DD\n", eth_index);
            offset = WB_QSFPDD_TX_DISABLE_OFFSET;
            if (value != 0) {
                write_buf[0] = 0xff;
            }
        } else {
            SFF_ERR("eth%u module is unknown, module_type:%d\n", eth_index, module_type[0]);
            return -EINVAL;
        }

        ret = g_sff_drv->write_eth_eeprom_data(eth_index, write_buf, offset, 1);
        if (ret < 0) {
            SFF_ERR("set eth%u tx disable status %d failed, ret: %d\n", eth_index, value, ret);
            return -EIO;
        }
    }

    SFF_DBG("set eth%u tx disable status %d success\n", eth_index, value);
    return count;
}

static ssize_t eth_present_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;
    int ret;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_present_status);

    eth_index = obj->index;
    SFF_DBG("eth index: %u\n", eth_index);
    ret = g_sff_drv->get_eth_present_status(eth_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SFF_ERR("get eth%u present status failed, ret: %d\n", eth_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t eth_rx_los_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;
    int ret;
    char module_type[1], value[1];
    loff_t offset;
    char mask;

    check_p(g_sff_drv);
    check_p(g_sff_drv->read_eth_eeprom_data);
    check_p(g_sff_drv->get_eth_rx_los_status);

    eth_index = obj->index;
    SFF_DBG("eth index: %u\n", eth_index);
    memset(module_type, 0, sizeof(module_type));
    memset(value, 0, sizeof(value));
    ret = g_sff_drv->read_eth_eeprom_data(eth_index, module_type, 0, 1);
    if (ret < 0) {
        SFF_ERR("get eth%u module type failed, ret: %d\n", eth_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }

    if (module_type[0] == 0x03) {
        SFF_DBG("get eth%u module type is SFP\n", eth_index);
        ret = g_sff_drv->get_eth_rx_los_status(eth_index, buf, PAGE_SIZE);
        if (ret < 0) {
            SFF_ERR("get eth%u rx los status failed, ret: %d\n", eth_index, ret);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
        }
    } else {
        if ((module_type[0] == 0x11) || (module_type[0] == 0x0D)) {
            SFF_DBG("get eth%u module type is QSFP\n", eth_index);
            offset = WB_QSFP_RX_LOS_OFFSET;
            mask = 0xf;
        } else if ((module_type[0] == 0x18) || (module_type[0] == 0x1e)) {
            SFF_DBG("get eth%u module type is QSFP-DD\n", eth_index);
            offset = WB_QSFPDD_RX_LOS_OFFSET;
            mask = 0xff;
        } else {
            SFF_ERR("eth%u module is unknown, module_type:%d\n", eth_index, module_type[0]);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
        }

        ret = g_sff_drv->read_eth_eeprom_data(eth_index, value, offset, 1);
        if (ret < 0) {
            SFF_ERR("get eth%u module rx los value failed, ret: %d\n", eth_index, ret);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
        }

        if ((value[0] & mask) != 0) {
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 1);
        } else {
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 0);
        }
    }

    return ret;
}

static ssize_t eth_reset_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;
    int ret;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_reset_status);

    eth_index = obj->index;
    SFF_DBG("eth index: %u\n", eth_index);
    ret = g_sff_drv->get_eth_reset_status(eth_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SFF_ERR("get eth%u reset status failed, ret: %d\n", eth_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t eth_reset_store(struct switch_obj *obj, struct switch_attribute *attr,
                               const char* buf, size_t count)
{
    unsigned int eth_index;
    int ret, value;

    check_p(g_sff_drv);
    check_p(g_sff_drv->set_eth_reset_status);

    sscanf(buf, "%d", &value);
    eth_index = obj->index;
    ret = g_sff_drv->set_eth_reset_status(eth_index, value);
    if (ret < 0) {
        SFF_ERR("set eth%u reset status %d failed, ret: %d\n", eth_index, value, ret);
        return -EIO;
    }
    SFF_DBG("set eth%u reset status %d success\n", eth_index, value);
    return count;
}

static ssize_t eth_low_power_mode_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;
    int ret;
    char module_type[1], value[1];
    loff_t offset;
    char mask;

    check_p(g_sff_drv);
    check_p(g_sff_drv->read_eth_eeprom_data);
    check_p(g_sff_drv->get_eth_low_power_mode_status);

    eth_index = obj->index;
    SFF_DBG("eth index: %u\n", eth_index);
    memset(module_type, 0, sizeof(module_type));
    memset(value, 0, sizeof(value));
    ret = g_sff_drv->read_eth_eeprom_data(eth_index, module_type, 0, 1);
    if (ret < 0) {
        SFF_ERR("get eth%u module type failed, ret: %d\n", eth_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }

    if (module_type[0] == 0x03) {
        SFF_ERR("eth%u SFP module low power mode no support\n", eth_index);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    } else {
        if ((module_type[0] == 0x11) || (module_type[0] == 0x0D)) {
            SFF_DBG("get eth%u module type is QSFP\n", eth_index);
            offset = WB_QSFP_LP_MODE_OFFSET;
            mask = 0x3;
        } else if ((module_type[0] == 0x18) || (module_type[0] == 0x1e)) {
            SFF_DBG("get eth%u module type is QSFP-DD\n", eth_index);
            offset = WB_QSFPDD_LP_MODE_OFFSET;
            mask = 0x10;
        } else {
            SFF_ERR("eth%u module is unknown, module_type:%d\n", eth_index, module_type[0]);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
        }

        ret = g_sff_drv->read_eth_eeprom_data(eth_index, value, offset, 1);
        if (ret < 0) {
            SFF_ERR("get eth%u module lp mode value failed, ret: %d\n", eth_index, ret);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
        }

        if ((value[0] & mask) == mask) {
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 1);
        } else {
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 0);
        }
    }

    return ret;
}

static ssize_t eth_low_power_mode_store(struct switch_obj *obj, struct switch_attribute *attr,
                                        const char* buf, size_t count)
{
    unsigned int eth_index;
    int ret, value;
    char module_type[1], tmp_v[1];
    loff_t offset;
    unsigned char mask;

    check_p(g_sff_drv);
    check_p(g_sff_drv->read_eth_eeprom_data);
    check_p(g_sff_drv->write_eth_eeprom_data);
    check_p(g_sff_drv->get_eth_low_power_mode_status);

    sscanf(buf, "%d", &value);
    eth_index = obj->index;
    SFF_DBG("eth index: %u\n", eth_index);
    if (value < 0 || value > 1) {
        SFF_ERR("invalid value: %d, can't set eth%u lp mode status.\n", value, eth_index);
        return -EINVAL;
    }

    mask = 0;
    memset(module_type, 0, sizeof(module_type));
    memset(tmp_v, 0, sizeof(tmp_v));
    ret = g_sff_drv->read_eth_eeprom_data(eth_index, module_type, 0, 1);
    if (ret < 0) {
        SFF_ERR("get eth%u module type failed, ret: %d\n", eth_index, ret);
        return -EIO;
    }
    SFF_DBG("module type:0x%x\n", module_type[0]);

    if (module_type[0] == 0x03) {
        SFF_ERR("eth%u SFP module low power mode no support\n", eth_index);
        return -EINVAL;
    } else {
        if ((module_type[0] == 0x11) || (module_type[0] == 0x0D)) {
            SFF_DBG("get eth%u module type is QSFP\n", eth_index);
            offset = WB_QSFP_LP_MODE_OFFSET;
            mask = 0x3;
        } else if ((module_type[0] == 0x18) || (module_type[0] == 0x1e)) {
            SFF_DBG("get eth%u module type is QSFP-DD\n", eth_index);
            offset = WB_QSFPDD_LP_MODE_OFFSET;
            mask = 0x10;
        } else {
            SFF_ERR("eth%u module is unknown, module_type:%d\n", eth_index, module_type[0]);
            return -EINVAL;
        }

        ret = g_sff_drv->read_eth_eeprom_data(eth_index, tmp_v, offset, 1);
        if (ret < 0) {
            SFF_ERR("get eth%u module lp mode value failed, ret: %d\n", eth_index, ret);
            return -EIO;
        }

        if (value == 1) {
            tmp_v[0] = tmp_v[0] | mask;
        } else {
            tmp_v[0] = tmp_v[0] & (~mask);
        }

        ret = g_sff_drv->write_eth_eeprom_data(eth_index, tmp_v, offset, 1);
        if (ret < 0) {
            SFF_ERR("set eth%u module lp mode value failed, ret: %d\n", eth_index, ret);
            return -EIO;
        }
    }

    return count;
}

static ssize_t eth_interrupt_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;
    int ret;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_interrupt_status);

    eth_index = obj->index;
    SFF_DBG("eth index: %u\n", eth_index);
    ret = g_sff_drv->get_eth_interrupt_status(eth_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SFF_ERR("get eth%u interrupt status failed, ret: %d\n", eth_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t eth_eeprom_read(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
                               char *buf, loff_t offset, size_t count)
{
    struct switch_obj *eth_obj;
    ssize_t rd_len;
    unsigned int eth_index;

    check_p(g_sff_drv);
    check_p(g_sff_drv->read_eth_eeprom_data);

    eth_obj = to_switch_obj(kobj);
    eth_index = eth_obj->index;
    memset(buf, 0, count);
    rd_len = g_sff_drv->read_eth_eeprom_data(eth_index, buf, offset, count);
    if (rd_len < 0) {
        SFF_ERR("read eth%u eeprom data error, offset: 0x%llx, read len: %lu, ret: %ld.\n",
                eth_index, offset, count, rd_len);
        return -EIO;
    }

    SFF_DBG("read eth%u eeprom data success, offset:0x%llx, read len:%lu, really read len:%ld.\n",
            eth_index, offset, count, rd_len);

    return rd_len;
}

static ssize_t eth_eeprom_write(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
                                char *buf, loff_t offset, size_t count)
{
    struct switch_obj *eth_obj;
    ssize_t wr_len;
    unsigned int eth_index;

    check_p(g_sff_drv);
    check_p(g_sff_drv->write_eth_eeprom_data);

    eth_obj = to_switch_obj(kobj);
    eth_index = eth_obj->index;
    wr_len = g_sff_drv->write_eth_eeprom_data(eth_index, buf, offset, count);
    if (wr_len < 0) {
        SFF_ERR("write eth%u eeprom data error, offset: 0x%llx, read len: %lu, ret: %ld.\n",
                eth_index, offset, count, wr_len);
        return -EIO;
    }

    SFF_DBG("write eth%u eeprom data success, offset:0x%llx, write len:%lu, really write len:%ld.\n",
            eth_index, offset, count, wr_len);

    return wr_len;
}

/************************************eth* signal attrs*******************************************/
static struct switch_attribute eth_power_on_attr = __ATTR(power_on, S_IRUGO | S_IWUSR, eth_power_on_show, eth_power_on_store);
static struct switch_attribute eth_tx_fault_attr = __ATTR(tx_fault, S_IRUGO, eth_tx_fault_show, NULL);
static struct switch_attribute eth_tx_disable_attr = __ATTR(tx_disable, S_IRUGO | S_IWUSR, eth_tx_disable_show, eth_tx_disable_store);
static struct switch_attribute eth_present_attr = __ATTR(present, S_IRUGO, eth_present_show, NULL);
static struct switch_attribute eth_rx_los_attr = __ATTR(rx_los, S_IRUGO, eth_rx_los_show, NULL);
static struct switch_attribute eth_reset_attr = __ATTR(reset, S_IRUGO | S_IWUSR, eth_reset_show, eth_reset_store);
static struct switch_attribute eth_low_power_mode_attr = __ATTR(low_power_mode, S_IRUGO | S_IWUSR, eth_low_power_mode_show, eth_low_power_mode_store);
static struct switch_attribute eth_interrupt_attr = __ATTR(interrupt, S_IRUGO, eth_interrupt_show, NULL);

static struct attribute *sff_signal_attrs[] = {
    &eth_power_on_attr.attr,
    &eth_tx_fault_attr.attr,
    &eth_tx_disable_attr.attr,
    &eth_present_attr.attr,
    &eth_rx_los_attr.attr,
    &eth_reset_attr.attr,
    &eth_low_power_mode_attr.attr,
    &eth_interrupt_attr.attr,
    NULL,
};

static struct attribute_group sff_signal_attr_group = {
    .attrs = sff_signal_attrs,
};

/*******************************transceiver dir and attrs*******************************************/
static struct switch_attribute transceiver_power_on_attr = __ATTR(power_on, S_IRUGO | S_IWUSR, transceiver_power_on_show, transceiver_power_on_store);
static struct switch_attribute transceiver_number_attr = __ATTR(number, S_IRUGO, transceiver_number_show, NULL);
static struct switch_attribute transceiver_present_attr = __ATTR(present, S_IRUGO, transceiver_present_show, NULL);

static struct attribute *transceiver_dir_attrs[] = {
    &transceiver_power_on_attr.attr,
    &transceiver_number_attr.attr,
    &transceiver_present_attr.attr,
    NULL,
};

static struct attribute_group sff_transceiver_attr_group = {
    .attrs = transceiver_dir_attrs,
};

/* create eth* eeprom attributes */
static int sff_sub_single_create_eeprom_attrs(unsigned int index)
{
    int ret, eeprom_size;
    struct sff_obj_s *curr_sff;

    check_p(g_sff_drv->get_eth_eeprom_size);
    eeprom_size = g_sff_drv->get_eth_eeprom_size(index);
    if (eeprom_size <= 0) {
        SFF_INFO("eth%u, eeprom_size: %d, don't need to creat eeprom attr.\n",
                 index, eeprom_size);
        return 0;
    }

    curr_sff = &g_sff.sff[index - 1];
    sysfs_bin_attr_init(&curr_sff->bin);
    curr_sff->bin.attr.name = "eeprom";
    curr_sff->bin.attr.mode = 0644;
    curr_sff->bin.read = eth_eeprom_read;
    curr_sff->bin.write = eth_eeprom_write;
    curr_sff->bin.size = eeprom_size;

    ret = sysfs_create_bin_file(&curr_sff->sff_obj->kobj, &curr_sff->bin);
    if (ret) {
        SFF_ERR("eth%u, create eeprom bin error, ret: %d. \n", index, ret);
        return -EBADRQC;
    }

    SFF_DBG("eth%u, create bin file success, eeprom size:%d.\n", index, eeprom_size);
    curr_sff->sff_creat_bin_flag = 1;
    return 0;
}

static int sff_sub_single_create_kobj(struct kobject *parent, unsigned int index)
{
    struct sff_obj_s *curr_sff;
    char sff_dir_name[DIR_NAME_MAX_LEN];

    curr_sff = &g_sff.sff[index - 1];
    memset(sff_dir_name, 0, sizeof(sff_dir_name));
    snprintf(sff_dir_name, sizeof(sff_dir_name), "eth%d", index);
    curr_sff->sff_obj = switch_kobject_create(sff_dir_name, parent);
    if (!curr_sff->sff_obj) {
        SFF_ERR("create eth%d object error! \n", index);
        return -EBADRQC;
    }
    curr_sff->sff_obj->index = index;
    if (sysfs_create_group(&curr_sff->sff_obj->kobj, &sff_signal_attr_group) != 0) {
        switch_kobject_delete(&curr_sff->sff_obj);
        return -EBADRQC;
    }

    SFF_DBG("create eth%d dir and attrs success\n", index);
    return 0;
}

/* remove eth directory and attributes */
static void sff_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct sff_obj_s *curr_sff;

    curr_sff = &g_sff.sff[index - 1];
    if (curr_sff->sff_obj) {
        if (curr_sff->sff_creat_bin_flag) {
            sysfs_remove_bin_file(&curr_sff->sff_obj->kobj, &curr_sff->bin);
            curr_sff->sff_creat_bin_flag = 0;
        }
        sysfs_remove_group(&curr_sff->sff_obj->kobj, &sff_signal_attr_group);
        switch_kobject_delete(&curr_sff->sff_obj);
    }

    return;
}

static int sff_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    int ret;

    ret = sff_sub_single_create_kobj(parent, index);
    if (ret < 0) {
        SFF_ERR("create eth%d dir error.\n", index);
        return ret;
    }

    sff_sub_single_create_eeprom_attrs(index);
    return 0;
}

static int sff_sub_create_kobj_and_attrs(struct kobject *parent, int sff_num)
{
    unsigned int sff_index, i;

    g_sff.sff = kzalloc(sizeof(struct sff_obj_s) * sff_num, GFP_KERNEL);
    if (!g_sff.sff) {
        SFF_ERR("kzalloc g_sff.sff error, sff number = %d.\n", sff_num);
        return -ENOMEM;
    }

    for (sff_index = 1; sff_index <= sff_num; sff_index++) {
        if (sff_sub_single_create_kobj_and_attrs(parent, sff_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = sff_index; i > 0; i--) {
        sff_sub_single_remove_kobj_and_attrs(i);
    }
    kfree(g_sff.sff);
    g_sff.sff = NULL;
    return -EBADRQC;
}

/* create eth directory and attributes */
static int sff_sub_create(void)
{
    int ret;

    ret = sff_sub_create_kobj_and_attrs(&g_sff_obj->kobj, g_sff.sff_number);
    return ret;
}

/* delete eth directory and attributes */
static void sff_sub_remove(void)
{
    unsigned int sff_index;

    if (g_sff.sff) {
        for (sff_index = g_sff.sff_number; sff_index > 0; sff_index--) {
            sff_sub_single_remove_kobj_and_attrs(sff_index);
        }
        kfree(g_sff.sff);
        g_sff.sff = NULL;
    }
    g_sff.sff_number = 0;
    return;
}

/* create transceiver directory and attributes */
static int sff_transceiver_create(void)
{
    g_sff_obj = switch_kobject_create("transceiver", NULL);
    if (!g_sff_obj) {
        SFF_ERR("switch_kobject_create transceiver error!\n");
        return -ENOMEM;
    }
    g_sff_obj->index = 0;
    if (sysfs_create_group(&g_sff_obj->kobj, &sff_transceiver_attr_group) != 0) {
        switch_kobject_delete(&g_sff_obj);
        SFF_ERR("create transceiver dir attrs error!\n");
        return -EBADRQC;
    }
    return 0;
}

/* delete transceiver directory and attributes */
static void sff_transceiver_remove(void)
{
    if (g_sff_obj) {
        sysfs_remove_group(&g_sff_obj->kobj, &sff_transceiver_attr_group);
        switch_kobject_delete(&g_sff_obj);
    }

    return;
}

int s3ip_sysfs_sff_drivers_register(struct s3ip_sysfs_transceiver_drivers_s *drv)
{
    int ret, sff_num;

    SFF_INFO("s3ip_sysfs_sff_drivers_register...\n");
    if (g_sff_drv) {
        SFF_ERR("g_sff_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_eth_number);
    g_sff_drv = drv;

    sff_num = g_sff_drv->get_eth_number();
    if (sff_num <= 0) {
        SFF_ERR("eth number: %d, don't need to create transceiver dirs and attrs.\n", sff_num);
        g_sff_drv = NULL;
        return -EINVAL;
    }

    memset(&g_sff, 0, sizeof(struct sff_s));
    g_sff.sff_number = sff_num;
    ret = sff_transceiver_create();
    if (ret < 0) {
        SFF_ERR("create transceiver root dir and attrs failed, ret: %d\n", ret);
        g_sff_drv = NULL;
        return ret;
    }
    ret = sff_sub_create();
    if (ret < 0) {
        SFF_ERR("create transceiver sub dir and attrs failed, ret: %d\n", ret);
        sff_transceiver_remove();
        g_sff_drv = NULL;
        return ret;
    }
    SFF_INFO("s3ip_sysfs_sff_drivers_register success\n");
    return ret;
}

void s3ip_sysfs_sff_drivers_unregister(void)
{
    if (g_sff_drv) {
        sff_sub_remove();
        sff_transceiver_remove();
        g_sff_drv = NULL;
        SFF_DBG("s3ip_sysfs_sff_drivers_unregister success.\n");
    }
    return;
}

EXPORT_SYMBOL(s3ip_sysfs_sff_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_sff_drivers_unregister);
module_param(g_sff_loglevel, int, 0644);
MODULE_PARM_DESC(g_sff_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");

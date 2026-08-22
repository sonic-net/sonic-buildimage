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

#include "switch.h"
#include "transceiver_sysfs.h"
#include <wb_platform_common.h>

static int g_sff_loglevel = 0;
static bool g_sff_present_debug = 0;

#define MAX_SFF_NUMBER  (512)  /* max port number in device */
static DECLARE_BITMAP(g_sff_present_debug_bd, MAX_SFF_NUMBER);
/* simulate present condition. 0: pull out, 1: plug in*/

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

/**
 * Structure to store the power group information.
 */
struct power_group {
    int num_ports;    /**< Number of ports in the power group */
    int last_group_value;
    int current_group_value;
    int ports[WB_PORT_IN_GROUP_MAX]; /**< Array of port indices within the power group */
};

struct sff_obj_s {
    struct switch_obj *sff_obj;
    struct bin_attribute bin;
    struct bin_attribute bin_upg; /* Upgrade EEPROM binary attribute */
    int sff_creat_bin_flag;
    int sff_creat_bin_upg_flag;
    int power_ctrl;  /*  saves the power-on configurations for multiple ports and needs to consider locking mechanisms */
    int power_group_index; /* power group value: min:1, max:sff_number */
};

struct sff_s {
    unsigned int sff_number;
    unsigned int power_group_num; /* group number of product */
    unsigned int power_group_cfg; /* Port group configuration flag: 1 for on, 0 for off */
    struct mutex power_ctrl_update_lock; /* power_ctrl update lock */
    struct sff_obj_s *sff;
};

struct sff_protocol_type_s {
    char protocol;  /* for example: sfp qsfp cmis */
    int attr;       /* for example: rx_los tx_dis tx_fault */
    int offset;     /* read sfp eeprom offset */
};

static struct sff_s g_sff;
static struct switch_obj *g_sff_obj = NULL;
static struct s3ip_sysfs_transceiver_drivers_s *g_sff_drv = NULL;

/**
 * Global array to store information of all power groups.
 */
static struct power_group g_power_groups[WB_PORT_IN_GROUP_MAX] = {0};

static struct sff_protocol_type_s protocol_table[] = {
    {SFF_PROTOCOL_QSFP, WB_SFF_RX_LOS, QSFP_RX_LOS_OFFSET},
    {SFF_PROTOCOL_QSFP, WB_SFF_TX_DIS, QSFP_TX_DISABLE_OFFSET},
    {SFF_PROTOCOL_QSFP, WB_SFF_TX_FAULT, QSFP_TX_FAULT_OFFSET},
    {SFF_PROTOCOL_CMIS, WB_SFF_RX_LOS, CMIS_RX_LOS_OFFSET},
    {SFF_PROTOCOL_CMIS, WB_SFF_TX_DIS, CMIS_TX_DISABLE_OFFSET},
    {SFF_PROTOCOL_CMIS, WB_SFF_TX_FAULT, CMIS_TX_FAULT_OFFSET}
};

static ssize_t sff_attr_dbg_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret;
    unsigned int sff_index;
    struct switch_device_attribute  *tmp_attr;

    check_p(g_sff_drv);
    check_p(g_sff_drv->set_main_board_debug_sff_attr);

    sff_index = obj->index;
    tmp_attr = to_switch_device_attr(attr);
    check_p(tmp_attr);

    ret = g_sff_drv->set_main_board_debug_sff_attr(tmp_attr->type, sff_index, buf);
    if (ret < 0) {
        SFF_ERR("sff index: %u , type: %d , debug_value : %s, set fail.\n", sff_index, tmp_attr->type, buf);
        return -EIO;
    }
    SFF_DBG("sff index: %u , type: %d , debug_value : %s, set success.\n", sff_index, tmp_attr->type, buf);
    return count;
}

/**
 * transceiver_check_power_group_index - Check if the power group index is valid.
 * @power_group_index: The index of the power group to check.
 *
 * Returns: true if the power group index is valid, false otherwise.
 */
static bool transceiver_check_power_group_index(int power_group_index)
{
    return power_group_index > 0 &&
           power_group_index <= WB_PORT_POWER_GROUP_MAX &&
           power_group_index <= g_sff.sff_number;
}

/**
 * transceiver_update_eth_power_ctrl_value - Update the power control value for a specific Ethernet port.
 * @eth_index: The index of the Ethernet port.
 * @value: The new power control value.
 */
static void transceiver_update_eth_power_ctrl_value(int eth_index, int value)
{
    g_sff.sff[eth_index - 1].power_ctrl = value;
}

/**
 * transceiver_get_eth_power_ctrl_value - Get the current power control value for a specific Ethernet port.
 * @eth_index: The index of the Ethernet port.
 *
 * Returns: The current power control value of the port.
 */
static int transceiver_get_eth_power_ctrl_value(int eth_index)
{
    return g_sff.sff[eth_index - 1].power_ctrl;
}

/**
 * transceiver_power_ctrl_bitmap_get - Get the power control bitmap for all Ethernet ports.
 * @buf: The buffer to store the power control bitmap string.
 *
 * Returns: The length of the power control bitmap string.
 */
static ssize_t transceiver_power_ctrl_bitmap_get(char *buf)
{
    int i, len;
    struct sff_obj_s *port;

    /* power group config check */
    if (g_sff.power_group_cfg != WB_PORT_POWER_GROUP_CFG) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    mutex_lock(&g_sff.power_ctrl_update_lock);
    len = 0;
    for (i = 0; i < g_sff.sff_number; i++) {
        port = &g_sff.sff[i];
        /* Append the power_ctrl status of the current port to the string */
        if (!transceiver_check_power_group_index(port->power_group_index)) {
            len += snprintf(buf + len, len >= PAGE_SIZE - 1 ? 0 : PAGE_SIZE - len, "%s", SWITCH_BIT_DEV_ERROR);
        } else if (port->power_ctrl < 0) {
            len += snprintf(buf + len, len >= PAGE_SIZE - 1 ? 0 : PAGE_SIZE - len, "%s", SWITCH_BIT_NOT_CFG);
        } else {
            len += snprintf(buf + len, len >= PAGE_SIZE - 1 ? 0 : PAGE_SIZE - len, "%d", g_sff.sff[i].power_ctrl);
        }
    }
    mutex_unlock(&g_sff.power_ctrl_update_lock);

    len += snprintf(buf + len, len >= PAGE_SIZE - 1 ? 0 : PAGE_SIZE - len, "%s", "\n");

    SFF_DBG("count: %d, len: %s.\n", len, buf);
    return len;
}

/**
 * transceiver_update_power_group_current_value - Update the current value for a power group.
 * @group_index: The index of the power group.
 * @match_result: A pointer to store the result of matching the power control values within the group.
 *
 * Returns: 0 if successful, non-zero otherwise.
 */
static ssize_t transceiver_update_power_group_current_value(int group_index, int *match_result)
{
    int i, ethx, first_port, first_port_value, value;
    struct power_group *group;

    /* Parameter check */
    if (match_result == NULL) {
        SFF_ERR("Parameter check error: match_result is NULL for group_index %d.\n", group_index);
        return -EINVAL;
    }

    /* Parameter check */
    if (!transceiver_check_power_group_index(group_index)) {
        SFF_ERR("group_index%d, parameter check error.\n", group_index);
        return -EINVAL;
    }

    group = &g_power_groups[group_index - 1];
    if (group->num_ports <= 0) {
        SFF_ERR("get_power_group_value error: group_index %d has no ports or invalid num_ports %d.\n", group_index, group->num_ports);
        return -EINVAL;
    }

    *match_result = 1;
    first_port = group->ports[0];
    first_port_value = transceiver_get_eth_power_ctrl_value(first_port);

    /* Check if all ports match */
    for (i = 1; i < group->num_ports; i++) {
        ethx = group->ports[i];
        if (transceiver_get_eth_power_ctrl_value(ethx) != first_port_value) {
            *match_result = 0;
            break;
        }
    }

    if (!(*match_result)) {
        /* Print the values of all ports in the group */
        printk(KERN_INFO "\n");
        printk(KERN_INFO "Ports with power group %d must all be set to the same value to take effect. values: ", group_index);
        for (i = 0; i < group->num_ports; i++) {
            ethx = group->ports[i];
            value = transceiver_get_eth_power_ctrl_value(ethx);
            switch (value) {
                case -1:
                    printk(KERN_INFO "eth%d=%s(%d), ", ethx, SWITCH_DEV_NO_CFG, value);
                    break;
                case 0:
                    printk(KERN_INFO "eth%d=Off(%d), ", ethx, value);
                    break;
                case 1:
                    printk(KERN_INFO "eth%d=On(%d), ", ethx, value);
                    break;
                default:
                    printk(KERN_INFO "eth%d=%s(%d), ", ethx, SWITCH_DEV_ERROR, value);
                    break;
            }
        }
        printk(KERN_INFO "\n");
    } else {
        group->current_group_value = first_port_value;
        SFF_DBG("All ports in power group %d are set to the same value: %d.\n", group_index, first_port_value);
    }

    return 0;
}

/**
 * transceiver_get_power_group_last_value - Get the last value for a power group.
 * @group_index: The index of the power group.
 *
 * Returns: 0 if successful, non-zero otherwise.
 */
static ssize_t transceiver_get_power_group_last_value(int group_index)
{
    int ethx, ret;
    struct power_group *group;
    char power_on_buf[256];

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_power_on_status);

    if (!transceiver_check_power_group_index(group_index)) {
        SFF_ERR("group_index%d, parameter check error.\n", group_index);
        return -EINVAL;
    }

    group = &g_power_groups[group_index - 1];
    if (group->num_ports <= 0) {
        SFF_ERR("group_index%d get_power_group_value error. group->num_ports=%d\n", group_index, group->num_ports);
        return -EINVAL;
    }

    /* last_group_value not set, get from hardware */
    if (group->last_group_value == -1) {
        ethx = group->ports[0];
        mem_clear(power_on_buf, sizeof(power_on_buf));
        ret = g_sff_drv->get_eth_power_on_status(ethx, power_on_buf, sizeof(power_on_buf));
        if (ret < 0) {
            SFF_DBG("get_eth_power_on_status failed, sff index:%u, ret:%d\n", ethx, ret);
            return ret;
        }
        if ((strncmp(power_on_buf, SWITCH_DEV_NO_SUPPORT, strlen(SWITCH_DEV_NO_SUPPORT)) == 0) ||
            (strncmp(power_on_buf, SWITCH_DEV_ERROR, strlen(SWITCH_DEV_ERROR)) == 0)) {
            SFF_DBG("get_eth_power_on_status unsupport or error. sff index:%u, ret:%d\n", ethx, ret);
            return -EINVAL;
        }

        group->last_group_value = power_on_buf[0] - '0';
    }

    SFF_DBG("last_group_value of group%d is %d\n", group_index, group->last_group_value);
    return 0;
}

/**
 * transceiver_update_power_group_value - Update the power control value for a power group.
 * @group_index: The index of the power group.
 *
 * Returns: 0 if successful, non-zero otherwise.
 */
static int transceiver_update_power_group_value(int group_index)
{
    int ret;
    struct power_group *group;

    /* Parameter check */
    if (!transceiver_check_power_group_index(group_index)) {
        SFF_ERR("Port group index %d is out of range.\n", group_index);
        return -EINVAL;
    }

    group = &g_power_groups[group_index - 1];
    SFF_DBG("To set power on status for group %d. current_value: %d, last_value: %d\n",
            group_index, group->current_group_value, group->last_group_value);

    /* Validate the current_group_value (must be 0 or 1) */
    if (group->current_group_value != 0 && group->current_group_value != 1) {
        printk(KERN_INFO "Update hardware state for Group%d Failed, value Invalid. last_group_value: %d, current_group_value: %d\n",
               group_index,
               group->last_group_value,
               group->current_group_value);
        return -EINVAL;
    }

    printk(KERN_INFO "Update hardware state for Group%d. last_group_value: %d, current_group_value: %d\n",
           group_index,
           group->last_group_value,
           group->current_group_value);

    /* update the hardware state for the group with current value */
    ret = g_sff_drv->set_eth_power_on_status(group->ports[0], group->current_group_value);
    if (ret < 0) {
        SFF_ERR("Failed to set power on status for group %d eth%d, ret: %d\n", group_index, group->ports[0], ret);
        return ret;
    }
    group->last_group_value = group->current_group_value;

    SFF_DBG("Succeed to set power on status for group %d, ret: %d\n", group_index, ret);
    return 0;
}

/**
 * eth_power_ctrl_single_group_store - Store the power control value for a single power group.
 * @group_index: The index of the power group.
 *
 * Returns: 0 if successful, non-zero otherwise.
 */
static ssize_t eth_power_ctrl_single_group_store(int group_index)
{
    int ret;
    int match_result;

    /* power group config check */
    if (g_sff.power_group_cfg != WB_PORT_POWER_GROUP_CFG) {
        SFF_ERR("Power group is not configured.\n");
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    /* If the power group is not initialized, return an error indicating unsupported operation */
    if (!transceiver_check_power_group_index(group_index)) {
        SFF_ERR("Power group is not initialized, cannot set group %d power control status.\n", group_index);
        return -EINVAL;
    }

    ret = transceiver_update_power_group_current_value(group_index, &match_result);
    if (ret < 0 || match_result == 0) {
        SFF_ERR("Failed to update current value for group %d, match_result: %d, ret: %d\n", group_index, match_result, ret);
        return ret;
    }

    ret = transceiver_get_power_group_last_value(group_index);
    if (ret < 0) {
        SFF_ERR("Failed to update last value for group %d, ret: %d\n", group_index, ret);
        /* return ret; last value get failed*/
    }

    ret = transceiver_update_power_group_value(group_index);
    if (ret < 0) {
        SFF_ERR("Failed to update power control status for group %d, ret: %d\n", group_index, ret);
        return ret;
    }

    return 0;
}

/**
 * transceiver_power_ctrl - Update power control for all Ethernet ports and their groups.
 * @buf: Input buffer containing power control bitmap string ('0'/'1' chars).
 * @actual_strlen: Length of the control string (must be 1 or sff_number).
 *
 * This function sets the power control value for each Ethernet port according to
 * the input string, and then updates the hardware power control status for all the
 * configured power groups.
 *
 * Returns: 0 on success, negative error code on failure.
 */
static int transceiver_power_ctrl(const char *buf, size_t actual_strlen)
{
    int ret, i;

    for (i = 0; i < g_sff.sff_number; i++) {
        /* support echo 0 or 1 > /sys/s3ip/transceiver/power_on */
        if (actual_strlen == 1) {
            transceiver_update_eth_power_ctrl_value(i + 1, buf[0] - '0');
        } else {
            transceiver_update_eth_power_ctrl_value(i + 1, buf[i] - '0');
        }
    }

    for (i = 0; i < g_sff.power_group_num && i < WB_PORT_IN_GROUP_MAX; i++) {
        ret = eth_power_ctrl_single_group_store(i + 1);
        if (ret < 0) {
            SFF_ERR("Failed to update power control status for group %d, ret: %d\n", i + 1, ret);
            return ret;
        }
    }

    return 0;
}

static int transceiver_power_ctrl_lock(const char *buf, size_t actual_strlen)
{
    int ret;
    mutex_lock(&g_sff.power_ctrl_update_lock);
    ret = transceiver_power_ctrl(buf, actual_strlen);
    mutex_unlock(&g_sff.power_ctrl_update_lock);
    return ret;
}

/**
 * transceiver_power_ctrl_bitmap_set - Set the power control bitmap for all Ethernet ports.
 * @buf: The buffer containing the new power control bitmap string.
 * @count: The length of the buffer.
 *
 * Returns: The length of the buffer, or an error code.
 */
static ssize_t transceiver_power_ctrl_bitmap_set(const char* buf, size_t count)
{
    int i, value, ret;
    size_t actual_strlen;
    char *pos;

    /* power group config check */
    if (g_sff.power_group_cfg != WB_PORT_POWER_GROUP_CFG) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    SFF_DBG("Start, power ctrl count: %zu. buf: %s\n", count, buf);
    pos = strchr(buf, '\n');
    if (pos != NULL) {
        /* There are line breaks in the shell operation. */
        actual_strlen = strlen(buf) - 1;
    } else {
        actual_strlen = strlen(buf);
    }
    SFF_DBG("Start, power ctrl actual_strlen: %zu. sff_number: %d\n", actual_strlen, g_sff.sff_number);
    
    /* Check if the input string is not too long */
    if (actual_strlen != g_sff.sff_number && actual_strlen != 1) {
        SFF_ERR("Invalid parameter, count: %zu, buf: %s.\n", actual_strlen, buf);
        return -EINVAL;
    }

    /* Iterate over each character in the input string */
    for (i = 0; i < g_sff.sff_number; i++) {
        /* Convert character to integer */
        value = buf[i] - '0';
        /* Validate the character (must be '0' or '1') */
        if (value != 0 && value != 1) {
            SFF_ERR("invaild parameter (must be '0' or '1'), eth%d count: %zu. buf: %s.\n", i + 1, count, buf);
            return -EINVAL;
        }
        /* support echo 0 or 1 > /sys/s3ip/transceiver/power_ctrl_bitmap */
        if (actual_strlen == 1) {
            break;
        }
    }

    for (i = 0; i < g_sff.sff_number; i++) {
        /* Validate power group index */
        if (!transceiver_check_power_group_index(g_sff.sff[i].power_group_index)) {
            SFF_ERR("invaild power_group_index eth%d power_group_index: %d.\n", i + 1, g_sff.sff[i].power_group_index);
            return -EINVAL;
        }

        SFF_DBG("check eth%d power control status %d success\n", i + 1, value);
    }

    ret = transceiver_power_ctrl_lock(buf, actual_strlen);
    if (ret) {
        SFF_DBG("Not all group update OK. count: %zu, buf: %s.\n", count, buf);
        return -EBADRQC;
    }

    SFF_DBG("All groups update OK. count: %zu, buf: %s.\n", count, buf);
    return count;
}

/**
 * transceiver_power_on_bitmap_show - Show the power on bitmap for all Ethernet ports.
 * @obj: The switch object.
 * @attr: The switch attribute.
 * @buf: The buffer to store the power on bitmap string.
 *
 * Returns: The length of the power on bitmap string.
 */
static ssize_t transceiver_power_on_bitmap_show(struct switch_obj *obj, struct switch_attribute *attr,
        char *buf)
{
    int i, len, ret;
    char value[256];

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_power_on_status);

    /* power group config check */
    if (g_sff.power_group_cfg != WB_PORT_POWER_GROUP_CFG) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    mem_clear(value, sizeof(value));
    len = 0;
    for (i = 0; i < g_sff.sff_number; i++) {
        ret = g_sff_drv->get_eth_power_on_status(i + 1, value, sizeof(value));
        if (ret < 0 || (strncmp(value, SWITCH_DEV_ERROR, strlen(SWITCH_DEV_ERROR)) == 0)) {
            SFF_ERR("eth%d get_eth_power_on_status error. ret=%d\n", i + 1, ret);
            len += snprintf(buf + len, len >= PAGE_SIZE - 1 ? 0 : PAGE_SIZE - len, "%s", SWITCH_BIT_DEV_ERROR);
            continue;
        }

        if (strncmp(value, SWITCH_DEV_NO_SUPPORT, strlen(SWITCH_DEV_NO_SUPPORT)) == 0) {
            SFF_ERR("eth%d get_eth_power_on_status unsupport.  buf=%s\n", i + 1, buf);
            len += snprintf(buf + len, len >= PAGE_SIZE - 1 ? 0 : PAGE_SIZE - len, "%s", SWITCH_BIT_NOT_CFG);
            continue;
        }

        /* Append the power_ctrl status of the current port to the string */
        len += snprintf(buf + len, len >= PAGE_SIZE - 1 ? 0 : PAGE_SIZE - len, "%d", value[0] - '0');
    }

    len += snprintf(buf + len, len >= PAGE_SIZE - 1 ? 0 : PAGE_SIZE - len, "%s", "\n");

    SFF_DBG("count: %d, len: %s.\n", len, buf);
    return len;
}

/**
 * transceiver_power_on_show - Show the power-on status of all transceivers
 * @obj:   Pointer to the switch object associated with this sysfs entry
 * @attr:  Pointer to the sysfs attribute structure (unused)
 * @buf:   Buffer to store the formatted output string
 *
 * This function outputs the power-on states of all transceivers via sysfs.
 * If the power group configuration is not supported, returns "NA".
 * On error, an error message is logged.
 *
 * Return: On success, returns the length of the string written to @buf.
 *         On failure, returns negative error code.
 */
static ssize_t transceiver_power_on_show(struct switch_obj *obj, struct switch_attribute *attr,
        char *buf)
{
    int len, i, power_on_num;
    int ret;

    if (g_sff.power_group_cfg == WB_PORT_POWER_GROUP_CFG) {
        len = transceiver_power_on_bitmap_show(obj, attr, buf);
        if (len - 1 == g_sff.sff_number) {
            power_on_num = 0;
            for (i = 0; i < g_sff.sff_number; i++) {
                if (buf[i] == '1') {
                    power_on_num++;
                }
            }

            mem_clear(buf, PAGE_SIZE);
            if (power_on_num == 0) {
                len = snprintf(buf, PAGE_SIZE, "%d\n", 0);
            } else if (power_on_num == g_sff.sff_number) {
                len = snprintf(buf, PAGE_SIZE, "%d\n", 1);
            } else {
                check_p(g_sff_drv);
                check_p(g_sff_drv->get_sff_power_status_mix_value); 

                ret = g_sff_drv->get_sff_power_status_mix_value(buf, PAGE_SIZE);
                len = snprintf(buf, PAGE_SIZE, "%d\n", ret);
            }
        } else {
            SFF_ERR("Failed to show power on bitmap, ret: %d\n", len);
            return -EINVAL;
        }
    } else {
        check_p(g_sff_drv);
        check_p(g_sff_drv->get_transceiver_power_on_status);

        len = g_sff_drv->get_transceiver_power_on_status(buf, PAGE_SIZE);
        if (len < 0) {
            SFF_ERR("Failed to show power on, ret: %d\n", len);
            return -EINVAL;
        }
    }

    return len;
}

/**
 * transceiver_power_ctrl_bitmap_show - Show the power control bitmap for all Ethernet ports.
 * @obj: The switch object.
 * @attr: The switch attribute.
 * @buf: The buffer to store the power control bitmap string.
 *
 * Returns: The length of the power control bitmap string.
 */
static ssize_t transceiver_power_ctrl_bitmap_show(struct switch_obj *obj, struct switch_attribute *attr,
        char *buf)
{
    int len;

    len = transceiver_power_ctrl_bitmap_get(buf);
    SFF_DBG("count: %d, len: %s.\n", len, buf);
    return len;
}

/**
 * transceiver_power_ctrl_bitmap_store - Store the power control bitmap for all Ethernet ports.
 * @obj: The switch object.
 * @attr: The switch attribute.
 * @buf: The buffer containing the new power control bitmap string.
 * @count: The length of the buffer.
 *
 * Returns: The length of the buffer, or an error code.
 */
static ssize_t transceiver_power_ctrl_bitmap_store(struct switch_obj *obj, struct switch_attribute *attr,
        const char* buf, size_t count)
{
    int ret;

    ret = transceiver_power_ctrl_bitmap_set(buf, count);

    SFF_DBG("count: %d, buf: %s.\n", ret, buf);
    return ret;
}

/**
 * transceiver_debug_show - Show debug information for all Ethernet ports.
 * @obj: The switch object.
 * @attr: The switch attribute.
 * @buf: The buffer to store the debug information string.
 *
 * Returns: The length of the debug information string.
 */
static ssize_t transceiver_debug_show(struct switch_obj *obj, struct switch_attribute *attr,
                                      char *buf)
{
    int i, ret;
    struct sff_obj_s *port;
    char power_on_buf[256], power_group[16], power_ctrl[16];

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_power_on_status);

    /* Check if power group config is enabled */
    if (g_sff.power_group_cfg != WB_PORT_POWER_GROUP_CFG) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    printk(KERN_INFO "Port\tPort Group\tConfig\tPower Status\n");

    mutex_lock(&g_sff.power_ctrl_update_lock);
    /*iterate the port */
    for (i = 0; i < g_sff.sff_number; i++) {
        port = &g_sff.sff[i];

        mem_clear(power_group, sizeof(power_group));
        mem_clear(power_ctrl, sizeof(power_ctrl));
        if (!transceiver_check_power_group_index(port->power_group_index)) {
            snprintf(power_group, sizeof(power_group), SWITCH_DEV_ERROR);
            snprintf(power_ctrl, sizeof(power_ctrl), SWITCH_DEV_ERROR);
        } else {
            snprintf(power_group, sizeof(power_group), "%d", port->power_group_index);
            if (port->power_ctrl < 0) {
                snprintf(power_ctrl, sizeof(power_ctrl), SWITCH_DEV_NO_CFG);
            } else {
                snprintf(power_ctrl, sizeof(power_ctrl), "%d", port->power_ctrl);
            }
        }

        mem_clear(power_on_buf, sizeof(power_on_buf));
        ret = g_sff_drv->get_eth_power_on_status(i + 1, power_on_buf, sizeof(power_on_buf));
        if (ret < 0) {
            snprintf(power_on_buf, sizeof(power_on_buf), SWITCH_DEV_ERROR);
            SFF_DBG("get_eth_power_on_status failed, sff index:%u, ret:%d\n", i + 1, ret);
        }

        printk(KERN_INFO "%d\t\t%s\t%s\t%s\n",
               i + 1, power_group, power_ctrl, power_on_buf);
    }
    mutex_unlock(&g_sff.power_ctrl_update_lock);

    return (ssize_t)snprintf(buf, PAGE_SIZE, "Please view the debug information using the 'dmesg' command.\n\n");
}

static ssize_t eth_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;
    struct switch_device_attribute *tmp_attr;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_attr);

    eth_index = obj->index;
    SFF_DBG("eth index: %u\n", eth_index);
    tmp_attr = to_switch_device_attr(attr);
    check_p(tmp_attr);

    return g_sff_drv->get_eth_attr(eth_index, tmp_attr->type, buf, PAGE_SIZE);
}

static ssize_t eth_attr_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int eth_index, value;
    int ret;
    struct switch_device_attribute *tmp_attr;

    check_p(g_sff_drv);
    check_p(g_sff_drv->set_eth_attr);

    value = 0;
    ret = kstrtoint(buf, 0, &value);
    if (ret != 0) {
        SFF_ERR("Invaild value ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    eth_index = obj->index;
    if (value < 0 || value > 1) {
        SFF_ERR("invalid value: %d, can't set eth%u attr.\n", value, eth_index);
        return -EINVAL;
    }

    tmp_attr = to_switch_device_attr(attr);
    check_p(tmp_attr);

    ret = g_sff_drv->set_eth_attr(eth_index, tmp_attr->type, value);
    if (ret < 0) {
        SFF_ERR("set eth%u attr:%d failed, value:0x%x, ret: %d.\n", eth_index, tmp_attr->type, value, ret);
        return ret;
    }

    SFF_DBG("set eth%u attr:%d success, value: 0x%x.\n", eth_index, tmp_attr->type, value);
    return count;
}

static ssize_t transceiver_power_on_set(struct switch_obj *obj, struct switch_attribute *attr,
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

static ssize_t transceiver_power_on_store(struct switch_obj *obj, struct switch_attribute *attr,
        const char* buf, size_t count)
{
    int ret;

    if (g_sff.power_group_cfg == WB_PORT_POWER_GROUP_CFG) {
        ret = transceiver_power_ctrl_bitmap_store(obj, attr, buf, count);
    } else {
        ret = transceiver_power_on_set(obj, attr, buf, count);
    }

    return ret;
}

static ssize_t transceiver_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_sff.sff_number);
}

static ssize_t sw_init_enable_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret, value;

    check_p(g_sff_drv);
    check_p(g_sff_drv->set_sw_init_enable);

    ret = kstrtoint(buf, 0, &value);
    if (ret) {
        SFF_ERR("system_value_store, input parameter: %s error. ret:%d\n", buf, ret);
        return ret;
    }

    if ((value != 0) && (value != 1)) {
        SFF_ERR("Invaild value: %d, only support write 0 or 1 to sw_init_enable.\n", value);
        return -EINVAL;
    }

    ret = g_sff_drv->set_sw_init_enable(WB_SYSTEM_SW_INIT_ENABLE, value);
    if (ret < 0) {
        SFF_ERR("set sw_init_enable %d failed, ret: %d\n", value, ret);
        return -EIO;
    }
    SFF_DBG("set sw_init_enable %d success\n", value);

    return count;
}

static ssize_t sw_init_enable_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    check_p(g_sff_drv);
    check_p(g_sff_drv->get_sw_init_enable);

    return g_sff_drv->get_sw_init_enable(WB_SYSTEM_SW_INIT_ENABLE, buf, PAGE_SIZE);
}

static ssize_t eth_optoe_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int sff_index;
    int optoe_type;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_optoe_type);

    sff_index = obj->index;
    SFF_DBG("eth_optoe_type_show, sff index:%u\n", sff_index);
    return g_sff_drv->get_eth_optoe_type(sff_index, &optoe_type, buf, PAGE_SIZE);
}

static ssize_t eth_optoe_type_store(struct switch_obj *obj, struct switch_attribute *attr,
                                        const char* buf, size_t count)
{
    unsigned int sff_index;
    int ret;
    int optoe_type;

    check_p(g_sff_drv);
    check_p(g_sff_drv->set_eth_optoe_type);

    optoe_type = 0;
    ret = kstrtoint(buf, 0, &optoe_type);
    if (ret != 0) {
        SFF_ERR("invaild optoe_type ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    sff_index = obj->index;
    SFF_DBG("eth_optoe_type_store, sff index:%u, optoe_type:%d\n", sff_index, optoe_type);
    ret = g_sff_drv->set_eth_optoe_type(sff_index, optoe_type);
    if(ret < 0) {
        SFF_ERR("set_eth_optoe_type error. sff index:%u, ret:%d\n", sff_index, ret);
        return ret;
    }

    SFF_DBG("eth_optoe_type_store ok. sff index:%u, optoe_type:%d\n", sff_index, optoe_type);
    return count;
}

static ssize_t dev_available_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf) 
{
    unsigned int sff_index;
    int ret;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_dev_available);

    sff_index = obj->index;
    SFF_DBG("dev_available_show, sff index:%u\n", sff_index);
    ret = g_sff_drv->get_eth_dev_available(sff_index, buf, PAGE_SIZE);
    if(ret < 0) {
        SFF_ERR("get_eth_dev_available error. sff index:%u, ret:%d\n", sff_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_ERROR);
    }

    return ret;
}

static ssize_t dev_available_store(struct switch_obj *obj, struct switch_attribute *attr,
                                        const char* buf, size_t count)
{
    unsigned int sff_index;
    int ret;
    int status;

    check_p(g_sff_drv);
    check_p(g_sff_drv->set_eth_dev_available);

    status = 0;
    ret = kstrtoint(buf, 0, &status);
    if (ret != 0) {
        SFF_ERR("invaild dev_available status ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    sff_index = obj->index;
    SFF_DBG("dev_available_store, sff index:%u, status:%d\n", sff_index, status);
    ret = g_sff_drv->set_eth_dev_available(sff_index, status);
    if(ret < 0) {
        SFF_ERR("set_eth_dev_available error. sff index:%u, ret:%d\n", sff_index, ret);
        return ret;
    }

    SFF_DBG("dev_available_store ok. sff index:%u, status:%d\n", sff_index, status);
    return count;
}

static ssize_t transceiver_present_show(struct switch_obj *obj, struct switch_attribute *attr,
                                        char *buf)
{
    check_p(g_sff_drv);
    check_p(g_sff_drv->get_transceiver_present_status);

    return g_sff_drv->get_transceiver_present_status(buf, PAGE_SIZE);
}

static ssize_t eth_port_led_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int sff_index;
    int ret;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_port_led_status);

    sff_index = obj->index;
    SFF_DBG("eth_port_led_show, sff index:%u\n", sff_index);
    ret = g_sff_drv->get_eth_port_led_status(sff_index, buf, PAGE_SIZE);
    if(ret < 0) {
        SFF_ERR("get_eth_port_led_status error. sff index:%u, ret:%d\n", sff_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_ERROR);
    }

    return ret;
}

static ssize_t eth_port_bus_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int sff_index;
    int ret;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_port_bus_status);

    sff_index = obj->index;
    SFF_DBG("get_eth_port_bus_status, sff index:%u\n", sff_index);
    ret = g_sff_drv->get_eth_port_bus_status(sff_index, buf, PAGE_SIZE);
    if(ret < 0) {
        SFF_ERR("eth_port_bus_status_show error. sff index:%u, ret:%d\n", sff_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_ERROR);
    }

    return ret;
}

static ssize_t eth_port_led_store(struct switch_obj *obj, struct switch_attribute *attr,
                                        const char* buf, size_t count)
{
    unsigned int sff_index;
    int ret;
    int led_status;

    check_p(g_sff_drv);
    check_p(g_sff_drv->set_eth_port_led_status);

    sscanf(buf, "%d", &led_status);

    sff_index = obj->index;
    SFF_DBG("port_led_type_store, sff index:%u, led_status:%d\n", sff_index, led_status);
    ret = g_sff_drv->set_eth_port_led_status(sff_index, led_status);
    if(ret < 0) {
        SFF_ERR("set_eth_port_led_status error. sff index:%u, ret:%d\n", sff_index, ret);
        return ret;
    }

    SFF_DBG("port_led_type_store ok. sff index:%u, led_status:%d\n", sff_index, led_status);
    return count;
}

ssize_t eth_debug_present_get(char *buf)
{
    unsigned int i;

    memset(buf, 0, PAGE_SIZE);
    for (i = 0; i < g_sff.sff_number; i++)
    {
        buf[i] = test_bit(i, g_sff_present_debug_bd) ? '1' : '0';
    }
    buf[i] = '\n';
    return (ssize_t)strlen(buf);
}

ssize_t eth_debug_present_set(const char* buf, size_t count)
{
    unsigned int i;

    /* parameter check */
    for (i = 0; (i < count - 1) && i < g_sff.sff_number; i++) {
        if (buf[i] != '0' && buf[i] != '1') {
            SFF_ERR("param invalid, can not set debug present: %d.\n", buf[i]);
            return -EINVAL;
        }

        if (buf[i] - '0') {
            set_bit(i, g_sff_present_debug_bd);
        } else {
            clear_bit(i, g_sff_present_debug_bd);
        }
    }

    return count;
}

static ssize_t eth_power_on_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_power_on_status);

    eth_index = obj->index;
    SFF_DBG("eth index: %u\n", eth_index);
    return g_sff_drv->get_eth_power_on_status(eth_index, buf, PAGE_SIZE);
}

static ssize_t eth_power_on_set(struct switch_obj *obj, struct switch_attribute *attr,
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
        return ret;
    }
    SFF_DBG("set eth%u power on status %d success\n", eth_index, value);
    return count;
}

static ssize_t eth_eeprom_low_page_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_e2_low_page);

    eth_index = obj->index;
    SFF_DBG("eth index: %u\n", eth_index);
    return g_sff_drv->get_eth_e2_low_page(eth_index, buf, PAGE_SIZE);
}

static ssize_t eth_temp_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_temp);

    eth_index = obj->index;
    SFF_DBG("eth index: %u\n", eth_index);
    return g_sff_drv->get_eth_temp(eth_index, buf, PAGE_SIZE);
}

static ssize_t eth_present_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;
    int ret, res;
    char debug_file_buf[DEBUG_FILE_SIZE];

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_present_status);

    eth_index = obj->index;
    SFF_DBG("eth index: %u\n", eth_index);

    if(test_bit(eth_index - 1, g_sff_present_debug_bd) == 0) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", MODULE_STATUS_ABSENT);
    }

    ret = g_sff_drv->get_eth_present_status(eth_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SFF_ERR("get eth%u present status failed, ret: %d\n", eth_index, ret);
        return ret;
    }

    if (g_sff_present_debug) {
        SFF_INFO("s3ip sysfs sff present debug is enable\n");
        if (strcmp(buf, DEV_ABSENT_STR) == 0) {
            SFF_DBG("eth%d absent, return act value\n", eth_index);
            return ret;
        }

        if ((strncmp(buf, SWITCH_DEV_NO_SUPPORT, strlen(SWITCH_DEV_NO_SUPPORT)) == 0) || (strncmp(buf, SWITCH_DEV_ERROR, strlen(SWITCH_DEV_ERROR)) == 0)) {
            SFF_DBG("eth%d status sysfs unsupport or error\n", eth_index);
            return ret;
        }

        mem_clear(debug_file_buf, sizeof(debug_file_buf));
        res = dev_debug_file_read(SINGLE_TRANSCEIVER_PRESENT_DEBUG_FILE, eth_index, debug_file_buf, sizeof(debug_file_buf));
        if (res) {
            SFF_ERR("eth%u present debug file read failed, ret: %d\n", eth_index, res);
            return ret;
        }

        if ((strcmp(debug_file_buf, DEV_PRESEN_STR) == 0) || (strcmp(debug_file_buf, DEV_ABSENT_STR) == 0)) {
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s", debug_file_buf);
        } else {
            SFF_ERR("eth%d present debug file value err, value: %s, not 0 or 1\n", eth_index, debug_file_buf);
            return ret;
        }
    }
    return ret;
}

static ssize_t eth_i2c_bus_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_i2c_bus);

    eth_index = obj->index;
    SFF_DBG("eth index: %u\n", eth_index);
    return g_sff_drv->get_eth_i2c_bus(eth_index, buf, PAGE_SIZE);
}

static ssize_t eth_cage_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_cage_type);

    eth_index = obj->index;
    SFF_DBG("eth index: %u\n", eth_index);
    return g_sff_drv->get_eth_cage_type(eth_index, buf, PAGE_SIZE);
}

static int eth_get_protocol_type(unsigned int eth_index, char *protocol_type)
{
    int ret;
    char reg_val;

    check_p(g_sff_drv->get_eth_present_status);

    ret = g_sff_drv->read_eth_eeprom_data(eth_index, &reg_val, SFF_E2_IDENTIFIER_REG, 1, E2_NON_UPGRADE_FLAG);
    if (ret < 0) {
        SFF_ERR("get eth%u e2 protocol type failed, ret: %d\n", eth_index, ret);
        /* not present or read failed */
        return ret;
    }

    switch (reg_val) {
    case SFF_Identifier_SFP:
        *protocol_type = SFF_PROTOCOL_SFP;
        break;
    case SFF_Identifier_QSFP_PLUS:
    case SFF_Identifier_QSFP28:
        *protocol_type = SFF_PROTOCOL_QSFP;
        break;
    case SFF_Identifier_QSFPDD:
    case SFF_Identifier_OSFP:
    case SFF_Identifier_DSFP:
    case SFF_Identifier_QSFP_CMIS:
        *protocol_type = SFF_PROTOCOL_CMIS;
        break;
    default:
        SFF_ERR("protocol type:%d not support.\n", *protocol_type);
        return -EIO;
    }

    return 0;
}

static ssize_t eth_protocol_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;
    int ret;
    char protocol_type;

    eth_index = obj->index;
    SFF_DBG("eth index: %u\n", eth_index);

    if(test_bit(eth_index - 1, g_sff_present_debug_bd) == 0) {
        SFF_DBG("eth:%d open present debug, return not present.\n", eth_index);
        return -ENOENT;
    }

    ret = eth_get_protocol_type(eth_index, &protocol_type);
    if (ret < 0) {
        if (ret == -WB_SYSFS_RV_ABSENT) {
            SFF_DBG("transceiver:%u protocol type absent.\n", eth_index);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            SFF_ERR("transceiver:%u get protocol type failed, return value: %d.\n", eth_index, ret);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", protocol_type);
}

static int eth_get_offset_by_protocol_type(char protocol_type, int attr_type)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(protocol_table); i++) {
        if (protocol_table[i].protocol == protocol_type && protocol_table[i].attr == attr_type) {
            return protocol_table[i].offset;
        }
    }

    SFF_ERR("get offset by protocol failed. type=%d, attr=%d\n", protocol_type, attr_type);
    return -EIO;
}

#if PRODUCT_STRATEGY != PRODUCT_STRATEGY_2
static ssize_t eth_protocol_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;
    struct switch_device_attribute  *tmp_attr;
    int ret;
    char protocol_type, mask;
    char value;
    loff_t offset;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_attr);

    eth_index = obj->index;
    tmp_attr = to_switch_device_attr(attr);
    check_p(tmp_attr);
    SFF_DBG("eth index: %u\n", eth_index);

    if(test_bit(eth_index - 1, g_sff_present_debug_bd) == 0) {
        SFF_DBG("eth:%d open present debug, return not present.\n", eth_index);
        return -ENOENT;
    }
    
    ret = eth_get_protocol_type(eth_index, &protocol_type);
    if (ret < 0) {
        if (ret == -WB_SYSFS_RV_ABSENT) {
            SFF_DBG("transceiver:%u protocol attribute absent.\n", eth_index);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            SFF_ERR("transceiver:%u get protocol attribute failed, return value: %d.\n", eth_index, ret);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    if (protocol_type == SFF_PROTOCOL_SFP) {
        SFF_DBG("get eth%u module type is SFP\n", eth_index);
        return g_sff_drv->get_eth_attr(eth_index, tmp_attr->type, buf, PAGE_SIZE);
    }

    ret = eth_get_offset_by_protocol_type(protocol_type, tmp_attr->type);
    if (ret < 0) {
        SFF_ERR("get offset failed when get attr, protocol_type: %d, type: %d\n", 
            protocol_type, tmp_attr->type);
        return ret;
    }
    offset = ret;

    ret = g_sff_drv->read_eth_eeprom_data(eth_index, &value, offset, 1, E2_NON_UPGRADE_FLAG);
    if (ret < 0) {
        SFF_ERR("get eth%u module rx los value failed, ret: %d\n", eth_index, ret);
        if ((ret == -WB_SYSFS_RV_UNSUPPORT) || (ret == -WB_SYSFS_RV_ABSENT)) {
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    if (protocol_type == SFF_PROTOCOL_QSFP) {
        SFF_DBG("get eth%u module type is QSFP\n", eth_index);
        mask = 0xf;
    } else if (protocol_type == SFF_PROTOCOL_CMIS) {
        SFF_DBG("get eth%u module type is CMIS\n", eth_index);
        mask = 0xff;
    } else {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_ERROR);
    }

    if ((value & mask) != 0) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 1);
    } else {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 0);
    }

    return ret;
}
#endif

static ssize_t eth_tx_disable_store(struct switch_obj *obj, struct switch_attribute *attr,
                                    const char* buf, size_t count)
{
    unsigned int eth_index;
    struct switch_device_attribute *tmp_attr;
    int ret, value;
    char protocol_type, write_buf;
    loff_t offset;

    check_p(g_sff_drv);
    check_p(g_sff_drv->read_eth_eeprom_data);
    check_p(g_sff_drv->write_eth_eeprom_data);
    tmp_attr = to_switch_device_attr(attr);
    check_p(tmp_attr);

    sscanf(buf, "%d", &value);
    eth_index = obj->index;
    SFF_DBG("eth index: %u, tx_disable:0x%x\n", eth_index, value);

    if (value < 0 || value > 1) {
        SFF_ERR("invalid value: %d, can't set eth%u tx disable status.\n", value, eth_index);
        return -EINVAL;
    }

    write_buf = 0;

    if(test_bit(eth_index - 1, g_sff_present_debug_bd) == 0) {
        SFF_DBG("eth:%d open present debug, return not present.\n", eth_index);
        return -ENOENT;
    }

    ret = eth_get_protocol_type(eth_index, &protocol_type);
    if (ret < 0) {
        if (ret == -WB_SYSFS_RV_ABSENT) {
            SFF_DBG("transceiver:%u protocol attribute absent.\n", eth_index);
            return -ENOENT;
        } else {
            SFF_ERR("transceiver:%u get protocol attribute failed, return value: %d.\n", eth_index, ret);
            return -EIO;
        }
    }

    if (protocol_type == SFF_PROTOCOL_SFP) {
        SFF_DBG("get eth%u module type is SFP\n", eth_index);
        ret = g_sff_drv->set_eth_attr(eth_index, tmp_attr->type, value);
        if (ret < 0) {
            SFF_ERR("set eth%u tx disable status %d failed, ret: %d\n", eth_index, value, ret);
            return ret;
        }
        SFF_DBG("set eth%u tx disable status %d success\n", eth_index, value);
        return count;
    }

    ret = eth_get_offset_by_protocol_type(protocol_type, tmp_attr->type);
    if (ret < 0) {
        SFF_ERR("get offset failed when set attr, protocol_type: %d, type: %d\n", 
            protocol_type, tmp_attr->type);
        return ret;
    }
    offset = ret;

    if (protocol_type == SFF_PROTOCOL_QSFP) {
        SFF_DBG("get eth%u module type is QSFP\n", eth_index);
        if (value != 0) {
            write_buf = 0xf;
        }
    } else if (protocol_type == SFF_PROTOCOL_CMIS) {
        SFF_DBG("get eth%u module type is CMIS\n", eth_index);
        if (value != 0) {
            write_buf = 0xff;
        }
    } else {
        SFF_ERR("eth%u module is unknown, protocol_type:%d\n", eth_index, protocol_type);
        return -EINVAL;
    }

    ret = g_sff_drv->write_eth_eeprom_data(eth_index, &write_buf, offset, 1, E2_NON_UPGRADE_FLAG);
    if (ret < 0) {
        SFF_ERR("set eth%u tx disable status %d failed, ret: %d\n", eth_index, value, ret);
        return ret;
    }

    SFF_DBG("set eth%u tx disable status %d success\n", eth_index, value);
    return count;
}

static ssize_t eth_eeprom_read(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
                               char *buf, loff_t offset, size_t count)
{
    struct switch_obj *eth_obj;
    ssize_t ret;
    unsigned int eth_index;

    check_p(g_sff_drv);
    check_p(g_sff_drv->read_eth_eeprom_data);

    eth_obj = to_switch_obj(kobj);
    eth_index = eth_obj->index;

    SFF_DBG("start read eth%u eeprom data\n", eth_index);

    if(test_bit(eth_index - 1, g_sff_present_debug_bd) == 0) {
        SFF_DBG("eth:%d open present debug, return not present.\n", eth_index);
        return -ENOENT;
    }

    mem_clear(buf, count);
    ret = g_sff_drv->read_eth_eeprom_data(eth_index, buf, offset, count, E2_NON_UPGRADE_FLAG);
    if (ret < 0) {
        if (ret == -WB_SYSFS_RV_ABSENT) {
            SFF_INFO("read transceiver:%u eeprom data absent.\n", eth_index);
            return -ENOENT;
        }
        SFF_ERR("read eth%u eeprom data error, offset: 0x%llx, read len: %zu, ret: %zd.\n",
                eth_index, offset, count, ret);
        return -EIO;
    }

    SFF_DBG("read eth%u eeprom data success, offset:0x%llx, read len:%zu, really read len:%zd.\n",
            eth_index, offset, count, ret);

    return ret;
}

static ssize_t eth_eeprom_write(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
                                char *buf, loff_t offset, size_t count)
{
    struct switch_obj *eth_obj;
    ssize_t ret;
    unsigned int eth_index;

    check_p(g_sff_drv);
    check_p(g_sff_drv->write_eth_eeprom_data);

    eth_obj = to_switch_obj(kobj);
    eth_index = eth_obj->index;

    SFF_DBG("start write eth%u eeprom data\n", eth_index);

    if(test_bit(eth_index - 1, g_sff_present_debug_bd) == 0) {
        SFF_DBG("eth:%d open present debug, return not present.\n", eth_index);
        return -ENOENT;
    }
    
    ret = g_sff_drv->write_eth_eeprom_data(eth_index, buf, offset, count, E2_NON_UPGRADE_FLAG);
    if (ret < 0) {
        if (ret == -WB_SYSFS_RV_ABSENT) {
            SFF_INFO("write transceiver:%u eeprom data absent.\n", eth_index);
            return -ENOENT;
        }
        SFF_ERR("write eth%u eeprom data error, offset: 0x%llx, write len: %zu, ret: %zd.\n",
                eth_index, offset, count, ret);
        return -EIO;
    }

    SFF_DBG("write eth%u eeprom data success, offset:0x%llx, write len:%zu, really write len:%zd.\n",
            eth_index, offset, count, ret);

    return ret;
}

static ssize_t eth_eeprom_upg_read(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
                                   char *buf, loff_t offset, size_t count)
{
    struct switch_obj *eth_obj;
    ssize_t rd_len;
    unsigned int eth_index;

    check_p(g_sff_drv);
    check_p(g_sff_drv->read_eth_eeprom_data);

    eth_obj = to_switch_obj(kobj);
    eth_index = eth_obj->index;
    mem_clear(buf, count);
    rd_len = g_sff_drv->read_eth_eeprom_data(eth_index, buf, offset, count, E2_UPGRADE_FLAG);
    if (rd_len < 0) {
        SFF_ERR("read eth%u eeprom_upg data error, offset: 0x%llx, read len: %zu, ret: %zd.\n",
                eth_index, offset, count, rd_len);
        return rd_len;
    }

    SFF_DBG("read eth%u eeprom_upg data success, offset:0x%llx, read len:%zu, really read len:%zd.\n",
            eth_index, offset, count, rd_len);

    return rd_len;
}

static ssize_t eth_eeprom_upg_write(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
                                char *buf, loff_t offset, size_t count)
{
    struct switch_obj *eth_obj;
    ssize_t wr_len;
    unsigned int eth_index;

    check_p(g_sff_drv);
    check_p(g_sff_drv->write_eth_eeprom_data);

    eth_obj = to_switch_obj(kobj);
    eth_index = eth_obj->index;
    wr_len = g_sff_drv->write_eth_eeprom_data(eth_index, buf, offset, count, E2_UPGRADE_FLAG);
    if (wr_len < 0) {
        SFF_ERR("write eth%u eeprom_upg data error, offset: 0x%llx, read len: %zu, ret: %zd.\n",
                eth_index, offset, count, wr_len);
        return wr_len;
    }

    SFF_DBG("write eth%u eeprom_upg data success, offset:0x%llx, write len:%zu, really write len:%zd.\n",
            eth_index, offset, count, wr_len);

    return wr_len;
}

/**
 * eth_power_ctrl_show - Show the power control value for a specific Ethernet port.
 * @obj: The switch object.
 * @attr: The switch attribute.
 * @buf: The buffer to store the power control value string.
 *
 * Returns: The length of the power control value string.
 */
static ssize_t eth_power_ctrl_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;
    int len;

    /* power group config check */
    if (g_sff.power_group_cfg != WB_PORT_POWER_GROUP_CFG) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    eth_index = obj->index;

    if (!transceiver_check_power_group_index(g_sff.sff[eth_index - 1].power_group_index)) {
        SFF_ERR("Power group is not initialized, cannot set eth%u power control status.\n", eth_index);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_ERROR);
    }

    mutex_lock(&g_sff.power_ctrl_update_lock);
    len = (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_sff.sff[eth_index - 1].power_ctrl);
    mutex_unlock(&g_sff.power_ctrl_update_lock);

    return len;
}

/**
 * eth_power_ctrl_store - Store the power control value for a specific Ethernet port.
 * @obj: The switch object.
 * @attr: The switch attribute.
 * @buf: The buffer containing the new power control value string.
 * @count: The length of the buffer.
 *
 * Returns: The length of the buffer, or an error code.
 */
static ssize_t eth_power_ctrl_store(struct switch_obj *obj, struct switch_attribute *attr,
                                    const char* buf, size_t count)
{
    unsigned int eth_index;
    int value, ret;

    sscanf(buf, "%d", &value);
    eth_index = obj->index;

    /* Validate the character (must be '0' or '1') */
    if (value != 0 && value != 1) {
        SFF_ERR("invalid parameter (must be '0' or '1'), eth%d count: %zu, buf: %s.\n", eth_index, count, buf);
        return -EINVAL;
    }

    mutex_lock(&g_sff.power_ctrl_update_lock);
    transceiver_update_eth_power_ctrl_value(eth_index, value);
    ret = eth_power_ctrl_single_group_store(g_sff.sff[eth_index - 1].power_group_index);
    mutex_unlock(&g_sff.power_ctrl_update_lock);
    if (ret < 0) {
        return ret;
    }

    SFF_DBG("set eth%u power on %d success\n", eth_index, value);
    return count;
}

static ssize_t eth_power_on_store(struct switch_obj *obj, struct switch_attribute *attr,
                                  const char* buf, size_t count)
{
    int ret;

    if (g_sff.power_group_cfg == WB_PORT_POWER_GROUP_CFG) {
        ret = eth_power_ctrl_store(obj, attr, buf, count);
    } else {
        ret = eth_power_on_set(obj, attr, buf, count);
    }

    return ret;
}

/**
 * eth_power_group_show - Show the power group index for a specific Ethernet port.
 * @obj: The switch object.
 * @attr: The switch attribute.
 * @buf: The buffer to store the power group index string.
 *
 * Returns: The length of the power group index string.
 */
static ssize_t eth_power_group_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eth_index;

    /* power group config check */
    if (g_sff.power_group_cfg != WB_PORT_POWER_GROUP_CFG) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    eth_index = obj->index;
    if (!transceiver_check_power_group_index(g_sff.sff[eth_index - 1].power_group_index)) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_ERROR);
    }
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_sff.sff[eth_index - 1].power_group_index);
}

/**
 * eth_init_power_group - Initialize the power groups for all Ethernet ports.
 *
 * Returns: 0 if successful, non-zero otherwise.
 */
static ssize_t eth_init_power_group(void)
{
    int power_group, eth_index;
    int ret;
    struct sff_obj_s *curr_sff;

    check_p(g_sff_drv);
    check_p(g_sff_drv->get_eth_power_group);

    mem_clear(g_power_groups, sizeof(g_power_groups));
    g_sff.power_group_cfg = WB_PORT_POWER_GROUP_NOT_CFG;
    g_sff.power_group_num = 0;
    mutex_init(&g_sff.power_ctrl_update_lock);
    for (eth_index = 1; eth_index <= g_sff.sff_number; eth_index++) {
        power_group = 0;
        ret = g_sff_drv->get_eth_power_group(eth_index, &power_group);
        if (ret != -WB_SYSFS_RV_UNSUPPORT) {
            /* one of port group is configured, flag set 1 */
            g_sff.power_group_cfg = WB_PORT_POWER_GROUP_CFG;
        }
        if (ret < 0) {
            SFF_ERR("Get Power group failed. eth_index:%d, ret:%d\n", eth_index, ret);
            return ret;
        }

        if (!transceiver_check_power_group_index(power_group)) {
            SFF_ERR("Power group out of range. eth_index:%d, power_group:%d\n", eth_index, power_group);
            return -EINVAL;
        }

        if ((g_power_groups[power_group - 1].num_ports > WB_PORT_POWER_GROUP_MAX) ||
            (g_power_groups[power_group - 1].num_ports > g_sff.sff_number)) {
            SFF_ERR("num_ports of group out of range. eth_index:%d, power_group:%d, num_ports: %d, sff_number: %d\n",
                    eth_index, power_group,
                    g_power_groups[power_group - 1].num_ports,
                    g_sff.sff_number);
            return -EINVAL;
        }

        curr_sff = &g_sff.sff[eth_index - 1];
        /* init the ctrl */
        curr_sff->power_ctrl = -1;

        /* set group info to sff_obj_s data */
        curr_sff->power_group_index = power_group;

        /* Initialize the mutex for the group to prevent duplicate initialization and get calculate group number */
        if (g_power_groups[power_group - 1].num_ports == 0) {
            g_sff.power_group_num++;
        }

        /* update power group to ports info table */
        g_power_groups[power_group - 1].ports[g_power_groups[power_group - 1].num_ports] = eth_index;
        g_power_groups[power_group - 1].num_ports++;
        g_power_groups[power_group - 1].current_group_value = -1;
        g_power_groups[power_group - 1].last_group_value = -1;
    }

    SFF_DBG("set eth%u power group %d success\n", eth_index, power_group);
    return 0;
}

/************************************eth* signal attrs*******************************************/
static struct switch_attribute eth_power_on_attr = __ATTR(power_on, S_IRUGO | S_IWUSR, eth_power_on_show, eth_power_on_store);
static struct switch_attribute eth_optoe_type_attr = __ATTR(optoe_type, S_IRUGO | S_IWUSR, eth_optoe_type_show, eth_optoe_type_store);
static struct switch_attribute port_led_attr = __ATTR(port_led, S_IRUGO | S_IWUSR, eth_port_led_show, eth_port_led_store);
static struct switch_attribute eth_port_bus_status_attr = __ATTR(bus_status, S_IRUGO, eth_port_bus_status_show, NULL);
static struct switch_attribute eth_i2c_bus_attr = __ATTR(i2c_bus, S_IRUGO, eth_i2c_bus_show, NULL);
static struct switch_attribute eth_power_ctrl_attr = __ATTR(power_ctrl, S_IRUGO | S_IWUSR, eth_power_ctrl_show, eth_power_ctrl_store);
static struct switch_attribute eth_power_group_attr = __ATTR(power_group, S_IRUGO, eth_power_group_show, NULL);
static struct switch_attribute eth_eeprom_low_page_attr = __ATTR(eeprom_low_page, S_IRUGO, eth_eeprom_low_page_show, NULL);
static struct switch_attribute eth_temp_attr = __ATTR(temp, S_IRUGO, eth_temp_show, NULL);
static struct switch_attribute eth_cage_type_attr = __ATTR(cage_type, S_IRUGO, eth_cage_type_show, NULL);
static struct switch_attribute eth_dev_available_attr = __ATTR(dev_available, S_IRUGO | S_IWUSR, dev_available_show, dev_available_store);

static SWITCH_DEVICE_ATTR(present, S_IRUGO | S_IWUSR, eth_present_show, sff_attr_dbg_store, WB_SFF_MODULE_PRESENT);

static SWITCH_DEVICE_ATTR(reset, S_IRUGO | S_IWUSR, eth_attr_show, eth_attr_store, WB_SFF_RESET);
static SWITCH_DEVICE_ATTR(low_power_mode, S_IRUGO | S_IWUSR, eth_attr_show, eth_attr_store, WB_SFF_LPMODE);
static SWITCH_DEVICE_ATTR(interrupt, S_IRUGO | S_IWUSR, eth_attr_show, eth_attr_store, WB_SFF_INTERRUPT);
static SWITCH_DEVICE_ATTR(sff_type, S_IRUGO, eth_protocol_type_show, NULL, 0);

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
static SWITCH_DEVICE_ATTR(tx_fault, S_IRUGO, eth_attr_show, NULL, WB_SFF_TX_FAULT);
static SWITCH_DEVICE_ATTR(tx_disable, S_IRUGO | S_IWUSR, eth_attr_show, eth_tx_disable_store, WB_SFF_TX_DIS);
static SWITCH_DEVICE_ATTR(rx_los, S_IRUGO, eth_attr_show, NULL, WB_SFF_RX_LOS);
#else
static SWITCH_DEVICE_ATTR(tx_fault, S_IRUGO, eth_protocol_attr_show, NULL, WB_SFF_TX_FAULT);
static SWITCH_DEVICE_ATTR(tx_disable, S_IRUGO | S_IWUSR, eth_protocol_attr_show, eth_tx_disable_store, WB_SFF_TX_DIS);
static SWITCH_DEVICE_ATTR(rx_los, S_IRUGO, eth_protocol_attr_show, NULL, WB_SFF_RX_LOS);
#endif
static SWITCH_DEVICE_ATTR(tx_los, S_IRUGO, eth_attr_show, NULL, WB_SFF_TX_LOS);
static SWITCH_DEVICE_ATTR(tx_cdr_lol, S_IRUGO, eth_attr_show, NULL, WB_SFF_TX_CDR_LOL);
static SWITCH_DEVICE_ATTR(rx_cdr_lol, S_IRUGO, eth_attr_show, NULL, WB_SFF_RX_CDR_LOL);
static SWITCH_DEVICE_ATTR(module_status, S_IRUGO, eth_attr_show, NULL, WB_SFF_MODULE_STATUS);
static SWITCH_DEVICE_ATTR(datapath_status, S_IRUGO, eth_attr_show, NULL, WB_SFF_DATAPATH_STATUS);
static SWITCH_DEVICE_ATTR(host_snr, S_IRUGO, eth_attr_show, NULL, WB_SFF_HOST_SNR);
static SWITCH_DEVICE_ATTR(media_snr, S_IRUGO, eth_attr_show, NULL, WB_SFF_MEDIA_SNR);
static SWITCH_DEVICE_ATTR(diagnostic, S_IRUGO, eth_attr_show, NULL, WB_SFF_DIAGNOSTIC);

static struct attribute *sff_signal_attrs[] = {
    &eth_power_on_attr.attr,
    &eth_optoe_type_attr.attr,
    &port_led_attr.attr,
    &eth_i2c_bus_attr.attr,
    &eth_power_ctrl_attr.attr,
    &eth_power_group_attr.attr,
    &eth_eeprom_low_page_attr.attr,
    &eth_temp_attr.attr,
    &eth_cage_type_attr.attr,
    &eth_port_bus_status_attr.attr,

    &switch_dev_attr_present.switch_attr.attr,
    &switch_dev_attr_reset.switch_attr.attr,
    &switch_dev_attr_low_power_mode.switch_attr.attr,
    &switch_dev_attr_interrupt.switch_attr.attr,
    &switch_dev_attr_sff_type.switch_attr.attr,
    &eth_dev_available_attr.attr,
    &switch_dev_attr_tx_fault.switch_attr.attr,
    &switch_dev_attr_tx_disable.switch_attr.attr,
    &switch_dev_attr_rx_los.switch_attr.attr,
    &switch_dev_attr_diagnostic.switch_attr.attr,
    &switch_dev_attr_tx_los.switch_attr.attr,
    &switch_dev_attr_tx_cdr_lol.switch_attr.attr,
    &switch_dev_attr_rx_cdr_lol.switch_attr.attr,
    &switch_dev_attr_module_status.switch_attr.attr,
    &switch_dev_attr_datapath_status.switch_attr.attr,
    &switch_dev_attr_host_snr.switch_attr.attr,
    &switch_dev_attr_media_snr.switch_attr.attr,
    NULL,
};

static struct attribute_group sff_signal_attr_group = {
    .attrs = sff_signal_attrs,
};

/*******************************transceiver dir and attrs*******************************************/
static struct switch_attribute transceiver_power_on_attr = __ATTR(power_on, S_IRUGO | S_IWUSR, transceiver_power_on_show, transceiver_power_on_store);
static struct switch_attribute transceiver_number_attr = __ATTR(number, S_IRUGO, transceiver_number_show, NULL);
static struct switch_attribute transceiver_present_attr = __ATTR(present, S_IRUGO, transceiver_present_show, NULL);
static struct switch_attribute transceiver_power_on_bitmap_attr = __ATTR(power_on_bitmap, S_IRUGO, transceiver_power_on_bitmap_show, NULL);
static struct switch_attribute transceiver_power_ctrl_bitmap_attr = __ATTR(power_ctrl_bitmap, S_IRUGO | S_IWUSR, transceiver_power_ctrl_bitmap_show, transceiver_power_ctrl_bitmap_store);
static struct switch_attribute transceiver_debug_attr = __ATTR(debug, S_IRUGO, transceiver_debug_show, NULL);
static struct switch_attribute sw_init_enable_attr = __ATTR(sw_init_enable, S_IRUGO | S_IWUSR, sw_init_enable_show, sw_init_enable_store);

static struct attribute *transceiver_dir_attrs[] = {
    &transceiver_power_on_attr.attr,
    &transceiver_number_attr.attr,
    &transceiver_present_attr.attr,
    &transceiver_power_on_bitmap_attr.attr,
    &transceiver_power_ctrl_bitmap_attr.attr,
    &transceiver_debug_attr.attr,
    &sw_init_enable_attr.attr,
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
    
    /* Create eeprom_upg bin file */
    curr_sff->bin_upg.attr.name = "eeprom_upg";
    curr_sff->bin_upg.attr.mode = 0644;
    curr_sff->bin_upg.read = eth_eeprom_upg_read;
    curr_sff->bin_upg.write = eth_eeprom_upg_write;
    curr_sff->bin_upg.size = eeprom_size;

    ret = sysfs_create_bin_file(&curr_sff->sff_obj->kobj, &curr_sff->bin_upg);
    if (ret) {
        SFF_ERR("eth%u, create eeprom_upg bin error, ret: %d. \n", index, ret);
        sysfs_remove_bin_file(&curr_sff->sff_obj->kobj, &curr_sff->bin);
        curr_sff->sff_creat_bin_flag = 0;
        return -EBADRQC;
    }

    SFF_DBG("eth%u, create bin_upg file success, eeprom size:%d.\n", index, eeprom_size);
    curr_sff->sff_creat_bin_upg_flag = 1;
    
    return 0;
}

static int sff_sub_single_create_kobj(struct kobject *parent, unsigned int index)
{
    struct sff_obj_s *curr_sff;
    char sff_dir_name[DIR_NAME_MAX_LEN];

    curr_sff = &g_sff.sff[index - 1];
    mem_clear(sff_dir_name, sizeof(sff_dir_name));
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
        
        if (curr_sff->sff_creat_bin_upg_flag) {
            sysfs_remove_bin_file(&curr_sff->sff_obj->kobj, &curr_sff->bin_upg);
            curr_sff->sff_creat_bin_upg_flag = 0;
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
    int ret;

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

    ret = eth_init_power_group();
    if (g_sff.power_group_cfg == WB_PORT_POWER_GROUP_CFG && ret < 0) {
        SFF_ERR("Init Power group failed. ret:%d\n", ret);
        goto error;
    }

    return 0;
error:
    for (i = sff_index - 1; i > 0; i--) {
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

    bitmap_fill(g_sff_present_debug_bd, MAX_SFF_NUMBER);
    mem_clear(&g_sff, sizeof(struct sff_s));
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
EXPORT_SYMBOL(eth_debug_present_get);
EXPORT_SYMBOL(eth_debug_present_set);
module_param(g_sff_loglevel, int, 0644);
MODULE_PARM_DESC(g_sff_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
module_param(g_sff_present_debug, bool, 0644);
MODULE_PARM_DESC(g_sff_present_debug, "the sff present debug switch(0: disable, 1:enable, defalut: 0).\n");

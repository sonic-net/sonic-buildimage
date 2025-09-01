/*
 * Copyright(C) 2001-2012 whitebox Network. All rights reserved.
 */
/*
 * wb_sff_driver.c
 * Original Author: sonic_rd@whitebox 2020-02-17
 *
 * sff related attribute read and write function
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@whitebox         2020-02-17          Initial version
 */

#include <linux/module.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"
#include "wb_sff_driver.h"

int g_dfd_sff_dbg_level = 0;
module_param(g_dfd_sff_dbg_level, int, S_IRUGO | S_IWUSR);

/**
 * dfd_set_sff_cpld_info - Example Set the CPLD register status of the optical module
 * @sff_index: Optical module number, starting from 1
 * @cpld_reg_type: Optical module CPLD register type
 * @value: Writes the value to the register
 * return: Success :0
 *       : Failed: A negative value is returned
 */
int dfd_set_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, int value)
{
    uint64_t key;
    int ret;
    int reg_value;

    if ((value != 0) && (value != 1)) {
        DFD_SFF_DEBUG(DBG_ERROR, "sff%u cpld reg type %d, can't set invalid value: %d\n",
            sff_index, cpld_reg_type, value);
        return -DFD_RV_INVALID_VALUE;
    }

    reg_value = value;
    ret = dfd_get_reg_key(WB_MAIN_DEV_SFF, cpld_reg_type, value, &reg_value);
    if (ret < 0) {
        if (ret != -DFD_RV_DEV_NOTSUPPORT) {
            DFD_SFF_DEBUG(DBG_ERROR, "get sff type %d reg key err[%d].\n", cpld_reg_type, ret);
            return ret;
        }
        DFD_SFF_DEBUG(DBG_WARN, "get sff type %d reg key failed[%d], using defalut value.\n", cpld_reg_type, ret);
        reg_value = value;
    }

    DFD_SFF_DEBUG(DBG_VERBOSE, "set sff%u cpld reg type %d value 0x%x\n", sff_index, cpld_reg_type, reg_value);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_CPLD_REG, sff_index, cpld_reg_type);
    ret = dfd_info_set_int(key, reg_value);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "set sff%u cpld reg type %d error, key_name: %s, ret: %d.\n",
            sff_index, cpld_reg_type, key_to_name(DFD_CFG_ITEM_SFF_CPLD_REG), ret);
        return ret;
    }

    return DFD_RV_OK;
}

/**
 * dfd_get_sff_cpld_info - Obtain the CPLD register status of the optical module
 * @sff_index: Optical module number, starting from 1
 * @cpld_reg_type: Optical module CPLD register type
 * @buf: Optical module E2 receives information from buf
 * @count: buf length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, char *buf, size_t count)
{
    uint64_t key;
    int ret, value, sysfs_value;

    if (buf == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, buf is NULL. sff_index: %u, cpld_reg_type: %d\n",
            sff_index, cpld_reg_type);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "buf size error, count: %zu, sff index: %u, cpld_reg_type: %d\n",
            count, sff_index, cpld_reg_type);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_CPLD_REG, sff_index, cpld_reg_type);
    ret = dfd_info_get_int(key, &value, NULL);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff%u cpld reg type %d error, key_name: %s, ret: %d\n",
            sff_index, cpld_reg_type, key_to_name(DFD_CFG_ITEM_SFF_CPLD_REG), ret);
        return ret;
    }

    sysfs_value = value;
    ret = dfd_get_sysfs_decode(WB_MAIN_DEV_SFF, cpld_reg_type, value, &sysfs_value);
    if (ret < 0) {
        if (ret != -DFD_RV_DEV_NOTSUPPORT) {
            DFD_SFF_DEBUG(DBG_ERROR, "get sff type %d value[0x%x] map to sysfs value talbe faile[%d], using defalut value.\n", 
                cpld_reg_type, value, ret);
            return ret;
        }
        DFD_SFF_DEBUG(DBG_WARN, "get sff type %d value[0x%x] map to sysfs value talbe faile[%d], using defalut value.\n", 
                cpld_reg_type, value, ret);
        sysfs_value = value;
    }

    return (ssize_t)snprintf(buf, count, "%d\n", sysfs_value);
}

static int get_optoe_type_path_by_bus(int index, char *optoe_type_path, size_t count) 
{
    uint64_t key;
    int *i2c_bus;

    /* Obtain the eeprom bus*/
    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_BUS, WB_MAIN_DEV_SFF, index);
    i2c_bus = dfd_ko_cfg_get_item(key);
    if (i2c_bus == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff%d i2c bus fail.\n", index);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    snprintf(optoe_type_path, count, SFF_OPTOE_TYPE_PATH, *i2c_bus);
    DFD_SFF_DEBUG(DBG_VERBOSE, "sff%d, i2c_bus: %d, optoe_type_path: %s\n", index, *i2c_bus, optoe_type_path);
    return 0;
}

/**
 * dfd_get_single_eth_optoe_type - get sff optoe type
 * @sff_index: Optical module number, starting from 1
 * @optoe_type: Optical module type
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_get_single_eth_optoe_type(unsigned int sff_index, int *optoe_type)
{
    uint64_t key;
    int ret, value;
    uint8_t temp_type;
    char optoe_type_path[OPTOE_TYPE_PATH_SIZE];

    mem_clear(optoe_type_path, sizeof(optoe_type_path));
    ret = get_optoe_type_path_by_bus(sff_index, optoe_type_path, sizeof(optoe_type_path));
    if (ret == 0) {
        temp_type = 0;
        ret = dfd_ko_read_file(optoe_type_path, 0, &temp_type, OPTOE_TYPE_RW_LEN);
        if (ret < 0) {
            DFD_SFF_DEBUG(DBG_ERROR, "read optoe_type failed, loc: %s, ret: %d,\n", optoe_type_path, ret);
        } else {
            DFD_SFF_DEBUG(DBG_VERBOSE, "read optoe_type success, loc: %s, rd_len: %d,\n", optoe_type_path, ret);
            *optoe_type = temp_type - '0';
        }
        return ret;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_OPTOE_TYPE, sff_index, 0);
    ret = dfd_info_get_int(key, &value, NULL);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff optoe type error, key_name: %s, ret:%d.\n", 
			key_to_name(DFD_CFG_ITEM_SFF_OPTOE_TYPE), ret);
        return ret;
    }

    /* assic int to int */
    *optoe_type = value - '0';
    return ret;
}

/**
 * dfd_set_single_eth_optoe_type - set sff optoe type
 * @sff_index: Optical module number, starting from 1
 * @optoe_type: Optical module type
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_set_single_eth_optoe_type(unsigned int sff_index, int optoe_type)
{
    uint64_t key;
    int ret, value;
    uint8_t temp_type;
    char optoe_type_path[OPTOE_TYPE_PATH_SIZE];

    mem_clear(optoe_type_path, sizeof(optoe_type_path));
    ret = get_optoe_type_path_by_bus(sff_index, optoe_type_path, sizeof(optoe_type_path));
    if (ret == 0) {
        temp_type = (uint8_t)optoe_type + '0';
        ret = dfd_ko_write_file(optoe_type_path, 0, &temp_type, OPTOE_TYPE_RW_LEN);
        if (ret < 0) {
            DFD_SFF_DEBUG(DBG_ERROR, "write optoe_type failed, loc: %s, ret: %d,\n", optoe_type_path, ret);
        } else {
            DFD_SFF_DEBUG(DBG_VERBOSE, "write optoe_type success, loc: %s, rd_len: %d,\n", optoe_type_path, ret);
        }
        return ret;
    }

    /* int to assic int */
    value = optoe_type + '0';
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_OPTOE_TYPE, sff_index, 0);
    ret = dfd_info_set_int(key, value);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "set sff optoe type error, key_name: %s, ret:%d.\n", 
			key_to_name(DFD_CFG_ITEM_SFF_OPTOE_TYPE), ret);
        return ret;
    }

    return ret;
}

/**
 * dfd_get_single_eth_i2c_bus - get sff i2c_bus
 * @sff_index: Optical module number, starting from 1
 * @buf: Optical module E2 receives information from buf
 * @count: buf length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_get_single_eth_i2c_bus(unsigned int sff_index, char *buf, size_t count)
{
    uint64_t key;
    int *i2c_bus;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_BUS, WB_MAIN_DEV_SFF, sff_index);
    i2c_bus = dfd_ko_cfg_get_item(key);
    if (i2c_bus == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff i2c bus error, key_name: %s.\n", key_to_name(DFD_CFG_ITEM_EEPROM_BUS));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    return (ssize_t)snprintf(buf, count, "%d\n", *i2c_bus);
}

/**
 * dfd_get_single_eth_power_group - get sff port power group
 * @sff_index: Optical module number, starting from 1
 * @power_group: power group id
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_get_single_eth_power_group(unsigned int sff_index, int *power_group)
{
    uint64_t key;
    int *p_value;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_POWER_GROUP, sff_index, 0);
    p_value = dfd_ko_cfg_get_item(key);
    if (p_value == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff power group id error, key_name: %s.\n",
                      key_to_name(DFD_CFG_ITEM_SFF_POWER_GROUP));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    *power_group = *p_value;
    return 0;
}

/**
 * dfd_set_sff_power_group_state - Set the power state of an SFP power group.
 * @group: Power group number, starting from 1.
 * @group_state: Power state to set for the group (0 for off, 1 for on).
 *
 * Returns: 0 on success, or a negative error code on failure.
 */
int dfd_set_sff_power_group_state(int group, int group_state)
{
    uint64_t key;
    int ret;

    /** Validate the power state value */
    if ((group_state != 0) && (group_state != 1)) {
        DFD_SFF_DEBUG(DBG_ERROR, "Power group %u, can't set invalid value: %d\n",
                      group, group_state);
        return -DFD_RV_INVALID_VALUE;
    }

    /** Construct the key for the power group configuration item */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_PORT_POWER_GROUP_REG, group, 0);

    /** Set the power state for the specified SFP power group */
    ret = dfd_info_set_int(key, group_state);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "Set power group %u error, key_name: %s, ret: %d.\n",
                      group, key_to_name(DFD_CFG_ITEM_PORT_POWER_GROUP_REG), ret);
        return ret;
    }

    DFD_SFF_DEBUG(DBG_VERBOSE, "Successfully set power state for power group %u to %d.\n",
                  group, group_state);
    return DFD_RV_OK;
}

/**
 * dfd_get_sff_power_group_state - Obtain the power group state.
 * @group: Power group number, starting from 1.
 * @buf: Buffer to store the power group state information.
 * @count: buf length.
 * return: Success: Returns the length of fill buf.
 *        Failed: A negative value is returned.
 */
ssize_t dfd_get_sff_power_group_state(int group, char *buf, size_t count)
{
    uint64_t key;
    int ret, power_group_state;

    /* Check for NULL buffer and non-positive count */
    if (buf == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, buf is NULL. group: %u\n", group);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "buf size error, count: %zu, group: %u\n", count, group);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_PORT_POWER_GROUP_REG, group, 0);

    ret = dfd_info_get_int(key, &power_group_state, NULL);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get power group %u state error, key_name: %s, ret: %d\n",
                      group, key_to_name(DFD_CFG_ITEM_PORT_POWER_GROUP_REG), ret);
        return ret;
    }

    DFD_SFF_DEBUG(DBG_VERBOSE, "Successfully get power state for power group %u state: %d.\n",
                  group, power_group_state);

    return (ssize_t)snprintf(buf, count, "%d\n", power_group_state);
}

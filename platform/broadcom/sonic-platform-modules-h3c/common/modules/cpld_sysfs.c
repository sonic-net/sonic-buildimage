/*
* Copyright (c) 2019  <sonic@h3c.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
/*公有文件引入*/
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/hwmon-sysfs.h>
#include <linux/libata.h>
/*私有文件*/
#include "pub.h"
#include "bsp_base.h"
#include "cpld_base.h"

module_param_named(cpld_debug_level, bsp_module_debug_level[BSP_CPLD_MODULE], int, 0644);
MODULE_PARM_DESC(cpld_debug_level, "DEBUG 0x4, ERROR 0x2, INFO 0x1, ALL 0x7, DEBUG_OFF 0x0; Default value is ERROR");

enum CPLD_ATTR_E
{
    NUM_CPLDS,
    ALIAS,
    TYPE,
    HW_VERSION,
    BROAD_VERSION,
    REG_RESET_CPU,
    REG_MAC_INIT_OK,
    REG_REBOOT,
    REG_CLR_RST,
    REG_LAST_REBOOT,
    REG_CPLD_TXDIS,
    I2C_WDT_CTRL,
    CPU_INIT_OK,
    I2C_WDT_FEED,
    REG_MAC_INNER_TEMP,
    RAW_DATA,
    REG_MAC_WIDTH_TEMP
};

static struct kobject *kobj_cpld_debug = NULL;
static struct kobject *kobj_cpld_root = NULL;
static struct kobject *kobj_cpld_sub[MAX_CPLD_NUM] = {NULL};

static struct kobj_attribute slot_cpld_attr[MAX_SLOT_NUM] = {{{0}}};
static char attr_name[MAX_SLOT_NUM][20];

u8 buffer_char = 0;

ssize_t bsp_cpld_sysfs_show_cpld(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    u16 i, j;
    size_t index = 0;
    u16 cpld_size = 0;
    u8 temp_value = 0;
    int slot_index = -1;
    //u8 cpld_mem_buf[256] = {0};
    int ret = ERROR_SUCCESS;
    int start_addr = 0;

    int (* cpld_read_byte_func)(u8 * value, u16 offset) = NULL;

    if (strcmp(attr->attr.name, __stringify(CPU_CPLD_NAME)) == 0)
    {
        cpld_size = (u16) bsp_get_cpu_cpld_size();
        cpld_read_byte_func = bsp_cpu_cpld_read_byte;
        start_addr = bsp_get_board_data()->cpld_hw_addr_cpu;
    }
    else if (strcmp(attr->attr.name, __stringify(BOARD_CPLD_NAME)) == 0)
    {
        cpld_size = (u16) bsp_cpld_get_size();
        cpld_read_byte_func = bsp_cpld_read_byte;
        start_addr = bsp_get_board_data()->cpld_hw_addr_board;
    }
    else if (strcmp(attr->attr.name, __stringify(BIOS_CPLD)) == 0)
    {
        cpld_size = (u16) bsp_get_bios_cpld_size();
        cpld_read_byte_func = bsp_bios_cpld_read_byte;
        start_addr = bsp_get_board_data()->cpld_hw_addr_bios;
    }
    else if (sscanf(attr->attr.name, SLOT_CPLD_NAME, &slot_index) <= 0)
    {
        DBG_ECHO(DEBUG_ERR, "show cpld attrbute '%s' invalid", attr->attr.name);
        return scnprintf(buf, PAGE_SIZE, "Invalid attrbute '%s'\n", attr->attr.name);
    }
    else if (slot_index > MAX_SLOT_NUM)
    {
        DBG_ECHO(DEBUG_ERR, "show cpld slot index %d invalid\n", slot_index);
        return scnprintf(buf, PAGE_SIZE, "Invalid slot index %d\n", slot_index);
    }
    else
    {
        slot_index = slot_index - 1;
        cpld_size = (u16) bsp_cpld_get_slot_size(slot_index);
        cpld_read_byte_func = NULL;
        start_addr = bsp_get_board_data()->cpld_hw_addr_slot[slot_index];
    }

    index += scnprintf(buf + index, PAGE_SIZE - index, CPLD_OPER_WARN_MSG0);
    index += scnprintf(buf + index, PAGE_SIZE - index, CPLD_OPER_WARN_MSG1);
    index += scnprintf(buf + index, PAGE_SIZE - index, CPLD_OPER_WARN_MSG2);

    index += scnprintf(buf + index, PAGE_SIZE - index, "\nhw address start: 0x%04x\n", start_addr);

    for (i = 0; i < cpld_size; i += 16)
    {
        //avoid overflow
        if (index >= (PAGE_SIZE - 200))
        {
            DBG_ECHO(DEBUG_INFO, "show cpld buf size reached %d, break to avoid overflow.", (int)index);
            break;
        }
        index += scnprintf(buf + index, PAGE_SIZE - index, "0x%04x: ", i);

        for (j = 0; j < 16; j++)
        {
            ret = cpld_read_byte_func == NULL ? bsp_slot_cpld_read_byte(slot_index, &temp_value, i + j) : cpld_read_byte_func(&temp_value, i + j);

            if (ret == ERROR_SUCCESS)
            {
                index += scnprintf(buf + index, PAGE_SIZE - index, "%02x %s", temp_value, j == 7 ? " " : "");
            }
            else
            {
                index += scnprintf(buf + index, PAGE_SIZE - index, "XX ");
            }
        }

        index += scnprintf(buf + index, PAGE_SIZE - index, "\n");
    }
    return index;
}

static ssize_t bsp_cpld_sysfs_set_cpld(struct kobject *kobjs, struct kobj_attribute *attr, const char *buf, size_t count)
{
    unsigned long temp_offset = 0x0;
    unsigned long temp_value = 0x0;
    int slot_index = -1;
    int ret = ERROR_SUCCESS;

    if (sscanf(buf, "0x%lx:0x%lx", &temp_offset, &temp_value) < 2)
    {
        DBG_ECHO(DEBUG_ERR, "set cpld format '%s' invalid", buf);
        count = -EINVAL;
        return count;
    }

    if (strcmp(attr->attr.name, __stringify(CPU_CPLD_NAME)) == 0)
    {
        ret = bsp_cpu_cpld_write_byte((u8)temp_value, (u16)temp_offset);
    }
    else if (strcmp(attr->attr.name, __stringify(BOARD_CPLD_NAME)) == 0)
    {
        ret = bsp_cpld_write_byte((u8)temp_value, (u16)temp_offset);
    }
    else if (strcmp(attr->attr.name, __stringify(BIOS_CPLD)) == 0)
    {
        ret = bsp_bios_cpld_write_byte((u8)temp_value, (u16)temp_offset);
    }
    else if (sscanf(attr->attr.name, SLOT_CPLD_NAME, &slot_index) <= 0)
    {
        DBG_ECHO(DEBUG_ERR, "set cpld attrbute '%s' invalid", attr->attr.name);
        count = -EINVAL;
        return count;
    }
    else if (slot_index > MAX_SLOT_NUM)
    {
        DBG_ECHO(DEBUG_ERR, "set cpld slot index %d invalid", slot_index);
        count = -EINVAL;
        return count;
    }
    else
    {
        slot_index = slot_index - 1;
        ret = bsp_slot_cpld_write_byte(slot_index, (u8)temp_value, (u16)temp_offset);
    }

    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "set cpld slot(%d) write 0x%lx to offset 0x%lx failed!", slot_index, temp_value, temp_offset);
        count = ret;
    }
    return count;
}

static ssize_t bsp_cpld_sysfs_set_cpld_clr_rst(struct kobject *kobjs, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int temp = 0;
    u8 get_value = 0;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_sysfs_set_cpld_clr_rst get bd failed");
        return -EINVAL;
    }

    if (sscanf(buf, "%d", &temp) <= 0)
    {
        DBG_ECHO(DEBUG_ERR, "reg_clr_rst format error '%s' , kobjs->name=%s, attr->name=%s", buf, kobjs->name, attr->attr.name);
        count = -EINVAL;
    }
    else if (ERROR_SUCCESS == bsp_cpu_cpld_read_byte(&get_value, bd->cpld_addr_clr_rst))
    {
        if (ERROR_SUCCESS != bsp_cpu_cpld_write_byte(get_value | ((1 << bd->cpld_offs_clr_rst) & bd->cpld_mask_clr_rst), bd->cpld_addr_clr_rst))
        {
            DBG_ECHO(DEBUG_ERR, "reg_clr_rst set failed for kobjs->name=%s, attr->name=%s", kobjs->name, attr->attr.name);
            count = -EIO;
        }
    }
    else
    {
        DBG_ECHO(DEBUG_ERR, "clr_rst failed");
        count = -EINVAL;
    }
    return count;
}

static ssize_t bsp_cpld_custom_root_sysfs_write(struct kobject *kobjs, struct kobj_attribute *attr, const char *buf, size_t count, u32 node_index)
{
    int ret = ERROR_SUCCESS;

    switch (node_index)
    {
        case REG_CLR_RST:
            ret = bsp_cpld_sysfs_set_cpld_clr_rst(kobjs, attr, buf, count);
            if (ERROR_SUCCESS != ret)
            {
                count = ret;
            }
            break;
        default:
            DBG_ECHO(DEBUG_INFO, "cpld write attribute %s -> %s not support", kobjs->name, attr->attr.name);
            count = -ENOSYS;
            break;
    }

    return count;
}

//读cpu特定地址, 返回值放到buffer里。仅用于cpld升级
static ssize_t bsp_cpld_sysfs_buffer_show(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "%c\n", buffer_char);
}

static ssize_t bsp_cpld_sysfs_buffer_set(struct kobject *kobjs, struct kobj_attribute *attr, const char *buf, size_t count)
{
    //cpu_read_0xaa
    //brd_read_0xaa
    //slot_read_n_0xaa
    int temp_value = 0;
    int slot_index = 0;
    int ret = ERROR_SUCCESS;

    if (strstr(buf, "cpu_read") != NULL)
    {
        if (sscanf(buf, "cpu_read_0x%x", &temp_value) == 1)
        {
            ret = bsp_cpu_cpld_read_byte(&buffer_char, (u16)temp_value);
        }
        else
        {
            DBG_ECHO(DEBUG_ERR, "cpld buf set: '%s' format error! 'cpu_read_0xaa'", buf);
            count = -EINVAL;
        }
    }
    else if (strstr(buf, "brd_read") != NULL)
    {
        if (sscanf(buf, "brd_read_0x%x", &temp_value) == 1)
        {
            ret = bsp_cpld_read_byte(&buffer_char, (u16)temp_value);
        }
        else
        {
            DBG_ECHO(DEBUG_ERR, "cpld buf set: '%s' format error! 'brd_read_0xaa'", buf);
            count = -EINVAL;
        }
    }
    else if (strstr(buf, "slot_read") != NULL)
    {
        if (sscanf(buf, "slot_read_%d_0x%x", &slot_index, &temp_value) == 2)
        {
            ret = bsp_slot_cpld_read_byte(slot_index, &buffer_char, (u16)temp_value);
        }
        else
        {
            DBG_ECHO(DEBUG_ERR, "cpld buf set: '%s' format error! 'slot_read_n_0xaa'", buf);
            count = -EINVAL;
        }
    }
    else
    {
        ret = -EINVAL;
        DBG_ECHO(DEBUG_ERR, "cpld buf set: '%s' format error!", buf);
    }

    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "cpld buf set for %s failed", buf);
        count = ret;
    }
    return count;
}

static ssize_t bsp_cpld_custom_sysfs_read(struct kobject *kobjs, struct kobj_attribute *ka, char *buf, u32 node_index)
{
    ssize_t len = 0;
    u32 cpld_index = 0;
    int ret = ERROR_SUCCESS;
    u8 temp = 0;
    u16 mac_temp = 0;

    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_custom_sysfs_write get bd failed");
        return ERROR_FAILED;
    }
    sscanf(kobjs->name, "cpld%d", &cpld_index);
    cpld_index = cpld_index - 1;

    switch (node_index)
    {
        case ALIAS:
            len = bsp_cpld_custom_read_alias(cpld_index, buf);
            break;
        case TYPE:
            len = bsp_cpld_custom_read_type(cpld_index, buf);
            break;
        case HW_VERSION:
            len = bsp_cpld_custom_read_hw_version(cpld_index, buf); // bs100h0_get_sw_version
            break;
        case BROAD_VERSION:
            ret = bsp_cpld_get_board_version(cpld_index, &temp);
            if (ERROR_SUCCESS == ret)
            {
                len = scnprintf(buf, PAGE_SIZE, "%c\n", temp);
            }
            else
            {
                DBG_ECHO(DEBUG_ERR, "get pcb version failed");
                len = ret;
            }
            break;
        case REG_RESET_CPU:
            ret = bsp_cpld_get_bit(bd->cpld_addr_cpu_rst, bd->cpld_offs_cpu_rst, &temp);
            if (ERROR_SUCCESS == ret)
            {
                len = scnprintf(buf, PAGE_SIZE, "%d\n", (int)temp);
            }
            else
            {
                DBG_ECHO(DEBUG_ERR, "get cpu reset reg failed\n");
                len = ret;
            }
            break;
        case I2C_WDT_CTRL:
            ret = bsp_cpld_read_byte(&temp, bd->cpld_addr_i2c_wdt_ctrl);
            if (ERROR_SUCCESS == ret)
            {
                len = scnprintf(buf, PAGE_SIZE, "%d\n", (int)temp);
            }
            else
            {
                DBG_ECHO(DEBUG_ERR, "get i2c_wdt_ctrl reg failed\n");
                len = ret;
            }
            break;
        case CPU_INIT_OK:
            ret = bsp_cpld_read_part(&temp, bd->cpld_addr_cpu_init_ok, bd->cpld_mask_cpu_init_ok, bd->cpld_offs_cpu_init_ok);
            if (ERROR_SUCCESS == ret)
            {
                len = scnprintf(buf, PAGE_SIZE, "%d\n", (int)temp);
            }
            else
            {
                DBG_ECHO(DEBUG_ERR, "get cpu_init_ok reg failed\n");
                len = ret;
            }
            break;
        case I2C_WDT_FEED:
            ret = bsp_cpld_read_byte(&temp, bd->cpld_addr_i2c_wdt_feed);
            if (ERROR_SUCCESS == ret)
            {
                len = scnprintf(buf, PAGE_SIZE, "%d\n", (int)temp);
            }
            else
            {
                DBG_ECHO(DEBUG_ERR, "get i2c_wdt_feed reg failed\n");
                len = ret;
            }
            break;
        case RAW_DATA:
            len = bsp_cpld_custom_read_raw_data(cpld_index, buf);
            break;
        case REG_MAC_INIT_OK:
            ret = bsp_cpld_read_part(&temp, bd->cpld_addr_mac_init_ok, bd->cpld_mask_mac_init_ok, bd->cpld_offs_mac_init_ok);
            if (ERROR_SUCCESS == ret)
            {
                len = scnprintf(buf, PAGE_SIZE, "%x\n", (int)temp);
            }
            else
            {
                DBG_ECHO(DEBUG_ERR, "get cpu_init_ok reg failed\n");
                len = ret;
            }
            break;
        case REG_MAC_INNER_TEMP:
            ret = bsp_cpld_read_mac_inner_temp(&mac_temp);
            if (ERROR_SUCCESS == ret)
            {
                len = scnprintf(buf, PAGE_SIZE, "%d\n", mac_temp);
            }
            else
            {
                DBG_ECHO(DEBUG_ERR, "get mac inner temp error");
                len = ret;
            }
            break;
        case REG_MAC_WIDTH_TEMP:
            ret = bsp_cpld_read_mac_width_temp(&mac_temp);
            if (ERROR_SUCCESS == ret)
            {
                len = scnprintf(buf, PAGE_SIZE, "%d\n", mac_temp);
            }
            else
            {
                DBG_ECHO(DEBUG_ERR, "get mac width temp error");
                len = ret;
            }
            break;
        case REG_CPLD_TXDIS:
            len = bsp_cpld_custom_read_cpld_tx_dis(buf);
            break;
        default:
            DBG_ECHO(DEBUG_ERR, "no attribute index %d", node_index);
            len = -ENOSYS;
            break;
    }

    return len;
}

static ssize_t bsp_cpld_custom_root_sysfs_read(struct kobject *kobjs, struct kobj_attribute *ka, char *buf, u32 node_index)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    u8 temp = 0;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_custom_sysfs_write get bd failed");
        return -EINVAL;
    }

    switch (node_index)
    {
        case NUM_CPLDS:
            len = scnprintf(buf, PAGE_SIZE, "%d\n", (int)bd->cpld_num);
            break;
        case REG_REBOOT:
            ret = bsp_cpu_cpld_read_byte(&temp, bd->cpld_addr_reset_type);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "cpld read hw_reset_type failed!");
            len = scnprintf(buf, PAGE_SIZE, "0x%02x\n", (temp & bd->cpld_mask_reset_type) >> bd->cpld_offs_reset_type);
            break;
        case REG_LAST_REBOOT:
            ret = bsp_cpu_cpld_read_byte(&temp, bd->cpld_addr_last_reset_type);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "cpld read hw_last_reset_type failed!");
            len = scnprintf(buf, PAGE_SIZE, "0x%02x\n", (temp & bd->cpld_mask_last_reset_type) >> bd->cpld_offs_last_reset_type);
            break;
        case REG_CLR_RST:
            ret = bsp_cpu_cpld_read_byte(&temp, bd->cpld_addr_clr_rst);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "cpld read reg_clr_rst failed!");
            len = scnprintf(buf, PAGE_SIZE, "%d\n", (temp & bd->cpld_mask_clr_rst) >> bd->cpld_offs_clr_rst);
            break;
        default:
            DBG_ECHO(DEBUG_ERR, "cpld read attribute index %d invalid", node_index);
            len = -ENOSYS;
            break;
    }
exit:
    return len;
}

static ssize_t bsp_cpld_custom_sysfs_write(struct kobject *kobjs, struct kobj_attribute *ka,
                                           const char *buf, size_t count, u32 node_index)
{
    int temp = 0;
    int ret = ERROR_SUCCESS;

    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_custom_sysfs_write get bd failed");
        return -EINVAL;
    }

    if (1 != sscanf(buf, "%d", &temp))
    {
        return -EINVAL;
    }

    switch (node_index)
    {
        case REG_RESET_CPU:
            temp = (temp != 0) ? 1 : 0;
            ret = bsp_cpld_set_bit(bd->cpld_addr_cpu_rst, bd->cpld_offs_cpu_rst, (u8)temp);
            if (ret != ERROR_SUCCESS)
            {
                DBG_ECHO(DEBUG_ERR, "cpu reset set bit %d failed", temp);
                count = ret;
            }
            break;
        case I2C_WDT_CTRL:
            ret = bsp_cpld_write_byte(temp, bd->cpld_addr_i2c_wdt_ctrl);
            if (ret != ERROR_SUCCESS)
            {
                DBG_ECHO(DEBUG_ERR, "set i2c_wdt_ctrl %d failed", temp);
                count = ret;
            }
            break;
        case CPU_INIT_OK:
            ret = bsp_cpld_set_bit(bd->cpld_addr_cpu_init_ok, bd->cpld_offs_cpu_init_ok, (u8)temp);
            if (ret != ERROR_SUCCESS)
            {
                DBG_ECHO(DEBUG_ERR, "set cpu_init_ok %d failed", temp);
                count = ret;
            }
            break;
        case I2C_WDT_FEED:
            ret = bsp_cpld_write_byte(temp, bd->cpld_addr_i2c_wdt_feed);
            if (ret != ERROR_SUCCESS)
            {
                DBG_ECHO(DEBUG_ERR, "set i2c_wdt_feed %d failed", temp);
                count = ret;
            }
            break;
        case REG_MAC_INIT_OK:
            temp = (temp != 0) ? 1 : 0;
            ret = bsp_set_mac_init_ok((u8)temp);
            if (ret != ERROR_SUCCESS)
            {
                DBG_ECHO(DEBUG_ERR, "set mac_init_ok %d failed", temp);
                count = ret;
            }
            break;
        case REG_CPLD_TXDIS:
            temp = (temp != 0) ? 0xff : 0;
            ret = bsp_set_cpld_tx_dis((u8)temp);
            if (ret != ERROR_SUCCESS)
            {
                //DBG_ECHO(DEBUG_ERR, "set cpld_tx_dis %d failed", temp);
                count = ret;
            }
            break;
        default:
            DBG_ECHO(DEBUG_ERR, "no support attribute index %d", node_index);
            count = -EINVAL;
            break;
    }
    return count;
}

#define CPLD_CUSTOM_ATTR_DEF(_name, _nodeindex) \
    static ssize_t CPLD_CUSTOM_SYSFS_READ_##_nodeindex(struct kobject *kobjs, struct kobj_attribute *ka, char *buf) { \
        return bsp_cpld_custom_sysfs_read(kobjs, ka, buf, _nodeindex); \
    } \
    SYSFS_PREFIX_RO_ATTR_DEF(cpld, _name, CPLD_CUSTOM_SYSFS_READ_##_nodeindex)

#define CPLD_CUSTOM_RW_ATTR_DEF(_name, _nodeindex) \
    static ssize_t CPLD_CUSTOM_SYSFS_READ_##_nodeindex(struct kobject *kobjs, struct kobj_attribute *ka, char *buf) { \
        return bsp_cpld_custom_sysfs_read(kobjs, ka, buf, _nodeindex); \
    } \
    static ssize_t CPLD_CUSTOM_SYSFS_WRITE_##_nodeindex(struct kobject *kobjs, struct kobj_attribute *ka, const char *buf, size_t count) { \
        return bsp_cpld_custom_sysfs_write(kobjs, ka, buf, count, _nodeindex); \
    } \
    SYSFS_PREFIX_RW_ATTR_DEF(cpld, _name, CPLD_CUSTOM_SYSFS_READ_##_nodeindex, CPLD_CUSTOM_SYSFS_WRITE_##_nodeindex)

#define CPLD_CUSTOM_ROOT_ATTR_DEF(_name, _nodeindex) \
    static ssize_t CPLD_CUSTOM_ROOT_SYSFS_READ_##_nodeindex(struct kobject *kobjs, struct kobj_attribute *ka, char *buf) { \
        return bsp_cpld_custom_root_sysfs_read(kobjs, ka, buf, _nodeindex); \
    } \
    SYSFS_PREFIX_RO_ATTR_DEF(cpld, _name, CPLD_CUSTOM_ROOT_SYSFS_READ_##_nodeindex)

#define CPLD_CUSTOM_ROOT_RW_ATTR_DEF(_name, _nodeindex) \
    static ssize_t CPLD_CUSTOM_ROOT_SYSFS_READ_##_nodeindex(struct kobject *kobjs, struct kobj_attribute *ka, char *buf) { \
        return bsp_cpld_custom_root_sysfs_read(kobjs, ka, buf, _nodeindex); \
    } \
    static ssize_t CPLD_CUSTOM_ROOT_SYSFS_WRITE_##_nodeindex(struct kobject *kobjs, struct kobj_attribute *ka, const char *buf, size_t count) { \
        return bsp_cpld_custom_root_sysfs_write(kobjs, ka, buf, count, _nodeindex); \
    } \
    SYSFS_PREFIX_RW_ATTR_DEF(cpld, _name, CPLD_CUSTOM_ROOT_SYSFS_READ_##_nodeindex, CPLD_CUSTOM_ROOT_SYSFS_WRITE_##_nodeindex)

#define CPLD_CUSTOM_ROOT_WO_ATTR_DEF(_name, _nodeindex) \
    static ssize_t CPLD_CUSTOM_ROOT_SYSFS_WRITE_##_nodeindex(struct kobject *kobjs, struct kobj_attribute *ka, const char *buf, size_t count) { \
        return bsp_cpld_custom_root_sysfs_write(kobjs, ka, buf, count, _nodeindex); \
    } \
    SYSFS_PREFIX_WO_ATTR_DEF(cpld, _name, CPLD_CUSTOM_ROOT_SYSFS_WRITE_##_nodeindex)


SYSFS_RW_ATTR_DEF(CPU_CPLD_NAME,   bsp_cpld_sysfs_show_cpld, bsp_cpld_sysfs_set_cpld);
SYSFS_RW_ATTR_DEF(BOARD_CPLD_NAME, bsp_cpld_sysfs_show_cpld, bsp_cpld_sysfs_set_cpld);
SYSFS_RW_ATTR_DEF(BIOS_CPLD, bsp_cpld_sysfs_show_cpld, bsp_cpld_sysfs_set_cpld);
SYSFS_RW_ATTR_DEF(BUFF_NAME,       bsp_cpld_sysfs_buffer_show, bsp_cpld_sysfs_buffer_set);

CPLD_CUSTOM_ATTR_DEF(alias, ALIAS);
CPLD_CUSTOM_ATTR_DEF(type, TYPE);
CPLD_CUSTOM_ATTR_DEF(hw_version, HW_VERSION);
CPLD_CUSTOM_ATTR_DEF(board_version, BROAD_VERSION);
CPLD_CUSTOM_ATTR_DEF(raw_data, RAW_DATA);
CPLD_CUSTOM_ATTR_DEF(reg_mac_inner_temp, REG_MAC_INNER_TEMP);
CPLD_CUSTOM_ATTR_DEF(reg_mac_width_temp, REG_MAC_WIDTH_TEMP);

CPLD_CUSTOM_RW_ATTR_DEF(reg_reset_cpu, REG_RESET_CPU);
CPLD_CUSTOM_RW_ATTR_DEF(reg_cpu_init_ok, CPU_INIT_OK);
CPLD_CUSTOM_RW_ATTR_DEF(reg_mac_init_ok, REG_MAC_INIT_OK);
CPLD_CUSTOM_RW_ATTR_DEF(reg_cpld_txdis, REG_CPLD_TXDIS);
//CPLD_CUSTOM_ROOT_ATTR_DEF(reg_i2c_wdt_ctrl, I2C_WDT_CTRL);
//CPLD_CUSTOM_ROOT_ATTR_DEF(reg_i2c_wdt_feed, I2C_WDT_FEED);
CPLD_CUSTOM_ROOT_ATTR_DEF(num, NUM_CPLDS);
CPLD_CUSTOM_ROOT_ATTR_DEF(reg_reboot, REG_REBOOT);
CPLD_CUSTOM_ROOT_ATTR_DEF(reg_last_reboot, REG_LAST_REBOOT);
CPLD_CUSTOM_ROOT_WO_ATTR_DEF(reg_clr_rst, REG_CLR_RST);

BSPMODULE_DEBUG_ATTR_DEF(debug, BSP_CPLD_MODULE);
BSPMODULE_DEBUG_RW_ATTR_DEF(loglevel, BSP_CPLD_MODULE);

//root
static struct attribute *cpld_custom_root_attributes[] =
{
    &cpld_num.attr,
    &bspmodule_debug.attr,
    &bspmodule_loglevel.attr,
    NULL
};

static const struct attribute_group cpld_custom_root_attr_group =
{
    .attrs = cpld_custom_root_attributes,
};

//cpld0
static struct attribute *cpld_custom_cpu_attributes[] =
{
    &cpld_alias.attr,
    &cpld_type.attr,
    &cpld_hw_version.attr,
    &cpld_board_version.attr,
    //&cpld_reg_i2c_wdt_ctrl.attr,
    //&cpld_reg_i2c_wdt_feed.attr,
    &cpld_reg_clr_rst.attr,
    &cpld_reg_reboot.attr,
    &cpld_reg_last_reboot.attr,
    &cpld_raw_data.attr,
    NULL
};

static const struct attribute_group cpld_custom_cpu_attr_group =
{
    .attrs = cpld_custom_cpu_attributes,
};

//cpld1
static struct attribute *cpld_custom_board0_attributes[] =
{
    &cpld_alias.attr,
    &cpld_type.attr,
    &cpld_hw_version.attr,
    &cpld_board_version.attr,
    &cpld_reg_mac_init_ok.attr,
    &cpld_reg_cpu_init_ok.attr,
    &cpld_reg_reset_cpu.attr,
    &cpld_reg_mac_inner_temp.attr,
    &cpld_raw_data.attr,
    &cpld_reg_cpld_txdis.attr,
    &cpld_reg_mac_width_temp.attr,
    NULL
};

static const struct attribute_group cpld_custom_board0_attr_group =
{
    .attrs = cpld_custom_board0_attributes,
};

//cpld2
static struct attribute *cpld_custom_board1_attributes[] =
{
    &cpld_alias.attr,
    &cpld_type.attr,
    &cpld_hw_version.attr,
    &cpld_board_version.attr,
    &cpld_raw_data.attr,
    NULL
};

static struct attribute *cpld_custom_dom_attributes[] =
{
    &cpld_alias.attr,
    &cpld_type.attr,
    &cpld_hw_version.attr,
    NULL
};

static const struct attribute_group cpld_custom_board1_attr_group =
{
    .attrs = cpld_custom_board1_attributes,
};

static const struct attribute_group cpld_custom_dom_attr_group =
{
    .attrs = cpld_custom_dom_attributes,
};

/*
//cpld3
static struct attribute *cpld_custom_board2_attributes[] =
{
    &cpld_alias.attr,
    &cpld_type.attr,
    &cpld_hw_version.attr,
    &cpld_board_version.attr,
    &cpld_raw_data.attr,
    NULL
};

static const struct attribute_group cpld_custom_board2_attr_group =
{
    .attrs = cpld_custom_board2_attributes,
};

//cpld4
static struct attribute *cpld_custom_board3_attributes[] =
{
    &cpld_alias.attr,
    &cpld_type.attr,
    &cpld_hw_version.attr,
    &cpld_board_version.attr,
    &cpld_raw_data.attr,
    NULL
};

static const struct attribute_group cpld_custom_board3_attr_group =
{
    .attrs = cpld_custom_board3_attributes,
};*/

void cpld_release_kobj(void)
{
    int cpld_index = 0;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: cpld_release_kobj get bd failed");
        return ;
    }

    for (cpld_index = 0; cpld_index < bd->cpld_num; cpld_index ++)
    {
        if (kobj_cpld_sub[cpld_index] != NULL)
        {
            if (0 == cpld_index)
            {
                sysfs_remove_group(kobj_cpld_sub[cpld_index], &cpld_custom_cpu_attr_group);
            }
            else if (1 == cpld_index)
            {
                sysfs_remove_group(kobj_cpld_sub[cpld_index], &cpld_custom_board0_attr_group);
            }
            else
            {
                sysfs_remove_group(kobj_cpld_sub[cpld_index], &cpld_custom_board1_attr_group);
            }
            kobject_put(kobj_cpld_sub[cpld_index]);
        }
    }

    if (kobj_cpld_root != NULL)
    {
        sysfs_remove_group(kobj_cpld_root, &cpld_custom_root_attr_group);
        kobject_put(kobj_cpld_root);
    }

    if (kobj_cpld_debug != NULL)
        kobject_put(kobj_cpld_debug);

    DBG_ECHO(DEBUG_INFO, "module cpld uninstalled !\n");
    return;
}

//设置初始化入口函数
int cpld_sysfs_init(void)
{
    int ret = ERROR_SUCCESS;
    int slot_index;
    int i = 0;
    char temp_str[128] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: cpld_sysfs_init get bd failed");
        return -EINVAL;
    }

    memset(attr_name, 0, sizeof(attr_name));

    //create node for cpld debug
    kobj_cpld_debug = kobject_create_and_add("cpld", kobj_debug);
    if (kobj_cpld_debug == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "kobj_cpld_debug create falled!\n");
        ret = -ENOMEM;
        goto exit;
    }

    //add the attribute files and check the result, exit if failed.
    CHECK_CREATE_SYSFS_FILE(kobj_cpld_debug, BOARD_CPLD_NAME, ret);
    CHECK_CREATE_SYSFS_FILE(kobj_cpld_debug, CPU_CPLD_NAME, ret);
    CHECK_CREATE_SYSFS_FILE(kobj_cpld_debug, BUFF_NAME, ret);
    CHECK_CREATE_SYSFS_FILE(kobj_cpld_debug, BIOS_CPLD, ret);

    //add subslot cpld
    for (slot_index = 0; slot_index < bd->slot_num; slot_index ++)
    {
        if (bd->cpld_size_slot[slot_index] != 0)
        {
            scnprintf(attr_name[slot_index], sizeof(attr_name[slot_index]), SLOT_CPLD_NAME, slot_index + 1);
            slot_cpld_attr[slot_index].attr.name = attr_name[slot_index];
            slot_cpld_attr[slot_index].attr.mode = (S_IRUGO | S_IWUSR);
            slot_cpld_attr[slot_index].show = bsp_cpld_sysfs_show_cpld;
            slot_cpld_attr[slot_index].store = bsp_cpld_sysfs_set_cpld;
            ret = sysfs_create_file(kobj_cpld_debug, &(slot_cpld_attr[slot_index].attr));
            if (ERROR_SUCCESS != ret)
            {
                DBG_ECHO(DEBUG_ERR, "sysfs create attribute %s failed!", slot_cpld_attr[slot_index].attr.name);
                goto exit;
            }
        }
    }
    //root
    kobj_cpld_root = kobject_create_and_add("cpld", kobj_switch);
    if (kobj_cpld_root == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "kobj_switch create falled!\n");
        ret = -ENOMEM;
        goto exit;
    }

    ret = sysfs_create_group(kobj_cpld_root, &cpld_custom_root_attr_group);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "create cpld root group failed");


    //cpld1
    kobj_cpld_sub[0] = kobject_create_and_add("cpld1", kobj_cpld_root);
    if (kobj_cpld_sub[0] == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "kobj_cpld_root create falled!\n");
        ret = -ENOMEM;
        goto exit;
    }

    ret = sysfs_create_group(kobj_cpld_sub[0], &cpld_custom_cpu_attr_group);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "create cpld sub group failed");


    //cpld2
    kobj_cpld_sub[1] = kobject_create_and_add("cpld2", kobj_cpld_root);
    if (kobj_cpld_sub[1] == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "kobj_cpld_root create falled!\n");
        ret = -ENOMEM;
        goto exit;
    }
    ret = sysfs_create_group(kobj_cpld_sub[1], &cpld_custom_board0_attr_group);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "create cpld sub group failed");

    for (i = 2; i < bd->cpld_num; i++)
    {
        scnprintf(temp_str, sizeof(temp_str), "cpld%d", i + 1);
        kobj_cpld_sub[i] = kobject_create_and_add(temp_str, kobj_cpld_root);
        if (kobj_cpld_sub[i] == NULL)
        {
            DBG_ECHO(DEBUG_ERR, "kobj_cpld_root create falled!\n");
            ret = -ENOMEM;
            goto exit;
        }
        ret = sysfs_create_group(kobj_cpld_sub[i], &cpld_custom_board1_attr_group);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "create cpld sub group failed");
    }

exit:
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "cpld module init failed! result=%d\n", ret);
        cpld_release_kobj();
    }
    else
    {
        INIT_PRINT("module init finished and success!");
    }

    return ret;
}

//设置初始化入口函数
static int __init cpld_init(void)
{
    int ret = ERROR_SUCCESS;

    INIT_PRINT("module init started");

    ret = cpld_sysfs_init();
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "cpld module init failed! result=%d", ret);
    }
    else
    {
        INIT_PRINT("module init finished and success!");
    }    

    return ret;
}

//设置出口函数
static void __exit cpld_exit(void)
{
    cpld_release_kobj();
    

    INIT_PRINT("module uninstalled !\n");
    return;
}

module_init(cpld_init);
module_exit(cpld_exit);

MODULE_LICENSE("Dual BSD/GPL");

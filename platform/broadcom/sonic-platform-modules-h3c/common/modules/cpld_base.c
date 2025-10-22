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
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/file.h>
/*私有文件*/
#include "pub.h"
#include "bsp_base.h"
#include "cpld_base.h"

enum PCB_9825_64D_CPLD_TYPE // for PDT_TYPE_LS_9825_64D_W1
{
    CPU_CPLD_TYPE,
    MAIN_BOARD_TYPE_1,  // 主控板
    MAIN_BOARD_TYPE_2,  // 主控板
    SWITCH_BOARD_TYPE_1,    // 交换板
    SWITCH_BOARD_TYPE_2,    // 交换板
    DOWN_PORT_TYPE          // 下端口版
};



int bsp_cpld_get_cpld_version(int cpld_index, u8 *cpld_version_hex)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_get_cpld_version bd failed");
        return -EINVAL;
    }

    if (0 == cpld_index)   /*CPU 从零开始*/
    {
        ret = bsp_cpu_cpld_read_part(cpld_version_hex, bd->cpld_addr_cpld_ver[cpld_index], bd->cpld_mask_cpld_ver[cpld_index], bd->cpld_offs_cpld_ver[cpld_index]);
    }
    else
    {
        ret = bsp_bios_cpld_read_part(cpld_version_hex, bd->cpld_addr_cpld_ver[cpld_index], bd->cpld_mask_cpld_ver[cpld_index], bd->cpld_offs_cpld_ver[cpld_index]);
    }

    return ret;
}

int bsp_cpld_get_board_version(int cpld_index, u8 *board_version)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_get_board_version bd failed");
        return -EINVAL;
    }

    if (PDT_TYPE_LS_9825_64D_W1 == bd->product_type)
    {
        if (CPU_CPLD_TYPE == cpld_index)     /*CPU 从零开始*/
        {
            ret = bsp_cpu_cpld_read_part(board_version, bd->cpld_addr_pcb_ver[cpld_index], bd->cpld_mask_pcb_ver[cpld_index], bd->cpld_offs_pcb_ver[cpld_index]);
        }
        else if (MAIN_BOARD_TYPE_1 == cpld_index || MAIN_BOARD_TYPE_2 == cpld_index || DOWN_PORT_TYPE == cpld_index)
        {
            ret = bsp_cpld_read_part(board_version, bd->cpld_addr_pcb_ver[cpld_index], bd->cpld_mask_pcb_ver[cpld_index], bd->cpld_offs_pcb_ver[cpld_index]);
        }
        else if (SWITCH_BOARD_TYPE_1 == cpld_index || SWITCH_BOARD_TYPE_2 == cpld_index)
        {
            ret = bsp_bios_cpld_read_part(board_version, bd->cpld_addr_pcb_ver[cpld_index], bd->cpld_mask_pcb_ver[cpld_index], bd->cpld_offs_pcb_ver[cpld_index]);
        }
    }
    else
    {
        if (0 == cpld_index)     /*CPU 从零开始*/
        {
            ret = bsp_cpu_cpld_read_part(board_version, bd->cpld_addr_pcb_ver[cpld_index], bd->cpld_mask_pcb_ver[cpld_index], bd->cpld_offs_pcb_ver[cpld_index]);
        }
        else
        {
            ret = bsp_cpld_read_part(board_version, bd->cpld_addr_pcb_ver[cpld_index], bd->cpld_mask_pcb_ver[cpld_index], bd->cpld_offs_pcb_ver[cpld_index]);
        }
    }
    if (*board_version <= 9)
        *board_version = *board_version + 65;
    else if (*board_version >= 10  && *board_version <= 15)
        *board_version = *board_version + 55;

    return ret;
}

int bsp_set_mac_init_ok(u8 bit)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_set_mac_init_ok bd failed");
        return -EINVAL;
    }
    CHECK_IF_ZERO_GOTO_EXIT(-ENODEV, ret, bd->cpld_addr_mac_init_ok, "mainboard mac_init_ok reg is not defined!");
    ret = bsp_cpld_set_bit(bd->cpld_addr_mac_init_ok, bd->cpld_offs_mac_init_ok, bit);

exit:
    return ret;
}

int bsp_set_cpld_tx_dis(u8 val)
{
    int i;
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_set_cpld_tx_dis get bd failed");
        return -EINVAL;
    }

    if (0 != bd->cpld_tx_dis_num)
    {
        for (i = 0; i < bd->cpld_tx_dis_num; i++)
        {
            ret = bsp_cpld_write_byte(val, bd->cpld_addr_cpld_tx_dis[i]);
            if (ERROR_SUCCESS != ret)
            {
                DBG_ECHO(DEBUG_ERR, "write cpld tx dis:%x error\n", bd->cpld_addr_cpld_tx_dis[i]);
            }
        }
        return ret;
    }
    else
    {
        return -ENOSYS;
    }
}

int bsp_set_ssd_power_reset(unsigned int  bit)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_set_ssd_power_reset bd failed");
        return -EINVAL;
    }

    printk(KERN_WARNING "bsp_set_ssd_power_reset begin");
    bit = (bit == 0) ? 0 : 1;
    CHECK_IF_ZERO_GOTO_EXIT(-ENODEV, ret, bd->cpld_addr_ssd_pwr_down, "mainboard bsp_set_ssd_power_reset reg is not defined!");
    ret = bsp_cpld_set_bit(bd->cpld_addr_ssd_pwr_down, bd->cpld_offs_ssd_pwr_down, 1);
    if (ret != ERROR_SUCCESS)
    {
        printk(KERN_ERR "bsp_set_ssd_power_reset set 1 failed");
    }
    else
    {
        printk(KERN_WARNING "bsp_set_ssd_power_reset set 1 to down");
    }
    msleep(3000);
    ret = bsp_cpld_set_bit(bd->cpld_addr_ssd_pwr_down, bd->cpld_offs_ssd_pwr_down, 0);
    if (ret != ERROR_SUCCESS)
    {
        printk(KERN_ERR "bsp_set_ssd_power_reset set 0 failed");
    }
    else
    {
        printk(KERN_WARNING "bsp_set_ssd_power_reset set 0 to up");
    }

    printk(KERN_WARNING "bsp_set_ssd_power_reset end");
exit:
    return ret;
}

ssize_t bsp_cpld_custom_read_cpld_tx_dis(char *buf)
{
    int i, j, pos;
    ssize_t len;
    u8 temp_value = 0;
    int ret = ERROR_SUCCESS;
    char txdis[CPLD_MAX_TX_DIS_ALL_NUM] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_custom_read_cpld_tx_dis get bd failed");
        return -EINVAL;
    }

    if (0 != bd->cpld_tx_dis_num)
    {
        pos = 0;
        for (i = 0; i < bd->cpld_tx_dis_num; i++)
        {
            ret = bsp_cpld_read_byte(&temp_value, bd->cpld_addr_cpld_tx_dis[i]);
            if (ERROR_SUCCESS != ret)
            {
                DBG_ECHO(DEBUG_ERR, "read cpld tx dis:%x error\n", bd->cpld_addr_cpld_tx_dis[i]);
                temp_value = 0;
            }

            for (j = 0; j < 8; j++)
            {
                if (0 != (temp_value & (1 << j)))
                {
                    txdis[pos] = '1';
                }
                else
                {
                    txdis[pos] = '0';
                }
                pos++;
            }
        }
        txdis[pos] = '\0';
        len = scnprintf(buf, PAGE_SIZE, "%s\n", txdis);
        return len;
    }
    else
    {
        return -ENOSYS;
    }
}

ssize_t bsp_cpld_custom_read_alias(int cpld_index, char *buf)
{
    ssize_t len = 0;

    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_custom_read_alias get bd failed");
        return -EINVAL;
    }
    if ((cpld_index < bd->cpld_num) && (bd->cpld_location_describe[cpld_index] != NULL))
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", bd->cpld_location_describe[cpld_index]);
    }
    else
    {
        DBG_ECHO(DEBUG_ERR, "error cpld index %d or alias unknown\n", cpld_index + 1);
        len = -EINVAL;
    }
    return len;
}

ssize_t bsp_cpld_custom_read_type(int cpld_index, char *buf)
{
    ssize_t len = 0;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_custom_read_type get bd failed");
        return -EINVAL;
    }
    if ((cpld_index < bd->cpld_num) && (bd->cpld_type_describe[cpld_index] != NULL))
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", bd->cpld_type_describe[cpld_index]);
    }
    else
    {
        DBG_ECHO(DEBUG_ERR, "error cpld index %d or type unknown\n", cpld_index + 1);
        len = -EINVAL;
    }
    return len;
}

ssize_t bsp_cpld_custom_read_hw_version(int cpld_index, char *buf)
{
    int ret = ERROR_SUCCESS;
    ssize_t len = 0;
    u8 temp = 0;

    ret = bsp_cpld_get_cpld_version(cpld_index, &temp);
    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%x\n", temp);
    }
    else
    {
        len = ret;
    }

    return len;
}

size_t bsp_cpld_custom_read_raw_data(int cpld_index, char *buf)
{

    u16 cpld_size = 0;
    int start_addr = 0;
    int slot_index = -1;
    u16 i, j;
    ssize_t len = 0;
    u8 temp = 0;
    int ret = ERROR_SUCCESS;
    int (* cpld_read_byte_func)(u8 * value, u16 offset) = NULL;

    if (cpld_index == 0)
    {
        cpld_size = (u16) bsp_get_cpu_cpld_size();
        cpld_read_byte_func = bsp_cpu_cpld_read_byte;
        start_addr = bsp_get_board_data()->cpld_hw_addr_cpu;
    }
    else
    {
        cpld_size = (u16) bsp_cpld_get_size();
        cpld_read_byte_func = bsp_cpld_read_byte;
        start_addr = bsp_get_board_data()->cpld_hw_addr_board;
    }
    len += scnprintf(buf + len, PAGE_SIZE - len, "\nhw address start: 0x%4x\n", start_addr);

    for (i = 0; i < cpld_size; i += 16)
    {
        //avoid overflow
        if (len >= (PAGE_SIZE - 200))
        {
            DBG_ECHO(DEBUG_INFO, "show cpld buf size reached %d, break to avoid overflow.", (int)len);
            break;
        }
        len += scnprintf(buf + len, PAGE_SIZE - len, "0x%04x: ", i);

        for (j = 0; j < 16; j++)
        {
            ret = cpld_read_byte_func == NULL ? bsp_slot_cpld_read_byte(slot_index, &temp, i + j) : cpld_read_byte_func(&temp, i + j);

            if (ret == ERROR_SUCCESS)
            {
                len += scnprintf(buf + len, PAGE_SIZE - len, "%02x %s", temp, j == 7 ? " " : "");
            }
            else
            {
                len += scnprintf(buf + len, PAGE_SIZE - len, "XX ");
            }

        }

        len += scnprintf(buf + len, PAGE_SIZE - len, "\n");
    }
    return len;
}


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
#include <asm/io.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/rtc.h>
#include <linux/version.h>
/*私有文件*/
#include "i2c_dev_reg.h"
#include "pub.h"
#include "bsp_base.h"

/* module_name_string和enum BSPMODULE_INDEX_E一一对应，不要漏！  */
char *module_name_string[BSP_MODULE_MAX] = {"debug", "cpld", "fan", "psu", "sensor", "syseeprom", "sysled", "transceiver", "watchdog", "fpga", "ft"};

struct kobject *kobj_switch = NULL;         //switch根目录, 客户要求的sysfs结构
struct kobject *kobj_debug  = NULL;         //debug根目录，用于内部debug命令
struct kobject *kobj_debug_i2c = NULL;      //i2c debug node

int g_i2crst_time = 3000;//us
int g_i2c_select_time = 10;//ms
int g_psuretry_count = 5;
module_param(g_i2crst_time, int, 0644);
module_param(g_i2c_select_time, int, 0644);
module_param(g_psuretry_count, int, 0644);
module_param_named(bsp_debug_level, bsp_module_debug_level[BSP_BASE_MODULE], int, 0644);
MODULE_PARM_DESC(bsp_debug_level, "DEBUG 0x4, ERROR 0x2, INFO 0x1, ALL 0x7, DEBUG_OFF 0x0; Default value is ERROR");

#define MODULE_NAME "bsp_base"
#define DBG_ECHO(level, fmt, args...) DEBUG_PRINT(bsp_module_debug_level[BSP_BASE_MODULE], level, BSP_LOG_FILE, fmt, ##args)

struct i2c_debug_info i2c_debug_info_read = {0};
struct i2c_debug_info i2c_debug_info_write = {0};
extern struct i2c_diag_records i2c_diag_info;
extern struct bsp_log_filter bsp_recent_log;


extern struct mutex bsp_i2c_path_lock;
extern struct mutex bsp_cpld_lock;
extern struct mutex bsp_slot_cpld_lock[MAX_SLOT_NUM];
extern struct mutex bsp_logfile_lock;
extern struct mutex bsp_psu_logfile_lock;

extern struct mutex bsp_recent_log_lock;
extern struct mutex bsp_fan_speed_lock;
extern struct mutex bsp_mac_inner_t;

extern  bool log_to_private_file;
extern bool log_filter_to_dmesg;
extern int bsp_dmesg_log_level;
extern char *curr_h3c_log_file;

//i2c选通表
ssize_t bsp_sysfs_print_i2c_select_table(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    return bsp_print_i2c_select_table(buf);
}


ssize_t bsp_sysfs_debug_dump_i2c_mem(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    u8 temp_buffer[1024] = {0};
    u16 tempu16 = 0;
    ssize_t len = 0;
    int i = 0;
    int ret = ERROR_SUCCESS;
    len += scnprintf(buf, PAGE_SIZE - len, "Read dev id 0x%x address 0x%x from 0x%x length 0x%x\n", i2c_debug_info_read.dev_path_id, i2c_debug_info_read.i2c_addr, i2c_debug_info_read.inner_addr, i2c_debug_info_read.read_len);

    switch (i2c_debug_info_read.rw_mode)
    {
        case 0x1:
            ret = bsp_i2c_common_eeprom_read_bytes(i2c_debug_info_read.i2c_addr, i2c_debug_info_read.inner_addr, i2c_debug_info_read.read_len, temp_buffer, i2c_debug_info_read.dev_path_id);
            if (ret == ERROR_SUCCESS)
            {
                len += bsp_print_memory(temp_buffer, i2c_debug_info_read.read_len, buf + len, PAGE_SIZE - len, i2c_debug_info_read.inner_addr, 4);
            }
            break;
        case 0x2:
            ret = bsp_i2c_24LC128_eeprom_read_bytes(i2c_debug_info_read.i2c_addr, i2c_debug_info_read.inner_addr, i2c_debug_info_read.read_len, temp_buffer, i2c_debug_info_read.dev_path_id);
            if (ret == ERROR_SUCCESS)
            {
                len += bsp_print_memory(temp_buffer, i2c_debug_info_read.read_len, buf + len, PAGE_SIZE - len, i2c_debug_info_read.inner_addr, 4);
            }
            break;
        case 0x3:
            for (i = 0; i < i2c_debug_info_read.read_len; i++)
            {
                ret = bsp_i2c_isl68127_read_reg(i2c_debug_info_read.i2c_addr, i2c_debug_info_read.inner_addr + i, &tempu16, 2, i2c_debug_info_read.dev_path_id);
                if (ret == ERROR_SUCCESS)
                {
                    len += scnprintf(buf + len, PAGE_SIZE - len, "0x%02x: 0x%04x\n", i2c_debug_info_read.inner_addr + i, tempu16);
                }
                else
                {
                    len += scnprintf(buf + len, PAGE_SIZE - len, "0x%02x: Failed!\n", i2c_debug_info_read.inner_addr + i);
                }
            }
            break;
    }

    if (ret != ERROR_SUCCESS)
    {
        len += scnprintf(buf + len, PAGE_SIZE - len, "\nFailed!\n");
    }
    return len;
}

ssize_t bsp_sysfs_debug_i2c_do_write(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;

    if (i2c_debug_info_write.is_valid != 1)
    {
        len = scnprintf(buf, PAGE_SIZE, "param is not set for writing, nothing to do.\n");
        goto exit;
    }

    len += scnprintf(buf, PAGE_SIZE, "write dev id 0x%x address 0x%x inner 0x%x value 0x%x\n", i2c_debug_info_write.dev_path_id, i2c_debug_info_write.i2c_addr, i2c_debug_info_write.inner_addr, i2c_debug_info_write.write_value);

    //借用相似函数
    switch (i2c_debug_info_write.rw_mode)
    {
        case 0x1:
            ret = bsp_i2c_common_eeprom_write_byte(i2c_debug_info_write.i2c_addr, i2c_debug_info_write.inner_addr, (u8)i2c_debug_info_write.write_value, i2c_debug_info_write.dev_path_id);
            break;
        case 0x2:
            ret = bsp_i2c_24LC128_eeprom_write_byte(i2c_debug_info_write.i2c_addr, i2c_debug_info_write.inner_addr, i2c_debug_info_write.write_value, i2c_debug_info_write.dev_path_id);
            break;
        case 0x3:
            ret = bsp_i2c_isl68127_write_reg(i2c_debug_info_write.i2c_addr, i2c_debug_info_write.inner_addr, i2c_debug_info_write.write_value, 2, i2c_debug_info_write.dev_path_id);
            break;
    }

    len += scnprintf(buf + len, PAGE_SIZE - len, "%s", ret == ERROR_SUCCESS ? "success!\n" : "failed!\n");

exit:
    return len;
}

ssize_t bsp_sysfs_i2c_debug_op_param_get(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    ssize_t len = 0;
    int i = 0;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_sysfs_i2c_debug_op_param_get get bd failed");
        return -EINVAL;
    }

    len += scnprintf(buf + len, PAGE_SIZE - len, "\nExample :\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    Turn i2c path to path_id 0x1, I2C device address is 0x50, read 0x10 bytes starts from inner address 0x0\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "        echo 'path 0x1 addr 0x50 read from 0x0 len 0x10 mode 0x1' > param \n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    Turn i2c path to path_id 0x1, I2C device address is 0x50, write 0x1 to inner address 0x0\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "        echo 'path 0x1 addr 0x50 write inner 0x0 value 0x1 mode 0x1' > param \n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    *all integer must be hex. \n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    mode 0x1: inner address is  8 bit, data width  8 bit\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    mode 0x2: inner address is 16 bit, data width  8 bit\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    mode 0x3: inner address is  8 bit, data width 16 bit\n");

    len += scnprintf(buf + len, PAGE_SIZE - len, "\nCurrent read settings:\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    Path_ID    : %d\n",   i2c_debug_info_read.dev_path_id);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    DevI2CAddr : 0x%x\n", i2c_debug_info_read.i2c_addr);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    InnerAddr  : 0x%x\n", i2c_debug_info_read.inner_addr);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    ReadLength : 0x%x\n", i2c_debug_info_read.read_len);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    ReadMode   : 0x%x\n", i2c_debug_info_read.rw_mode);

    len += scnprintf(buf + len, PAGE_SIZE - len, "\nCurrent write settings:\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    Path_ID    : %d\n",   i2c_debug_info_write.dev_path_id);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    DevI2CAddr : 0x%x\n", i2c_debug_info_write.i2c_addr);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    InnerAddr  : 0x%x\n", i2c_debug_info_write.inner_addr);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    WriteValue : 0x%x\n", i2c_debug_info_write.write_value);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    WriteMode  : 0x%x\n", i2c_debug_info_write.rw_mode);

    len += scnprintf(buf + len, PAGE_SIZE - len, "\nI2C Path_ID defination:\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    %-25s :%4d~%d (%2d/slot)\n", __stringify(I2C_DEV_OPTIC_IDX_START), I2C_DEV_OPTIC_IDX_START, I2C_DEV_OPTIC_BUTT - 1, MAX_OPTIC_PER_SLOT);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    %-25s :%4d~%d (%2d/slot)\n", __stringify(I2C_DEV_EEPROM), I2C_DEV_EEPROM, I2C_DEV_EEPROM_BUTT - 1, MAX_EEPROM_PER_SLOT);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    %-25s :%4d~%d (%2d/slot)\n", __stringify(I2C_DEV_LM75), I2C_DEV_LM75, I2C_DEV_LM75_BUTT - 1, MAX_LM75_NUM_PER_SLOT);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    %-25s :%4d~%d (%2d/slot)\n", __stringify(I2C_DEV_MAX6696), I2C_DEV_MAX6696, I2C_DEV_MAX6696_BUTT - 1, MAX_MAX6696_NUM_PER_SLOT);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    %-25s :%4d~%d\n",  __stringify(I2C_DEV_PSU), I2C_DEV_PSU, I2C_DEV_PSU_BUTT - 1);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    %-25s :%4d~%d\n", __stringify(I2C_DEV_INA219), I2C_DEV_INA219, I2C_DEV_INA219_BUTT - 1);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    %-25s :%4d~%d\n", __stringify(I2C_DEV_I350), I2C_DEV_I350, I2C_DEV_I350);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    %-25s :%4d~%d\n", __stringify(I2C_DEV_FAN), I2C_DEV_FAN, I2C_DEV_FAN_BUTT - 1);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    %-25s :%4d~%d\n", __stringify(I2C_DEV_ISL68127), I2C_DEV_ISL68127, I2C_DEV_ISL68127_BUTT - 1);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    %-25s :%4d~%d\n", __stringify(I2C_DEV_ADM1166), I2C_DEV_ADM1166, I2C_DEV_ADM1166_BUTT - 1);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    %-25s :%4d~%d\n", __stringify(I2C_DEV_TPS53659), I2C_DEV_TPS53659, I2C_DEV_TPS53659_BUTT - 1);
    len += scnprintf(buf + len, PAGE_SIZE - len, "    %-25s :%4d~%d\n", __stringify(I2C_DEV_TPS53622), I2C_DEV_TPS53622, I2C_DEV_TPS53622_BUTT - 1);


    len += scnprintf(buf + len, PAGE_SIZE - len, "\nI2C Device address:\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    EEPROM     : 0x%x\n", bd->i2c_addr_eeprom);
    for (i = 0; i < bd->lm75_num; i++)
        len += scnprintf(buf + len, PAGE_SIZE - len, "    LM75(%d)    : 0x%x\n", i + 1, bd->i2c_addr_lm75[i]);
    for (i = 0; i < bd->max6696_num; i++)
        len += scnprintf(buf + len, PAGE_SIZE - len, "    MAX6696(%d) : 0x%x\n", i + 1, bd->i2c_addr_max6696[i]);
    for (i = 0; i < bd->psu_num; i++)
    {
        len += scnprintf(buf + len, PAGE_SIZE - len, "    PSU(%d)     : 0x%x\n", i + 1, bd->i2c_addr_psu[i]);
        len += scnprintf(buf + len, PAGE_SIZE - len, "    PMBus(%d)   : 0x%x\n", i + 1, bd->i2c_addr_psu_pmbus[i]);
        len += scnprintf(buf + len, PAGE_SIZE - len, "    INA219(%d)  : 0x%x\n", i + 1, bd->i2c_addr_ina219[i]);
    }
    for (i = 0; i < bd->fan_num; i++)
        len += scnprintf(buf + len, PAGE_SIZE - len, "    FAN(%d)      : 0x%x\n", i + 1, bd->i2c_addr_fan[i]);
    for (i = 0; i < bd->isl68127_num; i++)
        len += scnprintf(buf + len, PAGE_SIZE - len, "    ISL68127(%d) : 0x%x\n", i + 1, bd->i2c_addr_isl68127[i]);
    for (i = 0; i < bd->adm1166_num; i++)
        len += scnprintf(buf + len, PAGE_SIZE - len, "    ADM1166(%d)  : 0x%x\n", i + 1, bd->i2c_addr_adm1166[i]);
    for (i = 0; i < bd->tps53659_num; i++)
        len += scnprintf(buf + len, PAGE_SIZE - len, "    tps53659(%d) : 0x%x\n", i + 1, bd->i2c_addr_tps53659);
    for (i = 0; i < bd->tps53622_num; i++)
        len += scnprintf(buf + len, PAGE_SIZE - len, "    tps53622(%d) : 0x%x\n", i + 1, bd->i2c_addr_tps53622);
    for (i = 0; i < bd->ra228_num; i++)
        len += scnprintf(buf + len, PAGE_SIZE - len, "    ra228(%d)    : 0x%x\n", i + 1, bd->i2c_addr_ra228);
    return len;
}

ssize_t bsp_sysfs_i2c_debug_op_param_set(struct kobject *kobjs, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int temp_dev_id = 0;
    //int temp_is_write = 0;
    int temp_i2c_addr = 0;
    int temp_inner_addr = 0;
    int temp_read_len = 0;
    int temp_write_value = 0;
    int temp_rw_mode = 0;

    if (sscanf(buf, "path 0x%x addr 0x%x read from 0x%x len 0x%x mode 0x%x", &temp_dev_id, &temp_i2c_addr, &temp_inner_addr, &temp_read_len, &temp_rw_mode) == 5)
    {
        i2c_debug_info_read.dev_path_id = temp_dev_id;
        i2c_debug_info_read.is_write    = 0;
        i2c_debug_info_read.i2c_addr    = temp_i2c_addr;
        i2c_debug_info_read.inner_addr  = temp_inner_addr;
        i2c_debug_info_read.read_len    = temp_read_len;
        i2c_debug_info_read.rw_mode     = temp_rw_mode;
        i2c_debug_info_read.is_valid = 1;
    }
    else if (sscanf(buf, "path 0x%x addr 0x%x write inner 0x%x value 0x%x mode 0x%x", &temp_dev_id, &temp_i2c_addr, &temp_inner_addr, &temp_write_value, &temp_rw_mode) == 5)
    {
        i2c_debug_info_write.dev_path_id = temp_dev_id;
        i2c_debug_info_write.is_write    = 1;
        i2c_debug_info_write.i2c_addr    = temp_i2c_addr;
        i2c_debug_info_write.inner_addr  = temp_inner_addr;
        i2c_debug_info_write.write_value = temp_write_value;
        i2c_debug_info_write.rw_mode     = temp_rw_mode;
        i2c_debug_info_write.is_valid = 1;
    }
    else
    {
        DBG_ECHO(DEBUG_ERR, "given: '%s'", buf);
        DBG_ECHO(DEBUG_ERR, "format: 'path 0x1 addr 0x50 read from 0x0 len 0x10 mode 0x1', all number must be hex");
        DBG_ECHO(DEBUG_ERR, "format: 'path 0x1 addr 0x50 write inner 0x0 value 0x1 mode 0x1', all number must be hex");
        count = -EINVAL;
    }

    return count;
}


ssize_t bsp_sysfs_debug_i2c_diag(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    ssize_t len = 0;
    int i = 0;
    struct rtc_time tm = {0};

    u64 timezone_sec_diff = (u64)sys_tz.tz_minuteswest * 60;

    len += scnprintf(buf + len, PAGE_SIZE - len, "i2c diag record count %d/%d, current index %d\n", i2c_diag_info.rec_count, MAX_I2C_DIAG_RECORD_COUNT, i2c_diag_info.curr_index);
    len += scnprintf(buf + len, PAGE_SIZE - len, "use 'dmesg' to check detail\n");

    printk(KERN_DEBUG"rv    : return value\n");
    printk(KERN_DEBUG"R/W   : Read or Write\n");
    printk(KERN_DEBUG"retry : retry times\n");
    printk(KERN_DEBUG"path  : I2C path id\n");
    printk(KERN_DEBUG"addr  : I2C device address\n");
    printk(KERN_DEBUG"pro   : protocol\n");
    printk(KERN_DEBUG"iaddr : I2C device inner address/command code\n");
    printk(KERN_DEBUG"time  : UTC time, need to diff timezone to convert to local time\n");
    printk(KERN_DEBUG"index  rv R/W retry path addr pro iaddr  time\n");

    //        [0000] -6  R  3     3    0x50 0x8 0x0000 2016/11/19 13:26:36
    for (i = 0; i < i2c_diag_info.rec_count; i++)
    {
        if (i2c_diag_info.is_valid)
        {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0)
            rtc_time_to_tm(i2c_diag_info.record[i].time_sec - timezone_sec_diff, &tm);
#else
            rtc_time64_to_tm(i2c_diag_info.record[i].time_sec - timezone_sec_diff, &tm);
#endif

            printk(KERN_DEBUG"[%04d]%3d  %s  %d   %3d    0x%2x 0x%x 0x%04x %04d/%02d/%02d %02d:%02d:%02d\n",
                   i,
                   i2c_diag_info.record[i].error_code,
                   i2c_diag_info.record[i].read_write == I2C_SMBUS_READ ? "R" : (i2c_diag_info.record[i].read_write == I2C_SMBUS_WRITE ? "W" : "?"),
                   i2c_diag_info.record[i].retry_times,
                   i2c_diag_info.record[i].path_id,
                   i2c_diag_info.record[i].i2c_addr,
                   i2c_diag_info.record[i].protocol,
                   i2c_diag_info.record[i].inner_addr,
                   tm.tm_year + 1900,
                   tm.tm_mon + 1,
                   tm.tm_mday,
                   tm.tm_hour,
                   tm.tm_min,
                   tm.tm_sec
                  );
        }
    }

    return len;
}
ssize_t bsp_sysfs_psutry_count_get(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    ssize_t len = scnprintf(buf, PAGE_SIZE, "%d\n", g_psuretry_count);
    return len;
}
ssize_t bsp_sysfs_i2crst_time_get(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    ssize_t len = scnprintf(buf, PAGE_SIZE, "%d\n", g_i2crst_time);
    return len;
}

ssize_t bsp_sysfs_i2c_select_time_get(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    ssize_t len = scnprintf(buf, PAGE_SIZE, "%d\n", g_i2c_select_time);
    return len;
}

ssize_t bsp_sysfs_debug_log_get(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    ssize_t len = 0;
    int i = 0;
    len += scnprintf(buf + len, PAGE_SIZE - len, "recent log config size %d\n", BSP_LOG_FILETER_RECENT_LOG_COUNT);
    len += scnprintf(buf + len, PAGE_SIZE - len, "current log index %d\n", bsp_recent_log.curr_record_index);

    len += scnprintf(buf + len, PAGE_SIZE - len, "* 'echo 0 > recent_log' to clear the records\n");

    mutex_lock(&bsp_recent_log_lock);
    for (i = 0; i < BSP_LOG_FILETER_RECENT_LOG_COUNT; i++)
    {
        len += scnprintf(buf + len, PAGE_SIZE - len, "[%02d] used=%d hit=%06d line=%05d file=%s\n",
                         i,
                         bsp_recent_log.used[i],
                         bsp_recent_log.hit_count[i],
                         bsp_recent_log.line_no[i],
                         (bsp_recent_log.filename[i][0] == 0) ? "none" : bsp_recent_log.filename[i]
                        );
    }

    mutex_unlock(&bsp_recent_log_lock);

    return len;
}

ssize_t bsp_sysfs_psutry_count_set(struct kobject *kobjs, struct kobj_attribute *attr, const char *buf, size_t count)
{
    sscanf(buf, "%d", &g_psuretry_count);
    return count;
}

ssize_t bsp_sysfs_i2crst_time_set(struct kobject *kobjs, struct kobj_attribute *attr, const char *buf, size_t count)
{
    sscanf(buf, "%d", &g_i2crst_time);
    return count;
}

ssize_t bsp_sysfs_i2c_select_time_set(struct kobject *kobjs, struct kobj_attribute *attr, const char *buf, size_t count)
{
    sscanf(buf, "%d", &g_i2c_select_time);
    return count;
}

ssize_t bsp_sysfs_i2c_debug_log_set(struct kobject *kobjs, struct kobj_attribute *attr, const char *buf, size_t count)
{
    mutex_lock(&bsp_recent_log_lock);

    memset(&bsp_recent_log, 0, sizeof(bsp_recent_log));

    mutex_unlock(&bsp_recent_log_lock);
    return count;
}

ssize_t bsp_sysfs_debug_private_log_get(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    ssize_t len = 0;

    len += scnprintf(buf + len, PAGE_SIZE - len, "private log switch is %s\n", log_to_private_file == TRUE ? "ON" : "OFF");
    len += scnprintf(buf + len, PAGE_SIZE - len, "current logfile is %s\n", curr_h3c_log_file);
    len += scnprintf(buf + len, PAGE_SIZE - len, "echo 0 > private_log is turn off switch\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "echo 1 > private_log is turn on  switch\n");

    return len;
}

ssize_t bsp_sysfs_i2c_debug_private_log_set(struct kobject *kobjs, struct kobj_attribute *attr, const char *buf, size_t count)
{

    int on_off = 0;
    if (sscanf(buf, "%d", &on_off) == 1)
    {
        if (on_off == 1)
        {
            bsp_h3c_open_init_log();
        }
        else
        {
            bsp_h3c_close_init_log();
        }
    }
    else
    {
        DBG_ECHO(DEBUG_ERR, "format error for %s", buf);
        count = -EINVAL;
    }

    return count;
}

ssize_t bsp_sysfs_debug_dmesg_filter_get(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    ssize_t len = 0;

    len += scnprintf(buf + len, PAGE_SIZE - len, "dmesg filter is %s\n", log_filter_to_dmesg == TRUE ? "ON" : "OFF");
    len += scnprintf(buf + len, PAGE_SIZE - len, "echo 0 > dmesg_filter is turn off filter\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "echo 1 > dmesg_filter is turn on  filter\n");

    return len;

}

ssize_t bsp_sysfs_debug_dmesg_filter_set(struct kobject *kobjs, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int on_off = 0;
    if (sscanf(buf, "%d", &on_off) == 1)
    {
        if (on_off == 1)
        {
            log_filter_to_dmesg = TRUE;
        }
        else
        {
            log_filter_to_dmesg = FALSE;
        }
    }
    else
    {
        count = -EINVAL;
        DBG_ECHO(DEBUG_ERR, "format error for %s", buf);
    }

    return count;
}

ssize_t bsp_sysfs_dmesg_loglevel_get(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    ssize_t len = 0;

    len += scnprintf(buf + len, PAGE_SIZE - len, "dmesg loglevel is %d\n", bsp_dmesg_log_level);
    len += scnprintf(buf + len, PAGE_SIZE - len, "DEBUG(%d) INFO(%d) ERROR(%d)\n", DEBUG_DBG, DEBUG_INFO, DEBUG_ERR);
    len += scnprintf(buf + len, PAGE_SIZE - len, "echo %d > dmesg_loglevel is set level to ERROR\n", DEBUG_ERR);
    len += scnprintf(buf + len, PAGE_SIZE - len, "echo %d > dmesg_loglevel is set level to ERROR and INFO\n", DEBUG_ERR | DEBUG_INFO);

    return len;

}

ssize_t bsp_sysfs_dmesg_loglevel_set(struct kobject *kobjs, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int debug_level = 0;
    if (1 == (sscanf(buf, "%x", &debug_level)))
    {
        bsp_dmesg_log_level = debug_level;

    }
    else
    {
        DBG_ECHO(DEBUG_ERR, "format error for %s", buf);
        count = -EINVAL;
    }

    return count;
}

ssize_t bsp_sysfs_reset_smbus_slave_get(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    ssize_t len = 0;

    len += scnprintf(buf + len, PAGE_SIZE - len, "reset smbus i2c slave\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "usage:\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    echo 'path_id' > reset_slave\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    path_id is the i2c path id for the i2c slave where can be seen from i2c_read.py \n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "example: \n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    echo %d > reset_slave (reset)\n", I2C_DEV_PSU);
    return len;

}


ssize_t bsp_sysfs_reset_smbus_slave_set(struct kobject *kobjs, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int i2c_device_id = 0;
    if (sscanf(buf, "%d", &i2c_device_id) == 1)
    {
        if (i2c_device_id < I2C_DEV_OPTIC_IDX_START || i2c_device_id >= I2C_DEV_BUTT)
        {
            DBG_ECHO(DEBUG_INFO, "param err: i2c device %d is not in [%d, %d)", i2c_device_id, I2C_DEV_OPTIC_IDX_START, I2C_DEV_BUTT);
            count = -EINVAL;
        }
        else
        {
            if (bsp_reset_smbus_slave(i2c_device_id) != ERROR_SUCCESS)
            {
                DBG_ECHO(DEBUG_ERR, "reset smbus slave %d failed", i2c_device_id);
                count = -EINVAL;
            }
        }
    }
    else
    {
        DBG_ECHO(DEBUG_INFO, "format error for %s", buf);
        count = -EINVAL;
    }

    return count;
}

ssize_t bsp_sysfs_memory_operation_get(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    ssize_t len = 0;

    len += scnprintf(buf + len, PAGE_SIZE - len, "kernel memory peak/set\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "usage:\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "read <n> times byte/short/word from address <addr>\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    echo r.b:<addr>.<n> > mem_op  #read n bytes\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    echo r.h:<addr>.<n> > mem_op  #read n shorts\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    echo r.w:<addr>.<n> > mem_op  #read n words\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    (not support) echo r.<symbol>.<n> > mem_op  #read n words from symbol address\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "write a byte/short/word at address <addr> value <n>s\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    echo w.b:<addr>.<n> > mem_op  #write a byte\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    echo w.h:<addr>.<n> > mem_op  #write a short\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    echo w.w:<addr>.<n> > mem_op  #write a word\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "    echo w.w:<addr>.<n> > mem_op  #write a word\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "<addr> and <n> is hex, prefix starts with '0x' see the result in dmesg\n");

    return len;
}

ssize_t bsp_sysfs_memory_operation_set(struct kobject *kobjs, struct kobj_attribute *attr, const char *buf, size_t count)
{
    char mode = 0;
    char unit = 0;
    unsigned long addr = 0;
    u8  *baddr = NULL;
    u16 *haddr = NULL;
    u32 *waddr = NULL;
    int word_count = 0;
    int i = 0;

    if (bsp_module_debug_level[BSP_BASE_MODULE] != 0x5b7f)
    {
        printk(KERN_INFO"not support for %s.", buf);
        count = -ENOSYS;
        return count;
    }

    if (sscanf(buf, "%c.%c:0x%lx.0x%x", &mode, &unit, &addr, &word_count) != 4)
    {
        printk(KERN_INFO"format error for %s.", buf);
        count = -EINVAL;
    }
    else
    {
        baddr = (u8 *)addr;
        haddr = (u16 *)addr;
        waddr = (u32 *)addr;

        if (mode == 'r')
        {
            printk(KERN_DEBUG"kernel memory read:\n");
            for (i = 0; i < word_count; i ++)
            {
                if (unit == 'b')
                {
                    printk(KERN_DEBUG"    %p:%02x\n", baddr + i, *(baddr + i));
                }
                else if (unit == 'h')
                {
                    printk(KERN_DEBUG"    %p:%04x\n", haddr + i, *(haddr + i));
                }
                else if (unit == 'w')
                {
                    printk(KERN_DEBUG"    %p:%08x\n", waddr + i, *(waddr + i));
                }
                else
                {
                    printk(KERN_DEBUG"unknown unit %c\n", unit);
                    count = -EINVAL;
                    break;
                }
            }
        }
        else if (mode == 'w')
        {
            printk(KERN_INFO"kernel memory writed:");
            if (unit == 'b')
            {
                *(baddr) = word_count;
                printk(KERN_DEBUG"    %p:%x\n", baddr, *(baddr));
            }
            else if (unit == 'h')
            {
                *(haddr) = word_count;
                printk(KERN_DEBUG"    %p:%x\n", haddr, *(haddr));
            }
            else if (unit == 'w')
            {
                *(waddr) = word_count;
                printk(KERN_DEBUG"    %p:%x\n", waddr, *(waddr));
            }
            else
            {
                count = -EINVAL;
                printk(KERN_DEBUG"unknown unit %c\n", unit);
            }

        }
        else
        {
            count = -EINVAL;
            printk(KERN_DEBUG"unknown mode %c\n", mode);
        }
    }

    return count;
}

ssize_t bsp_sysfs_debug_access_get(struct kobject *kobjs, struct kobj_attribute *ka, char *buf, int node_index)
{
    ssize_t len = 0;

    if ((node_index < BSP_BASE_MODULE) || (node_index >= BSP_MODULE_MAX))
    {
        len = scnprintf(buf, PAGE_SIZE, "Not supported bsp module\n");
    }
    else
    {
        len += scnprintf(buf + len, PAGE_SIZE - len, "The debug information levels of bsp %s are defined as below\n", module_name_string[node_index]);
        len += scnprintf(buf + len, PAGE_SIZE - len, "    INFO : 1\n    ERR  : 2\n    DEBUG: 4\n");
        len += scnprintf(buf + len, PAGE_SIZE - len, "To get debug info, you'd better set loglevel by either of the following methods\n");
        len += scnprintf(buf + len, PAGE_SIZE - len, " 1.through sysfs\n");
        len += scnprintf(buf + len, PAGE_SIZE - len, "  example: echo 7 > /sys/switch/%s/loglevel turn on all.\n", module_name_string[node_index]);
        len += scnprintf(buf + len, PAGE_SIZE - len, "  example: echo 0 > /sys/switch/%s/loglevel turn off all.\n", module_name_string[node_index]);
        len += scnprintf(buf + len, PAGE_SIZE - len, " 2.through module param\n");
        if (BSP_XCVR_MODULE == node_index)
        {
            len += scnprintf(buf + len, PAGE_SIZE - len, "  example: echo 2 > /sys/module/xcvr/parameters/xcvr_debug_level turn ERR on.\n");
        }
        else if (BSP_FPGA_MODULE == node_index)
        {
            len += scnprintf(buf + len, PAGE_SIZE - len, "  example: echo 2 > /sys/module/dom/parameters/dom_debug_level turn ERR on.\n");
        }
        else if (BSP_WATCHDOG_MODULE == node_index)
        {
            len += scnprintf(buf + len, PAGE_SIZE - len, "  example: echo 2 > /sys/module/hardware_watchdog/parameters/watchdog_debug_level turn ERR on.\n");
        }
        else if (BSP_BASE_MODULE == node_index)
        {
            len += scnprintf(buf + len, PAGE_SIZE - len, "  example: echo 2 > /sys/module/bsp_base/parameters/bsp_debug_level turn ERR on.\n");
        }
        else
        {
            len += scnprintf(buf + len, PAGE_SIZE - len, "  example: echo 2 > /sys/module/%s/parameters/%s_debug_level turn ERR on.\n", module_name_string[node_index], module_name_string[node_index]);
        }
        len += scnprintf(buf + len, PAGE_SIZE - len, "then you may use dmesg to get the related debug info.\n");
    }
    return len;
}

ssize_t bsp_sysfs_debug_access_set(struct kobject *kobjs, struct kobj_attribute *ka, const char *buf, size_t count, int node_index)
{
    return -ENOSYS;
}

ssize_t bsp_sysfs_debug_loglevel_get(struct kobject *kobjs, struct kobj_attribute *ka, char *buf, int node_index)
{
    ssize_t len = 0;
    int debug_level = 0;

    if ((node_index < BSP_BASE_MODULE) || (node_index >= BSP_MODULE_MAX))
    {
        len = scnprintf(buf, PAGE_SIZE, "Not supported bsp module\n");
    }
    else
    {
        debug_level = bsp_module_debug_level[node_index] & 0x7;
        len += scnprintf(buf + len, PAGE_SIZE - len, "Current setting for bsp %s debug information level is %d:\n", module_name_string[node_index], debug_level);
        len += scnprintf(buf + len, PAGE_SIZE - len, "    INFO (1): %s\n", debug_level & 0x1 ? "on" : "off");
        len += scnprintf(buf + len, PAGE_SIZE - len, "    ERR  (2): %s\n", debug_level & 0x2 ? "on" : "off");
        len += scnprintf(buf + len, PAGE_SIZE - len, "    DEBUG(4): %s\n", debug_level & 0x4 ? "on" : "off");
        len += scnprintf(buf + len, PAGE_SIZE - len, "example: echo 7 > loglevel turn on all.\n");
    }

    return len;
}

ssize_t bsp_sysfs_debug_loglevel_set(struct kobject *kobjs, struct kobj_attribute *ka, const char *buf, size_t count, int node_index)
{
    int debug_level = 0;

    if ((node_index < BSP_BASE_MODULE) || (node_index >= BSP_MODULE_MAX))
    {
        DBG_ECHO(DEBUG_INFO, "attribute %s -> %s not support", kobjs->name, ka->attr.name);
        count = -EINVAL;
    }
    else
    {
        if (1 == (sscanf(buf, "%x", &debug_level)))
        {
            bsp_module_debug_level[node_index] = debug_level;
        }
        else
        {
            DBG_ECHO(DEBUG_ERR, "set debug loglevel format error for %s", buf);
            count = -EINVAL;
        }
    }
    return count;
}

ssize_t bsp_sysfs_show_rtc(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    size_t index = 0;
    ULONG ulRet = 0;
    DRV_RTC_TIME_S stRtcTm;
    memset(&stRtcTm, 0, (INT)sizeof(DRV_RTC_TIME_S));
    ulRet = drv_rtc_GetDs1337Time(&stRtcTm);

    if (ulRet != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "drv_rtc_GetDs1337Time ret = %lx\r\n", ulRet);
    }

    index = sprintf(buf, "%ld-%ld-%ld %ld:%ld:%ld\r\n", stRtcTm.iYear, stRtcTm.iMonth, stRtcTm.iDate,
                    stRtcTm.iHour, stRtcTm.iMinute, stRtcTm.iSecond);
    return index;
}

static ssize_t bsp_sysfs_set_rtc(struct kobject *kobjs, struct kobj_attribute *attr, const char *buf, size_t count)
{
    ULONG ulRet = 0;
    DRV_RTC_TIME_S stRtcTm;
    memset(&stRtcTm, 0, (INT)sizeof(DRV_RTC_TIME_S));

    if (sscanf(buf, "%ld-%ld-%ld %ld:%ld:%ld", &stRtcTm.iYear, &stRtcTm.iMonth, &stRtcTm.iDate, &stRtcTm.iHour, &stRtcTm.iMinute, &stRtcTm.iSecond) < 6)
    {
        DBG_ECHO(DEBUG_ERR, "Invalid format '%s'\n", buf);
        return count;
    }
    ulRet = drv_rtc_SetDs1337Time(&stRtcTm);
    if (ulRet != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "drv_rtc_SetCurrentTime ret = %lx\r\n", ulRet);
    }
    return count;
}


SYSFS_RO_ATTR_DEF(i2c_select_table, bsp_sysfs_print_i2c_select_table);
SYSFS_RO_ATTR_DEF(mem_dump, bsp_sysfs_debug_dump_i2c_mem);
SYSFS_RO_ATTR_DEF(do_write, bsp_sysfs_debug_i2c_do_write);
SYSFS_RO_ATTR_DEF(diag_info, bsp_sysfs_debug_i2c_diag);
SYSFS_RW_ATTR_DEF(param, bsp_sysfs_i2c_debug_op_param_get, bsp_sysfs_i2c_debug_op_param_set);
//SYSFS_RW_ATTR_DEF(debug_level, bsp_sysfs_debug_level_get, bsp_sysfs_i2c_debug_level_set);
SYSFS_RW_ATTR_DEF(recent_log, bsp_sysfs_debug_log_get, bsp_sysfs_i2c_debug_log_set);
SYSFS_RW_ATTR_DEF(private_log, bsp_sysfs_debug_private_log_get, bsp_sysfs_i2c_debug_private_log_set);
SYSFS_RW_ATTR_DEF(dmesg_filter, bsp_sysfs_debug_dmesg_filter_get, bsp_sysfs_debug_dmesg_filter_set);
SYSFS_RW_ATTR_DEF(dmesg_loglevel, bsp_sysfs_dmesg_loglevel_get, bsp_sysfs_dmesg_loglevel_set);
SYSFS_RW_ATTR_DEF(reset_slave, bsp_sysfs_reset_smbus_slave_get, bsp_sysfs_reset_smbus_slave_set);
SYSFS_RW_ATTR_DEF(mem_op, bsp_sysfs_memory_operation_get, bsp_sysfs_memory_operation_set);
SYSFS_RW_ATTR_DEF(rtc, bsp_sysfs_show_rtc, bsp_sysfs_set_rtc);
SYSFS_RW_ATTR_DEF(i2crst_time, bsp_sysfs_i2crst_time_get, bsp_sysfs_i2crst_time_set);
SYSFS_RW_ATTR_DEF(i2c_select_time, bsp_sysfs_i2c_select_time_get, bsp_sysfs_i2c_select_time_set);
SYSFS_RW_ATTR_DEF(psuretry_count, bsp_sysfs_psutry_count_get, bsp_sysfs_psutry_count_set);

BSPMODULE_DEBUG_ATTR_DEF(debug, BSP_BASE_MODULE);
BSPMODULE_DEBUG_RW_ATTR_DEF(loglevel, BSP_BASE_MODULE);

static struct attribute *bspbase_debug_i2c_attributes[] =
{
    &i2c_select_table.attr,
    &mem_dump.attr,
    &param.attr,
    &do_write.attr,
    //&debug_level.attr,
    &diag_info.attr,
    &reset_slave.attr,
    &i2crst_time.attr,
    &i2c_select_time.attr,
    &psuretry_count.attr,
    NULL
};

static const struct attribute_group bspbase_debug_i2c_attributes_group =
{
    .attrs = bspbase_debug_i2c_attributes,
};

static struct attribute *bspbase_debug_attributes[] =
{
    &bspmodule_debug.attr,
    &bspmodule_loglevel.attr,
    &recent_log.attr,
    &private_log.attr,
    &dmesg_filter.attr,
    &dmesg_loglevel.attr,
    &mem_op.attr,
    &rtc.attr,
    NULL
};

static const struct attribute_group bspbase_debug_attr_group =
{
    .attrs = bspbase_debug_attributes,
};


int  bsp_sysfs_init(void)
{
    int ret = ERROR_SUCCESS;

    kobj_switch = kobject_create_and_add("switch", kernel_kobj->parent);
    CHECK_IF_NULL_GOTO_EXIT(-ENOMEM, ret, kobj_switch, "kobj_switch create falled!");

    kobj_debug = kobject_create_and_add("debug", kobj_switch);
    CHECK_IF_NULL_GOTO_EXIT(-ENOMEM, ret, kobj_debug, "kobj_debug create falled!");

    kobj_debug_i2c = kobject_create_and_add("i2c", kobj_debug);
    CHECK_IF_NULL_GOTO_EXIT(-ENOMEM, ret, kobj_debug_i2c, "create kobj_debug_i2c failed!");

    ret = sysfs_create_group(kobj_debug, &bspbase_debug_attr_group);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "create debug group failed");

    ret = sysfs_create_group(kobj_debug_i2c, &bspbase_debug_i2c_attributes_group);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "create debug/i2c group failed!");
exit:
    return ret;
}


void bsp_sysfs_release_kobjs(void)
{
    if (kobj_debug_i2c != NULL)
    {
        sysfs_remove_group(kobj_debug_i2c, &bspbase_debug_i2c_attributes_group);
        kobject_put(kobj_debug_i2c);
    }
    if (kobj_debug != NULL)
    {
        sysfs_remove_group(kobj_debug, &bspbase_debug_attr_group);
        kobject_put(kobj_debug);
    }
    if (kobj_switch != NULL)
    {
        kobject_put(kobj_switch);
    }
    return;
}

static int __init bsp_init(void)
{
    int ret = ERROR_SUCCESS;

    INIT_PRINT("module init started");
    ret = bsp_base_init();
    CHECK_IF_ERROR_GOTO_EXIT(ret, "bsp_base_init failed!");
    ret = bsp_sysfs_init();
    CHECK_IF_ERROR_GOTO_EXIT(ret, "bsp sysfs init failed!");
    INIT_PRINT("module init successed!");

exit:
    return ret;
}

static void __exit bsp_exit(void)
{
    i2c_deinit();
    bsp_cpld_deinit();

    bsp_sysfs_release_kobjs();
    INIT_PRINT("module uninstalled!");
    return;
}


module_init(bsp_init);
module_exit(bsp_exit);
MODULE_AUTHOR("Wang Xue <wang.xue@h3c.com>");
MODULE_DESCRIPTION("h3c system cpld driver");
MODULE_LICENSE("Dual BSD/GPL");

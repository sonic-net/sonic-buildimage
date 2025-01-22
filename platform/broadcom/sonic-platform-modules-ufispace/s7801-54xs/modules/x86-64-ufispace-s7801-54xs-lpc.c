/*
 * A lpc driver for the ufispace_s7801_54xs
 *
 * Copyright (C) 2017-2022 UfiSpace Technology Corporation.
 * Jason Tsai <jason.cy.tsai@ufispace.com>
 *
 * Based on ad7414.c
 * Copyright 2006 Stefan Roese <sr at denx.de>, DENX Software Engineering
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
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/hwmon-sysfs.h>
#include <linux/gpio.h>

#define BSP_LOG_R(fmt, args...) \
    _bsp_log (LOG_READ, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#define BSP_LOG_W(fmt, args...) \
    _bsp_log (LOG_WRITE, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#define BSP_PR(level, fmt, args...) _bsp_log (LOG_SYS, level "[BSP]" fmt "\r\n", ##args)

#define _SENSOR_DEVICE_ATTR_RO(_name, _func, _index)     \
    SENSOR_DEVICE_ATTR(_name, S_IRUGO, read_##_func, NULL, _index)

#define _SENSOR_DEVICE_ATTR_WO(_name, _func, _index)     \
    SENSOR_DEVICE_ATTR(_name, S_IWUSR, NULL, write_##_func, _index)

#define _SENSOR_DEVICE_ATTR_RW(_name, _func, _index)     \
    SENSOR_DEVICE_ATTR(_name, S_IRUGO | S_IWUSR, read_##_func, write_##_func, _index)

#define _DEVICE_ATTR(_name)     \
    &sensor_dev_attr_##_name.dev_attr.attr

#define BSP_PR(level, fmt, args...) _bsp_log (LOG_SYS, level "[BSP]" fmt "\r\n", ##args)

#define DRIVER_NAME "x86_64_ufispace_s7801_54xs_lpc"

/* LPC registers */

#define REG_BASE_MB                       0x700
#define REG_BASE_EC                       0xE300

#define REG_NONE                          0x00
//MB CPLD
#define REG_BRD_ID_0                      (REG_BASE_MB + 0x00)
#define REG_BRD_ID_1                      (REG_BASE_MB + 0x01)
#define REG_CPLD_VERSION                  (REG_BASE_MB + 0x02)
#define REG_CPLD_ID                       (REG_BASE_MB + 0x03)
#define REG_CPLD_BUILD                    (REG_BASE_MB + 0x04)
#define REG_CPLD_CHIP                     (REG_BASE_MB + 0x05)
#define REG_BRD_EXT_ID                    (REG_BASE_MB + 0x06)
#define REG_I2C_MUX_RESET                 (REG_BASE_MB + 0x46)
#define REG_I2C_MUX_RESET_2               (REG_BASE_MB + 0x47)
#define REG_MUX_CTRL                      (REG_BASE_MB + 0x5C)
#define REG_MISC_CTRL                     (REG_BASE_MB + 0x5D)
#define REG_MISC_CTRL_2                   (REG_BASE_MB + 0x5E)

//EC
#define REG_BIOS_BOOT                     (REG_BASE_EC + 0x0C)
#define REG_CPU_REV                       (REG_BASE_EC + 0x17)

// BMC mailbox
#define REG_TEMP_MAC_HWM                  (REG_BASE_MB + 0xC0)

//MASK
#define MASK_ALL                          (0xFF)
#define MASK_CPLD_MAJOR_VER               (0b11000000)
#define MASK_CPLD_MINOR_VER               (0b00111111)
#define MASK_HW_ID                        (0b00000011)
#define MASK_DEPH_ID                      (0b00000100)
#define MASK_BUILD_ID                     (0b00011000)
#define MASK_EXT_ID                       (0b00000111)
#define MASK_MUX_RESET_ALL                (0x37) // 2#00110111
#define MASK_MUX_RESET                    (MASK_ALL)
#define MASK_BIOS_BOOT_ROM                (0b01000000)

#define LPC_MDELAY                        (5)
#define MDELAY_RESET_INTERVAL             (100)
#define MDELAY_RESET_FINISH               (500)

/* LPC sysfs attributes index  */
enum lpc_sysfs_attributes {
    //MB CPLD
    ATT_BRD_ID_0,
    ATT_BRD_ID_1,
    ATT_BRD_SKU_ID,
    ATT_BRD_HW_ID,
    ATT_BRD_DEPH_ID,
    ATT_BRD_BUILD_ID,
    ATT_BRD_EXT_ID,

    ATT_CPLD_ID,
    ATT_CPLD_BUILD,
    ATT_CPLD_CHIP,

    ATT_CPLD_VERSION_MAJOR,
    ATT_CPLD_VERSION_MINOR,
    ATT_CPLD_VERSION_BUILD,
    ATT_CPLD_VERSION_H,

    ATT_MUX_RESET,
    ATT_MUX_CTRL,

    //EC
    ATT_CPU_HW_ID,
    ATT_CPU_DEPH_ID,
    ATT_CPU_BUILD_ID,
    ATT_BIOS_BOOT_ROM,
    //BMC mailbox
    ATT_TEMP_MAC_HWM,

    //BSP
    ATT_BSP_VERSION,
    ATT_BSP_DEBUG,
    ATT_BSP_PR_INFO,
    ATT_BSP_PR_ERR,
    ATT_BSP_REG,
    ATT_BSP_GPIO_MAX,
    ATT_MAX
};

enum data_type {
    DATA_HEX,
    DATA_DEC,
    DATA_S_DEC,
    DATA_UNK,
};

enum bsp_log_types {
    LOG_NONE,
    LOG_RW,
    LOG_READ,
    LOG_WRITE,
    LOG_SYS
};

enum bsp_log_ctrl {
    LOG_DISABLE,
    LOG_ENABLE
};

struct lpc_data_s {
    struct mutex    access_lock;
};

typedef struct sysfs_info_s
{
    u16 reg;
    u8 mask;
    u8 data_type;
} sysfs_info_t;

static sysfs_info_t sysfs_info[] = {
    [ATT_BRD_ID_0]     = {REG_BRD_ID_0,   MASK_ALL,      DATA_HEX},
    [ATT_BRD_ID_1]     = {REG_BRD_ID_1,   MASK_ALL,      DATA_HEX},
    [ATT_BRD_SKU_ID]   = {REG_BRD_ID_0,   MASK_ALL,      DATA_DEC},
    [ATT_BRD_HW_ID]    = {REG_BRD_ID_1,   MASK_HW_ID,    DATA_DEC},
    [ATT_BRD_DEPH_ID]  = {REG_BRD_ID_1,   MASK_DEPH_ID,  DATA_DEC},
    [ATT_BRD_BUILD_ID] = {REG_BRD_ID_1,   MASK_BUILD_ID, DATA_DEC},
    [ATT_BRD_EXT_ID]   = {REG_BRD_EXT_ID, MASK_EXT_ID,   DATA_DEC},

    [ATT_CPLD_ID]      = {REG_CPLD_ID,     MASK_ALL, DATA_DEC},
    [ATT_CPLD_BUILD]   = {REG_CPLD_BUILD,  MASK_ALL, DATA_DEC},
    [ATT_CPLD_CHIP]    = {REG_CPLD_CHIP,   MASK_ALL, DATA_DEC},

    [ATT_CPLD_VERSION_MAJOR] = {REG_CPLD_VERSION, MASK_CPLD_MAJOR_VER, DATA_DEC},
    [ATT_CPLD_VERSION_MINOR] = {REG_CPLD_VERSION, MASK_CPLD_MINOR_VER, DATA_DEC},
    [ATT_CPLD_VERSION_BUILD] = {REG_CPLD_BUILD,   MASK_ALL,            DATA_DEC},
    [ATT_CPLD_VERSION_H]     = {REG_CPLD_VERSION, MASK_ALL,            DATA_UNK},

    [ATT_MUX_RESET] = {REG_NONE,    MASK_ALL,  DATA_DEC},
    [ATT_MUX_CTRL]  = {REG_MUX_CTRL, MASK_ALL, DATA_HEX},

    //EC
    [ATT_CPU_HW_ID]     = {REG_CPU_REV,   MASK_HW_ID,         DATA_DEC},
    [ATT_CPU_DEPH_ID]   = {REG_CPU_REV,   MASK_DEPH_ID,       DATA_DEC},
    [ATT_CPU_BUILD_ID]  = {REG_CPU_REV,   MASK_BUILD_ID,      DATA_DEC},
    [ATT_BIOS_BOOT_ROM] = {REG_BIOS_BOOT, MASK_BIOS_BOOT_ROM, DATA_DEC},

    //BMC mailbox
    [ATT_TEMP_MAC_HWM] = {REG_TEMP_MAC_HWM , MASK_ALL, DATA_S_DEC},

    //BSP
    [ATT_BSP_VERSION] = {REG_NONE, MASK_ALL, DATA_UNK},
    [ATT_BSP_DEBUG]   = {REG_NONE, MASK_ALL, DATA_UNK},
    [ATT_BSP_PR_INFO] = {REG_NONE, MASK_ALL, DATA_UNK},
    [ATT_BSP_PR_ERR]  = {REG_NONE, MASK_ALL, DATA_UNK},
    [ATT_BSP_REG]     = {REG_NONE, MASK_ALL, DATA_HEX},
};

struct lpc_data_s *lpc_data;
char bsp_version[16]="";
char bsp_debug[2]="0";
char bsp_reg[8]="0x0";
u8 enable_log_read  = LOG_DISABLE;
u8 enable_log_write = LOG_DISABLE;
u8 enable_log_sys   = LOG_ENABLE;
u8 mailbox_inited=0;

/* reg shift */
static u8 _shift(u8 mask)
{
    int i=0, mask_one=1;

    for(i=0; i<8; ++i) {
        if ((mask & mask_one) == 1)
            return i;
        else
            mask >>= 1;
    }

    return -1;
}

/* reg mask and shift */
static u8 _mask_shift(u8 val, u8 mask)
{
    int shift=0;

    shift = _shift(mask);

    return (val & mask) >> shift;
}

static u8 _parse_data(char *buf, unsigned int data, u8 data_type)
{
    if(buf == NULL) {
        return -1;
    }

    if(data_type == DATA_HEX) {
        return sprintf(buf, "0x%02x", data);
    } else if(data_type == DATA_DEC) {
        return sprintf(buf, "%u", data);
    } else {
        return -1;
    }
    return 0;
}

static int _bsp_log(u8 log_type, char *fmt, ...)
{
    if ((log_type==LOG_READ  && enable_log_read) ||
        (log_type==LOG_WRITE && enable_log_write) ||
        (log_type==LOG_SYS && enable_log_sys) ) {
        va_list args;
        int r;

        va_start(args, fmt);
        r = vprintk(fmt, args);
        va_end(args);

        return r;
    } else {
        return 0;
    }
}

static int _config_bsp_log(u8 log_type)
{
    switch(log_type) {
        case LOG_NONE:
            enable_log_read = LOG_DISABLE;
            enable_log_write = LOG_DISABLE;
            break;
        case LOG_RW:
            enable_log_read = LOG_ENABLE;
            enable_log_write = LOG_ENABLE;
            break;
        case LOG_READ:
            enable_log_read = LOG_ENABLE;
            enable_log_write = LOG_DISABLE;
            break;
        case LOG_WRITE:
            enable_log_read = LOG_DISABLE;
            enable_log_write = LOG_ENABLE;
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static void _outb(u8 data, u16 port)
{
    outb(data, port);
    mdelay(LPC_MDELAY);
}

/* init bmc mailbox, get from BMC team */
static int bmc_mailbox_init(void)
{
    if (mailbox_inited) {
        return mailbox_inited;
    }

    //Enable super io writing
    _outb(0xa5, 0x2e);
    _outb(0xa5, 0x2e);

    //Logic device number
    _outb(0x07, 0x2e);
    _outb(0x0e, 0x2f);

    //Disable mailbox
    _outb(0x30, 0x2e);
    _outb(0x00, 0x2f);

    //Set base address bit
    _outb(0x60, 0x2e);
    _outb(0x07, 0x2f);
    _outb(0x61, 0x2e);
    _outb(0xc0, 0x2f);

    //Select bit[3:0] of SIRQ
    _outb(0x70, 0x2e);
    _outb(0x07, 0x2f);

    //Low level trigger
    _outb(0x71, 0x2e);
    _outb(0x01, 0x2f);

    //Enable mailbox
    _outb(0x30, 0x2e);
    _outb(0x01, 0x2f);

    //Disable super io writing
    _outb(0xaa, 0x2e);

    //Mailbox initial
    _outb(0x00, 0x786);
    _outb(0x00, 0x787);

    //set mailbox_inited
    mailbox_inited = 1;

    return mailbox_inited;
}

/* get lpc register value */
static u8 _read_lpc_reg(u16 reg, u8 mask)
{
    u8 reg_val=0x0, reg_mk_shf_val = 0x0;

    mutex_lock(&lpc_data->access_lock);
    reg_val = inb(reg);
    mutex_unlock(&lpc_data->access_lock);

    reg_mk_shf_val = _mask_shift(reg_val, mask);

    BSP_LOG_R("reg=0x%03x, reg_val=0x%02x, mask=0x%02x, reg_mk_shf_val=0x%02x", reg, reg_val, mask, reg_mk_shf_val);

    return reg_mk_shf_val;
}

/* get lpc register value */
static ssize_t read_lpc_reg(u16 reg, u8 mask, char *buf, u8 data_type)
{
    u8 reg_val;
    int len=0;

    reg_val = _read_lpc_reg(reg, mask);

    // may need to change to hex value ?
    len=_parse_data(buf, reg_val, data_type);

    return len;
}

/* set lpc register value */
static ssize_t write_lpc_reg(u16 reg, u8 mask, const char *buf, size_t count, u8 data_type)
{
    u8 reg_val, reg_val_now, shift;

    if (kstrtou8(buf, 0, &reg_val) < 0) {
        if(data_type == DATA_S_DEC) {
            if (kstrtos8(buf, 0, &reg_val) < 0) {
                return -EINVAL;
            }
        } else {
            return -EINVAL;
        }
    }

    //apply continuous bits operation if mask is specified, discontinuous bits are not supported
    if (mask != MASK_ALL) {
        reg_val_now = _read_lpc_reg(reg, MASK_ALL);
        //clear bits in reg_val_now by the mask
        reg_val_now &= ~mask;
        //get bit shift by the mask
        shift = _shift(mask);
        //calculate new reg_val
        reg_val = reg_val_now | (reg_val << shift);
    }

    mutex_lock(&lpc_data->access_lock);

    _outb(reg_val, reg);

    mutex_unlock(&lpc_data->access_lock);

    BSP_LOG_W("reg=0x%03x, reg_val=0x%02x, mask=0x%02x", reg, reg_val, mask);

    return count;
}

/* get bsp value */
static ssize_t read_bsp(char *buf, char *str)
{
    ssize_t len=0;

    mutex_lock(&lpc_data->access_lock);
    len=sprintf(buf, "%s", str);
    mutex_unlock(&lpc_data->access_lock);

    BSP_LOG_R("reg_val=%s", str);

    return len;
}

/* set bsp value */
static ssize_t write_bsp(const char *buf, char *str, size_t str_len, size_t count)
{
    mutex_lock(&lpc_data->access_lock);
    snprintf(str, str_len, "%s", buf);
    mutex_unlock(&lpc_data->access_lock);

    BSP_LOG_W("reg_val=%s", str);

    return count;
}

/* get gpio max value */
static ssize_t read_gpio_max(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (attr->index == ATT_BSP_GPIO_MAX) {
        return sprintf(buf, "%d\n", ARCH_NR_GPIOS-1);
    }
    return -1;
}

/* get mb cpld version in human readable format */
static ssize_t read_mb_cpld_version_h(struct device *dev,
        struct device_attribute *da, char *buf)
{
    ssize_t len=0;
    u8 major = 0, minor = 0, build = 0;
    major = _read_lpc_reg(REG_CPLD_VERSION, MASK_CPLD_MAJOR_VER);
    minor = _read_lpc_reg(REG_CPLD_VERSION, MASK_CPLD_MINOR_VER);
    build = _read_lpc_reg(REG_CPLD_BUILD, MASK_ALL);
    len=sprintf(buf, "%u.%02u.%03u", major, minor, build);

    return len;
}

/* get lpc register value */
static ssize_t read_lpc_callback(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u16 reg = 0;
    u8 mask = MASK_ALL;
    u8 data_type = DATA_UNK;

    if (attr->index == ATT_BSP_REG) {
        //copy value from bsp_reg
        if (kstrtou16(bsp_reg, 0, &reg) < 0)
            return -EINVAL;

        data_type = sysfs_info[attr->index].data_type;
    } else {
        reg = sysfs_info[attr->index].reg;
        mask = sysfs_info[attr->index].mask;
        data_type = sysfs_info[attr->index].data_type;
    }

    return read_lpc_reg(reg, mask, buf, data_type);
}

/* set lpc register value */
static ssize_t write_lpc_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u16 reg = 0;
    u8 mask = MASK_ALL;
    u8 data_type = DATA_UNK;

    reg = sysfs_info[attr->index].reg;
    mask = sysfs_info[attr->index].mask;
    data_type = sysfs_info[attr->index].data_type;

    if(attr->index == ATT_TEMP_MAC_HWM) {
        bmc_mailbox_init();
    }

    return write_lpc_reg(reg, mask, buf, count, data_type);
}

/* get bsp parameter value */
static ssize_t read_bsp_callback(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    char *str=NULL;

    switch (attr->index) {
        case ATT_BSP_VERSION:
            str = bsp_version;
            break;
        case ATT_BSP_DEBUG:
            str = bsp_debug;
            break;
        case ATT_BSP_REG:
            str = bsp_reg;
            break;
        default:
            return -EINVAL;
    }
    return read_bsp(buf, str);
}

/* set bsp parameter value */
static ssize_t write_bsp_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int str_len=0;
    char *str=NULL;
    u16 reg = 0;
    u8 bsp_debug_u8 = 0;

    switch (attr->index) {
        case ATT_BSP_VERSION:
            str = bsp_version;
            str_len = sizeof(bsp_version);
            break;
        case ATT_BSP_DEBUG:
            str = bsp_debug;
            str_len = sizeof(bsp_debug);
            break;
        case ATT_BSP_REG:
            if (kstrtou16(buf, 0, &reg) < 0)
                return -EINVAL;

            str = bsp_reg;
            str_len = sizeof(bsp_reg);
            break;
        default:
            return -EINVAL;
    }

    if (attr->index == ATT_BSP_DEBUG) {
        if (kstrtou8(buf, 0, &bsp_debug_u8) < 0) {
            return -EINVAL;
        } else if (_config_bsp_log(bsp_debug_u8) < 0) {
            return -EINVAL;
        }
    }

    return write_bsp(buf, str, str_len, count);
}

static ssize_t write_bsp_pr_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int str_len = strlen(buf);

    if(str_len <= 0)
        return str_len;

    switch (attr->index) {
        case ATT_BSP_PR_INFO:
            BSP_PR(KERN_INFO, "%s", buf);
            break;
        case ATT_BSP_PR_ERR:
            BSP_PR(KERN_ERR, "%s", buf);
            break;
        default:
            return -EINVAL;
    }

    return str_len;
}

/* set mux_reset register value */
static ssize_t write_mux_reset(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    u16 reg = REG_I2C_MUX_RESET;
    u8 val = 0;
    u8 mux_reset_reg_val = 0;
    static int mux_reset_flag = 0;

    if (kstrtou8(buf, 0, &val) < 0)
        return -EINVAL;

    if (mux_reset_flag == 0) {
        if (val == 0) {
            mutex_lock(&lpc_data->access_lock);
            mux_reset_flag = 1;
            BSP_LOG_W("i2c mux reset is triggered...");

            //reset mux on SFP/QSFP ports
            mux_reset_reg_val = inb(reg);
            _outb((mux_reset_reg_val & (u8) (~MASK_MUX_RESET)), reg);
            BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", reg, mux_reset_reg_val & 0x0);

            //unset mux on SFP/QSFP ports
            outb((mux_reset_reg_val | MASK_MUX_RESET), reg);
            mdelay(500);
            BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", reg, mux_reset_reg_val | 0xFF);

            mux_reset_flag = 0;
            mutex_unlock(&lpc_data->access_lock);
        } else {
            return -EINVAL;
        }
    } else {
        BSP_LOG_W("i2c mux is resetting... (ignore)");
        mutex_lock(&lpc_data->access_lock);
        mutex_unlock(&lpc_data->access_lock);
    }

    return count;
}

//SENSOR_DEVICE_ATTR - MB
static _SENSOR_DEVICE_ATTR_RO(board_id_0,          lpc_callback,      ATT_BRD_ID_0);
static _SENSOR_DEVICE_ATTR_RO(board_id_1,          lpc_callback,      ATT_BRD_ID_1);
static _SENSOR_DEVICE_ATTR_RO(board_sku_id,        lpc_callback,      ATT_BRD_SKU_ID);
static _SENSOR_DEVICE_ATTR_RO(board_hw_id,         lpc_callback,      ATT_BRD_HW_ID);
static _SENSOR_DEVICE_ATTR_RO(board_deph_id,       lpc_callback,      ATT_BRD_DEPH_ID);
static _SENSOR_DEVICE_ATTR_RO(board_build_id,      lpc_callback,      ATT_BRD_BUILD_ID);
static _SENSOR_DEVICE_ATTR_RO(board_ext_id,        lpc_callback,      ATT_BRD_EXT_ID);
static _SENSOR_DEVICE_ATTR_RO(cpld_version_major,  lpc_callback,      ATT_CPLD_VERSION_MAJOR);
static _SENSOR_DEVICE_ATTR_RO(cpld_version_minor,  lpc_callback,      ATT_CPLD_VERSION_MINOR);
static _SENSOR_DEVICE_ATTR_RO(cpld_version_build,  lpc_callback,      ATT_CPLD_VERSION_BUILD);
static _SENSOR_DEVICE_ATTR_RO(cpld_version_h,      mb_cpld_version_h, ATT_CPLD_VERSION_H);
static _SENSOR_DEVICE_ATTR_RO(cpld_id,             lpc_callback,      ATT_CPLD_ID);

static _SENSOR_DEVICE_ATTR_WO(mux_reset, mux_reset, ATT_MUX_RESET);
static _SENSOR_DEVICE_ATTR_RW(mux_ctrl, lpc_callback, ATT_MUX_CTRL);

//SENSOR_DEVICE_ATTR - BMC mailbox
static _SENSOR_DEVICE_ATTR_WO(temp_mac_hwm        , lpc_callback     , ATT_TEMP_MAC_HWM);

//SENSOR_DEVICE_ATTR - EC
static _SENSOR_DEVICE_ATTR_RO(cpu_hw_id,     lpc_callback, ATT_CPU_HW_ID);
static _SENSOR_DEVICE_ATTR_RO(cpu_deph_id,   lpc_callback, ATT_CPU_DEPH_ID);
static _SENSOR_DEVICE_ATTR_RO(cpu_build_id,  lpc_callback, ATT_CPU_BUILD_ID);
static _SENSOR_DEVICE_ATTR_RO(bios_boot_rom, lpc_callback, ATT_BIOS_BOOT_ROM);

//SENSOR_DEVICE_ATTR - BSP
static _SENSOR_DEVICE_ATTR_RW(bsp_version, bsp_callback,    ATT_BSP_VERSION);
static _SENSOR_DEVICE_ATTR_RW(bsp_debug,   bsp_callback,    ATT_BSP_DEBUG);
static _SENSOR_DEVICE_ATTR_WO(bsp_pr_info, bsp_pr_callback, ATT_BSP_PR_INFO);
static _SENSOR_DEVICE_ATTR_WO(bsp_pr_err,  bsp_pr_callback, ATT_BSP_PR_ERR);
static SENSOR_DEVICE_ATTR(bsp_reg,         S_IRUGO | S_IWUSR, read_lpc_callback, write_bsp_callback, ATT_BSP_REG);
static SENSOR_DEVICE_ATTR(bsp_gpio_max,    S_IRUGO, read_gpio_max, NULL, ATT_BSP_GPIO_MAX);

static struct attribute *mb_cpld_attrs[] = {
    _DEVICE_ATTR(board_id_0),
    _DEVICE_ATTR(board_id_1),
    _DEVICE_ATTR(board_sku_id),
    _DEVICE_ATTR(board_hw_id),
    _DEVICE_ATTR(board_deph_id),
    _DEVICE_ATTR(board_build_id),
    _DEVICE_ATTR(board_ext_id),
    _DEVICE_ATTR(cpld_version_major),
    _DEVICE_ATTR(cpld_version_minor),
    _DEVICE_ATTR(cpld_version_build),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(mux_reset),
    _DEVICE_ATTR(mux_ctrl),
    NULL,
};

static struct attribute *bsp_attrs[] = {
    _DEVICE_ATTR(bsp_version),
    _DEVICE_ATTR(bsp_debug),
    _DEVICE_ATTR(bsp_pr_info),
    _DEVICE_ATTR(bsp_pr_err),
    _DEVICE_ATTR(bsp_reg),
    _DEVICE_ATTR(bsp_gpio_max),
    NULL,
};

static struct attribute *ec_attrs[] = {
    _DEVICE_ATTR(cpu_hw_id),
    _DEVICE_ATTR(cpu_deph_id),
    _DEVICE_ATTR(cpu_build_id),
    _DEVICE_ATTR(bios_boot_rom),
    NULL,
};

static struct attribute *bmc_mailbox_attrs[] = {
    _DEVICE_ATTR(temp_mac_hwm),
    NULL,
};

static struct attribute_group mb_cpld_attr_grp = {
    .name = "mb_cpld",
    .attrs = mb_cpld_attrs,
};

static struct attribute_group bsp_attr_grp = {
    .name = "bsp",
    .attrs = bsp_attrs,
};

static struct attribute_group ec_attr_grp = {
    .name = "ec",
    .attrs = ec_attrs,
};

static struct attribute_group bmc_mailbox_attr_grp = {
    .name = "bmc_mailbox",
    .attrs = bmc_mailbox_attrs,
};

static void lpc_dev_release( struct device * dev)
{
    return;
}

static struct platform_device lpc_dev = {
    .name           = DRIVER_NAME,
    .id             = -1,
    .dev = {
                    .release = lpc_dev_release,
    }
};

static int lpc_drv_probe(struct platform_device *pdev)
{
    int i = 0, grp_num = 4;
    int err[4] = {0};
    struct attribute_group *grp;

    lpc_data = devm_kzalloc(&pdev->dev, sizeof(struct lpc_data_s),
                    GFP_KERNEL);
    if (!lpc_data)
        return -ENOMEM;

    mutex_init(&lpc_data->access_lock);

    for (i=0; i<grp_num; ++i) {
        switch (i) {
            case 0:
                grp = &mb_cpld_attr_grp;
                break;
            case 1:
                grp = &bmc_mailbox_attr_grp;
                break;
            case 2:
                grp = &bsp_attr_grp;
                break;
            case 3:
                grp = &ec_attr_grp;
                break;
            default:
                break;
        }

        err[i] = sysfs_create_group(&pdev->dev.kobj, grp);
        if (err[i]) {
            printk(KERN_ERR "Cannot create sysfs for group %s\n", grp->name);
            goto exit;
        } else {
            continue;
        }
    }

    return 0;

exit:
    for (i=0; i<grp_num; ++i) {
        switch (i) {
            case 0:
                grp = &mb_cpld_attr_grp;
                break;
            case 1:
                grp = &bmc_mailbox_attr_grp;
                break;
            case 2:
                grp = &bsp_attr_grp;
                break;
            case 3:
                grp = &ec_attr_grp;
                break;
            default:
                break;
        }

        sysfs_remove_group(&pdev->dev.kobj, grp);
        if (!err[i]) {
            //remove previous successful cases
            continue;
        } else {
            //remove first failed case, then return
            return err[i];
        }
    }
    return 0;
}

static int lpc_drv_remove(struct platform_device *pdev)
{
    sysfs_remove_group(&pdev->dev.kobj, &mb_cpld_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &bmc_mailbox_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &bsp_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &ec_attr_grp);

    return 0;
}

static struct platform_driver lpc_drv = {
    .probe  = lpc_drv_probe,
    .remove = __exit_p(lpc_drv_remove),
    .driver = {
    .name   = DRIVER_NAME,
    },
};

int lpc_init(void)
{
    int err = 0;

    err = platform_driver_register(&lpc_drv);
    if (err) {
    	printk(KERN_ERR "%s(#%d): platform_driver_register failed(%d)\n",
                __func__, __LINE__, err);

    	return err;
    }

    err = platform_device_register(&lpc_dev);
    if (err) {
    	printk(KERN_ERR "%s(#%d): platform_device_register failed(%d)\n",
                __func__, __LINE__, err);
    	platform_driver_unregister(&lpc_drv);
    	return err;
    }

    return err;
}

void lpc_exit(void)
{
    platform_driver_unregister(&lpc_drv);
    platform_device_unregister(&lpc_dev);
}

MODULE_AUTHOR("Jason Tsai <jason.cy.tsai@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_s7801_54xs_lpc driver");
MODULE_LICENSE("GPL");

module_init(lpc_init);
module_exit(lpc_exit);

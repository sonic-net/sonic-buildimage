/*
 * A lpc driver for the ufispace_s9300_32d
 *
 * Copyright (C) 2017-2020 UfiSpace Technology Corporation.
 * Jason Tsai <jason.cy.tsai@ufispace.com>
 * Leo Lin <leo.yt.lin@ufispace.com>
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
#include <linux/version.h>

#define BSP_LOG_R(fmt, args...)                            \
    _bsp_log(LOG_READ, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
             __FILE__, __func__, __LINE__, ##args)
#define BSP_LOG_W(fmt, args...)                             \
    _bsp_log(LOG_WRITE, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
             __FILE__, __func__, __LINE__, ##args)

#define BSP_PR(level, fmt, args...) _bsp_log(LOG_SYS, level "[BSP]" fmt "\r\n", ##args)

#define DRIVER_NAME "x86_64_ufispace_s9300_32d_lpc"
#define CPU_BDE 0
#define CPU_SKY 1
#define CPU_TYPE CPU_SKY

/* LPC registers */

#define REG_BASE_CPU 0x600

#if CPU_TYPE == CPU_SKY
#define REG_BASE_MB 0xE00
#define REG_BASE_I2C_ALERT 0x700
#else
#define REG_BASE_MB 0x700
#define REG_BASE_I2C_ALERT 0xF000
#endif

// CPU CPLD
#define REG_CPU_CPLD_VERSION (REG_BASE_CPU + 0x00)
#define REG_CPU_STATUS_0 (REG_BASE_CPU + 0x01)
#define REG_CPU_STATUS_1 (REG_BASE_CPU + 0x02)
#define REG_CPU_CTRL_0 (REG_BASE_CPU + 0x03)
#define REG_CPU_CTRL_1 (REG_BASE_CPU + 0x04)
#define REG_CPU_CPLD_BUILD (REG_BASE_CPU + 0xE0)

// MB CPLD
// TBD, need to change after CPLD spec release
#define REG_MB_BRD_ID_0 (REG_BASE_MB + 0x00)
#define REG_MB_BRD_ID_1 (REG_BASE_MB + 0x01)
#define REG_MB_CPLD_VERSION (REG_BASE_MB + 0x02)
#define REG_MB_CPLD_BUILD (REG_BASE_MB + 0x04)
#define REG_MB_MUX_RESET (REG_BASE_MB + 0x46)
#define REG_MB_MUX_CTRL (REG_BASE_MB + 0x5c)

// I2C Alert
#if CPU_TYPE == CPU_SKY
#define REG_ALERT_STATUS (REG_BASE_I2C_ALERT + 0x80)
#else
#define REG_ALERT_STATUS (REG_BASE_I2C_ALERT + 0x00)
#define REG_ALERT_DISABLE (REG_BASE_I2C_ALERT + 0x11)
#endif

#define MASK_ALL (0xFF)
#define LPC_MDELAY (5)

/* LPC sysfs attributes index  */
enum lpc_sysfs_attributes
{
    // CPU CPLD
    ATT_CPU_CPLD_VERSION,
    ATT_CPU_CPLD_VERSION_H,
    ATT_CPU_BIOS_BOOT_ROM,
    ATT_CPU_BIOS_BOOT_CFG,
    ATT_CPU_CPLD_BUILD,
    // MB CPLD
    ATT_MB_BRD_ID_0,
    ATT_MB_BRD_ID_1,
    ATT_MB_CPLD_1_VERSION,
    ATT_MB_CPLD_1_VERSION_H,
    ATT_MB_CPLD_1_BUILD,
    ATT_MB_MUX_CTRL,
    ATT_MB_MUX_RESET,
    ATT_MB_BRD_SKU_ID,
    ATT_MB_BRD_HW_ID,
    ATT_MB_BRD_ID_TYPE,
    ATT_MB_BRD_BUILD_ID,
    ATT_MB_BRD_DEPH_ID,
    // I2C Alert
    ATT_ALERT_STATUS,
#if CPU_TYPE == CPU_BDE
    ATT_ALERT_DISABLE,
#endif
    // BSP
    ATT_BSP_VERSION,
    ATT_BSP_DEBUG,
    ATT_BSP_PR_INFO,
    ATT_BSP_PR_ERR,
    ATT_BSP_REG,
    ATT_BSP_GPIO_MAX,
    ATT_BSP_GPIO_BASE,
    ATT_MAX
};

enum bsp_log_types
{
    LOG_NONE,
    LOG_RW,
    LOG_READ,
    LOG_WRITE,
    LOG_SYS
};

enum bsp_log_ctrl
{
    LOG_DISABLE,
    LOG_ENABLE
};

struct lpc_data_s
{
    struct mutex access_lock;
};

struct lpc_data_s *lpc_data;
char bsp_version[16] = "";
char bsp_debug[2] = "0";
char bsp_reg[8] = "0x0";
u8 enable_log_read = LOG_DISABLE;
u8 enable_log_write = LOG_DISABLE;
u8 enable_log_sys = LOG_ENABLE;

int lpc_init(void);
void lpc_exit(void);

/* reg shift */
static u8 _shift(u8 mask)
{
    int i = 0, mask_one = 1;

    for (i = 0; i < 8; ++i)
    {
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
    int shift = 0;

    shift = _shift(mask);

    return (val & mask) >> shift;
}

static u8 _bit_operation(u8 reg_val, u8 bit, u8 bit_val)
{
    if (bit_val == 0)
        reg_val = reg_val & ~(1 << bit);
    else
        reg_val = reg_val | (1 << bit);
    return reg_val;
}

static int _bsp_log(u8 log_type, char *fmt, ...)
{
    if ((log_type == LOG_READ && enable_log_read) ||
        (log_type == LOG_WRITE && enable_log_write) ||
        (log_type == LOG_SYS && enable_log_sys))
    {
        va_list args;
        int r;

        va_start(args, fmt);
        r = vprintk(fmt, args);
        va_end(args);

        return r;
    }
    else
    {
        return 0;
    }
}

static int _config_bsp_log(u8 log_type)
{
    switch (log_type)
    {
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

/* get lpc register value */
static u8 _read_lpc_reg(u16 reg, u8 mask)
{
    u8 reg_val;

    mutex_lock(&lpc_data->access_lock);
    reg_val = _mask_shift(inb(reg), mask);
    mutex_unlock(&lpc_data->access_lock);

    BSP_LOG_R("reg=0x%03x, reg_val=0x%02x", reg, reg_val);

    return reg_val;
}

/* get lpc register value */
static ssize_t read_lpc_reg(u16 reg, u8 mask, char *buf)
{
    u8 reg_val;
    int len = 0;

    reg_val = _read_lpc_reg(reg, mask);
    len = sprintf(buf, "0x%x\n", reg_val);

    return len;
}

/* set lpc register value */
static ssize_t write_lpc_reg(u16 reg, u8 mask, const char *buf, size_t count)
{
    u8 reg_val, reg_val_now, shift;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    // apply SINGLE BIT operation if mask is specified, multiple bits are not supported
    if (mask != MASK_ALL)
    {
        reg_val_now = _read_lpc_reg(reg, 0x0);
        shift = _shift(mask);
        reg_val = _bit_operation(reg_val_now, shift, reg_val);
    }

    mutex_lock(&lpc_data->access_lock);

    outb(reg_val, reg);
    mdelay(LPC_MDELAY);

    mutex_unlock(&lpc_data->access_lock);

    BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", reg, reg_val);

    return count;
}

/* get bsp value */
static ssize_t read_bsp(char *buf, char *str)
{
    ssize_t len = 0;

    mutex_lock(&lpc_data->access_lock);
    len = sprintf(buf, "%s", str);
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

/* get cpu cpld version in human readable format */
static ssize_t read_cpu_cpld_version_h(struct device *dev,
                                       struct device_attribute *da, char *buf)
{
    ssize_t len = 0;
    u16 reg = REG_CPU_CPLD_VERSION;
    u8 mask = MASK_ALL;
    u8 mask_major = 0b11000000;
    u8 mask_minor = 0b00111111;
    u8 reg_val;
    u8 major, minor, build;

    mutex_lock(&lpc_data->access_lock);
    reg_val = _mask_shift(inb(reg), mask);
    major = _mask_shift(reg_val, mask_major);
    minor = _mask_shift(reg_val, mask_minor);
    reg = REG_CPU_CPLD_BUILD;
    build = _mask_shift(inb(reg), mask);
    len = sprintf(buf, "%d.%02d.%03d\n", major, minor, build);
    mutex_unlock(&lpc_data->access_lock);

    BSP_LOG_R("reg=0x%03x, reg_val=0x%02x", reg, reg_val);

    return len;
}

/* get mb cpld version in human readable format */
static ssize_t read_mb_cpld_1_version_h(struct device *dev,
                                        struct device_attribute *da, char *buf)
{
    ssize_t len = 0;
    u16 reg = REG_MB_CPLD_VERSION;
    u8 mask = MASK_ALL;
    u8 mask_major = 0b11000000;
    u8 mask_minor = 0b00111111;
    u8 reg_val;
    u8 major, minor, build;

    mutex_lock(&lpc_data->access_lock);
    reg_val = _mask_shift(inb(reg), mask);
    major = _mask_shift(reg_val, mask_major);
    minor = _mask_shift(reg_val, mask_minor);
    reg = REG_MB_CPLD_BUILD;
    build = _mask_shift(inb(reg), mask);
    len = sprintf(buf, "%d.%02d.%03d\n", major, minor, build);
    mutex_unlock(&lpc_data->access_lock);

    BSP_LOG_R("reg=0x%03x, reg_val=0x%02x", reg, reg_val);

    return len;
}

/* get mux_reset register value */
static ssize_t read_mux_reset_callback(struct device *dev,
                                       struct device_attribute *da, char *buf)
{
    int len = 0;
    u16 reg = REG_MB_MUX_RESET;
    u8 mask = 0b00011111;
    u8 reg_val;

    mutex_lock(&lpc_data->access_lock);
    reg_val = _mask_shift(inb(reg), mask);
    BSP_LOG_R("reg=0x%03x, reg_val=0x%02x", reg, reg_val);
    len = sprintf(buf, "%d\n", reg_val);
    mutex_unlock(&lpc_data->access_lock);

    return len;
}

/* set mux_reset register value */
static ssize_t write_mux_reset_callback(struct device *dev,
                                        struct device_attribute *da, const char *buf, size_t count)
{
    u8 val = 0;
    u16 reg = REG_MB_MUX_RESET;
    u8 reg_val = 0;
    u8 mask = 0b00011111;
    static int mux_reset_flag = 0;

    if (kstrtou8(buf, 0, &val) < 0)
        return -EINVAL;

    if (mux_reset_flag == 0)
    {
        if (val == 0)
        {
            mutex_lock(&lpc_data->access_lock);
            mux_reset_flag = 1;
            BSP_LOG_W("i2c mux reset is triggered...");

            reg_val = inb(reg);
            outb((reg_val & ~mask), reg);
            mdelay(LPC_MDELAY);
            BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", reg, reg_val & ~mask);
            mdelay(500);
            outb((reg_val | mask), reg);
            mdelay(LPC_MDELAY);
            BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", reg, reg_val | mask);
            mdelay(500);
            mux_reset_flag = 0;
            mutex_unlock(&lpc_data->access_lock);
        }
        else
        {
            return -EINVAL;
        }
    }
    else
    {
        BSP_LOG_W("i2c mux is resetting... (ignore)");
        mutex_lock(&lpc_data->access_lock);
        mutex_unlock(&lpc_data->access_lock);
    }

    return count;
}

/* get lpc register value */
static ssize_t read_lpc_callback(struct device *dev,
                                 struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u16 reg = 0;
    u8 mask = MASK_ALL;

    switch (attr->index)
    {
    // CPU CPLD
    case ATT_CPU_CPLD_VERSION:
        reg = REG_CPU_CPLD_VERSION;
        break;
    case ATT_CPU_BIOS_BOOT_ROM:
        reg = REG_CPU_STATUS_1;
        mask = 0x80;
        break;
    case ATT_CPU_BIOS_BOOT_CFG:
        reg = REG_CPU_CTRL_1;
        mask = 0x80;
        break;
    case ATT_CPU_CPLD_BUILD:
        reg = REG_CPU_CPLD_BUILD;
        break;
    // MB CPLD
    case ATT_MB_BRD_ID_0:
        reg = REG_MB_BRD_ID_0;
        break;
    case ATT_MB_BRD_ID_1:
        reg = REG_MB_BRD_ID_1;
        break;
    case ATT_MB_CPLD_1_VERSION:
        reg = REG_MB_CPLD_VERSION;
        break;
    case ATT_MB_CPLD_1_BUILD:
        reg = REG_MB_CPLD_BUILD;
        break;
    case ATT_MB_BRD_SKU_ID:
        reg = REG_MB_BRD_ID_0;
        mask = 0xFF;
        break;
    case ATT_MB_BRD_HW_ID:
        reg = REG_MB_BRD_ID_1;
        mask = 0x03;
        break;
    case ATT_MB_BRD_ID_TYPE:
        reg = REG_MB_BRD_ID_1;
        mask = 0x80;
        break;
    case ATT_MB_BRD_BUILD_ID:
        reg = REG_MB_BRD_ID_1;
        mask = 0x38;
        break;
    case ATT_MB_BRD_DEPH_ID:
        reg = REG_MB_BRD_ID_1;
        mask = 0x04;
        break;
    case ATT_MB_MUX_CTRL:
        reg = REG_MB_MUX_CTRL;
        break;
    // I2C Alert
    case ATT_ALERT_STATUS:
        reg = REG_ALERT_STATUS;
        mask = 0x20;
        break;
#if CPU_TYPE == CPU_BDE
    case ATT_ALERT_DISABLE:
        reg = REG_ALERT_DISABLE;
        mask = 0x04;
        break;
#endif
    // BSP
    case ATT_BSP_REG:
        if (kstrtou16(bsp_reg, 0, &reg) < 0)
            return -EINVAL;
        break;
    default:
        return -EINVAL;
    }
    return read_lpc_reg(reg, mask, buf);
}

/* set lpc register value */
static ssize_t write_lpc_callback(struct device *dev,
                                  struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u16 reg = 0;
    u8 mask = MASK_ALL;

    switch (attr->index)
    {
    case ATT_MB_MUX_CTRL:
        reg = REG_MB_MUX_CTRL;
        break;
    default:
        return -EINVAL;
    }
    return write_lpc_reg(reg, mask, buf, count);
}

/* get bsp parameter value */
static ssize_t read_bsp_callback(struct device *dev,
                                 struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int str_len = 0;
    char *str = NULL;

    switch (attr->index)
    {
    case ATT_BSP_VERSION:
        str = bsp_version;
        str_len = sizeof(bsp_version);
        break;
    case ATT_BSP_DEBUG:
        str = bsp_debug;
        str_len = sizeof(bsp_debug);
        break;
    case ATT_BSP_REG:
        str = bsp_reg;
        str_len = sizeof(bsp_reg);
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
    int str_len = 0;
    char *str = NULL;
    u16 reg = 0;
    u8 bsp_debug_u8 = 0;

    switch (attr->index)
    {
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

    if (attr->index == ATT_BSP_DEBUG)
    {
        if (kstrtou8(buf, 0, &bsp_debug_u8) < 0)
        {
            return -EINVAL;
        }
        else if (_config_bsp_log(bsp_debug_u8) < 0)
        {
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

    if (str_len <= 0)
        return str_len;

    switch (attr->index)
    {
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

/* get gpio max value */
static ssize_t read_gpio_max(struct device *dev,
                             struct device_attribute *da,
                             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (attr->index == ATT_BSP_GPIO_MAX)
    {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
        return sprintf(buf, "%d\n", ARCH_NR_GPIOS - 1);
#else
        return sprintf(buf, "%d\n", -1);
#endif
    }
    return -1;
}

/* get gpio base value */
static ssize_t read_gpio_base(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (attr->index == ATT_BSP_GPIO_BASE) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
        return sprintf(buf, "%d\n", -1);
#else
        return sprintf(buf, "%d\n", GPIO_DYNAMIC_BASE);
#endif
    }
    return -1;
}

// SENSOR_DEVICE_ATTR - CPU
static SENSOR_DEVICE_ATTR(cpu_cpld_version, S_IRUGO, read_lpc_callback, NULL, ATT_CPU_CPLD_VERSION);
static SENSOR_DEVICE_ATTR(cpu_cpld_version_h, S_IRUGO, read_cpu_cpld_version_h, NULL, ATT_CPU_CPLD_VERSION_H);
static SENSOR_DEVICE_ATTR(boot_rom, S_IRUGO, read_lpc_callback, NULL, ATT_CPU_BIOS_BOOT_ROM);
static SENSOR_DEVICE_ATTR(boot_cfg, S_IRUGO, read_lpc_callback, NULL, ATT_CPU_BIOS_BOOT_CFG);
static SENSOR_DEVICE_ATTR(cpu_cpld_build, S_IRUGO, read_lpc_callback, NULL, ATT_CPU_CPLD_BUILD);
// SENSOR_DEVICE_ATTR - MB
static SENSOR_DEVICE_ATTR(board_id_0, S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_ID_0);
static SENSOR_DEVICE_ATTR(board_id_1, S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_ID_1);
static SENSOR_DEVICE_ATTR(mb_cpld_1_version, S_IRUGO, read_lpc_callback, NULL, ATT_MB_CPLD_1_VERSION);
static SENSOR_DEVICE_ATTR(mb_cpld_1_version_h, S_IRUGO, read_mb_cpld_1_version_h, NULL, ATT_MB_CPLD_1_VERSION_H);
static SENSOR_DEVICE_ATTR(mb_cpld_1_build, S_IRUGO, read_lpc_callback, NULL, ATT_MB_CPLD_1_BUILD);
static SENSOR_DEVICE_ATTR(mux_ctrl, S_IRUGO | S_IWUSR, read_lpc_callback, write_lpc_callback, ATT_MB_MUX_CTRL);
static SENSOR_DEVICE_ATTR(mux_reset, S_IRUGO | S_IWUSR, read_mux_reset_callback, write_mux_reset_callback, ATT_MB_MUX_RESET);
static SENSOR_DEVICE_ATTR(board_sku_id, S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_SKU_ID);
static SENSOR_DEVICE_ATTR(board_hw_id, S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_HW_ID);
static SENSOR_DEVICE_ATTR(board_id_type, S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_ID_TYPE);
static SENSOR_DEVICE_ATTR(board_build_id, S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_BUILD_ID);
static SENSOR_DEVICE_ATTR(board_deph_id, S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_DEPH_ID);
// SENSOR_DEVICE_ATTR - I2C Alert
static SENSOR_DEVICE_ATTR(alert_status, S_IRUGO, read_lpc_callback, NULL, ATT_ALERT_STATUS);
#if CPU_TYPE == CPU_BDE
static SENSOR_DEVICE_ATTR(alert_disable, S_IRUGO, read_lpc_callback, NULL, ATT_ALERT_DISABLE);
#endif
// SENSOR_DEVICE_ATTR - BSP
static SENSOR_DEVICE_ATTR(bsp_version, S_IRUGO | S_IWUSR, read_bsp_callback, write_bsp_callback, ATT_BSP_VERSION);
static SENSOR_DEVICE_ATTR(bsp_debug, S_IRUGO | S_IWUSR, read_bsp_callback, write_bsp_callback, ATT_BSP_DEBUG);
static SENSOR_DEVICE_ATTR(bsp_pr_info, S_IWUSR, NULL, write_bsp_pr_callback, ATT_BSP_PR_INFO);
static SENSOR_DEVICE_ATTR(bsp_pr_err, S_IWUSR, NULL, write_bsp_pr_callback, ATT_BSP_PR_ERR);
static SENSOR_DEVICE_ATTR(bsp_reg, S_IRUGO | S_IWUSR, read_lpc_callback, write_bsp_callback, ATT_BSP_REG);
static SENSOR_DEVICE_ATTR(bsp_gpio_max, S_IRUGO, read_gpio_max, NULL, ATT_BSP_GPIO_MAX);
static SENSOR_DEVICE_ATTR(bsp_gpio_base, S_IRUGO, read_gpio_base, NULL, ATT_BSP_GPIO_BASE);

static struct attribute *cpu_cpld_attrs[] = {
    &sensor_dev_attr_cpu_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpu_cpld_version_h.dev_attr.attr,
    &sensor_dev_attr_cpu_cpld_build.dev_attr.attr,
    NULL,
};

static struct attribute *mb_cpld_attrs[] = {
    &sensor_dev_attr_board_id_0.dev_attr.attr,
    &sensor_dev_attr_board_id_1.dev_attr.attr,
    &sensor_dev_attr_mb_cpld_1_version.dev_attr.attr,
    &sensor_dev_attr_mb_cpld_1_version_h.dev_attr.attr,
    &sensor_dev_attr_mb_cpld_1_build.dev_attr.attr,
    &sensor_dev_attr_board_sku_id.dev_attr.attr,
    &sensor_dev_attr_board_hw_id.dev_attr.attr,
    &sensor_dev_attr_board_id_type.dev_attr.attr,
    &sensor_dev_attr_board_build_id.dev_attr.attr,
    &sensor_dev_attr_board_deph_id.dev_attr.attr,
    &sensor_dev_attr_mux_ctrl.dev_attr.attr,
    &sensor_dev_attr_mux_reset.dev_attr.attr,
    NULL,
};

static struct attribute *bios_attrs[] = {
    &sensor_dev_attr_boot_rom.dev_attr.attr,
    &sensor_dev_attr_boot_cfg.dev_attr.attr,
    NULL,
};

static struct attribute *i2c_alert_attrs[] = {
    &sensor_dev_attr_alert_status.dev_attr.attr,
#if CPU_TYPE == CPU_BDE
    &sensor_dev_attr_alert_disable.dev_attr.attr,
#endif
    NULL,
};

static struct attribute *bsp_attrs[] = {
    &sensor_dev_attr_bsp_version.dev_attr.attr,
    &sensor_dev_attr_bsp_debug.dev_attr.attr,
    &sensor_dev_attr_bsp_pr_info.dev_attr.attr,
    &sensor_dev_attr_bsp_pr_err.dev_attr.attr,
    &sensor_dev_attr_bsp_reg.dev_attr.attr,
    &sensor_dev_attr_bsp_gpio_max.dev_attr.attr,
    &sensor_dev_attr_bsp_gpio_base.dev_attr.attr,
    NULL,
};

static struct attribute_group cpu_cpld_attr_grp = {
    .name = "cpu_cpld",
    .attrs = cpu_cpld_attrs,
};

static struct attribute_group mb_cpld_attr_grp = {
    .name = "mb_cpld",
    .attrs = mb_cpld_attrs,
};

static struct attribute_group bios_attr_grp = {
    .name = "bios",
    .attrs = bios_attrs,
};

static struct attribute_group i2c_alert_attr_grp = {
    .name = "i2c_alert",
    .attrs = i2c_alert_attrs,
};

static struct attribute_group bsp_attr_grp = {
    .name = "bsp",
    .attrs = bsp_attrs,
};

static void lpc_dev_release(struct device *dev)
{
    return;
}

static struct platform_device lpc_dev = {
    .name = DRIVER_NAME,
    .id = -1,
    .dev = {
        .release = lpc_dev_release,
    }};

static int lpc_drv_probe(struct platform_device *pdev)
{
    int i = 0, grp_num = 5;
    int err[5] = {0};
    struct attribute_group *grp;

    lpc_data = devm_kzalloc(&pdev->dev, sizeof(struct lpc_data_s),
                            GFP_KERNEL);
    if (!lpc_data)
        return -ENOMEM;

    mutex_init(&lpc_data->access_lock);

    for (i = 0; i < grp_num; ++i)
    {
        switch (i)
        {
        case 0:
            grp = &cpu_cpld_attr_grp;
            break;
        case 1:
            grp = &mb_cpld_attr_grp;
            break;
        case 2:
            grp = &bios_attr_grp;
            break;
        case 3:
            grp = &i2c_alert_attr_grp;
            break;
        case 4:
            grp = &bsp_attr_grp;
            break;
        default:
            break;
        }

        err[i] = sysfs_create_group(&pdev->dev.kobj, grp);
        if (err[i])
        {
            printk(KERN_ERR "Cannot create sysfs for group %s\n", grp->name);
            goto exit;
        }
        else
        {
            continue;
        }
    }

    return 0;

exit:
    for (i = 0; i < grp_num; ++i)
    {
        switch (i)
        {
        case 0:
            grp = &cpu_cpld_attr_grp;
            break;
        case 1:
            grp = &mb_cpld_attr_grp;
            break;
        case 2:
            grp = &bios_attr_grp;
            break;
        case 3:
            grp = &i2c_alert_attr_grp;
            break;
        case 4:
            grp = &bsp_attr_grp;
            break;
        default:
            break;
        }

        sysfs_remove_group(&pdev->dev.kobj, grp);
        if (!err[i])
        {
            // remove previous successful cases
            continue;
        }
        else
        {
            // remove first failed case, then return
            return err[i];
        }
    }
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
static int lpc_drv_remove(struct platform_device *pdev)
#else
static void lpc_drv_remove(struct platform_device *pdev)
#endif
{
    sysfs_remove_group(&pdev->dev.kobj, &cpu_cpld_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &mb_cpld_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &bios_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &i2c_alert_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &bsp_attr_grp);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
    return 0;
#endif
}

static struct platform_driver lpc_drv = {
    .probe = lpc_drv_probe,
    .remove = __exit_p(lpc_drv_remove),
    .driver = {
        .name = DRIVER_NAME,
    },
};

int lpc_init(void)
{
    int err = 0;

    err = platform_driver_register(&lpc_drv);
    if (err)
    {
        printk(KERN_ERR "%s(#%d): platform_driver_register failed(%d)\n",
               __func__, __LINE__, err);

        return err;
    }

    err = platform_device_register(&lpc_dev);
    if (err)
    {
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

MODULE_AUTHOR("Leo Lin <leo.yt.lin@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_s9300_32d_lpc driver");
MODULE_LICENSE("GPL");

module_init(lpc_init);
module_exit(lpc_exit);

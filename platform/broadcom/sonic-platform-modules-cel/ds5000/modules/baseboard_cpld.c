/*
 * cpld_b .c - The CPLD driver for the Base Board of Moonstone
 * The driver implement sysfs to access CPLD register on the baseboard of Moonstone via LPC bus.
 * Copyright (C) 2018 Celestica Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/acpi.h>
#include <linux/io.h>
#include <linux/dmi.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <uapi/linux/stat.h>
#include <linux/string.h>

#define DRIVER_NAME "sys_cpld"
/**
 * CPLD register address for read and write.
 */
#define VERSION_ADDR        0xA100
#define SCRATCH_ADDR        0xA101
#define CARD_PRESENCE_ADDR  0xA108
#define FAN1_LED_ADDR       0xA141
#define FAN1_R_SPD_ADDR     0xA142
#define FAN1_F_SPD_ADDR     0xA143
#define FAN2_LED_ADDR       0xA145
#define FAN2_R_SPD_ADDR     0xA146
#define FAN2_F_SPD_ADDR     0xA147
#define FAN3_LED_ADDR       0xA149
#define FAN3_R_SPD_ADDR     0xA14A
#define FAN3_F_SPD_ADDR     0xA14B
#define PSU_STAT2_ADDR      0xA15E
#define PSU_STAT1_ADDR      0xA160
#define PSU_LED_ADDR        0xA161
#define SYS_LED_ADDR        0xA162
#define ALARM_LED_ADDR      0xA163
#define FAN_LED_ADDR        0xA165
#define CPLD_REGISTER_SIZE  0xA5

struct cpld_b_data {
    struct mutex       cpld_lock;
    uint16_t           read_addr;
};

static struct cpld_b_data *cpld_data;

void cpld_b_lock(void)
{
    mutex_lock(&cpld_data->cpld_lock);
}
EXPORT_SYMBOL(cpld_b_lock);

void cpld_b_unlock(void)
{
    mutex_unlock(&cpld_data->cpld_lock);
}
EXPORT_SYMBOL(cpld_b_unlock);

/**
 * Read the value from scratch register as hex string.
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer for get value
 * @return         Hex string read from scratch register.
 */
static ssize_t scratch_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(SCRATCH_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf,"0x%2.2x\n", data);
}

/**
 * Set scratch register with specific hex string.
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer of set value
 * @param  count   number of bytes in buffer
 * @return         number of bytes written, or error code < 0.
 */
static ssize_t scratch_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    unsigned long data;
    char *last;

    mutex_lock(&cpld_data->cpld_lock);
    data = (uint16_t)strtoul(buf,&last,16);
    if(data == 0 && buf == last){
        mutex_unlock(&cpld_data->cpld_lock);
        return -EINVAL;
    }
    outb(data, SCRATCH_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return count;
}
static DEVICE_ATTR_RW(scratch);

/* CPLD version attributes */
static ssize_t version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int len = 0;
    // CPLD register is one byte
    mutex_lock(&cpld_data->cpld_lock);
    len = sprintf(buf, "0x%2.2x\n",inb(VERSION_ADDR));
    mutex_unlock(&cpld_data->cpld_lock);
    return len;
}
static DEVICE_ATTR_RO(version);

static ssize_t getreg_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    // CPLD register is one byte
    uint16_t addr;
    char *last;

    mutex_lock(&cpld_data->cpld_lock);
    addr = (uint16_t)strtoul(buf,&last,16);
    if(addr == 0 && buf == last){
        mutex_unlock(&cpld_data->cpld_lock);
        return -EINVAL;
    }
    cpld_data->read_addr = addr;
    return count;
}

static ssize_t getreg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int len = 0;
    // CPLD register is one byte
    len = sprintf(buf, "0x%2.2x\n",inb(cpld_data->read_addr));
    mutex_unlock(&cpld_data->cpld_lock);
    return len;
}
static DEVICE_ATTR_RW(getreg);

static ssize_t setreg_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    // CPLD register is one byte
    uint16_t addr;
    uint8_t value;
    char *tok;
    char *pclone = kmalloc(count, GFP_KERNEL);
    char *last;

    strcpy(pclone, buf);

    mutex_lock(&cpld_data->cpld_lock);
    tok = strsep((char**)&pclone, " ");
    if(tok == NULL){
        mutex_unlock(&cpld_data->cpld_lock);
        kfree(pclone);
        return -EINVAL;
    }
    addr = (uint16_t)strtoul(tok,&last,16);
    if(addr == 0 && tok == last){
        mutex_unlock(&cpld_data->cpld_lock);
        kfree(pclone);  
        return -EINVAL;
    }

    tok = strsep((char**)&pclone, " ");
    if(tok == NULL){
        mutex_unlock(&cpld_data->cpld_lock);
        kfree(pclone);
        return -EINVAL;
    }
    value = (uint8_t)strtoul(tok,&last,16);
    if(value == 0 && tok == last){
        mutex_unlock(&cpld_data->cpld_lock);
        kfree(pclone);
        return -EINVAL;
    }

    outb(value,addr);
    mutex_unlock(&cpld_data->cpld_lock);
    kfree(pclone);
    return count;
}
static DEVICE_ATTR_WO(setreg);

/**
 * Read all CPLD register in binary mode.
 * @return number of byte read.
 */
static ssize_t dump_read(struct file *filp, struct kobject *kobj,
                struct bin_attribute *attr, char *buf,
                loff_t off, size_t count)
{
    unsigned long i=0;
    ssize_t status;

    mutex_lock(&cpld_data->cpld_lock);
begin:
    if(i < count){
        buf[i++] = inb(VERSION_ADDR + off);
        off++;
        msleep(1);
        goto begin;
    }
    status = count;

    mutex_unlock(&cpld_data->cpld_lock);
    return status;
}
static BIN_ATTR_RO(dump, CPLD_REGISTER_SIZE);

/**
 * Show system led status - on/off/1hz/4hz
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer for get value
 * @return         Hex string read from scratch register.
 */
static ssize_t sys_led_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(SYS_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    data = data & 0x3;
    return sprintf(buf, "%s\n",
            data == 0x03 ? "off" : data == 0x02 ? "4hz" : data ==0x01 ? "1hz": "on");
}

/**
 * Set the status of system led - on/off/1hz/4hz
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer of set value
 * @param  count   number of bytes in buffer
 * @return         number of bytes written, or error code < 0.
 */
static ssize_t sys_led_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    unsigned char led_status,data;
    if(sysfs_streq(buf, "off")){
        led_status = 0x03;
    }else if(sysfs_streq(buf, "4hz")){
        led_status = 0x02;
    }else if(sysfs_streq(buf, "1hz")){
        led_status = 0x01;
    }else if(sysfs_streq(buf, "on")){
        led_status = 0x00;
    }else{
        count = -EINVAL;
        return count;
    }
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(SYS_LED_ADDR);
    data = data & ~(0x3);
    data = data | led_status;
    outb(data, SYS_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return count;
}
static DEVICE_ATTR_RW(sys_led);

/**
 * Show system led color - both/green/yellow/none
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer for get value
 * @return         Hex string read from scratch register.
 */
static ssize_t sys_led_color_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(SYS_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    data = (data >> 4) & 0x3;
    return sprintf(buf, "%s\n",
            data == 0x03 ? "off" : data == 0x02 ? "amber" : data ==0x01 ? "green": "both");
}

/**
 * Set the color of system led - both/green/yellow/none
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer of set value
 * @param  count   number of bytes in buffer
 * @return         number of bytes written, or error code < 0.
 */
static ssize_t sys_led_color_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    unsigned char led_status,data;
    if(sysfs_streq(buf, "off")){
        led_status = 0x03;
    }else if(sysfs_streq(buf, "amber")){
        led_status = 0x02;
    }else if(sysfs_streq(buf, "green")){
        led_status = 0x01;
    }else if(sysfs_streq(buf, "both")){
        led_status = 0x00;
    }else{
        count = -EINVAL;
        return count;
    }
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(SYS_LED_ADDR);
    data = data & ~( 0x3 << 4);
    data = data | (led_status << 4);
    outb(data, SYS_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return count;
}
static DEVICE_ATTR_RW(sys_led_color);

static ssize_t sys_led_raw_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(SYS_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "0x%02x\n", data);
}

static ssize_t sys_led_raw_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    unsigned char data;
    char *last;

    data = (uint8_t)strtoul(buf,&last,16);
    mutex_lock(&cpld_data->cpld_lock);
    outb(data, SYS_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    
    return count;
}
static DEVICE_ATTR_RW(sys_led_raw);

static ssize_t psu_led_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(PSU_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "0x%02x\n", data);
}

static ssize_t psu_led_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    unsigned char data;
    char *last;

    data = (uint8_t)strtoul(buf,&last,16);
    mutex_lock(&cpld_data->cpld_lock);
    outb(data, PSU_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    
    return count;
}
static DEVICE_ATTR_RW(psu_led);

static ssize_t alarm_led_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(ALARM_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "0x%02x\n", data);
}

static ssize_t alarm_led_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    unsigned char data;
    char *last;

    data = (uint8_t)strtoul(buf,&last,16);
    mutex_lock(&cpld_data->cpld_lock);
    outb(data, ALARM_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    
    return count;
}
static DEVICE_ATTR_RW(alarm_led);

static ssize_t fan_stat_led_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(FAN_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "0x%02x\n", data);
}

static ssize_t fan_stat_led_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    unsigned char data;
    char *last;

    data = (uint8_t)strtoul(buf,&last,16);
    mutex_lock(&cpld_data->cpld_lock);
    outb(data, FAN_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    
    return count;
}
static DEVICE_ATTR_RW(fan_stat_led);

static ssize_t fan1_led_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(FAN1_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "0x%02x\n", data);
}

static ssize_t fan1_led_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    unsigned char data;
    char *last;

    data = (uint8_t)strtoul(buf,&last,16);
    mutex_lock(&cpld_data->cpld_lock);
    outb(data, FAN1_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    
    return count;
}
static DEVICE_ATTR_RW(fan1_led);

static ssize_t fan2_led_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(FAN2_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "0x%02x\n", data);
}

static ssize_t fan2_led_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    unsigned char data;
    char *last;

    data = (uint8_t)strtoul(buf,&last,16);
    mutex_lock(&cpld_data->cpld_lock);
    outb(data, FAN2_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    
    return count;
}
static DEVICE_ATTR_RW(fan2_led);

static ssize_t fan3_led_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(FAN3_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "0x%02x\n", data);
}

static ssize_t fan3_led_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    unsigned char data;
    char *last;

    data = (uint8_t)strtoul(buf,&last,16);
    mutex_lock(&cpld_data->cpld_lock);
    outb(data, FAN3_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    
    return count;
}
static DEVICE_ATTR_RW(fan3_led);

static ssize_t psu1_present_l_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(PSU_STAT2_ADDR);
    data = (data & 0x01)? 1:0;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", data);
}
static DEVICE_ATTR_RO(psu1_present_l);

static ssize_t psu2_present_l_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(PSU_STAT2_ADDR);
    data = (data & 0x02)? 1:0;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", data);
}
static DEVICE_ATTR_RO(psu2_present_l);

static ssize_t psu3_present_l_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(PSU_STAT2_ADDR);
    data = (data & 0x04)? 1:0;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", data);
}
static DEVICE_ATTR_RO(psu3_present_l);

static ssize_t psu4_present_l_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(PSU_STAT2_ADDR);
    data = (data & 0x08)? 1:0;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", data);
}
static DEVICE_ATTR_RO(psu4_present_l);

static ssize_t psu1_pwr_ok_stat_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(PSU_STAT1_ADDR);
    data = (data & 0x01)? 1:0;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", data);
}
static DEVICE_ATTR_RO(psu1_pwr_ok_stat);

static ssize_t psu2_pwr_ok_stat_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(PSU_STAT1_ADDR);
    data = (data & 0x02)? 1:0;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", data);
}
static DEVICE_ATTR_RO(psu2_pwr_ok_stat);

static ssize_t psu3_pwr_ok_stat_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(PSU_STAT1_ADDR);
    data = (data & 0x04)? 1:0;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", data);
}
static DEVICE_ATTR_RO(psu3_pwr_ok_stat);

static ssize_t psu4_pwr_ok_stat_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(PSU_STAT1_ADDR);
    data = (data & 0x08)? 1:0;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", data);
}
static DEVICE_ATTR_RO(psu4_pwr_ok_stat);

static ssize_t fan1_rear_speed_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char raw = 0;
    uint16_t rpm = 0;


    mutex_lock(&cpld_data->cpld_lock);
    raw = inb(FAN1_R_SPD_ADDR);
    rpm = raw * 69;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", rpm);
}
static DEVICE_ATTR_RO(fan1_rear_speed);

static ssize_t fan1_front_speed_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char raw = 0;
    uint16_t rpm = 0;


    mutex_lock(&cpld_data->cpld_lock);
    raw = inb(FAN1_F_SPD_ADDR);
    rpm = raw * 78;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", rpm);
}
static DEVICE_ATTR_RO(fan1_front_speed);

static ssize_t fan2_rear_speed_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char raw = 0;
    uint16_t rpm = 0;


    mutex_lock(&cpld_data->cpld_lock);
    raw = inb(FAN2_R_SPD_ADDR);
    rpm = raw * 69;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", rpm);
}
static DEVICE_ATTR_RO(fan2_rear_speed);

static ssize_t fan2_front_speed_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char raw = 0;
    uint16_t rpm = 0;


    mutex_lock(&cpld_data->cpld_lock);
    raw = inb(FAN2_F_SPD_ADDR);
    rpm = raw * 78;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", rpm);
}
static DEVICE_ATTR_RO(fan2_front_speed);

static ssize_t fan3_rear_speed_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char raw = 0;
    uint16_t rpm = 0;


    mutex_lock(&cpld_data->cpld_lock);
    raw = inb(FAN3_R_SPD_ADDR);
    rpm = raw * 69;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", rpm);
}
static DEVICE_ATTR_RO(fan3_rear_speed);

static ssize_t fan3_front_speed_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char raw = 0;
    uint16_t rpm = 0;


    mutex_lock(&cpld_data->cpld_lock);
    raw = inb(FAN3_F_SPD_ADDR);
    rpm = raw * 78;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", rpm);
}
static DEVICE_ATTR_RO(fan3_front_speed);

static ssize_t fan_1_and_2_present_l_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(FAN1_LED_ADDR);
    data = (data & 0x04)? 1:0;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", data);
}
static DEVICE_ATTR_RO(fan_1_and_2_present_l);

static ssize_t fan_3_and_4_present_l_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(FAN2_LED_ADDR);
    data = (data & 0x04)? 1:0;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", data);
}
static DEVICE_ATTR_RO(fan_3_and_4_present_l);

static ssize_t fan_5_and_6_present_l_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(FAN3_LED_ADDR);
    data = (data & 0x04)? 1:0;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", data);
}
static DEVICE_ATTR_RO(fan_5_and_6_present_l);

static ssize_t bmc_present_l_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(CARD_PRESENCE_ADDR);
    data = (data & 0x01)? 1:0;
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d\n", data);
}
static DEVICE_ATTR_RO(bmc_present_l);

static struct attribute *cpld_b_attrs[] = {
    &dev_attr_version.attr,
    &dev_attr_scratch.attr,
    &dev_attr_getreg.attr,
    &dev_attr_setreg.attr,
    &dev_attr_sys_led.attr,
    &dev_attr_sys_led_color.attr,
    &dev_attr_sys_led_raw.attr,
    &dev_attr_psu_led.attr,
    &dev_attr_alarm_led.attr,
    &dev_attr_fan_stat_led.attr,
    &dev_attr_fan1_led.attr,
    &dev_attr_fan2_led.attr,
    &dev_attr_fan3_led.attr,
    &dev_attr_psu1_present_l.attr,
    &dev_attr_psu2_present_l.attr,
    &dev_attr_psu3_present_l.attr,
    &dev_attr_psu4_present_l.attr,
    &dev_attr_psu1_pwr_ok_stat.attr,
    &dev_attr_psu2_pwr_ok_stat.attr,
    &dev_attr_psu3_pwr_ok_stat.attr,
    &dev_attr_psu4_pwr_ok_stat.attr,
    &dev_attr_fan1_front_speed.attr,
    &dev_attr_fan1_rear_speed.attr,
    &dev_attr_fan2_front_speed.attr,
    &dev_attr_fan2_rear_speed.attr,
    &dev_attr_fan3_front_speed.attr,
    &dev_attr_fan3_rear_speed.attr,
    &dev_attr_fan_1_and_2_present_l.attr,
    &dev_attr_fan_3_and_4_present_l.attr,
    &dev_attr_fan_5_and_6_present_l.attr,
    &dev_attr_bmc_present_l.attr,
    NULL,
};

static struct bin_attribute *cpld_b_bin_attrs[] = {
    &bin_attr_dump,
    NULL,
};

static struct attribute_group cpld_b_attrs_grp = {
    .attrs = cpld_b_attrs,
    .bin_attrs = cpld_b_bin_attrs,
};

static struct resource cpld_b_resources[] = {
    {
        .start  = 0xA100,
        .end    = 0xA1FF,
        .flags  = IORESOURCE_IO,
    },
};

static void cpld_b_dev_release( struct device * dev)
{
    return;
}

static struct platform_device cpld_b_dev = {
    .name           = DRIVER_NAME,
    .id             = -1,
    .num_resources  = ARRAY_SIZE(cpld_b_resources),
    .resource       = cpld_b_resources,
    .dev = {
        .release = cpld_b_dev_release,
    }
};

static int cpld_b_drv_probe(struct platform_device *pdev)
{
    struct resource *res;
    int err = 0;

    cpld_data = devm_kzalloc(&pdev->dev, sizeof(struct cpld_b_data),
        GFP_KERNEL);
    if (!cpld_data)
        return -ENOMEM;

    mutex_init(&cpld_data->cpld_lock);

    cpld_data->read_addr = VERSION_ADDR;

    res = platform_get_resource(pdev, IORESOURCE_IO, 0);
    if (unlikely(!res)) {
        printk(KERN_ERR "Specified Resource Not Available...\n");
        return -ENODEV;
    }

    err = sysfs_create_group(&pdev->dev.kobj, &cpld_b_attrs_grp);
    if (err) {
        printk(KERN_ERR "Cannot create sysfs for baseboard CPLD\n");
        return err;
    }
    return 0;
}

static int cpld_b_drv_remove(struct platform_device *pdev)
{
    sysfs_remove_group(&pdev->dev.kobj, &cpld_b_attrs_grp);
    return 0;
}

static struct platform_driver cpld_b_drv = {
    .probe  = cpld_b_drv_probe,
    .remove = __exit_p(cpld_b_drv_remove),
    .driver = {
        .name   = DRIVER_NAME,
    },
};

int cpld_b_init(void)
{
    // Register platform device and platform driver
    platform_device_register(&cpld_b_dev);
    platform_driver_register(&cpld_b_drv);
    return 0;
}

void cpld_b_exit(void)
{
    // Unregister platform device and platform driver
    platform_driver_unregister(&cpld_b_drv);
    platform_device_unregister(&cpld_b_dev);
}

module_init(cpld_b_init);
module_exit(cpld_b_exit);


MODULE_AUTHOR("Celestica Inc.");
MODULE_DESCRIPTION("CELESTICA MOONSTONE WHITEBOX CPLD baseboard driver");
MODULE_VERSION("0.0.8");
MODULE_LICENSE("GPL");

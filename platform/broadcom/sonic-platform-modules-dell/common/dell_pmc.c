/* Copyright (c) 2017 Dell Inc.*
 * dell_s6100_smf.c - driver for Dell SMF
 *
 * Author: Per Fremrot <per_fremrot@dell.com>
 * Author: Paavaanan <paavaanan_t_n@dell.com>
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
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/acpi.h>
#include <linux/bitops.h>

#define SIO_DRVNAME             "SMF"
#define DEBUG                   1
#define LABELS                  1
#define SMF_VERSION_ADDR        0x0000

#define FANIN_MAX               12  /* Counted from 1 */
#define VSEN_MAX                48  /* VSEN1.. */
#define CURR_MAX                6
#define TCPU_MAX                15
#define PSU_MAX                 4 /* TODO change to actual sensors */

/* Where are the sensors address/data 
   registers relative to the region offset */

#define IOREGION_OFFSET          0x10
#define IOREGION_LENGTH           0x4
#define SMF_ADDR_REG_OFFSET         0
#define SMF_READ_DATA_REG_OFFSET    2
#define SMF_WRITE_DATA_REG_OFFSET   3
#define SMF_REG_ADDR            0x200
#define SMF_POR_SRC_REG         0x209
#define SMF_RST_SRC_REG         0x20A
#define SMF_PROBE_ADDR          0x210

#define SIO_REG_DEVID           0x1
#define SIO_Z9100_ID            0x1
#define SIO_S6100_ID            0x2
#define SIO_S4200_ID            0x3
#define SIO_S5148_ID            0x4

/* IOM presence */
#define IO_MODULE_STATUS        0x0310
#define IO_MODULE_PRESENCE      0x0311

/* FAN Tray */
#define S6100_MAX_NUM_FAN_TRAYS 4
#define Z9100_MAX_NUM_FAN_TRAYS 5

#define MAX_NUM_FAN_TRAYS       0x00F0
#define MAX_NUM_FANS_PER_TRAY   0x00F1
#define FAN_TRAY_PRESENCE       0x0113
#define FAN_STATUS_GROUP_A      0x0114
#define FAN_STATUS_GROUP_B      0x0115
#define FAN_TRAY_AIRFLOW        0x0116


/* FAN */
#define SMF_FAN_SPEED_ADDR      0x00F3
#define FAN_TRAY_1_SPEED        0x00F3
#define FAN_TRAY_1_FAN_2_SPEED  0x00F5
#define FAN_TRAY_2_SPEED        0x00F7
#define FAN_TRAY_2_FAN_2_SPEED  0x00F9
#define FAN_TRAY_3_SPEED        0x00FB
#define FAN_TRAY_3_FAN_2_SPEED  0x00FD
#define FAN_TRAY_4_SPEED        0x00FF
#define FAN_TRAY_4_FAN_2_SPEED  0x0101
#define FAN_TRAY_5_FAN_1_SPEED  0x0103
#define FAN_TRAY_5_FAN_2_SPEED  0x0105
#define FAN_TRAY_5                   4
#define FAN_1_SERIAL_CODE       0x0117
#define FAN_2_SERIAL_CODE       0x013A
#define FAN_3_SERIAL_CODE       0x015D
#define FAN_4_SERIAL_CODE       0x0180
#define FAN_5_SERIAL_CODE       0x01A3
#define FAN_601_FAULT           (2 + 1)
#define IN28_INPUT              (27 + 1)
#define IN404_INPUT             (43 + 1)
#define IOM_PRESENCE            (44 + 1)
#define IOM_PRESENCE_MAX        (45 + 1)
#define IN602_INPUT             (1 + 1)
#define CURR22_INPUT            (1 + 1)
#define CURR602_INPUT           (3 + 1)
#define TEMP13_INPUT            (12 + 1)
#define TEMP601_INPUT           (13 + 1)

/* PSUs */
#define S6100_MAX_NUM_PSUS      2
#define MAX_NUM_PSUS            0x0231
#define CURRENT_TOTAL_POWER     0x0232

/* PSU1 */
#define PSU_1_MAX_POWER         0x0234
#define PSU_1_FUNCTION_SUPPORT  0x0236
#define PSU_1_STATUS            0x0237
#define PSU_1_TEMPERATURE       0x0239
#define PSU_1_FAN_SPEED         0x023B
#define PSU_1_FAN_STATUS        0x023D
#define PSU_1_INPUT_VOLTAGE     0x023E
#define PSU_1_OUTPUT_VOLTAGE    0x0240
#define PSU_1_INPUT_CURRENT     0x0242
#define PSU_1_OUTPUT_CURRENT    0x0244
#define PSU_1_INPUT_POWER       0x0246
#define PSU_1_OUTPUT_POWER      0x0248
#define PSU_1_COUNTRY_CODE      0x024A

/* PSU2 */
#define PSU_2_MAX_POWER         0x026D
#define PSU_2_FUNCTION_SUPPORT  0x026F
#define PSU_2_STATUS            0x0270
#define PSU_2_TEMPERATURE       0x0272
#define PSU_2_FAN_SPEED         0x0274
#define PSU_2_FAN_STATUS        0x0276
#define PSU_2_INPUT_VOLTAGE     0x0277
#define PSU_2_OUTPUT_VOLTAGE    0x0279
#define PSU_2_INPUT_CURRENT     0x027B
#define PSU_2_OUTPUT_CURRENT    0x027D
#define PSU_2_INPUT_POWER       0x027F
#define PSU_2_OUTPUT_POWER      0x0281
#define PSU_2_COUNTRY_CODE      0x0283

/* TEMP */
#define TEMP_SENSOR_1           0x0014
#define TEMP_SENSOR_1_STATUS    0x00DC
#define TEMP_SENSOR_1_HW_LIMIT  0x003E

/* VOLTAGE */
#define CPU_1_VOLTAGE           0x02A8
#define IO_MODULE_1_VOLTAGE     0x02E8
#define SWITCH_CURRENT_S6100    0x02E4
#define SWITCH_CURRENT_Z9100    0x02E2

/* VOLTAGE S6100 */
#define CPU_1_MONITOR_STATUS    0x0308
#define CPU_2_MONITOR_STATUS    0x0309
#define CPU_3_MONITOR_STATUS    0x030A
#define CPU_4_MONITOR_STATUS    0x030B

/* VOLTAGE Z9100 */
#define CPU_5_MONITOR_STATUS    0x02E6
#define CPU_6_MONITOR_STATUS    0x02E7
#define CPU_7_MONITOR_STATUS    0x02E8
#define CPU_8_MONITOR_STATUS    0x02E9

/* EEPROM PPID */
/* FAN and PSU EEPROM PPID format is:
   COUNTRY_CODE-PART_NO-MFG_ID-MFG_DATE_CODE-SERIAL_NO-LABEL_REV */
#define EEPROM_COUNTRY_CODE_SIZE   2
#define EEPROM_PART_NO_SIZE        6
#define EEPROM_MFG_ID_SIZE         5
#define EEPROM_MFG_DATE_SIZE       8
#define EEPROM_DATE_CODE_SIZE      3
#define EEPROM_SERIAL_NO_SIZE      4
#define EEPROM_SERVICE_TAG_SIZE    7
#define EEPROM_LABEL_REV_SIZE      3
#define EEPROM_PPID_SIZE           28

/* Mailbox PowerOn Reason */
#define TRACK_POWERON_REASON    0x05FF

/* System Status LED */
#define SYSTEM_STATUS_LED       0x04DF

/* CPU Set IO Modules */
#define CPU_IOM1_CTRL_FLAG 0x04D9
#define CPU_IOM2_CTRL_FLAG 0x04DA
#define CPU_IOM3_CTRL_FLAG 0x04DB
#define CPU_IOM4_CTRL_FLAG 0x04DC


unsigned long  *mmio;
static struct kobject *dell_kobj;
static unsigned short force_id;
module_param(force_id, ushort, 0);
int smf_ver;


enum kinds {
        z9100smf, s6100smf
};


struct smf_devices {
        const char *name;
        u64 tcpu_mask;
        u64 vsen_mask;
        u32 curr_mask;
        u64 fanin_mask;
        u64 psu_mask;
        const char *const *temp_label;
        const char *const *vsen_label;
        const char *const *curr_label;
        const char *const *fan_label;
        const char *const *psu_label;
};


static const char *const z9100_temp_label[] = {
        "CPU On-board (U2900)",
        "BCM Switch On-Board #1 (U44)",
        "Front BCM On-Board (U4)",
        "Front BCM On-Board (U2)",
        "Unused",
        "BCM Switch On-Board #1 (U38)",
        "Unused",
        "Unused",
        "Rear (U2900)",
        "",
        "",
        "",
        "",
        "",
	"PSU 1",
	"PSU 2"
};


static const char *const s6100_temp_label[] = {
        "CPU On-board (U2900)",
        "BCM On-Board #1 (U44)",
        "Front BCM On-board (U4)",
        "Front BCM On-board (U2)",
        "IOM #1",
        "IOM #2",
        "IOM #3",
        "IOM #4",
        "U2 Switch board?",
        "Front GE",
        "Front SFP+",
        "BCM Internal",
        "CPU Internal",
        "",
	"PSU 1",
	"PSU 2"
};


static const char *const z9100_vsen_label[] = {
        /* CPU Board */
        "CPU XP3R3V_EARLY",
        "CPU XP5R0V_CP",
        "CPU XP3R3V_STD",
        "CPU XP3R3V_CP ",
        "CPU XP0R75V_VTT_A",
        "CPU XP0R75V_VTT_B",
        "CPU XP1R07V_CPU",
        "CPU XP1R0V_CPU",
        "CPU XP12R0V",
        "CPU VDDR_CPU_2",
        "CPU VDDR_CPU_1",
        "CPU XP1R5V_CLK",
        "CPU XP1R35V_CPU",
        "CPU XP1R8V_CPU",
        "CPU XP1R0V_CPU_VNN",
        "CPU XP1R0V_CPU_VCC",
        "CPU XP1R5V_EARLY",
        /* Switch Board */
        "SW XP12R0V_MON",
        "SW XP3R3V_MON",
        "SW XP1R8V_MON",
        "SW XP1R25V_MON",
        "SW XP1R2V_MON",
        "SW XP1R0V_SW_MON",
        "SW XP1R0V_ROV_SW_MON",
        "SW XP5V_MB_MON",
        "SW XP1R8V_FPGA_MON",
        "SW XP3R3V_FPGA_MON",
        "SW XP3R3V_EARLY_MON",
	/* PSU */
	"PSU1 VIN",
	"PSU1 VOUT",
	"PSU2 VIN",
	"PSU2 VOUT",
	/* IOM 1 */
	"",
	"",
	"",
	"",
	/* IOM 2 */
	"",
	"",
	"",
	"",
	/* IOM 3 */
	"",
	"",
	"",
	"",
	/* IOM 4 */
	"",
	"",
	"",
	""
};


static const char *const s6100_vsen_label[] = {
        /* CPU Board */
        "CPU XP3R3V_EARLY",
        "CPU XP5R0V_CP",
        "CPU XP3R3V_STD",
        "CPU XP3R3V_CP ",
        "CPU XP0R75V_VTT_A",
        "CPU XP0R75V_VTT_B",
        "CPU XP1R07V_CPU",
        "CPU XP1R0V_CPU",
        "CPU XP12R0V",
        "CPU VDDR_CPU_2",
        "CPU VDDR_CPU_1",
        "CPU XP1R5V_CLK",
        "CPU XP1R35V_CPU",
        "CPU XP1R8V_CPU",
        "CPU XP1R0V_CPU_VNN",
        "CPU XP1R0V_CPU_VCC",
        "CPU XP1R5V_EARLY",
        /* Switch Board */
        "SW XP12R0V_MON",
        "SW XP3R3V_MON", 
        "SW XP1R8V_MON",
        "SW XP1R25V_MON",
        "SW XP1R2V_MON",
        "SW XP1R0V_SW_MON",
        "SW XP1R0V_ROV_SW_MON",
        "XR1R0V_BCM84752_MON",
        "SW XP5V_MB_MON",
        "SW XP1R8V_FPGA_MON",
        "SW XP3R3V_FPGA_MON",

	/* PSU */
	"PSU1 VIN",
	"PSU1 VOUT",
	"PSU2 VIN",
	"PSU2 VOUT",

	/* IOM 1 */
	"IOM 1 #1",
	"IOM 1 #2",
	"IOM 1 #3",
	"IOM 1 #4",
	/* IOM 2 */
	"IOM 2 #1",
	"IOM 2 #2",
	"IOM 2 #3",
	"IOM 2 #4",
	/* IOM 1 */
	"IOM 3 #1",
	"IOM 3 #2",
	"IOM 3 #3",
	"IOM 3 #4",
	/* IOM 1 */
	"IOM 4 #1",
	"IOM 4 #2",
	"IOM 4 #3",
	"IOM 4 #4"
};


static const char *const z9100_curr_label[] = {
        "XP1R0V",
        "XP1R0V_ROV"
};


static const char *const s6100_fan_label[] = {
        "Tray1 Fan1",
        "",
        "Tray2 Fan1",
        "",
        "Tray3 Fan1",
        "",
        "Tray4 Fan1",
	"",
	"",
	"",
        "Psu1 Fan",
        "Psu2 Fan"
};

static const char *const z9100_fan_label[] = {
        "Tray1 Fan1",
        "Tray1 Fan2",
        "Tray2 Fan1",
        "Tray2 Fan2",
        "Tray3 Fan1",
        "Tray3 Fan2",
        "Tray4 Fan1",
        "Tray4 Fan2",
        "Tray5 Fan1",
        "Tray5 Fan2",
        "Psu1 Fan",
        "Psu2 Fan"
};


static const char *const s6100_psu_label[] = {
        "Psu1 Input",
        "Psu1 Output",
        "Psu2 Input",
        "Psu2 Output",
};

static const struct smf_devices smf_devices[] = {
        [z9100smf] = {
                .name = "SMF_Z9100_ON",
                .tcpu_mask=0xe1ff,
                .vsen_mask=0xfffdffff,
                .curr_mask=0x3f,
                .fanin_mask=0xfff,
                .psu_mask=0xf,
                .temp_label = z9100_temp_label,
                .vsen_label = z9100_vsen_label,
                .curr_label = z9100_curr_label,
                .fan_label = z9100_fan_label,
                .psu_label = s6100_psu_label
        },
        [s6100smf] = {
                .name = "SMF_S6100_ON",
                .tcpu_mask=0x7fff,
                .vsen_mask=0xfffffefdffff,
                .curr_mask=0x3f,
                .fanin_mask=0xc55,
                .psu_mask=0xf,
                .temp_label = s6100_temp_label,
                .vsen_label = s6100_vsen_label,
                .curr_label = z9100_curr_label,
                .fan_label = s6100_fan_label,
                .psu_label = s6100_psu_label

        }
};


/*
 * For each registered chip, we need to keep some data in memory.
 * The structure is dynamically allocated.
 */
struct smf_data {
        enum kinds kind;                /* Inherited from SuperIO kind */
        unsigned short addr;
        struct device *hwmon_dev;
        struct mutex lock;
        u64 tcpu_mask;
        u64 vsen_mask;
        u32 curr_mask;
        u64 fanin_mask;
        u64 psu_mask;
        const char * const *temp_label;
        const char * const *vsen_label;
        const char * const *curr_label;
        const char * const *fan_label;
        const char * const *psu_label;
};


struct smf_sio_data {
        int sioreg;
        enum kinds kind;
};

static int smf_write_reg(struct smf_data *data, u16 reg, u16 dev_data)
{
        int res = 0;

        mutex_lock(&data->lock);
        outb_p(reg>> 8, data->addr + SMF_ADDR_REG_OFFSET);
        outb_p(reg & 0xff, data->addr + SMF_ADDR_REG_OFFSET + 1);
        outb_p(dev_data & 0xff, data->addr + SMF_WRITE_DATA_REG_OFFSET);
        mutex_unlock(&data->lock);

        return res;
}

static int smf_read_reg(struct smf_data *data, u16 reg) 
{ 
        int res; 

        mutex_lock(&data->lock); 
        outb_p(reg>> 8, data->addr + SMF_ADDR_REG_OFFSET); 
        outb_p(reg & 0xff, data->addr + SMF_ADDR_REG_OFFSET + 1); 
        res = inb_p(data->addr + SMF_READ_DATA_REG_OFFSET); 
        mutex_unlock(&data->lock); 
        return res; 
} 


static int smf_read_reg16(struct smf_data *data, u16 reg)
{
        int res;

        mutex_lock(&data->lock);
        outb_p(reg>> 8, data->addr + SMF_ADDR_REG_OFFSET);
        outb_p(reg & 0xff, data->addr + SMF_ADDR_REG_OFFSET + 1);

        res = inb_p(data->addr + SMF_READ_DATA_REG_OFFSET);

        outb_p((reg + 1)>> 8, data->addr + SMF_ADDR_REG_OFFSET);
        outb_p((reg + 1) & 0xff, data->addr + SMF_ADDR_REG_OFFSET + 1);

        res = (res << 8) + inb_p(data->addr + SMF_READ_DATA_REG_OFFSET);

        mutex_unlock(&data->lock);
        return res;
}

/* SMF Version */
static ssize_t show_smf_version(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
    int              index = to_sensor_dev_attr(devattr)->index;
    unsigned int     ret = 0;
    unsigned int     smf_version = 0;
    unsigned int     smf_firmware_major_ver = 0;
    unsigned int     smf_firmware_minor_ver = 0;
    struct smf_data *data = dev_get_drvdata(dev);

    ret = smf_read_reg(data, (SMF_VERSION_ADDR + index*2));

    printk("smf_firmware_details-->0x%x index[%d]", ret, index);

    if (index > 0) {
        smf_firmware_major_ver = ((ret & (0xC0)) >> 6);
        smf_firmware_minor_ver = (ret & (0x3F));

        ret = sprintf(buf, "%u.%u\n", smf_firmware_major_ver,
                                      smf_firmware_minor_ver);
    } else {
        smf_version = ret;
        ret = sprintf(buf, "%u\n", smf_version);
    }

    return ret;
}


/* SMF Reset Reason */
static ssize_t show_reset_reason(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
    unsigned int ret = 0;

    ret = inb(SMF_RST_SRC_REG);

    if(ret < 0)
       return ret;

    return sprintf(buf, "%x\n", ret);
}

/* SMF Power ON Reason */
static ssize_t show_power_on_reason(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
    unsigned int ret = 0;

    ret = inb(SMF_POR_SRC_REG);

    if(ret < 0)
       return ret;

    return sprintf(buf, "%x\n", ret);
}

/* SMF Mailbox Power ON Reason */
static ssize_t set_mb_poweron_reason(struct device *dev,
              struct device_attribute *devattr, const char *buf, size_t count)
{
    int              err = 0;
    unsigned int     dev_data = 0;
    struct smf_data *data = dev_get_drvdata(dev);

    err = kstrtouint(buf, 16, &dev_data);
    if (err)
        return err;

    err = smf_write_reg(data, TRACK_POWERON_REASON, dev_data);

    if(err < 0)
       return err;

    return count;
}

static ssize_t show_mb_poweron_reason(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
    unsigned int     ret = 0;
    struct smf_data *data = dev_get_drvdata(dev);

    ret = smf_read_reg(data, TRACK_POWERON_REASON);

    if(ret < 0)
       return ret;

    return sprintf(buf, "0x%x\n", ret);
}

/* System Status LED */
static ssize_t set_sys_status_led(struct device *dev,
              struct device_attribute *devattr, const char *buf, size_t count)
{
    int              err = 0;
    unsigned int     dev_data = 0;
    struct smf_data *data = dev_get_drvdata(dev);

    if (data->kind == z9100smf)
	return -1;

    err = kstrtouint(buf, 16, &dev_data);
    if (err)
        return err;

    err = smf_write_reg(data, SYSTEM_STATUS_LED, dev_data);
    if(err < 0)
	return err;

    return count;
}

static ssize_t show_sys_status_led(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
    unsigned int     ret = 0;
    struct smf_data *data = dev_get_drvdata(dev);

    if (data->kind == z9100smf)
	return 0;

    ret = smf_read_reg(data, SYSTEM_STATUS_LED);
    if(ret < 0)
       return ret;

    return sprintf(buf, "0x%x\n", ret);
}

/* FANIN ATTR */
static ssize_t
show_fan_label(struct device *dev, struct device_attribute *attr, char *buf)
{
        struct smf_data *data = dev_get_drvdata(dev);
        int nr = to_sensor_dev_attr(attr)->index;
        return sprintf(buf, "%s\n", data->fan_label[nr]);
}


static ssize_t show_fan(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
        int index = to_sensor_dev_attr(devattr)->index;
        struct smf_data *data = dev_get_drvdata(dev);
        int export_hex=0;
        unsigned int ret = -1;
        unsigned rpm;


	if (index <10)
		ret = smf_read_reg16(data, SMF_FAN_SPEED_ADDR + index * 2);
	else switch (index) {
                case 10:
                        ret = smf_read_reg16(data, PSU_1_FAN_SPEED);
                        break;
                case 11:
                        ret = smf_read_reg16(data, PSU_2_FAN_SPEED);
                        break;
                case 12:
                        ret = (~smf_read_reg(data, FAN_TRAY_PRESENCE) & 0xff);
                        export_hex = 1;
                        break; 
		       
                default:
                        return ret;
		}


        if (ret < 0)
                return ret;

        if (ret & 0x8000)
                ret = - (ret & 0x7fff);

        rpm = ret;

        if(export_hex)
                return sprintf(buf, "%x\n", rpm);
        else
                return sprintf(buf, "%u\n", rpm);
}


static ssize_t show_fan_fault(struct device *dev,
                              struct device_attribute *devattr, char *buf)
{
        int index = to_sensor_dev_attr(devattr)->index;
        struct smf_data *data = dev_get_drvdata(dev);
        int ret=1, fan_status;

	index = index / 2;
        fan_status = ~smf_read_reg(data, FAN_TRAY_PRESENCE);

        if (fan_status & (1 << (index)))
                ret=0;

        if (ret < 0)
                return ret;

        return sprintf(buf, "%d\n", ret);  
}


static ssize_t show_fan_alarm(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
        int index = to_sensor_dev_attr(devattr)->index;
        struct smf_data *data = dev_get_drvdata(dev);
        int ret, psu_fan_status=0;

        if (data->kind == z9100smf) {
                if ((index % 2) == 0)
                        index = index / 2;
                else
                        index = (index / 2) + 5;
        }

        if (data->kind == s6100smf)
                index = index / 2;

        if (index > 7) {
                psu_fan_status = ~smf_read_reg(data, FAN_STATUS_GROUP_A);
                index = index % 8;
        } else
                psu_fan_status = ~smf_read_reg(data, FAN_STATUS_GROUP_B);

        if (psu_fan_status & (1 << (index)))
                ret=0;
        else
                ret=1;

        return sprintf(buf, "%d\n", ret);  
}


static ssize_t show_fan_airflow(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
        int index = to_sensor_dev_attr(devattr)->index;
        struct smf_data *data = dev_get_drvdata(dev);
        int ret, fan_airflow;

        if (data->kind == s6100smf && index == FAN_TRAY_5)
                return 0;

        fan_airflow = smf_read_reg(data, FAN_TRAY_AIRFLOW);
        ret = (fan_airflow >> index) & 1;

        return sprintf(buf, "%d\n", ret);  
}


static ssize_t show_psu_fan(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
        int index = to_sensor_dev_attr(devattr)->index;
        struct smf_data *data = dev_get_drvdata(dev);
        int ret=0, fan_status;

        if (index < FAN_601_FAULT){                               
                fan_status = smf_read_reg(data, PSU_1_FAN_STATUS);
                ret = (fan_status >> index) & 1;

        }
        else{ 
                fan_status = smf_read_reg(data, PSU_2_FAN_STATUS);
                ret = (fan_status >> (index - 3)) & 1;
        }

        if (ret < 0)
                return ret;

        return sprintf(buf, "%d\n", ret);  
}

static ssize_t show_cpu_iom_control(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
        int index = to_sensor_dev_attr(devattr)->index;
        struct smf_data *data = dev_get_drvdata(dev);
        int cpu_iom_status;

        if(index == 0)
            cpu_iom_status = smf_read_reg(data, CPU_IOM1_CTRL_FLAG);
        else if (index == 1)
            cpu_iom_status = smf_read_reg(data, CPU_IOM2_CTRL_FLAG);
        else if (index == 2)
            cpu_iom_status = smf_read_reg(data, CPU_IOM3_CTRL_FLAG);
        else if (index == 3)
            cpu_iom_status = smf_read_reg(data, CPU_IOM4_CTRL_FLAG);

        return sprintf(buf, "%x\n", cpu_iom_status);
}



static umode_t smf_fanin_is_visible(struct kobject *kobj,
                struct attribute *a, int n)
{
        struct device *dev = container_of(kobj, struct device, kobj);
        struct smf_data *data = dev_get_drvdata(dev);
        
        if (data->fanin_mask & (1 << (n % FANIN_MAX)))
                return a->mode;

        return 0;
}

static umode_t smf_dell_is_visible(struct kobject *kobj,
                struct attribute *a, int n)
{
	return a->mode;

}

static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, show_fan, NULL, 0);
static SENSOR_DEVICE_ATTR(fan2_input, S_IRUGO, show_fan, NULL, 1);
static SENSOR_DEVICE_ATTR(fan3_input, S_IRUGO, show_fan, NULL, 2);
static SENSOR_DEVICE_ATTR(fan4_input, S_IRUGO, show_fan, NULL, 3);
static SENSOR_DEVICE_ATTR(fan5_input, S_IRUGO, show_fan, NULL, 4);
static SENSOR_DEVICE_ATTR(fan6_input, S_IRUGO, show_fan, NULL, 5);
static SENSOR_DEVICE_ATTR(fan7_input, S_IRUGO, show_fan, NULL, 6);
static SENSOR_DEVICE_ATTR(fan8_input, S_IRUGO, show_fan, NULL, 7);
static SENSOR_DEVICE_ATTR(fan9_input, S_IRUGO, show_fan, NULL, 8);
static SENSOR_DEVICE_ATTR(fan10_input, S_IRUGO, show_fan, NULL,9);
/* PSU1 FAN */
static SENSOR_DEVICE_ATTR(fan11_input, S_IRUGO, show_fan, NULL, 10);
/* PSU2 FAN */
static SENSOR_DEVICE_ATTR(fan12_input, S_IRUGO, show_fan, NULL, 11);

static SENSOR_DEVICE_ATTR(fan1_alarm, S_IRUGO, show_fan_alarm, NULL, 0);
static SENSOR_DEVICE_ATTR(fan2_alarm, S_IRUGO, show_fan_alarm, NULL, 1);
static SENSOR_DEVICE_ATTR(fan3_alarm, S_IRUGO, show_fan_alarm, NULL, 2);
static SENSOR_DEVICE_ATTR(fan4_alarm, S_IRUGO, show_fan_alarm, NULL, 3);
static SENSOR_DEVICE_ATTR(fan5_alarm, S_IRUGO, show_fan_alarm, NULL, 4);
static SENSOR_DEVICE_ATTR(fan6_alarm, S_IRUGO, show_fan_alarm, NULL, 5);
static SENSOR_DEVICE_ATTR(fan7_alarm, S_IRUGO, show_fan_alarm, NULL, 6);
static SENSOR_DEVICE_ATTR(fan8_alarm, S_IRUGO, show_fan_alarm, NULL, 7);
static SENSOR_DEVICE_ATTR(fan9_alarm, S_IRUGO, show_fan_alarm, NULL, 8);
static SENSOR_DEVICE_ATTR(fan10_alarm, S_IRUGO, show_fan_alarm, NULL, 9);
static SENSOR_DEVICE_ATTR(fan11_alarm, S_IRUGO, show_psu_fan, NULL, 1);
static SENSOR_DEVICE_ATTR(fan12_alarm, S_IRUGO, show_psu_fan, NULL, 4);

static SENSOR_DEVICE_ATTR(fan1_fault, S_IRUGO, show_fan_fault, NULL, 0);
static SENSOR_DEVICE_ATTR(fan2_fault, S_IRUGO, show_fan_fault, NULL, 1);
static SENSOR_DEVICE_ATTR(fan3_fault, S_IRUGO, show_fan_fault, NULL, 2);
static SENSOR_DEVICE_ATTR(fan4_fault, S_IRUGO, show_fan_fault, NULL, 3);
static SENSOR_DEVICE_ATTR(fan5_fault, S_IRUGO, show_fan_fault, NULL, 4);
static SENSOR_DEVICE_ATTR(fan6_fault, S_IRUGO, show_fan_fault, NULL, 5);
static SENSOR_DEVICE_ATTR(fan7_fault, S_IRUGO, show_fan_fault, NULL, 6);
static SENSOR_DEVICE_ATTR(fan8_fault, S_IRUGO, show_fan_fault, NULL, 7);
static SENSOR_DEVICE_ATTR(fan9_fault, S_IRUGO, show_fan_fault, NULL, 8);
static SENSOR_DEVICE_ATTR(fan10_fault, S_IRUGO, show_fan_fault, NULL, 9);
static SENSOR_DEVICE_ATTR(fan11_fault, S_IRUGO, show_psu_fan, NULL, 2);
static SENSOR_DEVICE_ATTR(fan12_fault, S_IRUGO, show_psu_fan, NULL, 5);

static SENSOR_DEVICE_ATTR(fan1_label, S_IRUGO, show_fan_label, NULL, 0);
static SENSOR_DEVICE_ATTR(fan2_label, S_IRUGO, show_fan_label, NULL, 1);
static SENSOR_DEVICE_ATTR(fan3_label, S_IRUGO, show_fan_label, NULL, 2);
static SENSOR_DEVICE_ATTR(fan4_label, S_IRUGO, show_fan_label, NULL, 3);
static SENSOR_DEVICE_ATTR(fan5_label, S_IRUGO, show_fan_label, NULL, 4);
static SENSOR_DEVICE_ATTR(fan6_label, S_IRUGO, show_fan_label, NULL, 5);
static SENSOR_DEVICE_ATTR(fan7_label, S_IRUGO, show_fan_label, NULL, 6);
static SENSOR_DEVICE_ATTR(fan8_label, S_IRUGO, show_fan_label, NULL, 7);
static SENSOR_DEVICE_ATTR(fan9_label, S_IRUGO, show_fan_label, NULL, 8);
static SENSOR_DEVICE_ATTR(fan10_label, S_IRUGO, show_fan_label, NULL, 9);
static SENSOR_DEVICE_ATTR(fan11_label, S_IRUGO, show_fan_label, NULL, 10);
static SENSOR_DEVICE_ATTR(fan12_label, S_IRUGO, show_fan_label, NULL, 11);


static struct attribute *smf_fanin_attrs[] = {
        &sensor_dev_attr_fan1_input.dev_attr.attr,
        &sensor_dev_attr_fan2_input.dev_attr.attr,
        &sensor_dev_attr_fan3_input.dev_attr.attr,
        &sensor_dev_attr_fan4_input.dev_attr.attr,
        &sensor_dev_attr_fan5_input.dev_attr.attr,
        &sensor_dev_attr_fan6_input.dev_attr.attr,
        &sensor_dev_attr_fan7_input.dev_attr.attr,
        &sensor_dev_attr_fan8_input.dev_attr.attr,
        &sensor_dev_attr_fan9_input.dev_attr.attr,
        &sensor_dev_attr_fan10_input.dev_attr.attr,
        &sensor_dev_attr_fan11_input.dev_attr.attr,
        &sensor_dev_attr_fan12_input.dev_attr.attr,

        &sensor_dev_attr_fan1_label.dev_attr.attr,
        &sensor_dev_attr_fan2_label.dev_attr.attr,
        &sensor_dev_attr_fan3_label.dev_attr.attr,
        &sensor_dev_attr_fan4_label.dev_attr.attr,
        &sensor_dev_attr_fan5_label.dev_attr.attr,
        &sensor_dev_attr_fan6_label.dev_attr.attr,
        &sensor_dev_attr_fan7_label.dev_attr.attr,
        &sensor_dev_attr_fan8_label.dev_attr.attr,
        &sensor_dev_attr_fan9_label.dev_attr.attr,
        &sensor_dev_attr_fan10_label.dev_attr.attr,
        &sensor_dev_attr_fan11_label.dev_attr.attr,
        &sensor_dev_attr_fan12_label.dev_attr.attr,


        &sensor_dev_attr_fan1_alarm.dev_attr.attr,
        &sensor_dev_attr_fan2_alarm.dev_attr.attr,
        &sensor_dev_attr_fan3_alarm.dev_attr.attr,
        &sensor_dev_attr_fan4_alarm.dev_attr.attr,
        &sensor_dev_attr_fan5_alarm.dev_attr.attr,
        &sensor_dev_attr_fan6_alarm.dev_attr.attr,
        &sensor_dev_attr_fan7_alarm.dev_attr.attr,
        &sensor_dev_attr_fan8_alarm.dev_attr.attr,
        &sensor_dev_attr_fan9_alarm.dev_attr.attr,
        &sensor_dev_attr_fan10_alarm.dev_attr.attr,
        &sensor_dev_attr_fan11_alarm.dev_attr.attr,
        &sensor_dev_attr_fan12_alarm.dev_attr.attr,

        &sensor_dev_attr_fan1_fault.dev_attr.attr,
        &sensor_dev_attr_fan2_fault.dev_attr.attr,
        &sensor_dev_attr_fan3_fault.dev_attr.attr,
        &sensor_dev_attr_fan4_fault.dev_attr.attr,
        &sensor_dev_attr_fan5_fault.dev_attr.attr,
        &sensor_dev_attr_fan6_fault.dev_attr.attr,
        &sensor_dev_attr_fan7_fault.dev_attr.attr,
        &sensor_dev_attr_fan8_fault.dev_attr.attr,
        &sensor_dev_attr_fan9_fault.dev_attr.attr,
        &sensor_dev_attr_fan10_fault.dev_attr.attr,
        &sensor_dev_attr_fan11_fault.dev_attr.attr,
        &sensor_dev_attr_fan12_fault.dev_attr.attr,

        NULL
};




/* VSEN ATTR */
static ssize_t
show_voltage_label(struct device *dev, 
                struct device_attribute *attr, char *buf)
{
        struct smf_data *data = dev_get_drvdata(dev);
        int nr = to_sensor_dev_attr(attr)->index;
        return sprintf(buf, "%s\n", data->vsen_label[nr]);
}


static ssize_t show_voltage(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
        int index = to_sensor_dev_attr(devattr)->index;
        struct smf_data *data = dev_get_drvdata(dev);
        int volt, ret=0;
        int export_hex=0;

        /*0 to 27 */
        if (index < IN28_INPUT)                         /* Voltage sensors */
                ret = smf_read_reg16(data, CPU_1_VOLTAGE + index * 2);
        else if ((data->kind == s6100smf) && (index < IN404_INPUT))
                ret = smf_read_reg16(data, IO_MODULE_1_VOLTAGE + index * 2);
        else if ((data->kind == s6100smf) && (index < IOM_PRESENCE))
                ret = smf_read_reg(data, IO_MODULE_STATUS);
        else if ((data->kind == s6100smf) && (index < IOM_PRESENCE_MAX))
                ret = smf_read_reg(data, IO_MODULE_PRESENCE);

        if (ret < 0)
                return ret;
        
        if(index < 44)
                volt = ret*10;
        else
                export_hex=1;        

        if(export_hex)
                return sprintf(buf, "%x\n", ret);
        else
                return sprintf(buf, "%d\n", volt);
}


static ssize_t show_psu_voltage(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
        int index = to_sensor_dev_attr(devattr)->index;
        struct smf_data *data = dev_get_drvdata(dev);
        int ret;

        if (index < 2)                        /* PSU1 */
                ret = smf_read_reg16(data, PSU_1_INPUT_VOLTAGE + index * 2);
        else                                  /* PSU2 */         
                ret = smf_read_reg16(data, PSU_2_INPUT_VOLTAGE + ((index - 2) * 2));

        if (ret < 0)
                return ret;

        return sprintf(buf, "%d\n", ret*10);
}


static ssize_t show_voltage_alarm(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
        int index = to_sensor_dev_attr(devattr)->index;
        struct smf_data *data = dev_get_drvdata(dev);
        unsigned status=0; 
        int ret;

        if (index < 8) {

                if (data->kind == s6100smf)
                        status = smf_read_reg16(data, CPU_1_MONITOR_STATUS);
                else
                        status = smf_read_reg16(data, CPU_5_MONITOR_STATUS);

                ret = status & (1 << index);
        }
        else if (index < 15) {

                if (data->kind == s6100smf)
                        ret = smf_read_reg16(data, CPU_2_MONITOR_STATUS);
                else
                        ret = smf_read_reg16(data, CPU_6_MONITOR_STATUS);

                ret = status & (1 << index);
        }
        else if (index < 23) {

                if (data->kind == s6100smf)
                        ret = smf_read_reg16(data, CPU_3_MONITOR_STATUS);
                else
                        ret = smf_read_reg16(data, CPU_7_MONITOR_STATUS);

                ret = status & (1 << index);
        }
        else {

                if (data->kind == s6100smf)
                        ret = smf_read_reg16(data, CPU_4_MONITOR_STATUS);
                else
                        ret = smf_read_reg16(data, CPU_8_MONITOR_STATUS);

                ret = status & (1 << index);
        }


        if (ret < 0)
                return ret;

        return sprintf(buf, "%d\n", ret);
}


static umode_t smf_vsen_is_visible(struct kobject *kobj,
                struct attribute *a, int n)
{
        struct device *dev = container_of(kobj, struct device, kobj);
        struct smf_data *data = dev_get_drvdata(dev);

        if (data->vsen_mask & (1 << (n % VSEN_MAX)))
                return a->mode;
        return 0;
}

static SENSOR_DEVICE_ATTR(in1_input, S_IRUGO, show_voltage, NULL, 0);
static SENSOR_DEVICE_ATTR(in2_input, S_IRUGO, show_voltage, NULL, 1);
static SENSOR_DEVICE_ATTR(in3_input, S_IRUGO, show_voltage, NULL, 2);
static SENSOR_DEVICE_ATTR(in4_input, S_IRUGO, show_voltage, NULL, 3);
static SENSOR_DEVICE_ATTR(in5_input, S_IRUGO, show_voltage, NULL, 4);
static SENSOR_DEVICE_ATTR(in6_input, S_IRUGO, show_voltage, NULL, 5);
static SENSOR_DEVICE_ATTR(in7_input, S_IRUGO, show_voltage, NULL, 6);
static SENSOR_DEVICE_ATTR(in8_input, S_IRUGO, show_voltage, NULL, 7);
static SENSOR_DEVICE_ATTR(in9_input, S_IRUGO, show_voltage, NULL, 8);
static SENSOR_DEVICE_ATTR(in10_input, S_IRUGO, show_voltage, NULL, 9);
static SENSOR_DEVICE_ATTR(in11_input, S_IRUGO, show_voltage, NULL, 10);
static SENSOR_DEVICE_ATTR(in12_input, S_IRUGO, show_voltage, NULL, 11);
static SENSOR_DEVICE_ATTR(in13_input, S_IRUGO, show_voltage, NULL, 12);
static SENSOR_DEVICE_ATTR(in14_input, S_IRUGO, show_voltage, NULL, 13);
static SENSOR_DEVICE_ATTR(in15_input, S_IRUGO, show_voltage, NULL, 14);
static SENSOR_DEVICE_ATTR(in16_input, S_IRUGO, show_voltage, NULL, 15);
static SENSOR_DEVICE_ATTR(in17_input, S_IRUGO, show_voltage, NULL, 16);
static SENSOR_DEVICE_ATTR(in18_input, S_IRUGO, show_voltage, NULL, 17);
static SENSOR_DEVICE_ATTR(in19_input, S_IRUGO, show_voltage, NULL, 18);
static SENSOR_DEVICE_ATTR(in20_input, S_IRUGO, show_voltage, NULL, 19);
static SENSOR_DEVICE_ATTR(in21_input, S_IRUGO, show_voltage, NULL, 20);
static SENSOR_DEVICE_ATTR(in22_input, S_IRUGO, show_voltage, NULL, 21);
static SENSOR_DEVICE_ATTR(in23_input, S_IRUGO, show_voltage, NULL, 22);
static SENSOR_DEVICE_ATTR(in24_input, S_IRUGO, show_voltage, NULL, 23);
static SENSOR_DEVICE_ATTR(in25_input, S_IRUGO, show_voltage, NULL, 24);
static SENSOR_DEVICE_ATTR(in26_input, S_IRUGO, show_voltage, NULL, 25);
static SENSOR_DEVICE_ATTR(in27_input, S_IRUGO, show_voltage, NULL, 26);
static SENSOR_DEVICE_ATTR(in28_input, S_IRUGO, show_voltage, NULL, 27);

/* PSU1 Voltage*/ 
static SENSOR_DEVICE_ATTR(in29_input, S_IRUGO, show_psu_voltage, NULL, 0);
static SENSOR_DEVICE_ATTR(in30_input, S_IRUGO, show_psu_voltage, NULL, 1);

/* PSU2 Voltage*/ 
static SENSOR_DEVICE_ATTR(in31_input, S_IRUGO, show_psu_voltage, NULL, 2);
static SENSOR_DEVICE_ATTR(in32_input, S_IRUGO, show_psu_voltage, NULL, 3);

/*IO Modules Voltage*/
static SENSOR_DEVICE_ATTR(in101_input, S_IRUGO, show_voltage, NULL, 28);
static SENSOR_DEVICE_ATTR(in102_input, S_IRUGO, show_voltage, NULL, 29);
static SENSOR_DEVICE_ATTR(in103_input, S_IRUGO, show_voltage, NULL, 30);
static SENSOR_DEVICE_ATTR(in104_input, S_IRUGO, show_voltage, NULL, 31);
static SENSOR_DEVICE_ATTR(in201_input, S_IRUGO, show_voltage, NULL, 32);
static SENSOR_DEVICE_ATTR(in202_input, S_IRUGO, show_voltage, NULL, 33);
static SENSOR_DEVICE_ATTR(in203_input, S_IRUGO, show_voltage, NULL, 34);
static SENSOR_DEVICE_ATTR(in204_input, S_IRUGO, show_voltage, NULL, 35);
static SENSOR_DEVICE_ATTR(in301_input, S_IRUGO, show_voltage, NULL, 36);
static SENSOR_DEVICE_ATTR(in302_input, S_IRUGO, show_voltage, NULL, 37);
static SENSOR_DEVICE_ATTR(in303_input, S_IRUGO, show_voltage, NULL, 38);
static SENSOR_DEVICE_ATTR(in304_input, S_IRUGO, show_voltage, NULL, 39);
static SENSOR_DEVICE_ATTR(in401_input, S_IRUGO, show_voltage, NULL, 40);
static SENSOR_DEVICE_ATTR(in402_input, S_IRUGO, show_voltage, NULL, 41);
static SENSOR_DEVICE_ATTR(in403_input, S_IRUGO, show_voltage, NULL, 42);
static SENSOR_DEVICE_ATTR(in404_input, S_IRUGO, show_voltage, NULL, 43);



static SENSOR_DEVICE_ATTR(in1_label, S_IRUGO, show_voltage_label, NULL, 0);
static SENSOR_DEVICE_ATTR(in2_label, S_IRUGO, show_voltage_label, NULL, 1);
static SENSOR_DEVICE_ATTR(in3_label, S_IRUGO, show_voltage_label, NULL, 2);
static SENSOR_DEVICE_ATTR(in4_label, S_IRUGO, show_voltage_label, NULL, 3);
static SENSOR_DEVICE_ATTR(in5_label, S_IRUGO, show_voltage_label, NULL, 4);
static SENSOR_DEVICE_ATTR(in6_label, S_IRUGO, show_voltage_label, NULL, 5);
static SENSOR_DEVICE_ATTR(in7_label, S_IRUGO, show_voltage_label, NULL, 6);
static SENSOR_DEVICE_ATTR(in8_label, S_IRUGO, show_voltage_label, NULL, 7);
static SENSOR_DEVICE_ATTR(in9_label, S_IRUGO, show_voltage_label, NULL, 8);
static SENSOR_DEVICE_ATTR(in10_label, S_IRUGO, show_voltage_label, NULL, 9);
static SENSOR_DEVICE_ATTR(in11_label, S_IRUGO, show_voltage_label, NULL, 10);
static SENSOR_DEVICE_ATTR(in12_label, S_IRUGO, show_voltage_label, NULL, 11);
static SENSOR_DEVICE_ATTR(in13_label, S_IRUGO, show_voltage_label, NULL, 12);
static SENSOR_DEVICE_ATTR(in14_label, S_IRUGO, show_voltage_label, NULL, 13);
static SENSOR_DEVICE_ATTR(in15_label, S_IRUGO, show_voltage_label, NULL, 14);
static SENSOR_DEVICE_ATTR(in16_label, S_IRUGO, show_voltage_label, NULL, 15);
static SENSOR_DEVICE_ATTR(in17_label, S_IRUGO, show_voltage_label, NULL, 16);
static SENSOR_DEVICE_ATTR(in18_label, S_IRUGO, show_voltage_label, NULL, 17);
static SENSOR_DEVICE_ATTR(in19_label, S_IRUGO, show_voltage_label, NULL, 18);
static SENSOR_DEVICE_ATTR(in20_label, S_IRUGO, show_voltage_label, NULL, 19);
static SENSOR_DEVICE_ATTR(in21_label, S_IRUGO, show_voltage_label, NULL, 20);
static SENSOR_DEVICE_ATTR(in22_label, S_IRUGO, show_voltage_label, NULL, 21);
static SENSOR_DEVICE_ATTR(in23_label, S_IRUGO, show_voltage_label, NULL, 22);
static SENSOR_DEVICE_ATTR(in24_label, S_IRUGO, show_voltage_label, NULL, 23);
static SENSOR_DEVICE_ATTR(in25_label, S_IRUGO, show_voltage_label, NULL, 24);
static SENSOR_DEVICE_ATTR(in26_label, S_IRUGO, show_voltage_label, NULL, 25);
static SENSOR_DEVICE_ATTR(in27_label, S_IRUGO, show_voltage_label, NULL, 26);
static SENSOR_DEVICE_ATTR(in28_label, S_IRUGO, show_voltage_label, NULL, 27);

/* PSU1 Voltage Label*/ 
static SENSOR_DEVICE_ATTR(in29_label, S_IRUGO, show_voltage_label, NULL, 28);
static SENSOR_DEVICE_ATTR(in30_label, S_IRUGO, show_voltage_label, NULL, 29);

/* PSU2 Voltage Label*/ 
static SENSOR_DEVICE_ATTR(in31_label, S_IRUGO, show_voltage_label, NULL, 30);
static SENSOR_DEVICE_ATTR(in32_label, S_IRUGO, show_voltage_label, NULL, 31);

/*IO Modules Labels*/
static SENSOR_DEVICE_ATTR(in101_label, S_IRUGO, show_voltage_label, NULL, 32);
static SENSOR_DEVICE_ATTR(in102_label, S_IRUGO, show_voltage_label, NULL, 33);
static SENSOR_DEVICE_ATTR(in103_label, S_IRUGO, show_voltage_label, NULL, 34);
static SENSOR_DEVICE_ATTR(in104_label, S_IRUGO, show_voltage_label, NULL, 35);
static SENSOR_DEVICE_ATTR(in201_label, S_IRUGO, show_voltage_label, NULL, 36);
static SENSOR_DEVICE_ATTR(in202_label, S_IRUGO, show_voltage_label, NULL, 37);
static SENSOR_DEVICE_ATTR(in203_label, S_IRUGO, show_voltage_label, NULL, 38);
static SENSOR_DEVICE_ATTR(in204_label, S_IRUGO, show_voltage_label, NULL, 39);
static SENSOR_DEVICE_ATTR(in301_label, S_IRUGO, show_voltage_label, NULL, 40);
static SENSOR_DEVICE_ATTR(in302_label, S_IRUGO, show_voltage_label, NULL, 41);
static SENSOR_DEVICE_ATTR(in303_label, S_IRUGO, show_voltage_label, NULL, 42);
static SENSOR_DEVICE_ATTR(in304_label, S_IRUGO, show_voltage_label, NULL, 43);
static SENSOR_DEVICE_ATTR(in401_label, S_IRUGO, show_voltage_label, NULL, 44);
static SENSOR_DEVICE_ATTR(in402_label, S_IRUGO, show_voltage_label, NULL, 45);
static SENSOR_DEVICE_ATTR(in403_label, S_IRUGO, show_voltage_label, NULL, 46);
static SENSOR_DEVICE_ATTR(in404_label, S_IRUGO, show_voltage_label, NULL, 47);



/* CPU Voltage Alarm */
static SENSOR_DEVICE_ATTR(in1_alarm, S_IRUGO, show_voltage_alarm, NULL, 0);
static SENSOR_DEVICE_ATTR(in2_alarm, S_IRUGO, show_voltage_alarm, NULL, 1);
static SENSOR_DEVICE_ATTR(in3_alarm, S_IRUGO, show_voltage_alarm, NULL, 2);
static SENSOR_DEVICE_ATTR(in4_alarm, S_IRUGO, show_voltage_alarm, NULL, 3);
static SENSOR_DEVICE_ATTR(in5_alarm, S_IRUGO, show_voltage_alarm, NULL, 4);
static SENSOR_DEVICE_ATTR(in6_alarm, S_IRUGO, show_voltage_alarm, NULL, 5);
static SENSOR_DEVICE_ATTR(in7_alarm, S_IRUGO, show_voltage_alarm, NULL, 6);
static SENSOR_DEVICE_ATTR(in8_alarm, S_IRUGO, show_voltage_alarm, NULL, 7);

static SENSOR_DEVICE_ATTR(in9_alarm, S_IRUGO, show_voltage_alarm, NULL, 8);
static SENSOR_DEVICE_ATTR(in10_alarm, S_IRUGO, show_voltage_alarm, NULL, 9);
static SENSOR_DEVICE_ATTR(in11_alarm, S_IRUGO, show_voltage_alarm, NULL, 10);
static SENSOR_DEVICE_ATTR(in12_alarm, S_IRUGO, show_voltage_alarm, NULL, 11);
static SENSOR_DEVICE_ATTR(in13_alarm, S_IRUGO, show_voltage_alarm, NULL, 12);
static SENSOR_DEVICE_ATTR(in14_alarm, S_IRUGO, show_voltage_alarm, NULL, 13);
static SENSOR_DEVICE_ATTR(in15_alarm, S_IRUGO, show_voltage_alarm, NULL, 14);
static SENSOR_DEVICE_ATTR(in16_alarm, S_IRUGO, show_voltage_alarm, NULL, 15);

static SENSOR_DEVICE_ATTR(in17_alarm, S_IRUGO, show_voltage_alarm, NULL, 16);
static SENSOR_DEVICE_ATTR(in18_alarm, S_IRUGO, show_voltage_alarm, NULL, 17);
static SENSOR_DEVICE_ATTR(in19_alarm, S_IRUGO, show_voltage_alarm, NULL, 18);
static SENSOR_DEVICE_ATTR(in20_alarm, S_IRUGO, show_voltage_alarm, NULL, 19);
static SENSOR_DEVICE_ATTR(in21_alarm, S_IRUGO, show_voltage_alarm, NULL, 20);
static SENSOR_DEVICE_ATTR(in22_alarm, S_IRUGO, show_voltage_alarm, NULL, 21);
static SENSOR_DEVICE_ATTR(in23_alarm, S_IRUGO, show_voltage_alarm, NULL, 22);
static SENSOR_DEVICE_ATTR(in24_alarm, S_IRUGO, show_voltage_alarm, NULL, 23);

static SENSOR_DEVICE_ATTR(in25_alarm, S_IRUGO, show_voltage_alarm, NULL, 24);
static SENSOR_DEVICE_ATTR(in26_alarm, S_IRUGO, show_voltage_alarm, NULL, 25);
static SENSOR_DEVICE_ATTR(in27_alarm, S_IRUGO, show_voltage_alarm, NULL, 26);
static SENSOR_DEVICE_ATTR(in28_alarm, S_IRUGO, show_voltage_alarm, NULL, 27);



static struct attribute *smf_vsen_attrs[] = {
        &sensor_dev_attr_in1_input.dev_attr.attr,
        &sensor_dev_attr_in2_input.dev_attr.attr,
        &sensor_dev_attr_in3_input.dev_attr.attr,
        &sensor_dev_attr_in4_input.dev_attr.attr,
        &sensor_dev_attr_in5_input.dev_attr.attr,
        &sensor_dev_attr_in6_input.dev_attr.attr,
        &sensor_dev_attr_in7_input.dev_attr.attr,
        &sensor_dev_attr_in8_input.dev_attr.attr,
        &sensor_dev_attr_in9_input.dev_attr.attr,
        &sensor_dev_attr_in10_input.dev_attr.attr,
        &sensor_dev_attr_in11_input.dev_attr.attr,
        &sensor_dev_attr_in12_input.dev_attr.attr,
        &sensor_dev_attr_in13_input.dev_attr.attr,
        &sensor_dev_attr_in14_input.dev_attr.attr,
        &sensor_dev_attr_in15_input.dev_attr.attr,
        &sensor_dev_attr_in16_input.dev_attr.attr,
        &sensor_dev_attr_in17_input.dev_attr.attr,
        &sensor_dev_attr_in18_input.dev_attr.attr,
        &sensor_dev_attr_in19_input.dev_attr.attr,
        &sensor_dev_attr_in20_input.dev_attr.attr,
        &sensor_dev_attr_in21_input.dev_attr.attr,
        &sensor_dev_attr_in22_input.dev_attr.attr,
        &sensor_dev_attr_in23_input.dev_attr.attr,
        &sensor_dev_attr_in24_input.dev_attr.attr,
        &sensor_dev_attr_in25_input.dev_attr.attr,
        &sensor_dev_attr_in26_input.dev_attr.attr,
        &sensor_dev_attr_in27_input.dev_attr.attr,
        &sensor_dev_attr_in28_input.dev_attr.attr,

        &sensor_dev_attr_in29_input.dev_attr.attr,
        &sensor_dev_attr_in30_input.dev_attr.attr,
        &sensor_dev_attr_in31_input.dev_attr.attr,
        &sensor_dev_attr_in32_input.dev_attr.attr,

        &sensor_dev_attr_in101_input.dev_attr.attr,
        &sensor_dev_attr_in102_input.dev_attr.attr,
        &sensor_dev_attr_in103_input.dev_attr.attr,
        &sensor_dev_attr_in104_input.dev_attr.attr,
        &sensor_dev_attr_in201_input.dev_attr.attr,
        &sensor_dev_attr_in202_input.dev_attr.attr,
        &sensor_dev_attr_in203_input.dev_attr.attr,
        &sensor_dev_attr_in204_input.dev_attr.attr,
        &sensor_dev_attr_in301_input.dev_attr.attr,
        &sensor_dev_attr_in302_input.dev_attr.attr,
        &sensor_dev_attr_in303_input.dev_attr.attr,
        &sensor_dev_attr_in304_input.dev_attr.attr,
        &sensor_dev_attr_in401_input.dev_attr.attr,
        &sensor_dev_attr_in402_input.dev_attr.attr,
        &sensor_dev_attr_in403_input.dev_attr.attr,
        &sensor_dev_attr_in404_input.dev_attr.attr,


	&sensor_dev_attr_in1_label.dev_attr.attr,
        &sensor_dev_attr_in2_label.dev_attr.attr,
        &sensor_dev_attr_in3_label.dev_attr.attr,
        &sensor_dev_attr_in4_label.dev_attr.attr,
        &sensor_dev_attr_in5_label.dev_attr.attr,
        &sensor_dev_attr_in6_label.dev_attr.attr,
        &sensor_dev_attr_in7_label.dev_attr.attr,
        &sensor_dev_attr_in8_label.dev_attr.attr,
        &sensor_dev_attr_in9_label.dev_attr.attr,
        &sensor_dev_attr_in10_label.dev_attr.attr,
        &sensor_dev_attr_in11_label.dev_attr.attr,
        &sensor_dev_attr_in12_label.dev_attr.attr,
        &sensor_dev_attr_in13_label.dev_attr.attr,
        &sensor_dev_attr_in14_label.dev_attr.attr,
        &sensor_dev_attr_in15_label.dev_attr.attr,
        &sensor_dev_attr_in16_label.dev_attr.attr,
        &sensor_dev_attr_in17_label.dev_attr.attr,
        &sensor_dev_attr_in18_label.dev_attr.attr,
        &sensor_dev_attr_in19_label.dev_attr.attr,
        &sensor_dev_attr_in20_label.dev_attr.attr,
        &sensor_dev_attr_in21_label.dev_attr.attr,
        &sensor_dev_attr_in22_label.dev_attr.attr,
        &sensor_dev_attr_in23_label.dev_attr.attr,
        &sensor_dev_attr_in24_label.dev_attr.attr,
        &sensor_dev_attr_in25_label.dev_attr.attr,
        &sensor_dev_attr_in26_label.dev_attr.attr,
        &sensor_dev_attr_in27_label.dev_attr.attr,
        &sensor_dev_attr_in28_label.dev_attr.attr,

        &sensor_dev_attr_in29_label.dev_attr.attr,
        &sensor_dev_attr_in30_label.dev_attr.attr,
        &sensor_dev_attr_in31_label.dev_attr.attr,
        &sensor_dev_attr_in32_label.dev_attr.attr,

        &sensor_dev_attr_in101_label.dev_attr.attr,
        &sensor_dev_attr_in102_label.dev_attr.attr,
        &sensor_dev_attr_in103_label.dev_attr.attr,
        &sensor_dev_attr_in104_label.dev_attr.attr,
        &sensor_dev_attr_in201_label.dev_attr.attr,
        &sensor_dev_attr_in202_label.dev_attr.attr,
        &sensor_dev_attr_in203_label.dev_attr.attr,
        &sensor_dev_attr_in204_label.dev_attr.attr,
        &sensor_dev_attr_in301_label.dev_attr.attr,
        &sensor_dev_attr_in302_label.dev_attr.attr,
        &sensor_dev_attr_in303_label.dev_attr.attr,
        &sensor_dev_attr_in304_label.dev_attr.attr,
        &sensor_dev_attr_in401_label.dev_attr.attr,
        &sensor_dev_attr_in402_label.dev_attr.attr,
        &sensor_dev_attr_in403_label.dev_attr.attr,
        &sensor_dev_attr_in404_label.dev_attr.attr,


        &sensor_dev_attr_in1_alarm.dev_attr.attr,
        &sensor_dev_attr_in2_alarm.dev_attr.attr,
        &sensor_dev_attr_in3_alarm.dev_attr.attr,
        &sensor_dev_attr_in4_alarm.dev_attr.attr,
        &sensor_dev_attr_in5_alarm.dev_attr.attr,
        &sensor_dev_attr_in6_alarm.dev_attr.attr,
        &sensor_dev_attr_in7_alarm.dev_attr.attr,
        &sensor_dev_attr_in8_alarm.dev_attr.attr,
        &sensor_dev_attr_in9_alarm.dev_attr.attr,
        &sensor_dev_attr_in10_alarm.dev_attr.attr,
        &sensor_dev_attr_in11_alarm.dev_attr.attr,
        &sensor_dev_attr_in12_alarm.dev_attr.attr,
        &sensor_dev_attr_in13_alarm.dev_attr.attr,
        &sensor_dev_attr_in14_alarm.dev_attr.attr,
        &sensor_dev_attr_in15_alarm.dev_attr.attr,
        &sensor_dev_attr_in16_alarm.dev_attr.attr,
        &sensor_dev_attr_in17_alarm.dev_attr.attr,
        &sensor_dev_attr_in18_alarm.dev_attr.attr,
        &sensor_dev_attr_in19_alarm.dev_attr.attr,
        &sensor_dev_attr_in20_alarm.dev_attr.attr,
        &sensor_dev_attr_in21_alarm.dev_attr.attr,
        &sensor_dev_attr_in22_alarm.dev_attr.attr,
        &sensor_dev_attr_in23_alarm.dev_attr.attr,
        &sensor_dev_attr_in24_alarm.dev_attr.attr,
        &sensor_dev_attr_in25_alarm.dev_attr.attr,
        &sensor_dev_attr_in26_alarm.dev_attr.attr,
        &sensor_dev_attr_in27_alarm.dev_attr.attr,
        &sensor_dev_attr_in28_alarm.dev_attr.attr,


        NULL
};

static const struct attribute_group smf_vsen_group = {
        .attrs = smf_vsen_attrs,
        .is_visible = smf_vsen_is_visible,
};

/* CURRENT ATTR */
static ssize_t
show_current_label(struct device *dev, struct device_attribute *attr,
         char *buf)
{
        struct smf_data *data = dev_get_drvdata(dev);
        int nr = to_sensor_dev_attr(attr)->index;
        return sprintf(buf, "%s\n", data->curr_label[nr]);
}

static ssize_t show_current(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
        int index = to_sensor_dev_attr(devattr)->index;
        struct smf_data *data = dev_get_drvdata(dev);
        int ret=0;
        int curr;

        if (index < CURR22_INPUT)
                if (data->kind == s6100smf)
                        ret = smf_read_reg16(data, SWITCH_CURRENT_S6100 + index * 2);
                else
                        ret = smf_read_reg16(data, SWITCH_CURRENT_Z9100 + index * 2);
        else if (index < CURR602_INPUT)
                ret = smf_read_reg16(data, PSU_1_INPUT_CURRENT + (index % 2) * 2);
        else
                ret = smf_read_reg16(data, PSU_2_INPUT_CURRENT + (index % 4) * 2);


        if (ret < 0)
                return ret;

        curr = ret*10;

        return sprintf(buf, "%d\n", curr);
}


static umode_t smf_curr_is_visible(struct kobject *kobj,
                struct attribute *a, int n)
{
        struct device *dev = container_of(kobj, struct device, kobj);
        struct smf_data *data = dev_get_drvdata(dev);

        if (data->curr_mask & (1 << (n % CURR_MAX)))
                return a->mode;
        return 0;
}


static SENSOR_DEVICE_ATTR(curr21_input, S_IRUGO, show_current, NULL, 0);
static SENSOR_DEVICE_ATTR(curr22_input, S_IRUGO, show_current, NULL, 1);

static SENSOR_DEVICE_ATTR(curr601_input, S_IRUGO, show_current, NULL, 2);
static SENSOR_DEVICE_ATTR(curr602_input, S_IRUGO, show_current, NULL, 3);

static SENSOR_DEVICE_ATTR(curr701_input, S_IRUGO, show_current, NULL, 4);
static SENSOR_DEVICE_ATTR(curr702_input, S_IRUGO, show_current, NULL, 5);

static SENSOR_DEVICE_ATTR(curr21_label, S_IRUGO, show_current_label, NULL, 0);
static SENSOR_DEVICE_ATTR(curr22_label, S_IRUGO, show_current_label, NULL, 1);

static SENSOR_DEVICE_ATTR(curr601_label, S_IRUGO, show_current_label, NULL, 2);
static SENSOR_DEVICE_ATTR(curr602_label, S_IRUGO, show_current_label, NULL, 3);

static SENSOR_DEVICE_ATTR(curr701_label, S_IRUGO, show_current_label, NULL, 4);
static SENSOR_DEVICE_ATTR(curr702_label, S_IRUGO, show_current_label, NULL, 5);


static struct attribute *smf_curr_attrs[] = {
        &sensor_dev_attr_curr21_input.dev_attr.attr,
        &sensor_dev_attr_curr22_input.dev_attr.attr,

        &sensor_dev_attr_curr601_input.dev_attr.attr,
        &sensor_dev_attr_curr602_input.dev_attr.attr,

        &sensor_dev_attr_curr701_input.dev_attr.attr,
        &sensor_dev_attr_curr702_input.dev_attr.attr,

        &sensor_dev_attr_curr21_label.dev_attr.attr,
        &sensor_dev_attr_curr22_label.dev_attr.attr,

        &sensor_dev_attr_curr601_label.dev_attr.attr,
        &sensor_dev_attr_curr602_label.dev_attr.attr,

        &sensor_dev_attr_curr701_label.dev_attr.attr,
        &sensor_dev_attr_curr702_label.dev_attr.attr,

        NULL
};


static const struct attribute_group smf_curr_group = {
        .attrs = smf_curr_attrs,
        .is_visible = smf_curr_is_visible,
};


/* CPU_TEMP ATTR */
static ssize_t
show_temp_label(struct device *dev, struct device_attribute *attr, char *buf)
{
        struct smf_data *data = dev_get_drvdata(dev);
        int nr = to_sensor_dev_attr(attr)->index;
        return sprintf(buf, "%s\n", data->temp_label[nr]);
}


static ssize_t show_tcpu(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
        int index = to_sensor_dev_attr(devattr)->index;
        struct smf_data *data = dev_get_drvdata(dev);
        int ret;
        int temp;

        if (index < TEMP13_INPUT)               /* Temp sensors */
                ret = smf_read_reg16(data, TEMP_SENSOR_1 + index * 2);
        else if(index < TEMP601_INPUT)
                ret = smf_read_reg16(data, PSU_1_TEMPERATURE);
        else
                ret = smf_read_reg16(data, PSU_2_TEMPERATURE);

        if (ret < 0)
                return ret;

	if (ret > 65500)
		ret = 0;

        if (ret & 0x8000)
                ret = - (ret & 0x7fff);
	
	temp = ret*100;

        return sprintf(buf, "%d\n", temp);
}


static ssize_t show_temp_crit(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
        int index = to_sensor_dev_attr(devattr)->index;
        struct smf_data *data = dev_get_drvdata(dev);
        int ret;
        int temp;

        ret = smf_read_reg16(data, TEMP_SENSOR_1_HW_LIMIT + index * 2);
        if (ret < 0)
                return ret;

	if (ret == 65535)
		ret = 0;

        if (ret & 0x8000)
                ret = - (ret & 0x7fff);

        temp = ret*100;

        return sprintf(buf, "%d\n", temp);
}


static ssize_t show_temp_alarm(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
        int index = to_sensor_dev_attr(devattr)->index;
        struct smf_data *data = dev_get_drvdata(dev);
        int ret = 0;
        int temp = 0;

        ret = smf_read_reg(data, TEMP_SENSOR_1_STATUS + index);

        if (ret < 0) {
            return ret;
        }

        if (ret == 0xff) {
            ret = 0;
        }

        temp = ret;

        return sprintf(buf, "%d\n", temp);
}


static umode_t smf_tcpu_is_visible(struct kobject *kobj,
                struct attribute *a, int n)
{
        struct device *dev = container_of(kobj, struct device, kobj);
        struct smf_data *data = dev_get_drvdata(dev);

        if (data->tcpu_mask & (1 << (n % TCPU_MAX)))
                return a->mode;

        return 0;
}


static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_tcpu, NULL, 0);
static SENSOR_DEVICE_ATTR(temp2_input, S_IRUGO, show_tcpu, NULL, 1);
static SENSOR_DEVICE_ATTR(temp3_input, S_IRUGO, show_tcpu, NULL, 2);
static SENSOR_DEVICE_ATTR(temp4_input, S_IRUGO, show_tcpu, NULL, 3);
static SENSOR_DEVICE_ATTR(temp5_input, S_IRUGO, show_tcpu, NULL, 4);
static SENSOR_DEVICE_ATTR(temp6_input, S_IRUGO, show_tcpu, NULL, 5);
static SENSOR_DEVICE_ATTR(temp7_input, S_IRUGO, show_tcpu, NULL, 6);
static SENSOR_DEVICE_ATTR(temp8_input, S_IRUGO, show_tcpu, NULL, 7);
static SENSOR_DEVICE_ATTR(temp9_input, S_IRUGO, show_tcpu, NULL, 8);
static SENSOR_DEVICE_ATTR(temp10_input, S_IRUGO, show_tcpu, NULL, 9);
static SENSOR_DEVICE_ATTR(temp11_input, S_IRUGO, show_tcpu, NULL, 10);
static SENSOR_DEVICE_ATTR(temp12_input, S_IRUGO, show_tcpu, NULL, 11);
static SENSOR_DEVICE_ATTR(temp13_input, S_IRUGO, show_tcpu, NULL, 12);

/* PSU1 Fan Temp */
static SENSOR_DEVICE_ATTR(temp14_input, S_IRUGO, show_tcpu, NULL, 13);

/* PSU2 Fan Temp */
static SENSOR_DEVICE_ATTR(temp15_input, S_IRUGO, show_tcpu, NULL, 14);

static SENSOR_DEVICE_ATTR(temp1_label, S_IRUGO, show_temp_label, NULL, 0);
static SENSOR_DEVICE_ATTR(temp2_label, S_IRUGO, show_temp_label, NULL, 1);
static SENSOR_DEVICE_ATTR(temp3_label, S_IRUGO, show_temp_label, NULL, 2);
static SENSOR_DEVICE_ATTR(temp4_label, S_IRUGO, show_temp_label, NULL, 3);
static SENSOR_DEVICE_ATTR(temp5_label, S_IRUGO, show_temp_label, NULL, 4);
static SENSOR_DEVICE_ATTR(temp6_label, S_IRUGO, show_temp_label, NULL, 5);
static SENSOR_DEVICE_ATTR(temp7_label, S_IRUGO, show_temp_label, NULL, 6);
static SENSOR_DEVICE_ATTR(temp8_label, S_IRUGO, show_temp_label, NULL, 7);
static SENSOR_DEVICE_ATTR(temp9_label, S_IRUGO, show_temp_label, NULL, 8);
static SENSOR_DEVICE_ATTR(temp10_label, S_IRUGO, show_temp_label, NULL, 9);
static SENSOR_DEVICE_ATTR(temp11_label, S_IRUGO, show_temp_label, NULL, 10);
static SENSOR_DEVICE_ATTR(temp12_label, S_IRUGO, show_temp_label, NULL, 11);
static SENSOR_DEVICE_ATTR(temp13_label, S_IRUGO, show_temp_label, NULL, 12);

static SENSOR_DEVICE_ATTR(temp14_label, S_IRUGO, show_temp_label, NULL, 14);
static SENSOR_DEVICE_ATTR(temp15_label, S_IRUGO, show_temp_label, NULL, 15);


static SENSOR_DEVICE_ATTR(temp1_crit,  S_IRUGO, show_temp_crit, NULL, 1);
static SENSOR_DEVICE_ATTR(temp2_crit,  S_IRUGO, show_temp_crit, NULL, 5);
static SENSOR_DEVICE_ATTR(temp3_crit,  S_IRUGO, show_temp_crit, NULL, 9);
static SENSOR_DEVICE_ATTR(temp4_crit,  S_IRUGO, show_temp_crit, NULL, 13);
static SENSOR_DEVICE_ATTR(temp5_crit,  S_IRUGO, show_temp_crit, NULL, 17);
static SENSOR_DEVICE_ATTR(temp6_crit,  S_IRUGO, show_temp_crit, NULL, 21);
static SENSOR_DEVICE_ATTR(temp7_crit,  S_IRUGO, show_temp_crit, NULL, 25);
static SENSOR_DEVICE_ATTR(temp8_crit,  S_IRUGO, show_temp_crit, NULL, 29);
static SENSOR_DEVICE_ATTR(temp9_crit,  S_IRUGO, show_temp_crit, NULL, 33);
static SENSOR_DEVICE_ATTR(temp10_crit, S_IRUGO, show_temp_crit, NULL, 37);
static SENSOR_DEVICE_ATTR(temp11_crit, S_IRUGO, show_temp_crit, NULL, 41);
static SENSOR_DEVICE_ATTR(temp12_crit, S_IRUGO, show_temp_crit, NULL, 45);
static SENSOR_DEVICE_ATTR(temp13_crit, S_IRUGO, show_temp_crit, NULL, 49);

static SENSOR_DEVICE_ATTR(temp14_crit, S_IRUGO, show_temp_crit, NULL, 11);
static SENSOR_DEVICE_ATTR(temp15_crit, S_IRUGO, show_temp_crit, NULL, 11);


static SENSOR_DEVICE_ATTR(temp1_max,        S_IRUGO, show_temp_crit, NULL, 2);
static SENSOR_DEVICE_ATTR(temp2_max,        S_IRUGO, show_temp_crit, NULL, 6);
static SENSOR_DEVICE_ATTR(temp3_max,        S_IRUGO, show_temp_crit, NULL, 10);
static SENSOR_DEVICE_ATTR(temp4_max,        S_IRUGO, show_temp_crit, NULL, 14);
static SENSOR_DEVICE_ATTR(temp5_max,        S_IRUGO, show_temp_crit, NULL, 18);
static SENSOR_DEVICE_ATTR(temp6_max,        S_IRUGO, show_temp_crit, NULL, 22);
static SENSOR_DEVICE_ATTR(temp7_max,        S_IRUGO, show_temp_crit, NULL, 26);
static SENSOR_DEVICE_ATTR(temp8_max,        S_IRUGO, show_temp_crit, NULL, 30);
static SENSOR_DEVICE_ATTR(temp9_max,        S_IRUGO, show_temp_crit, NULL, 34);
static SENSOR_DEVICE_ATTR(temp10_max,       S_IRUGO, show_temp_crit, NULL, 38);
static SENSOR_DEVICE_ATTR(temp11_max,       S_IRUGO, show_temp_crit, NULL, 42);
static SENSOR_DEVICE_ATTR(temp12_max,       S_IRUGO, show_temp_crit, NULL, 46);
static SENSOR_DEVICE_ATTR(temp13_max,       S_IRUGO, show_temp_crit, NULL, 50);

static SENSOR_DEVICE_ATTR(temp14_max,       S_IRUGO, show_temp_crit, NULL, 46);
static SENSOR_DEVICE_ATTR(temp15_max,       S_IRUGO, show_temp_crit, NULL, 50);

static SENSOR_DEVICE_ATTR(temp1_alarm,      S_IRUGO, show_temp_alarm, NULL, 0);
static SENSOR_DEVICE_ATTR(temp2_alarm,      S_IRUGO, show_temp_alarm, NULL, 1);
static SENSOR_DEVICE_ATTR(temp3_alarm,      S_IRUGO, show_temp_alarm, NULL, 2);
static SENSOR_DEVICE_ATTR(temp4_alarm,      S_IRUGO, show_temp_alarm, NULL, 3);
static SENSOR_DEVICE_ATTR(temp5_alarm,      S_IRUGO, show_temp_alarm, NULL, 4);
static SENSOR_DEVICE_ATTR(temp6_alarm,      S_IRUGO, show_temp_alarm, NULL, 5);
static SENSOR_DEVICE_ATTR(temp7_alarm,      S_IRUGO, show_temp_alarm, NULL, 6);
static SENSOR_DEVICE_ATTR(temp8_alarm,      S_IRUGO, show_temp_alarm, NULL, 7);
static SENSOR_DEVICE_ATTR(temp9_alarm,      S_IRUGO, show_temp_alarm, NULL, 8);
static SENSOR_DEVICE_ATTR(temp10_alarm,     S_IRUGO, show_temp_alarm, NULL, 9);
static SENSOR_DEVICE_ATTR(temp11_alarm,     S_IRUGO, show_temp_alarm, NULL, 10);
static SENSOR_DEVICE_ATTR(temp12_alarm,     S_IRUGO, show_temp_alarm, NULL, 11);
static SENSOR_DEVICE_ATTR(temp13_alarm,     S_IRUGO, show_temp_alarm, NULL, 12);

static SENSOR_DEVICE_ATTR(temp14_alarm,     S_IRUGO, show_temp_alarm, NULL, 13);
static SENSOR_DEVICE_ATTR(temp15_alarm,     S_IRUGO, show_temp_alarm, NULL, 14);




static struct attribute *smf_tcpu_attrs[] = {
        &sensor_dev_attr_temp1_input.dev_attr.attr,
        &sensor_dev_attr_temp2_input.dev_attr.attr,
        &sensor_dev_attr_temp3_input.dev_attr.attr,
        &sensor_dev_attr_temp4_input.dev_attr.attr,
        &sensor_dev_attr_temp5_input.dev_attr.attr,
        &sensor_dev_attr_temp6_input.dev_attr.attr,
        &sensor_dev_attr_temp7_input.dev_attr.attr,
        &sensor_dev_attr_temp8_input.dev_attr.attr,
        &sensor_dev_attr_temp9_input.dev_attr.attr,
        &sensor_dev_attr_temp10_input.dev_attr.attr,
        &sensor_dev_attr_temp11_input.dev_attr.attr,
        &sensor_dev_attr_temp12_input.dev_attr.attr,
        &sensor_dev_attr_temp13_input.dev_attr.attr,
        &sensor_dev_attr_temp14_input.dev_attr.attr,
        &sensor_dev_attr_temp15_input.dev_attr.attr,

        &sensor_dev_attr_temp1_label.dev_attr.attr,
        &sensor_dev_attr_temp2_label.dev_attr.attr,
        &sensor_dev_attr_temp3_label.dev_attr.attr,
        &sensor_dev_attr_temp4_label.dev_attr.attr,
        &sensor_dev_attr_temp5_label.dev_attr.attr,
        &sensor_dev_attr_temp6_label.dev_attr.attr,
        &sensor_dev_attr_temp7_label.dev_attr.attr,
        &sensor_dev_attr_temp8_label.dev_attr.attr,
        &sensor_dev_attr_temp9_label.dev_attr.attr,
        &sensor_dev_attr_temp10_label.dev_attr.attr,
        &sensor_dev_attr_temp11_label.dev_attr.attr,
        &sensor_dev_attr_temp12_label.dev_attr.attr,
        &sensor_dev_attr_temp13_label.dev_attr.attr,
        &sensor_dev_attr_temp14_label.dev_attr.attr,
        &sensor_dev_attr_temp15_label.dev_attr.attr,

        &sensor_dev_attr_temp1_crit.dev_attr.attr,
        &sensor_dev_attr_temp2_crit.dev_attr.attr,
        &sensor_dev_attr_temp3_crit.dev_attr.attr,
        &sensor_dev_attr_temp4_crit.dev_attr.attr,     
        &sensor_dev_attr_temp5_crit.dev_attr.attr,
        &sensor_dev_attr_temp6_crit.dev_attr.attr,
        &sensor_dev_attr_temp7_crit.dev_attr.attr,
        &sensor_dev_attr_temp8_crit.dev_attr.attr,     
        &sensor_dev_attr_temp9_crit.dev_attr.attr,
        &sensor_dev_attr_temp10_crit.dev_attr.attr,     
        &sensor_dev_attr_temp11_crit.dev_attr.attr,     
        &sensor_dev_attr_temp12_crit.dev_attr.attr,     
        &sensor_dev_attr_temp13_crit.dev_attr.attr,     
        &sensor_dev_attr_temp14_crit.dev_attr.attr,     
        &sensor_dev_attr_temp15_crit.dev_attr.attr,     

        &sensor_dev_attr_temp1_max.dev_attr.attr,
        &sensor_dev_attr_temp2_max.dev_attr.attr,
        &sensor_dev_attr_temp3_max.dev_attr.attr,
        &sensor_dev_attr_temp4_max.dev_attr.attr,
        &sensor_dev_attr_temp5_max.dev_attr.attr,
        &sensor_dev_attr_temp6_max.dev_attr.attr,
        &sensor_dev_attr_temp7_max.dev_attr.attr,
        &sensor_dev_attr_temp8_max.dev_attr.attr,
        &sensor_dev_attr_temp9_max.dev_attr.attr,
        &sensor_dev_attr_temp10_max.dev_attr.attr,
        &sensor_dev_attr_temp11_max.dev_attr.attr,
        &sensor_dev_attr_temp12_max.dev_attr.attr,
        &sensor_dev_attr_temp13_max.dev_attr.attr,
        &sensor_dev_attr_temp14_max.dev_attr.attr,
        &sensor_dev_attr_temp15_max.dev_attr.attr,

        &sensor_dev_attr_temp1_alarm.dev_attr.attr,
        &sensor_dev_attr_temp2_alarm.dev_attr.attr,
        &sensor_dev_attr_temp3_alarm.dev_attr.attr,
        &sensor_dev_attr_temp4_alarm.dev_attr.attr,
        &sensor_dev_attr_temp5_alarm.dev_attr.attr,
        &sensor_dev_attr_temp6_alarm.dev_attr.attr,
        &sensor_dev_attr_temp7_alarm.dev_attr.attr,
        &sensor_dev_attr_temp8_alarm.dev_attr.attr,
        &sensor_dev_attr_temp9_alarm.dev_attr.attr,
        &sensor_dev_attr_temp10_alarm.dev_attr.attr,
        &sensor_dev_attr_temp11_alarm.dev_attr.attr,
        &sensor_dev_attr_temp12_alarm.dev_attr.attr,
        &sensor_dev_attr_temp13_alarm.dev_attr.attr,
        &sensor_dev_attr_temp14_alarm.dev_attr.attr,
        &sensor_dev_attr_temp15_alarm.dev_attr.attr,


        NULL
};  


static const struct attribute_group smf_tcpu_group = {
        .attrs = smf_tcpu_attrs,
        .is_visible = smf_tcpu_is_visible,
};


/* PSU ATTR */
static ssize_t
show_psu_label(struct device *dev, struct device_attribute *attr, char *buf)
{
        struct smf_data *data = dev_get_drvdata(dev);
        int nr = to_sensor_dev_attr(attr)->index;
        return sprintf(buf, "%s\n", data->psu_label[nr]);
}


static ssize_t show_psu(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
        int index = to_sensor_dev_attr(devattr)->index;
        struct smf_data *data = dev_get_drvdata(dev);
        int ret=0, export_hex=0;
        int psu_status=0, pow;
        int pow_val = 0;

        switch (index) {

                case 0:
                        pow = smf_read_reg16(data, PSU_1_MAX_POWER);
                        if (data->kind == s6100smf)
                            ret = 1000000 * 1100;
                        else
                            ret = 1000000 * 750;
                        break;
                case 1:
                        ret = smf_read_reg(data, PSU_1_STATUS);
                        export_hex=1;
                        break;
                case 2:
                        pow_val = smf_read_reg16(data, PSU_1_INPUT_POWER);
                        /* In case of absent psu, pow_val will be 0xffff */
                        if (pow_val == 0xffff) {
                            pow_val = 0;
                        }
                        ret = 100000 * pow_val;
                        break;
                case 3:
                        pow_val = smf_read_reg16(data, PSU_1_OUTPUT_POWER);
                        /* In case of absent psu, pow_val will be 0xffff */
                        if (pow_val == 0xffff) {
                            pow_val = 0;
                        }
                        ret = 100000 * pow_val;
                        break;
                case 4:
                        psu_status = smf_read_reg(data, PSU_1_STATUS);
                        if (psu_status &(1))
                                ret=1;
                        break;
                case 5:
                        pow = smf_read_reg16(data, PSU_2_MAX_POWER);
                        ret = 1000000 * pow;
                        if (data->kind == s6100smf)
                            ret = 1000000 * 1100;
                        else
                            ret = 1000000 * 750;
                        break;
                case 6:
                        ret = smf_read_reg(data, PSU_2_STATUS);
                        export_hex=1;
                        break;
                case 7:
                        pow_val = smf_read_reg16(data, PSU_2_INPUT_POWER);
                        /* In case of absent psu, pow_val will be 0xffff */
                        if (pow_val == 0xffff) {
                            pow_val = 0;
                        }
                        ret = 100000 * pow_val;
                        break;
                case 8:
                        pow_val = smf_read_reg16(data, PSU_2_OUTPUT_POWER);
                        /* In case of absent psu, pow_val will be 0xffff */
                        if (pow_val == 0xffff) {
                            pow_val = 0;
                        }
                        ret = 100000 * pow_val;
                        break;
                case 9:
                        psu_status = smf_read_reg(data, PSU_2_STATUS);
                        if (psu_status &(1))
                                ret=1;
                        break;
                case 10:
                        pow = smf_read_reg16(data, CURRENT_TOTAL_POWER);
                        /* In case of both psu absent, pow will be 0xffff */
                        if (pow == 0xffff) {
                            pow = 0;
                        }
                        ret = pow/10;
                        break;
                case 11:
                        psu_status = smf_read_reg(data, PSU_1_STATUS);
                        if (psu_status &(2))
                                ret=1;
                        break;
                case 12:
                        psu_status = smf_read_reg(data, PSU_2_STATUS);
                        if (psu_status &(2))
                                ret=1;
                        break;
                default:
                        return ret;
        }

        if (ret < 0)
                return ret;

        pow = ret;

        if(export_hex)
                return sprintf(buf, "%x\n", pow);
        else
                return sprintf(buf, "%u\n", pow);
}

/* FAN and PSU EEPROM PPID format is:
   COUNTRY_CODE-PART_NO-MFG_ID-MFG_DATE_CODE-SERIAL_NO-LABEL_REV */
static ssize_t show_ppid(struct device *dev,
                struct device_attribute *devattr, char *buf)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct smf_data *data = dev_get_drvdata(dev);
	char ppid[EEPROM_PPID_SIZE + 1] = {0};
	char psu_mfg_date[EEPROM_MFG_DATE_SIZE + 1] = {0};
	char psu_mfg_date_code[EEPROM_DATE_CODE_SIZE + 1] = {0};
	char temp;
	int i, reg, ret = 0, ppid_pos = 0;

	switch(index) {

        /* PPID starts from Country Code*/
        case 0:
            reg = FAN_1_SERIAL_CODE;
            break;
        case 1:
            reg = FAN_2_SERIAL_CODE;
            break;
        case 2:
            reg = FAN_3_SERIAL_CODE;
            break;
        case 3:
            reg = FAN_4_SERIAL_CODE;
            break;
        case 4:
            reg = FAN_5_SERIAL_CODE;
            break;
        case 10:
            reg = PSU_1_COUNTRY_CODE;
            break;
        case 11:
            reg = PSU_2_COUNTRY_CODE;
            break;
        default:
            return ret;
}

	// Get Country Code
	for( i = 0; i < EEPROM_COUNTRY_CODE_SIZE; i++) {
		ppid[ppid_pos++] = (char)smf_read_reg(data,reg++);
	}
	ppid[ppid_pos++] = '-';

	// Get Part Number
	for( i = 0; i < EEPROM_PART_NO_SIZE; i++) {
		ppid[ppid_pos++] = (char)smf_read_reg(data,reg++);
	}
	ppid[ppid_pos++] = '-';

	// Get Manufacture ID
	for( i = 0; i < EEPROM_MFG_ID_SIZE; i++) {
		ppid[ppid_pos++] = (char)smf_read_reg(data,reg++);
	}
	ppid[ppid_pos++] = '-';
	if(index > 9){   //Applicable only for PSU 
		// Get Manufacture date
		for( i = 0; i < EEPROM_MFG_DATE_SIZE; i++) {
			psu_mfg_date[i] = (char)smf_read_reg(data,reg++);
		}

		/* Converting 6 digit date code [yymmdd] to 3 digit[ymd]  
        	   Year  Starting from 2010 [0-9] , Day :  1-9 and A-V , Month : 1-9 and A-C */
		// Year Validation and Conversion
		if( ( psu_mfg_date[0] == '1' ) && ( psu_mfg_date[1] >= '0' ) && ( psu_mfg_date[1] <= '9') ) 
		{
			psu_mfg_date_code[0] = psu_mfg_date[1];      
		}
		else
		{
			psu_mfg_date_code[0] = ' ';
		}
	
		// Month Validation and Conversion 
		temp = ( ( psu_mfg_date[2] - 0x30 ) * 10 ) + ( psu_mfg_date[3] - 0x30 );
		if( ( temp >= 1) && ( temp < 10) ) 
		{
			psu_mfg_date_code[1] = temp + 0x30; // 0- 9
		}
		else if ( ( temp >= 10) && ( temp <= 12) )
		{
			psu_mfg_date_code[1] = temp + 0x37; // A-C 
		}
		else
		{
			psu_mfg_date_code[1]= ' ';
		}

		// Date	Validation and Conversion
		temp = ( ( psu_mfg_date[4] - 0x30 ) * 10 ) + ( psu_mfg_date[5] - 0x30 );
		if( ( temp >= 1) && ( temp < 10) )
		{   
			psu_mfg_date_code[2] = temp + 0x30; // 0- 9	
		}
		else if( ( temp >= 10) && ( temp <= 31) )
		{
			psu_mfg_date_code[2] = temp + 0x37; // A-V 
		}
		else
		{
			psu_mfg_date_code[2] = ' ';
		}
		for( i = 0; i < EEPROM_DATE_CODE_SIZE; i++) {
			ppid[ppid_pos++] = psu_mfg_date_code[i];
		}
	}else{
		for( i = 0; i < EEPROM_DATE_CODE_SIZE; i++) {
			ppid[ppid_pos++] = (char)smf_read_reg(data,reg++);
		}
	}

	ppid[ppid_pos++] = '-';

	// Get Serial Number
	for( i = 0; i < EEPROM_SERIAL_NO_SIZE; i++) {
		ppid[ppid_pos++] = (char)smf_read_reg(data,reg++);
	}
	ppid[ppid_pos++] = '-';

	if(index > 9){
	// Skipping PSU service tag in PPID 
		reg += EEPROM_SERVICE_TAG_SIZE;
    }
	else{
	// Skipping FAN partno tag in PPID 
		reg += EEPROM_PART_NO_SIZE;
    }

	// Get Label Revision
	for( i = 0; i < EEPROM_LABEL_REV_SIZE; i++) {
		ppid[ppid_pos++] = (char)smf_read_reg(data,reg++);
	}

	return sprintf(buf, "%s\n",ppid);
}

static umode_t smf_psu_is_visible(struct kobject *kobj,
                struct attribute *a, int n)
{
        struct device *dev = container_of(kobj, struct device, kobj);
        struct smf_data *data = dev_get_drvdata(dev);
        
        if (data->psu_mask & (1 << (n % PSU_MAX)))
                return a->mode;
        return 0;
}

/* PSU */
static SENSOR_DEVICE_ATTR(power1_input, S_IRUGO, show_psu, NULL, 2);
static SENSOR_DEVICE_ATTR(power2_input, S_IRUGO, show_psu, NULL, 3);
static SENSOR_DEVICE_ATTR(power3_input, S_IRUGO, show_psu, NULL, 7);
static SENSOR_DEVICE_ATTR(power4_input, S_IRUGO, show_psu, NULL, 8);

static SENSOR_DEVICE_ATTR(power1_label, S_IRUGO, show_psu_label, NULL, 0);
static SENSOR_DEVICE_ATTR(power2_label, S_IRUGO, show_psu_label, NULL, 1);
static SENSOR_DEVICE_ATTR(power3_label, S_IRUGO, show_psu_label, NULL, 2);
static SENSOR_DEVICE_ATTR(power4_label, S_IRUGO, show_psu_label, NULL, 3);

static SENSOR_DEVICE_ATTR(power1_max, S_IRUGO, show_psu, NULL, 0);
static SENSOR_DEVICE_ATTR(power2_max, S_IRUGO, show_psu, NULL, 0);
static SENSOR_DEVICE_ATTR(power3_max, S_IRUGO, show_psu, NULL, 5);
static SENSOR_DEVICE_ATTR(power4_max, S_IRUGO, show_psu, NULL, 5);


/* PSU2 */
//static SENSOR_DEVICE_ATTR(power602_alarm, S_IRUGO, show_psu, NULL, 4);
//static SENSOR_DEVICE_ATTR(power702_alarm, S_IRUGO, show_psu, NULL, 9);



static struct attribute *smf_psu_attrs[] = {

        &sensor_dev_attr_power1_input.dev_attr.attr,
        &sensor_dev_attr_power2_input.dev_attr.attr,
        &sensor_dev_attr_power3_input.dev_attr.attr,
        &sensor_dev_attr_power4_input.dev_attr.attr,

        &sensor_dev_attr_power1_label.dev_attr.attr,
        &sensor_dev_attr_power2_label.dev_attr.attr,
        &sensor_dev_attr_power3_label.dev_attr.attr,
        &sensor_dev_attr_power4_label.dev_attr.attr,

        &sensor_dev_attr_power1_max.dev_attr.attr,
        &sensor_dev_attr_power2_max.dev_attr.attr,
        &sensor_dev_attr_power3_max.dev_attr.attr,
        &sensor_dev_attr_power4_max.dev_attr.attr,

        NULL
};


static const struct attribute_group smf_psu_group = {
        .attrs = smf_psu_attrs,
        .is_visible = smf_psu_is_visible,
};


static const struct attribute_group smf_fanin_group = {
        .attrs = smf_fanin_attrs,
        .is_visible = smf_fanin_is_visible,
};


static SENSOR_DEVICE_ATTR(fan_tray_presence, S_IRUGO, show_fan, NULL, 12);
static SENSOR_DEVICE_ATTR(fan1_airflow, S_IRUGO, show_fan_airflow, NULL, 0);
static SENSOR_DEVICE_ATTR(fan3_airflow, S_IRUGO, show_fan_airflow, NULL, 1);
static SENSOR_DEVICE_ATTR(fan5_airflow, S_IRUGO, show_fan_airflow, NULL, 2);
static SENSOR_DEVICE_ATTR(fan7_airflow, S_IRUGO, show_fan_airflow, NULL, 3);
static SENSOR_DEVICE_ATTR(fan9_airflow, S_IRUGO, show_fan_airflow, NULL, 4);
static SENSOR_DEVICE_ATTR(fan11_airflow, S_IRUGO, show_psu_fan, NULL, 0);
static SENSOR_DEVICE_ATTR(fan12_airflow, S_IRUGO, show_psu_fan, NULL, 3);
static SENSOR_DEVICE_ATTR(fan1_serialno, S_IRUGO, show_ppid, NULL, 0);
static SENSOR_DEVICE_ATTR(fan3_serialno, S_IRUGO, show_ppid, NULL, 1);
static SENSOR_DEVICE_ATTR(fan5_serialno, S_IRUGO, show_ppid, NULL, 2);
static SENSOR_DEVICE_ATTR(fan7_serialno, S_IRUGO, show_ppid, NULL, 3);
static SENSOR_DEVICE_ATTR(fan9_serialno, S_IRUGO, show_ppid, NULL, 4);
/* IOM status */
static SENSOR_DEVICE_ATTR(iom_status, S_IRUGO, show_voltage, NULL, 44);
static SENSOR_DEVICE_ATTR(iom_presence, S_IRUGO, show_voltage, NULL, 45);

static SENSOR_DEVICE_ATTR(cpu_iom1_control, S_IRUGO, show_cpu_iom_control, NULL, 0);
static SENSOR_DEVICE_ATTR(cpu_iom2_control, S_IRUGO, show_cpu_iom_control, NULL, 1);
static SENSOR_DEVICE_ATTR(cpu_iom3_control, S_IRUGO, show_cpu_iom_control, NULL, 2);
static SENSOR_DEVICE_ATTR(cpu_iom4_control, S_IRUGO, show_cpu_iom_control, NULL, 3);

static SENSOR_DEVICE_ATTR(psu1_presence, S_IRUGO, show_psu, NULL, 1);
static SENSOR_DEVICE_ATTR(psu2_presence, S_IRUGO, show_psu, NULL, 6);
static SENSOR_DEVICE_ATTR(psu1_serialno, S_IRUGO, show_ppid, NULL, 10);
static SENSOR_DEVICE_ATTR(psu2_serialno, S_IRUGO, show_ppid, NULL, 11);
static SENSOR_DEVICE_ATTR(psu1_type, S_IRUGO, show_psu, NULL, 11);
static SENSOR_DEVICE_ATTR(psu2_type, S_IRUGO, show_psu, NULL, 12);
static SENSOR_DEVICE_ATTR(current_total_power, S_IRUGO, show_psu, NULL, 10);

/* SMF Version */
static SENSOR_DEVICE_ATTR(smf_version, S_IRUGO, show_smf_version, NULL, 0);
static SENSOR_DEVICE_ATTR(smf_firmware_ver, S_IRUGO, show_smf_version, NULL, 1);

/* SMF Reset Reason */
static SENSOR_DEVICE_ATTR(smf_reset_reason, S_IRUGO, show_reset_reason, NULL, 1);

/* SMF PowerOn Reason */
static SENSOR_DEVICE_ATTR(smf_poweron_reason, S_IRUGO,
                                            show_power_on_reason, NULL, 1);

/* Mailbox Power tracking Reason */
static SENSOR_DEVICE_ATTR(mb_poweron_reason, S_IRUGO|S_IWUSR,
                            show_mb_poweron_reason, set_mb_poweron_reason, 0);

/* System Status LED */
static SENSOR_DEVICE_ATTR(sys_status_led, S_IRUGO|S_IWUSR,
                            show_sys_status_led, set_sys_status_led, 0);

static struct attribute *smf_dell_attrs[] = {
        &sensor_dev_attr_smf_version.dev_attr.attr,
        &sensor_dev_attr_smf_firmware_ver.dev_attr.attr,
        &sensor_dev_attr_smf_reset_reason.dev_attr.attr,
        &sensor_dev_attr_smf_poweron_reason.dev_attr.attr,
        &sensor_dev_attr_mb_poweron_reason.dev_attr.attr,
        &sensor_dev_attr_sys_status_led.dev_attr.attr,
        &sensor_dev_attr_fan_tray_presence.dev_attr.attr,
        &sensor_dev_attr_fan1_airflow.dev_attr.attr,
        &sensor_dev_attr_fan3_airflow.dev_attr.attr,
        &sensor_dev_attr_fan5_airflow.dev_attr.attr,
        &sensor_dev_attr_fan7_airflow.dev_attr.attr,
        &sensor_dev_attr_fan9_airflow.dev_attr.attr,
        &sensor_dev_attr_fan11_airflow.dev_attr.attr,
        &sensor_dev_attr_fan12_airflow.dev_attr.attr,
        &sensor_dev_attr_iom_status.dev_attr.attr,
        &sensor_dev_attr_iom_presence.dev_attr.attr,
        &sensor_dev_attr_psu1_presence.dev_attr.attr,
        &sensor_dev_attr_psu2_presence.dev_attr.attr,
        &sensor_dev_attr_psu1_serialno.dev_attr.attr,
        &sensor_dev_attr_psu2_serialno.dev_attr.attr,
        &sensor_dev_attr_fan1_serialno.dev_attr.attr,
        &sensor_dev_attr_fan3_serialno.dev_attr.attr,
        &sensor_dev_attr_fan5_serialno.dev_attr.attr,
        &sensor_dev_attr_fan7_serialno.dev_attr.attr,
        &sensor_dev_attr_fan9_serialno.dev_attr.attr,
        &sensor_dev_attr_current_total_power.dev_attr.attr,
        &sensor_dev_attr_cpu_iom1_control.dev_attr.attr,
        &sensor_dev_attr_cpu_iom2_control.dev_attr.attr,
        &sensor_dev_attr_cpu_iom3_control.dev_attr.attr,
        &sensor_dev_attr_cpu_iom4_control.dev_attr.attr,
        NULL
};

static const struct attribute_group smf_dell_group = {
        .attrs = smf_dell_attrs,
        .is_visible = smf_dell_is_visible,
};


static const struct attribute_group *smf_groups[] = {
        &smf_psu_group,
        &smf_fanin_group,
        &smf_vsen_group,
        &smf_curr_group,
        &smf_tcpu_group,
        &smf_dell_group,
        NULL
};



static int smf_probe(struct platform_device *pdev)
{
        struct smf_data *data;
        struct device *dev = &pdev->dev;
        struct smf_sio_data *sio_data = dev_get_platdata(dev);
        struct resource *res;
        int err = 0;

        res = platform_get_resource(pdev, IORESOURCE_IO, 0);
        if (!request_region(res->start, IOREGION_LENGTH, 
                                smf_devices[sio_data->kind].name)) {
                err = -EBUSY;
                dev_err(dev, "Failed to request region 0x%lx-0x%lx\n",
                                (unsigned long)res->start,
                                (unsigned long)res->start + IOREGION_LENGTH - 1);
                return err;
        }

        data = devm_kzalloc(dev, sizeof(struct smf_data), GFP_KERNEL);
        /* TODO Use probe address value */
        data->addr = SMF_PROBE_ADDR;
        data->kind = sio_data->kind;

        if (!data)
                return -ENOMEM;

        mutex_init(&data->lock); 

        /* PSU attributes */
        data->psu_mask = smf_devices[data->kind].psu_mask;
        data->psu_label = smf_devices[data->kind].psu_label;

        /* FANIN attributes */ 
        data->fanin_mask = smf_devices[data->kind].fanin_mask;
        data->fan_label = smf_devices[data->kind].fan_label;

        /* VSEN attributes */
        data->vsen_mask = smf_devices[data->kind].vsen_mask;
        data->vsen_label = smf_devices[data->kind].vsen_label;

        /* CURR attributes */
        data->curr_mask = smf_devices[data->kind].curr_mask;
        data->curr_label = smf_devices[data->kind].curr_label;

        /* CPU_TEMP attributes */
        data->tcpu_mask = smf_devices[data->kind].tcpu_mask;
        data->temp_label = smf_devices[data->kind].temp_label;

        data->hwmon_dev = devm_hwmon_device_register_with_groups(dev,
                        smf_devices[data->kind].name,
                        data, smf_groups);

        return PTR_ERR_OR_ZERO(data->hwmon_dev);
}


static void smf_remove(struct platform_device *pdev)
{
        struct resource *res;

        res = platform_get_resource(pdev, IORESOURCE_IO, 0);
        release_region(res->start, IOREGION_LENGTH);
}


static struct platform_driver smf_driver = {
        .driver = {
                .name = "SMF",
        },
        .probe = smf_probe,
        .remove = smf_remove
};

int __init
smf_find(int sioaddr, unsigned short *addr, struct smf_sio_data *sio_data)
{

        int val;

        if (force_id)
                val = force_id;
        else
                val = inb(sioaddr + SIO_REG_DEVID);

        switch (val) {
                case SIO_Z9100_ID:
                        sio_data->kind = z9100smf;
                        break;
                case SIO_S6100_ID:
                        sio_data->kind = s6100smf;
                        break;

                default:
                        if (val != 0xffff)
                                pr_debug("unsupported chip ID: 0x%04x\n", val);
                        return -ENODEV;
        }

        /* TODO Use define, should this be 0x200 or 0x210??? */
        *addr = sioaddr;

        pr_info("Found %s chip at %#x\n", smf_devices[sio_data->kind].name, *addr);
        sio_data->sioreg = sioaddr;

        return (0);
}


/*
 * when Super-I/O functions move to a separate file, the Super-I/O
 * bus will manage the lifetime of the device and this module will only keep
 * track of the smf driver. But since we platform_device_alloc(), we
 * must keep track of the device
 */
static struct platform_device *pdev;

static int __init sensors_smf_init(void)
{
        int err;
        unsigned short address;
        struct resource res;
        struct smf_sio_data sio_data;

        /*
         * initialize sio_data->kind and sio_data->sioreg.
         * when Super-I/O functions move to a separate file, the Super-I/O
         * driver will probe and auto-detect the presence of a
         * smf hardware monitor, and call probe()
         */

        if (smf_find(SMF_REG_ADDR, &address, &sio_data))
                return -ENODEV;

        err = platform_driver_register(&smf_driver);
        if (err)
                goto exit;

        pdev = platform_device_alloc(SIO_DRVNAME, address);
        if (!pdev) {
                err = -ENOMEM;
                pr_err("Device allocation failed\n");
                goto exit_unregister;
        }

        err = platform_device_add_data(pdev, &sio_data,
                        sizeof(struct smf_sio_data));
        if (err) {
                pr_err("Platform data allocation failed\n");
                goto exit_device_put;
        }

        memset(&res, 0, sizeof(res));
        res.name = SIO_DRVNAME;
        res.start = address + IOREGION_OFFSET;
        res.end = address + IOREGION_OFFSET + IOREGION_LENGTH - 1;
        res.flags = IORESOURCE_IO;

        err = acpi_check_resource_conflict(&res);
        if (err)
                goto exit_device_put;

        err = platform_device_add_resources(pdev, &res, 1);
        if (err) {
                pr_err("Device resource addition failed (%d)\n", err);
                goto exit_device_put;
        }

        /* platform_device_add calls probe() */
        err = platform_device_add(pdev);
        if (err) {
                pr_err("Device addition failed (%d)\n", err);
                goto exit_device_put;
        }

        return 0;

exit_device_put:
        platform_device_put(pdev);
exit_unregister:
        platform_driver_unregister(&smf_driver);
exit:
        return err;
}


static void __exit sensors_smf_exit(void)
{
        platform_device_unregister(pdev);
        platform_driver_unregister(&smf_driver);

        /*Remove sysfs dell_kobj*/
        kobject_put(dell_kobj);
}


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SMF driver");
MODULE_PARM_DESC(force_id, "Override the detected device ID");
MODULE_AUTHOR("Per Fremrot <per_fremrot@dell.com>");
MODULE_AUTHOR("Paavaanan <paavaanan_t_n@dell.com>");

module_init(sensors_smf_init);
module_exit(sensors_smf_exit);

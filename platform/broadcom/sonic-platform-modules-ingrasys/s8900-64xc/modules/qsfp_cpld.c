/*
 * S8900-64XC QSFP CPLD driver
 *
 * Copyright (C) 2017 Ingrasys, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/i2c-mux.h>
#include <linux/platform_data/i2c-mux-gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>


#ifdef DEBUG
    #define DEBUG_PRINT(fmt, args...)            \
        printk (KERN_INFO "%s[%d]: " fmt "\r\n", \
                __FUNCTION__, __LINE__, ##args)
#else
    #define DEBUG_PRINT(fmt, args...)
#endif

#define ERROR_MSG(fmt, args...)                  \
        printk(KERN_ERR "%s[%d]: " fmt "\r\n",   \
               __FUNCTION__, __LINE__, ##args)




#define SFF_8436_MMAP_SIZE                 (256)
#define EEPROM_SIZE                        (5 * 128) /*  640 byte eeprom */
#define EEPROM_PAGE_SIZE                   (128)
#define EEPROM_DEFAULT_PAGE                (0)
#define I2C_RW_RETRY_COUNT                 (3)
#define I2C_RW_RETRY_INTERVAL              (100)   /* ms */
#define USE_I2C_BLOCK_READ                 (1)

#define SFP_EEPROM_A0_I2C_ADDR             (0xA0 >> 1)

#define SFF_8436_PAGE_PROV_ADDR            (0xC3)    /* Memory Page 01/02 Provided */
#define SFF_8436_PAGE_01_PRESENT           (1 << 6)  /* Memory Page 01 present */
#define SFF_8436_PAGE_02_PRESENT           (1 << 7)  /* Memory Page 02 present */
#define SFF_8436_PAGE_SELECT_ADDR          (0x7F)
#define SFF_8436_STATUS_ADDR               (0x02)
#define SFF_8436_STATUS_PAGE_03_PRESENT_L  (1 << 2) /* Flat Memory:0-Paging, 1-Page 0 only */

#define S8900_64XC_MUX_BASE_NR             1
#define S8900_64XC_SFP_EEPROM_BASE_NR      2
#define CPLD_DEVICE_NUM                    3
#define SFP_CPLD_DEVICE_NUM                2
#define QSFP_EEPROM_DEVICE_NUM             3
#define TOTAL_PORT_NUM                     64
#define CPLD_MUX_OFFSET                    24
#define SFP_EEPROM_DEV_NUM                 3
#define SFP_EEPROM_NAME_LEN                16
/* CPLD registers */
#define CPLD_REG_REV 0x01
#define CPLD_REG_ID  0x02
#define CPLD_MUX_REG 0x4a

/* QSFP signal bit in register */
#define BIT_RST 0
#define BIT_LPM 2
#define BIT_INT 0
#define BIT_ABS 1
#define BIT_ABS_2 5

static ssize_t sfp_eeprom_read(struct i2c_client *, loff_t, u8 *,int);
static ssize_t sfp_eeprom_write(struct i2c_client *, loff_t, const char *,int);


enum port_numbers {
    sfp1,  sfp2,  sfp3,  sfp4,  sfp5,  sfp6,  sfp7,  sfp8,
    sfp9,  sfp10, sfp11, sfp12, sfp13, sfp14, sfp15, sfp16,
    sfp17, sfp18, sfp19, sfp20, sfp21, sfp22, sfp23, sfp24,
    sfp25, sfp26, sfp27, sfp28, sfp29, sfp30, sfp31, sfp32,
    sfp33, sfp34, sfp35, sfp36, sfp37, sfp38, sfp39, sfp40,
    sfp41, sfp42, sfp43, sfp44, sfp45, sfp46, sfp47, sfp48,
    qsfp49, qsfp50, qsfp51, qsfp52, qsfp53, qsfp54, qsfp55, qsfp56,
    qsfp57, qsfp58, qsfp59, qsfp60, qsfp61, qsfp62, qsfp63, qsfp64
};

static const struct platform_device_id qsfp_device_id[] = {
    {"sfp1",  sfp1},  {"sfp2",  sfp2},  {"sfp3",  sfp3},  {"sfp4",  sfp4},
    {"sfp5",  sfp5},  {"sfp6",  sfp6},  {"sfp7",  sfp7},  {"sfp8",  sfp8},
    {"sfp9",  sfp9},  {"sfp10", sfp10}, {"sfp11", sfp11}, {"sfp12", sfp12},
    {"sfp13", sfp13}, {"sfp14", sfp14}, {"sfp15", sfp15}, {"sfp16", sfp16},
    {"sfp17", sfp17}, {"sfp18", sfp18}, {"sfp19", sfp19}, {"sfp20", sfp20},
    {"sfp21", sfp21}, {"sfp22", sfp22}, {"sfp23", sfp23}, {"sfp24", sfp24},
    {"sfp25", sfp25}, {"sfp26", sfp26}, {"sfp27", sfp27}, {"sfp28", sfp28},
    {"sfp29", sfp29}, {"sfp30", sfp30}, {"sfp31", sfp31}, {"sfp32", sfp32},
    {"sfp33", sfp33}, {"sfp34", sfp34}, {"sfp35", sfp35}, {"sfp36", sfp36},
    {"sfp37", sfp37}, {"sfp38", sfp38}, {"sfp39", sfp39}, {"sfp40", sfp40},
    {"sfp41", sfp41}, {"sfp42", sfp42}, {"sfp43", sfp43}, {"sfp44", sfp44},
    {"sfp45", sfp45}, {"sfp46", sfp46}, {"sfp47", sfp47}, {"sfp48", sfp48},
    {"qsfp49", qsfp49}, {"qsfp50", qsfp50}, {"qsfp51", qsfp51}, {"qsfp52", qsfp52},
    {"qsfp53", qsfp53}, {"qsfp54", qsfp54}, {"qsfp55", qsfp55}, {"qsfp56", qsfp56},
    {"qsfp57", qsfp57}, {"qsfp58", qsfp58}, {"qsfp59", qsfp59}, {"qsfp60", qsfp60},
    {"qsfp61", qsfp61}, {"qsfp62", qsfp62}, {"qsfp63", qsfp63}, {"qsfp64", qsfp64},
    {}
};
MODULE_DEVICE_TABLE(platform, qsfp_device_id);

/*
 * list of valid port types
 * note OOM_PORT_TYPE_NOT_PRESENT to indicate no
 * module is present in this port
 */
typedef enum oom_driver_port_type_e {
   OOM_DRIVER_PORT_TYPE_INVALID,
   OOM_DRIVER_PORT_TYPE_NOT_PRESENT,
   OOM_DRIVER_PORT_TYPE_SFP,
   OOM_DRIVER_PORT_TYPE_SFP_PLUS,
   OOM_DRIVER_PORT_TYPE_QSFP,
   OOM_DRIVER_PORT_TYPE_QSFP_PLUS,
   OOM_DRIVER_PORT_TYPE_QSFP28
} oom_driver_port_type_t;

enum driver_type_e {
   DRIVER_TYPE_SFP_MSA,
   DRIVER_TYPE_SFP_DDM,
   DRIVER_TYPE_QSFP
};

typedef enum eeprom_operation_e {
    EEPROM_READ,
    EEPROM_WRITE
} eeprom_operation_t;

/*
 * Each client has this additional data
 */
struct eeprom_data {
   char valid;                   /* !=0 if registers are valid */
   unsigned long last_updated;   /* In jiffies */
   struct bin_attribute bin;     /* eeprom data */
};

struct qsfp_data {
    char valid;                  /* !=0 if registers are valid */
    unsigned long last_updated;  /* In jiffies */
    u8 status[3];                /* bit0:port0, bit1:port1 and so on */
                                 /* index 0 => tx_fail
                                          1 => tx_disable
                                          2 => rx_loss
                                  */
    u8 device_id;
    struct eeprom_data eeprom;
};

struct sfp_port_data {
    struct mutex update_lock;
    enum driver_type_e driver_type;
    int port;                           /* CPLD port index */
    oom_driver_port_type_t port_type;
    u64 present;                        /* present status, bit0:port0, bit1:port1 and so on */

    struct qsfp_data *qsfp;

    struct i2c_client *client;
};

enum sfp_sysfs_attributes {
    PRESENT,
    PRESENT_ALL,
    PORT_NUMBER,
    PORT_TYPE,
    DDM_IMPLEMENTED,
    TX_FAULT,
    TX_FAULT1,
    TX_FAULT2,
    TX_FAULT3,
    TX_FAULT4,
    TX_DISABLE,
    TX_DISABLE1,
    TX_DISABLE2,
    TX_DISABLE3,
    TX_DISABLE4,
    TX_DISABLE_ALL,
    RX_LOS,
    RX_LOS1,
    RX_LOS2,
    RX_LOS3,
    RX_LOS4,
    RX_LOS_ALL,
};

static void device_release(struct device *dev)
{
    return;
}

/*
 * S8900-64XC CPLD register addresses
 */
static const int int_abs_reg[CPLD_DEVICE_NUM][2]= {
    {0x20, 0x31},
    {0x20, 0x31},
    {0x20, 0x2F},
};

static const int rst_lp_reg[CPLD_DEVICE_NUM][2]= {
    {0xFF, 0xFF}, /*dummy*/
    {0xFF, 0xFF}, /*dummy*/
    {0x30, 0x3F},
};

/*
 * S8900-64XC CPLD
 */

enum cpld_type {
    cpld_1,
    cpld_2,
    cpld_3,
};

enum qsfp_signal {
    sig_int,
    sig_abs,
    sig_rst,
    sig_lpm
};

struct cpld_platform_data {
    int reg_addr;
    struct i2c_client *client;
};

struct sfp_platform_data {
    int reg_addr;
    int parent;
    int cpld_reg;
    struct i2c_client *client;
};

static struct cpld_platform_data s8900_64xc_cpld_platform_data[] = {
    [cpld_1] = {
        .reg_addr = 0x33,
    },

    [cpld_2] = {
        .reg_addr = 0x33,
    },

    [cpld_3] = {
        .reg_addr = 0x33,
    }
};

static struct sfp_platform_data s8900_64xc_sfp_platform_data[] = {
    [sfp1] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x01,
    },

    [sfp2] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x02,
    },

    [sfp3] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x03,
    },
    [sfp4] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x04,
    },

    [sfp5] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x05,
    },

    [sfp6] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x06,
    },
    [sfp7] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x07,
    },

    [sfp8] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x08,
    },

    [sfp9] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x09,
    },
    [sfp10] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x0A,
    },

    [sfp11] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x0B,
    },

    [sfp12] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x0C,
    },
    [sfp13] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x0D,
    },

    [sfp14] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x0E,
    },

    [sfp15] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x0F,
    },
    [sfp16] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x10,
    },

    [sfp17] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x11,
    },

    [sfp18] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x12,
    },
    [sfp19] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x13,
    },

    [sfp20] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x14,
    },

    [sfp21] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x15,
    },
    [sfp22] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x16,
    },

    [sfp23] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x17,
    },

    [sfp24] = {
        .reg_addr = 0x50,
        .parent = 2,
        .cpld_reg = 0x18,
    },
    [sfp25] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x01,
    },

    [sfp26] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x02,
    },
    [sfp27] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x03,
    },

    [sfp28] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x04,
    },

    [sfp29] = {
        .reg_addr = 0x50,

        .parent = 3,
        .cpld_reg = 0x05,
    },
    [sfp30] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x06,
    },
    [sfp31] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x07,
    },
    [sfp32] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x08,
    },
    [sfp33] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x09,
    },
    [sfp34] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x0A,
    },
    [sfp35] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x0B,
    },
    [sfp36] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x0C,
    },
    [sfp37] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x0D,
    },
    [sfp38] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x0E,
    },
    [sfp39] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x0F,
    },
    [sfp40] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x10,
    },
    [sfp41] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x11,
    },
    [sfp42] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x12,
    },
    [sfp43] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x13,
    },
    [sfp44] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x14,
    },
    [sfp45] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x15,
    },
    [sfp46] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x16,
    },
    [sfp47] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x17,
    },
    [sfp48] = {
        .reg_addr = 0x50,
        .parent = 3,
        .cpld_reg = 0x18,
    },
    [qsfp49] = {
        .reg_addr = 0x50,
        .parent = 4,
        .cpld_reg = 0x01,
    },
    [qsfp50] = {
        .reg_addr = 0x50,
        .parent = 4,
        .cpld_reg = 0x02,
    },
    [qsfp51] = {
        .reg_addr = 0x50,
        .parent = 4,
        .cpld_reg = 0x03,
    },
    [qsfp52] = {
        .reg_addr = 0x50,
        .parent = 4,
        .cpld_reg = 0x04,
    },
    [qsfp53] = {
        .reg_addr = 0x50,
        .parent = 4,
        .cpld_reg = 0x05,
    },
    [qsfp54] = {
        .reg_addr = 0x50,
        .parent = 4,
        .cpld_reg = 0x06,
    },
    [qsfp55] = {
        .reg_addr = 0x50,
        .parent = 4,
        .cpld_reg = 0x07,
    },
    [qsfp56] = {
        .reg_addr = 0x50,
        .parent = 4,
        .cpld_reg = 0x08,
    },
    [qsfp57] = {
        .reg_addr = 0x50,
        .parent = 4,
        .cpld_reg = 0x09,
    },
    [qsfp58] = {
        .reg_addr = 0x50,
        .parent = 4,
        .cpld_reg = 0x0A,
    },
    [qsfp59] = {
        .reg_addr = 0x50,
        .parent = 4,
        .cpld_reg = 0x0B,
    },
    [qsfp60] = {
        .reg_addr = 0x50,
        .parent = 4,
        .cpld_reg = 0x0C,
    },
    [qsfp61] = {
        .reg_addr = 0x50,
        .parent = 4,
        .cpld_reg = 0x0D,
    },
    [qsfp62] = {
        .reg_addr = 0x50,
        .parent = 4,
        .cpld_reg = 0x0E,
    },
    [qsfp63] = {
        .reg_addr = 0x50,
        .parent = 4,
        .cpld_reg = 0x0F,
    },
    [qsfp64] = {
        .reg_addr = 0x50,
        .parent = 4,
        .cpld_reg = 0x10,
    }
};

static struct platform_device s8900_64xc_cpld = {
    .name               = "ingrasys-s8900-64xc-cpld",
    .id                 = 0,
    .dev                = {
                .platform_data   = s8900_64xc_cpld_platform_data,
                .release         = device_release
    },
};

static struct platform_device s8900_64xc_sfp = {
    .name               = "ingrasys-s8900-64xc-sfp",
    .id                 = 0,
    .dev                = {
                .platform_data   = s8900_64xc_sfp_platform_data,
                .release         = device_release
    },
};

/*
 * S8900-64XC I2C DEVICES
 */

struct i2c_device_platform_data {
    int parent;
    struct i2c_board_info           info;
    struct i2c_client              *client;
};

/* module_platform_driver */
static ssize_t
get_prs_cpld_reg(struct device *dev,
                 struct device_attribute *devattr,
                 char *buf, int signal)
{
    int ret;
    u64 data = 0;
    u64 shift = 0;
    int i = 0;
    int j = 0;
    int port = 0;
    int bit = 0;
    int bit_mask = 0;
    int bit_mask_2 = 0;
    int (*reg)[CPLD_DEVICE_NUM][2];
    struct cpld_platform_data *pdata = NULL;

    pdata = dev->platform_data;

    switch(signal) {
        case sig_int:
            bit = BIT_INT;
            reg = (typeof(reg)) &int_abs_reg;
            break;
        case sig_abs:
            bit = BIT_ABS;
            reg = (typeof(reg)) &int_abs_reg;
            break;
        default:
            return sprintf(buf, "signal/na");
    }
    bit_mask = 0x1 << bit;
    bit_mask_2 = 0x1 << BIT_ABS_2;

    for (i=0; i<CPLD_DEVICE_NUM; ++i) {
        for (j = (*reg)[i][0]; j <= (*reg)[i][1]; ++j) {
            /* CPLD2 & CPLD3 0x2a~0x2f is empty */
            if (j >= 0x2a && j <= 0x2f && i < SFP_CPLD_DEVICE_NUM) {
                continue;
            }
            ret = i2c_smbus_read_byte_data(pdata[i].client, j);
            if (ret < 0) {
                return sprintf((char *)buf, "i2c_smbus_read_byte_data/na");
            }
            shift = ((u64) ((ret & bit_mask) >> bit)) << port;
            data |= shift;
            DEBUG_PRINT("port=%d, shift=0x%016llx, ret=%x, bit_mask=%08x, bit=%08x, data=0x%016llx\n", port, shift, ret, bit_mask, bit, data);
            /* CPLD2 and CPLD3 have BIT 1 and BIT 5 for present */
            if (i < SFP_CPLD_DEVICE_NUM) {
                port++;
                shift = ((u64) ((ret & bit_mask_2) >> BIT_ABS_2)) << port;
                data |= shift;
                DEBUG_PRINT("port=%d, shift=0x%016llx, ret=%x, bit_mask_2=0x%x, bit=%x, data=0x%016llx\n", port, shift, ret, bit_mask_2, bit, data);
            }

            port++;
        }
    }

    return sprintf((char *)buf, "0x%016llx\n", data);
}

/* module_platform_driver */
static ssize_t
get_rst_lp_cpld_reg(struct device *dev,
                    struct device_attribute *devattr,
                    char *buf, int signal)
{
    int ret;
    u64 data = 0;
    u64 shift = 0;
    int i = 0;
    int j = 0;
    int port = 0;
    int bit = 0;
    int bit_mask = 0;
    int (*reg)[CPLD_DEVICE_NUM][2];
    struct cpld_platform_data *pdata = NULL;

    pdata = dev->platform_data;

    switch(signal) {
        case sig_rst:
            bit = BIT_RST;
            reg = (typeof(reg)) &rst_lp_reg;
            break;
        case sig_lpm:
            bit = BIT_LPM;
            reg = (typeof(reg)) &rst_lp_reg;
            break;
        default:
            return sprintf(buf, "na");
    }
    bit_mask = 0x1 << bit;

    for (i=2; i<CPLD_DEVICE_NUM; ++i) {
        for (j = (*reg)[i][0]; j <= (*reg)[i][1]; ++j) {
            ret = i2c_smbus_read_byte_data(pdata[i].client, j);
            if (ret < 0) {
                return sprintf(buf, "na");
            }
            shift = ((u64) ((ret & bit_mask) >> bit)) << port;
            data |= shift;
            port++;
        }
    }

    return sprintf(buf, "0x%04llx\n", data);
}

static ssize_t
set_rst_lp_cpld_reg(struct device *dev,
                    struct device_attribute *devattr,
                    const char *buf, size_t count, int signal)
{
    unsigned long data;
    int err;
    struct cpld_platform_data *pdata = dev->platform_data;
    u8 current_reg_val = 0;
    u8 new_reg_val = 0;
    int value;
    int i = 0;
    int j = 0;
    int port = 0;
    int ret = 0;
    int bit = 0;
    int (*reg)[CPLD_DEVICE_NUM][2];

    err = kstrtoul(buf, 16, &data);
    if (err)
        return err;

    switch(signal) {
        case sig_rst:
            bit = BIT_RST;
            reg = (typeof(reg)) &rst_lp_reg;
            break;
        case sig_lpm:
            bit = BIT_LPM;
            reg = (typeof(reg)) &rst_lp_reg;
            break;
        default:
            return sprintf((char *)buf, "signal/na");
    }

    for (i=2; i<CPLD_DEVICE_NUM; ++i) {
        for (j = (*reg)[i][0]; j <= (*reg)[i][1]; ++j) {
            //read reg value
            current_reg_val = i2c_smbus_read_byte_data(pdata[i].client, j);
            if (current_reg_val < 0) {
                return current_reg_val;
            }

            //get new value of port N from data
            value = (data >> port) & 0x1;

            //set value on bit N of new_reg_val
            if (value > 0) {
                new_reg_val = current_reg_val | (u8) (0x1 << bit);
            } else {
                new_reg_val = current_reg_val & (u8) ~(0x1 << bit);
            }
            //write reg value if changed
            if (current_reg_val != new_reg_val) {
                ret = i2c_smbus_write_byte_data(pdata[i].client, j,
                                                (u8)(new_reg_val));
                if (ret < 0){
                    return ret;
                }
            }
            port++;
        }
    }

    return count;
}

static ssize_t
get_lpmode(struct device *dev,
           struct device_attribute *devattr, char *buf)
{
    return get_rst_lp_cpld_reg(dev, devattr, buf, sig_lpm);
}

static ssize_t
set_lpmode(struct device *dev,
           struct device_attribute *devattr,
           const char *buf, size_t count)
{
    return set_rst_lp_cpld_reg(dev, devattr, buf, count, sig_lpm);
}

static ssize_t
get_reset(struct device *dev,
          struct device_attribute *devattr, char *buf)
{
    return get_rst_lp_cpld_reg(dev, devattr, buf, sig_rst);
}

static ssize_t
set_reset(struct device *dev,
          struct device_attribute *devattr,
          const char *buf, size_t count)
{
    return set_rst_lp_cpld_reg(dev, devattr, buf, count, sig_rst);
}

static ssize_t
get_modprs(struct device *dev,
           struct device_attribute *devattr, char *buf)
{
    return get_prs_cpld_reg(dev, devattr, buf, sig_abs);
}

static DEVICE_ATTR(qsfp_modprs, S_IRUGO, get_modprs, NULL);
static DEVICE_ATTR(qsfp_lpmode, S_IRUGO | S_IWUSR, get_lpmode, set_lpmode);
static DEVICE_ATTR(qsfp_reset,  S_IRUGO | S_IWUSR, get_reset, set_reset);

static struct attribute *s8900_64xc_cpld_attrs[] = {
    &dev_attr_qsfp_lpmode.attr,
    &dev_attr_qsfp_reset.attr,
    &dev_attr_qsfp_modprs.attr,
    NULL,
};

static struct attribute_group s8900_64xc_cpld_attr_grp = {
    .attrs = s8900_64xc_cpld_attrs,
};

/*
 * Assumes that sanity checks for offset happened at sysfs-layer.
 * Offset within Lower Page 00h and Upper Page 00h are not recomputed
 */
static uint8_t
sff_8436_translate_offset(loff_t *offset)
{
   unsigned page = 0;

   if (*offset < SFF_8436_MMAP_SIZE) {
       return 0;
   }

   page = (*offset >> 7)-1;

   if (page > 0 ) {
       *offset = 0x80 + (*offset & 0x7f);
   } else {
       *offset &= 0xff;
   }

   return page;
}

static ssize_t
eeprom_read(struct i2c_client *client,
            u8 command, const char *data,
            int data_len)
{
#if USE_I2C_BLOCK_READ
    int result, retry = I2C_RW_RETRY_COUNT;

    if (data_len > I2C_SMBUS_BLOCK_MAX) {
        data_len = I2C_SMBUS_BLOCK_MAX;
    }

    while (retry) {
        result = i2c_smbus_read_i2c_block_data(client, command,
                                               data_len, (u8 *)data);
        if (result < 0) {
            msleep(I2C_RW_RETRY_INTERVAL);
            retry--;
            continue;
        }

        break;
    }

    if (unlikely(result < 0)) {
        goto abort;
    }

    if (unlikely(result != data_len)) {
        result = -EIO;
        goto abort;
    }

abort:
    return result;
#else
    int result, retry = I2C_RW_RETRY_COUNT;

    while (retry) {
        result = i2c_smbus_read_byte_data(client, command);
        if (result < 0) {
            msleep(I2C_RW_RETRY_INTERVAL);
            retry--;
            continue;
        }

        break;
    }

    if (unlikely(result < 0)) {
        dev_dbg(&client->dev, "sfp read byte data failed, " \\
                "command(0x%2x), data(0x%2x)\r\n",
                command, result);
        goto abort;
    }

    *data  = (u8)result;
    result = 1;

abort:
    return result;
#endif
}

static ssize_t
eeprom_write(struct i2c_client *client,
             u8 command, const char *data,
             int data_len)
{
#if USE_I2C_BLOCK_READ
    int result, retry = I2C_RW_RETRY_COUNT;

    if (data_len > I2C_SMBUS_BLOCK_MAX) {
        data_len = I2C_SMBUS_BLOCK_MAX;
    }

    while (retry) {
        result = i2c_smbus_write_i2c_block_data(client, command, data_len, data);
        if (result < 0) {
            msleep(I2C_RW_RETRY_INTERVAL);
            retry--;
            continue;
        }

        break;
    }

    if (unlikely(result < 0)) {
        return result;
    }

    return data_len;
#else
    int result, retry = I2C_RW_RETRY_COUNT;

    while (retry) {
        result = i2c_smbus_write_byte_data(client, command, *data);
        if (result < 0) {
            msleep(I2C_RW_RETRY_INTERVAL);
            retry--;
            continue;
        }

        break;
    }

    if (unlikely(result < 0)) {
        return result;
    }

    return 1;
#endif
}

static ssize_t
sfp_eeprom_read_write(struct i2c_client *client,
                      eeprom_operation_t op,
                      loff_t off,
                      const char *data,
                      int data_len)
{
    u8 page, phy_page;
    u8 val, refresh_page = 0;
    int ret;
    ssize_t retval = 0;
    size_t pending_len = 0, page_len = 0;
    loff_t page_offset = 0, page_start_offset = 0;
    loff_t phy_offset;


    if (off > EEPROM_SIZE) {
        return 0;
    }

    if (off + data_len > EEPROM_SIZE) {
        data_len = EEPROM_SIZE - off;
    }

    /*
     * Refresh pages which covers the requested data
     * from offset to  off + len
     * Only refresh pages which contain requested bytes
     *
     */

    pending_len = data_len;

    for (page = off >> 7; page <= (off + data_len - 1) >> 7; page++) {
        refresh_page = 0;
        switch (page) {
            case 0:
                /* Lower page 00h */
                refresh_page = 1;
                break;
            case 1:
                /* Upper page 00h */
                refresh_page = 1;
                break;
            case 2:
                /* Upper page 01h */
                ret = eeprom_read(client, SFF_8436_PAGE_PROV_ADDR, &val, sizeof(val));
                if (ret < 0)  {
                    DEBUG_PRINT("Can't read EEPROM offset %d.\n",
                              SFF_8436_PAGE_PROV_ADDR);
                    goto error;
                }
                if (val & SFF_8436_PAGE_01_PRESENT) {
                    DEBUG_PRINT("Offset:%d Value:(0x%02x & 0x%02x)",
                                SFF_8436_PAGE_PROV_ADDR, val,
                                SFF_8436_PAGE_01_PRESENT);
                    refresh_page = 1;
                }
                break;
            case 3:
                /* Upper page 02h */
                ret = eeprom_read(client, SFF_8436_PAGE_PROV_ADDR, &val, sizeof(val));
                if (ret < 0)  {
                    ERROR_MSG("Can't read EEPROM offset %d.\n",
                              SFF_8436_PAGE_PROV_ADDR);
                    goto error;
                }
                if (val & SFF_8436_PAGE_02_PRESENT) {
                    DEBUG_PRINT("Offset:%d Value:(0x%02x & 0x%02x)",
                                SFF_8436_PAGE_PROV_ADDR, val,
                                SFF_8436_PAGE_02_PRESENT);
                    refresh_page = 1;
                }
                break;
            case 4:
                /* Upper page 03h */
                ret = eeprom_read(client, SFF_8436_STATUS_ADDR, &val, sizeof(val));
                if (ret < 0)  {
                    ERROR_MSG("Can't read EEPROM offset %d.\n",
                              SFF_8436_STATUS_ADDR);
                    goto error;
                }
                if (!(val & SFF_8436_STATUS_PAGE_03_PRESENT_L)) {
                    DEBUG_PRINT("Offset:%d Value:(0x%02x & 0x%02x)",
                                SFF_8436_STATUS_ADDR, val,
                                SFF_8436_STATUS_PAGE_03_PRESENT_L);
                    refresh_page = 1;
                }
                break;
            default:
                DEBUG_PRINT("Invalid Page %d\n", page);
                ret = retval;
                goto error;
                break;
        }

        if (!refresh_page) {
            /* if page is not valid or already refreshed */
            continue;
        }

        /*
         * Compute the offset and number of bytes to be read/write
         * w.r.t requested page
         *
         * 1. start at offset 0 (within the page), and read/write the entire page
         * 2. start at offset 0 (within the page) and read/write less than entire page
         * 3. start at an offset not equal to 0 and read/write the rest of the page
         * 4. start at an offset not equal to 0 and read/write less than (end of page - offset)
         *
         */
        page_start_offset = page * EEPROM_PAGE_SIZE;

        if (page_start_offset < off) {
            page_offset = off;
            if (off + pending_len < page_start_offset + EEPROM_PAGE_SIZE) {
                page_len = pending_len;
            } else {
                page_len = EEPROM_PAGE_SIZE - off;
            }
        } else {
            page_offset = page_start_offset;
            if (pending_len > EEPROM_PAGE_SIZE) {
                page_len = EEPROM_PAGE_SIZE;
            } else {
                page_len = pending_len;
            }
        }

        pending_len = pending_len - page_len;

        /* Change current EEPROM page */
        phy_offset = page_offset;
        phy_page = sff_8436_translate_offset(&phy_offset);
        if (phy_page > 0) {
            ret = eeprom_write(client, SFF_8436_PAGE_SELECT_ADDR,
                               &phy_page, sizeof(phy_page));
            if (ret < 0) {
                ERROR_MSG("Can't write EEPROM offset %d.\n",
                          SFF_8436_PAGE_SELECT_ADDR);
                goto error;
            }
        }

        /*
         * If page_len > 32, I2C client continue read or write EEPROM.
         */
        while (page_len) {
            if (op == EEPROM_READ) {
                ret = eeprom_read(client, phy_offset, data, page_len);
            } else if (op == EEPROM_WRITE) {
                ret = eeprom_write(client, phy_offset, data, page_len);
            } else {
                ERROR_MSG("Bad EEPROM operation %d.\n", op);
                break;
            }

            if (ret <= 0) {
                if (retval == 0) {
                    retval = ret;
                }
                break;
            }
            phy_offset += ret;
            off += ret;
            data += ret;
            page_len -= ret;
            retval += ret;
        }

        /* Restore EEPROM page to default */
        if (phy_page > 0) {
            phy_page = EEPROM_DEFAULT_PAGE;
            ret = eeprom_write(client, SFF_8436_PAGE_SELECT_ADDR,
                               &phy_page, sizeof(phy_page));
            if (ret < 0) {
                ERROR_MSG("Can't write EEPROM offset %d.\n",
                          SFF_8436_PAGE_SELECT_ADDR);
                goto error;
            }
        }
    }

    return retval;

error:
    return ret;
}


static inline ssize_t
sfp_eeprom_read(struct i2c_client *client,
                loff_t off, u8 *data,
                int data_len)
{
    return sfp_eeprom_read_write(client, EEPROM_READ,
                                 off, data, data_len);
}


static inline ssize_t
sfp_eeprom_write(struct i2c_client *client,
                 loff_t off, const char *data,
                 int data_len)
{
    return sfp_eeprom_read_write(client, EEPROM_WRITE,
                                 off, data, data_len);
}

static struct i2c_client *
cpld_sfp_port_client(int port,
                     int *data_reg)
{
    *data_reg = s8900_64xc_sfp_platform_data[port].cpld_reg;
    if (port >= sfp1 && port <= sfp24) {
        return s8900_64xc_cpld_platform_data[cpld_1].client;
    } else if (port >= sfp25 && port <= sfp48) {
        return s8900_64xc_cpld_platform_data[cpld_2].client;
    } else if (port >= qsfp49 && port <= qsfp64) {
        return s8900_64xc_cpld_platform_data[cpld_3].client;
    } else {
        ERROR_MSG("Unknown port: %d", port);
        return NULL;
    }
}

static ssize_t
sfp_port_read(struct sfp_port_data *data,
              char *buf, loff_t off,
              size_t count, int port)
{
    ssize_t retval = 0;
    int data_reg = 0;
    struct i2c_client *cpld_client = NULL;

    if (unlikely(!count)) {
        DEBUG_PRINT("Count = 0, return");
        return count;
    }

    /*
     * Read data from chip, protecting against concurrent updates
     * from this host, but not from other I2C masters.
     */
    mutex_lock(&data->update_lock);

    //CPLD MUX select
    cpld_client = cpld_sfp_port_client(port, &data_reg);
    DEBUG_PRINT("data_reg=%d, port=%d", data_reg, port);
    if (!cpld_client) {
        ERROR_MSG("Error i2c client for port %d", port);
        return 0;
    }
    i2c_smbus_write_byte_data(cpld_client, CPLD_MUX_REG ,data_reg);

    while (count) {
        ssize_t status;

        status = sfp_eeprom_read(data->client, off, buf, count);
        if (status <= 0) {
            if (retval == 0) {
                retval = status;
            }
            break;
        }

        buf += status;
        off += status;
        count -= status;
        retval += status;
    }

    //CPLD MUX deselect
    i2c_smbus_write_byte_data(cpld_client, CPLD_MUX_REG ,0x0);

    mutex_unlock(&data->update_lock);
    return retval;
}

static ssize_t
sfp_bin_read(struct file *filp, struct kobject *kobj,
             struct bin_attribute *attr,
             char *buf, loff_t off, size_t count)
{
    struct sfp_port_data *data;
    struct platform_device_id *dev_id = NULL;

    dev_id = (struct platform_device_id *)attr->private;

    DEBUG_PRINT("offset = (%lld), count = (%ld) dev_port=%d", off, count, (int)dev_id->driver_data);
    data = dev_get_drvdata(container_of(kobj, struct device, kobj));

    return sfp_port_read(data, buf, off, count, (int)dev_id->driver_data);
}

static ssize_t
sfp_port_write(struct sfp_port_data *data,
               const char *buf, loff_t off,
               size_t count, int port)
{
    ssize_t retval = 0;
    int data_reg = 0;
    struct i2c_client *cpld_client = NULL;

    if (unlikely(!count)) {
        return count;
    }

    /*
     * Write data to chip, protecting against concurrent updates
     * from this host, but not from other I2C masters.
     */
    mutex_lock(&data->update_lock);

    //CPLD MUX select
    cpld_client = cpld_sfp_port_client(port, &data_reg);
    DEBUG_PRINT("data_reg=%d, port=%d", data_reg, port);
    if (!cpld_client) {
        ERROR_MSG("Error i2c client for port %d", port);
    }
    i2c_smbus_write_byte_data(cpld_client, CPLD_MUX_REG ,data_reg);

    while (count) {
        ssize_t status;

        status = sfp_eeprom_write(data->client, off, buf, count);
        if (status <= 0) {
            if (retval == 0) {
                retval = status;
            }
            break;
        }
        buf += status;
        off += status;
        count -= status;
        retval += status;
    }

    //CPLD MUX deselect
    i2c_smbus_write_byte_data(cpld_client, CPLD_MUX_REG ,0x0);

    mutex_unlock(&data->update_lock);
    return retval;
}


static ssize_t
sfp_bin_write(struct file *filp, struct kobject *kobj,
          struct bin_attribute *attr,
          char *buf, loff_t off, size_t count)
{
    struct sfp_port_data *data;
    struct platform_device_id *dev_id = NULL;

    dev_id = (struct platform_device_id *)attr->private;

    DEBUG_PRINT("offset = (%lld), count = (%ld) dev_port=%d",
                off, count, (int)dev_id->driver_data);
    data = dev_get_drvdata(container_of(kobj, struct device, kobj));

    return sfp_port_write(data, buf, off, count, (int)dev_id->driver_data);
}

static int
sfp_sysfs_eeprom_init(struct kobject *kobj,
                      struct bin_attribute *eeprom,
                      const struct platform_device_id *dev_id)
{
    int err;

    sysfs_bin_attr_init(eeprom);
    eeprom->attr.name = dev_id->name;
    eeprom->attr.mode = S_IWUSR | S_IRUGO;
    eeprom->read     = sfp_bin_read;
    eeprom->write    = sfp_bin_write;
    eeprom->private  = (void *)dev_id;
    eeprom->size     = EEPROM_SIZE;

    /* Create eeprom file */
    err = sysfs_create_bin_file(kobj, eeprom);
    if (err) {
        DEBUG_PRINT("err=%d",  err);
        return err;
    }

    return 0;
}

static int
sfp_sysfs_eeprom_cleanup(struct kobject *kobj,
                         struct bin_attribute *eeprom)
{
    sysfs_remove_bin_file(kobj, eeprom);

    return 0;
}

static int
sfp_i2c_check_functionality(struct i2c_client *client)
{
#if USE_I2C_BLOCK_READ
    return i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK);
#else
    return i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA);
#endif
}

static int
qsfp_probe(struct i2c_client *client,
           const struct platform_device_id *dev_id,
           struct qsfp_data **data)
{
    int status;
    struct qsfp_data *qsfp;

    if (!sfp_i2c_check_functionality(client)) {
        status = -EIO;
        goto exit;
    }

    qsfp = kzalloc(sizeof(struct qsfp_data), GFP_KERNEL);
    if (!qsfp) {
        status = -ENOMEM;
        DEBUG_PRINT("No memory.");
        goto exit;
    }

    /* Register sysfs hooks */ //TBD: must remove
    /* status = sysfs_create_group(&client->dev.kobj, &qsfp_group);
    if (status) {
        goto exit_free;
    } */

    /* init eeprom */
    status = sfp_sysfs_eeprom_init(&client->dev.kobj, &qsfp->eeprom.bin, dev_id);
    if (status) {
        DEBUG_PRINT("sfp_sysfs_eeprom_init error");
        goto exit_free;
    }

    //TBD: Must remove
    /*if (s9130_32x_kobj) {
        status = sysfs_create_link(s9130_32x_kobj, &client->dev.kobj, client->name);
        if (status) {
            goto exit_remove;
        }
    }*/

    *data = qsfp;
    dev_info(&client->dev, "qsfp '%s'\n", client->name);

    return 0;

exit_free:
    kfree(qsfp);
exit:

    return status;
}

static int
qsfp_device_probe(struct platform_device *pdev)
{
    struct sfp_platform_data *pdata;
    struct sfp_port_data *data[TOTAL_PORT_NUM];
    struct i2c_adapter *parent[SFP_EEPROM_DEV_NUM];
    struct i2c_client *sfp_client[SFP_EEPROM_DEV_NUM];
    int i;
    int ret=0;

    DEBUG_PRINT("Start");

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        ERROR_MSG("Missing platform data\n");
        return -ENODEV;
    }

    //New eeprom device
    for (i=0; i < SFP_EEPROM_DEV_NUM; i++) {
        parent[i] = i2c_get_adapter(i+S8900_64XC_SFP_EEPROM_BASE_NR);
        if (!parent[i]) {
            ERROR_MSG("Parent adapter (%d) not found\n", i+S8900_64XC_SFP_EEPROM_BASE_NR);
            ret=-ENODEV;
            goto error;
        }
        sfp_client[i] = i2c_new_dummy(parent[i], SFP_EEPROM_A0_I2C_ADDR);
        if (!sfp_client[i]) {
            ERROR_MSG("[%d]: Fail to create dummy i2c client for parent %d addr 0x%x\n", i, i+S8900_64XC_SFP_EEPROM_BASE_NR, SFP_EEPROM_A0_I2C_ADDR);
            ret=-ENODEV;
            goto error;
        }
    }

    //Assign client to dummy device
    for (i = 0; i < TOTAL_PORT_NUM; i++) {
        switch (pdata[i].parent) {
            case 2:
                pdata[i].client = sfp_client[0];
                break;
            case 3:
                pdata[i].client = sfp_client[1];
                break;
            case 4:
                pdata[i].client = sfp_client[2];
                break;
            default:
                ERROR_MSG("Error parent number: %d, ", i);
                break;
        }

        if (!pdata[i].client) {
            ERROR_MSG("[%d]: Fail to create dummy i2c client for parent %d addr 0x%x\n", i, pdata[i].parent, pdata[i].reg_addr);
            ret=-ENODEV;
            goto error;
        }
    }


    for (i = 0; i < TOTAL_PORT_NUM; i++) {
        data[i] = kzalloc(sizeof(struct sfp_port_data), GFP_KERNEL);
        if (!data[i]) {
            ret=-ENOMEM;
            ERROR_MSG("No memory");
            goto error;
        }

        i2c_set_clientdata(pdata[i].client, data[i]);
        mutex_init(&data[i]->update_lock);
        data[i]->port   = qsfp_device_id[i].driver_data;
        data[i]->client = pdata[i].client;

        DEBUG_PRINT("data[%d]->port=%d", i, data[i]->port);

        if (pdata[i].client->addr != SFP_EEPROM_A0_I2C_ADDR) {
            ret=-ENODEV;
            ERROR_MSG("Not approve device address");
            goto error;
        }

        data[i]->driver_type = DRIVER_TYPE_QSFP;

        ret |= qsfp_probe(pdata[i].client, &qsfp_device_id[i], &data[i]->qsfp);
    }

    if (ret) {
        ERROR_MSG("qsfp_probe failed someone.");
        //goto error;
    }
    return 0;
error:
    DEBUG_PRINT("error start");
    i2c_put_adapter(parent[i]);
    i--;
    for (; i >= 0; i--) {
        if (pdata[i].client) {
            i2c_unregister_device(pdata[i].client);
            i2c_put_adapter(parent[i]);
        }
    }

    return ret;
}

static int
qsfp_remove(struct i2c_client *client,
           struct qsfp_data *data)
{
    //TBD: Must remove
    /*if (s9130_32x_kobj) {
        sysfs_remove_link(s9130_32x_kobj, client->name);
    }*/

    //TBD: Must remove all ports EEPROM BIN
    sfp_sysfs_eeprom_cleanup(&client->dev.kobj, &data->eeprom.bin);
    //TBD: Must remove sysfs_remove_group(&client->dev.kobj, &qsfp_group);
    kfree(data);

    return 0;
}

static int __exit
qsfp_device_remove(struct platform_device *pdev)
{
    struct sfp_port_data *data = NULL;
    struct sfp_platform_data *pdata = pdev->dev.platform_data;
    struct i2c_adapter *parent = NULL;
    int i;



    if (!pdata) {
        ERROR_MSG("Missing platform data\n");
        return -ENOENT;
    }

    for (i = 0; i < TOTAL_PORT_NUM; i+=CPLD_MUX_OFFSET) {
        data = i2c_get_clientdata(pdata[i].client);
        if (!data) {
            ERROR_MSG("Empty data. skip. i=%d", i);
            continue;
        }
        qsfp_remove(pdata[i].client, data->qsfp);
        if (pdata[i].client) {
            parent = (pdata[i].client)->adapter;
            i2c_unregister_device(pdata[i].client);
            i2c_put_adapter(parent);
        }
        kfree(data);
    }


    return 0;
}

static int
cpld_probe(struct platform_device *pdev)
{
    struct cpld_platform_data *pdata;
    struct i2c_adapter *parent[CPLD_DEVICE_NUM];
    int i;
    int ret;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        ERROR_MSG("Missing platform data\n");
        return -ENODEV;
    }

    for (i = 0; i < CPLD_DEVICE_NUM; i++) {
        parent[i] = i2c_get_adapter(S8900_64XC_MUX_BASE_NR + i + 1);
        if (!parent[i]) {
            ERROR_MSG("Parent adapter (%d) not found\n",
                S8900_64XC_MUX_BASE_NR + i + 1);
            return -ENODEV;
        }
        pdata[i].client = i2c_new_dummy(parent[i], pdata[i].reg_addr);
        if (!pdata[i].client) {
            ERROR_MSG("Fail to create dummy i2c client for addr %d\n", pdata[i].reg_addr);
            goto error;
        }
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &s8900_64xc_cpld_attr_grp);
    if (ret)
        goto error;

    return 0;

error:
    if (i < CPLD_DEVICE_NUM) {
        i2c_put_adapter(parent[i]);
    }
    i--;
    for (; i >= 0; i--) {
        if (pdata[i].client) {
            i2c_unregister_device(pdata[i].client);
            i2c_put_adapter(parent[i]);
        }
    }

    return -ENODEV;
}

static int __exit cpld_remove(struct platform_device *pdev)
{
    int i;
    struct i2c_adapter *parent = NULL;
    struct cpld_platform_data *pdata = pdev->dev.platform_data;

    sysfs_remove_group(&pdev->dev.kobj, &s8900_64xc_cpld_attr_grp);

    if (!pdata) {
        ERROR_MSG("Missing platform data\n");
    } else {
        for (i = 0; i < CPLD_DEVICE_NUM; i++) {
            if (pdata[i].client) {
                parent = (pdata[i].client)->adapter;
                i2c_unregister_device(pdata[i].client);
                i2c_put_adapter(parent);
            }
        }
    }

    return 0;
}

static struct platform_driver cpld_driver = {
    .probe  = cpld_probe,
    .remove = __exit_p(cpld_remove),
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "ingrasys-s8900-64xc-cpld",
    },
};

static struct platform_driver qsfp_driver = {
    .probe        = qsfp_device_probe,
    .remove       = __exit_p(qsfp_device_remove),
    //.id_table     = qsfp_device_id,
    .driver = {
        .owner  = THIS_MODULE,
        .name     = "ingrasys-s8900-64xc-sfp",
    },
};


static int __init ingrasys_s8900_64xc_platform_init(void)
{
    int ret = 0;

    DEBUG_PRINT("ingrasysl_s8900_64xc_platform module initialization\n");

    //mdelay(10000);

    ret = platform_driver_register(&cpld_driver);
    if (ret) {
        ERROR_MSG("Fail to register cpld driver\n");
        goto error_cpld_driver;
    }
    ret = platform_device_register(&s8900_64xc_cpld);
    if (ret) {
        ERROR_MSG("Fail to create cpld device\n");
        goto error_cpld;
    }

    ret = platform_driver_register(&qsfp_driver);
    if (ret) {
        ERROR_MSG("Fail to register sfp driver\n");
        goto error_cpld_driver;
    }

    ret = platform_device_register(&s8900_64xc_sfp);
    if (ret) {
        ERROR_MSG("Fail to create sfp device\n");
        goto error_cpld;
    }

    return 0;

error_cpld:
    platform_driver_unregister(&cpld_driver);
    platform_driver_unregister(&qsfp_driver);
error_cpld_driver:
    return ret;
}

static void __exit ingrasys_s8900_64xc_platform_exit(void)
{
    platform_device_unregister(&s8900_64xc_sfp);
    platform_device_unregister(&s8900_64xc_cpld);
    platform_driver_unregister(&qsfp_driver);
    platform_driver_unregister(&cpld_driver);
}

module_init(ingrasys_s8900_64xc_platform_init);
module_exit(ingrasys_s8900_64xc_platform_exit);

MODULE_DESCRIPTION("Ingrasys S8900-64XC Platform Support");
MODULE_AUTHOR("Wade He <feng.cf.lee@ingrasys.com>");
MODULE_LICENSE("GPL");

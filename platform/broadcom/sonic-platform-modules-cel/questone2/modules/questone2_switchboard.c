/*
 * seastone_switchboard.c - driver for seastone2/questone2 Switch board FPGA/CPLD.
 *
 * Author: Pradchaya Phucharoen
 *
 * Copyright (C) 2017 Celestica Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *   /
 *   \--sys
 *       \--devices
 *            \--platform
 *                \--seastone2
 *                    |--FPGA
 *                    |--CPLD1
 *                    |--CPLD2
 *                    \--SFF
 *                        |--QSFP[1..32]
 *                        \--SFP[1..2]
 *
 */

#ifndef TEST_MODE
#define MOD_VERSION "2.0.0"
#else
#define MOD_VERSION "TEST"
#endif

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/acpi.h>
#include <linux/io.h>
#include <linux/dmi.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <uapi/linux/stat.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>



static int  majorNumber;
#ifdef SEASTONE2
#define CLASS_NAME "seastone2_fpga"
#define DRIVER_NAME "seastone2"
#define FPGA_PCI_NAME "Seastone2_fpga_pci"
#else
#define CLASS_NAME "questone2_fpga"
#define DRIVER_NAME "questone2"
#define FPGA_PCI_NAME "questone2_fpga_pci"
#endif
#define DEVICE_NAME "fwupgrade"


static int smbus_access(struct i2c_adapter *adapter, u16 addr,
              unsigned short flags, char rw, u8 cmd,
              int size, union i2c_smbus_data *data);

static int fpga_i2c_access(struct i2c_adapter *adapter, u16 addr,
              unsigned short flags, char rw, u8 cmd,
              int size, union i2c_smbus_data *data);

static int fpgafw_init(void);
static void fpgafw_exit(void);


/*
========================================
FPGA PCIe BAR 0 Registers
========================================
Misc Control    0x00000000 – 0x000000FF.
I2C_CH1         0x00000100 - 0x00000110
I2C_CH2         0x00000200 - 0x00000210.
I2C_CH3         0x00000300 - 0x00000310.
I2C_CH4         0x00000400 - 0x00000410.
I2C_CH5         0x00000500 - 0x00000510.
I2C_CH6         0x00000600 - 0x00000610.
I2C_CH7         0x00000700 - 0x00000710.
I2C_CH8         0x00000800 - 0x00000810.
I2C_CH9         0x00000900 - 0x00000910.
I2C_CH10        0x00000A00 - 0x00000A10.
SPI Master      0x00001200 - 0x00001300.
PORT XCVR       0x00004000 - 0x00004FFF.
*/

/* MISC       */
#define FPGA_VERSION            0x0000
#define FPGA_VERSION_MJ_MSK     0xff00
#define FPGA_VERSION_MN_MSK     0x00ff
#define FPGA_SCRATCH            0x0004
#define FPGA_BROAD_TYPE         0x0008
#define FPGA_BROAD_REV_MSK      0x0038
#define FPGA_BROAD_ID_MSK       0x0007
#define FPGA_PLL_STATUS         0x0014
#define BMC_I2C_SCRATCH         0x0020
#define FPGA_SLAVE_CPLD_REST    0x0030
#define FPGA_PERIPH_RESET_CTRL  0x0034
#define FPGA_INT_STATUS         0x0040
#define FPGA_INT_SRC_STATUS     0x0044
#define FPGA_INT_FLAG           0x0048
#define FPGA_INT_MASK           0x004c
#define FPGA_MISC_CTRL          0x0050
#define FPGA_MISC_STATUS        0x0054
#define FPGA_AVS_VID_STATUS     0x0068
#define FPGA_FEATURE_CARD_GPIO  0x0070
#define FPGA_PORT_XCVR_READY    0x000c

/* I2C_MASTER BASE ADDR */
#define I2C_MASTER_FREQ_1           0x0100
#define I2C_MASTER_CTRL_1           0x0104
#define I2C_MASTER_STATUS_1         0x0108
#define I2C_MASTER_DATA_1           0x010c
#define I2C_MASTER_PORT_ID_1        0x0110
#define I2C_MASTER_CH_1             1
#define I2C_MASTER_CH_2             2
#define I2C_MASTER_CH_3             3
#define I2C_MASTER_CH_4             4
#define I2C_MASTER_CH_5             5
#define I2C_MASTER_CH_6             6
#define I2C_MASTER_CH_7             7
#define I2C_MASTER_CH_8             8
#define I2C_MASTER_CH_9             9
#define I2C_MASTER_CH_10            10
#define I2C_MASTER_CH_TOTAL I2C_MASTER_CH_10

/* SPI_MASTER */
#define SPI_MASTER_WR_EN            0x1200 /* one bit */
#define SPI_MASTER_WR_DATA          0x1204 /* 32 bits */
#define SPI_MASTER_CHK_ID           0x1208 /* one bit */
#define SPI_MASTER_VERIFY           0x120c /* one bit */
#define SPI_MASTER_STATUS           0x1210 /* 15 bits */
#define SPI_MASTER_MODULE_RST       0x1214 /* one bit */

/* FPGA FRONT PANEL PORT MGMT */
#define SFF_PORT_CTRL_BASE          0x4000
#define SFF_PORT_STATUS_BASE        0x4004
#define SFF_PORT_INT_STATUS_BASE    0x4008
#define SFF_PORT_INT_MASK_BASE      0x400c

#define PORT_XCVR_REGISTER_SIZE     0x1000

/* PORT CTRL REGISTER
[31:7]  RSVD
[6]     LPMOD   6
[5]     RSVD
[4]     RST     4
[3:1]   RSVD
[0]     TXDIS   0
*/
#define CTRL_LPMOD   6
#define CTRL_RST     4
#define CTRL_TXDIS   0

/* PORT STATUS REGISTER
[31:6]  RSVD
[5]     IRQ         5
[4]     PRESENT     4
[3]     RSVD
[2]     TXFAULT     2
[1]     RXLOS       1
[0]     MODABS      0
*/
#define STAT_IRQ         5
#define STAT_PRESENT     4
#define STAT_TXFAULT     2
#define STAT_RXLOS       1
#define STAT_MODABS      0

/* PORT INTRPT REGISTER
[31:6]  RSVD
[5]     INT_N       5
[4]     PRESENT     4
[3]     RSVD
[2]     RSVD
[1]     RXLOS       1
[0]     MODABS      0
*/
#define INTR_INT_N      5
#define INTR_PRESENT    4
#define INTR_TXFAULT    2
#define INTR_RXLOS      1
#define INTR_MODABS     0

/* PORT INT MASK REGISTER
[31:6]  RSVD
[5]     INT_N       5
[4]     PRESENT     4
[3]     RSVD
[2]     RSVD
[1]     RXLOS_INT   1
[0]     MODABS      0
*/
#define MASK_INT_N      5
#define MASK_PRESENT    4
#define MASK_TXFAULT    2
#define MASK_RXLOS      1
#define MASK_MODABS     0

enum {
    I2C_SR_BIT_RXAK = 0,
    I2C_SR_BIT_MIF,
    I2C_SR_BIT_SRW,
    I2C_SR_BIT_BCSTM,
    I2C_SR_BIT_MAL,
    I2C_SR_BIT_MBB,
    I2C_SR_BIT_MAAS,
    I2C_SR_BIT_MCF
};

enum {
    I2C_CR_BIT_BCST = 0,
    I2C_CR_BIT_RSTA = 2,
    I2C_CR_BIT_TXAK,
    I2C_CR_BIT_MTX,
    I2C_CR_BIT_MSTA,
    I2C_CR_BIT_MIEN,
    I2C_CR_BIT_MEN,
};

/**
 *
 * The function is i2c algorithm implement to allow master access to
 * correct endpoint devices trough the PCA9548 switch devices.
 *
 *  FPGA I2C Master [mutex resource]
 *              |
 *              |
 *    ---------------------------
 *    |        PCA9548(s)       |
 *    ---1--2--3--4--5--6--7--8--
 *       |  |  |  |  |  |  |  |
 *   EEPROM      ...          EEPROM
 *
 */


#ifdef SEASTONE2
#define VIRTUAL_I2C_QSFP_PORT           32
#define VIRTUAL_I2C_SFP_PORT            1
#define VIRTUAL_I2C_CPLD_PORT           1
#define VIRTUAL_I2C_POWER_CHIP_PORT     1
#define VIRTUAL_I2C_CPLD_B_PORT         1
#define VIRTUAL_I2C_PSU                 1
#define VIRTUAL_I2C_FAN_TRAY            4
#define VIRTUAL_I2C_POWER_MON           1
#define VIRTUAL_I2C_LM75                1

#define VIRTUAL_I2C_PORT_LENGTH \
            VIRTUAL_I2C_SFP_PORT+VIRTUAL_I2C_QSFP_PORT+VIRTUAL_I2C_POWER_CHIP_PORT+VIRTUAL_I2C_CPLD_PORT+VIRTUAL_I2C_CPLD_B_PORT+VIRTUAL_I2C_PSU+VIRTUAL_I2C_FAN_TRAY+VIRTUAL_I2C_POWER_MON+VIRTUAL_I2C_LM75

#define SFF_PORT_TOTAL    VIRTUAL_I2C_QSFP_PORT+VIRTUAL_I2C_SFP_PORT
#else
#define VIRTUAL_I2C_SFP_PORT            48
#define VIRTUAL_I2C_QSFP_PORT           8
#define VIRTUAL_I2C_CPLD_PORT           1
#define VIRTUAL_I2C_POWER_CHIP_PORT     1
#define VIRTUAL_I2C_CPLD_B_PORT         1
#define VIRTUAL_I2C_PSU                 1
#define VIRTUAL_I2C_FAN_TRAY            4
#define VIRTUAL_I2C_POWER_MON           1
#define VIRTUAL_I2C_LM75                1

#define VIRTUAL_I2C_PORT_LENGTH \
            VIRTUAL_I2C_SFP_PORT+VIRTUAL_I2C_QSFP_PORT+VIRTUAL_I2C_POWER_CHIP_PORT+VIRTUAL_I2C_CPLD_PORT+VIRTUAL_I2C_CPLD_B_PORT+VIRTUAL_I2C_PSU+VIRTUAL_I2C_FAN_TRAY+VIRTUAL_I2C_POWER_MON+VIRTUAL_I2C_LM75

#define SFF_PORT_TOTAL    VIRTUAL_I2C_QSFP_PORT+VIRTUAL_I2C_SFP_PORT
#endif

#define VIRTUAL_I2C_CPLD_INDEX SFF_PORT_TOTAL

#define VIRTUAL_I2C_BUS_OFFSET  2
#define CPLD1_SLAVE_ADDR        0x30
#define CPLD2_SLAVE_ADDR        0x31

static struct class*  fpgafwclass  = NULL; ///< The device-driver class struct pointer
static struct device* fpgafwdev = NULL; ///< The device-driver device struct pointer

#define PCI_VENDOR_ID_TEST 0x1af4

#ifndef PCI_VENDOR_ID_XILINX
#define PCI_VENDOR_ID_XILINX 0x10EE
#endif

#define FPGA_PCIE_DEVICE_ID 0x7021
#define TEST_PCIE_DEVICE_ID 0x1110


#ifdef DEBUG_KERN
#define info(fmt,args...)  printk(KERN_INFO "line %3d : "fmt,__LINE__,##args)
#define check(REG)         printk(KERN_INFO "line %3d : %-8s = %2.2X",__LINE__,#REG,ioread8(REG));
#else
#define info(fmt,args...)
#define check(REG)
#endif

#define GET_REG_BIT(REG,BIT)   ((ioread8(REG) >> BIT) & 0x01)
#define SET_REG_BIT_H(REG,BIT) iowrite8(ioread8(REG) |  (0x01 << BIT),REG)
#define SET_REG_BIT_L(REG,BIT) iowrite8(ioread8(REG) & ~(0x01 << BIT),REG)

static struct mutex fpga_i2c_master_locks[I2C_MASTER_CH_TOTAL];
/* Store lasted switch address and channel */
static uint16_t fpga_i2c_lasted_access_port[I2C_MASTER_CH_TOTAL];

enum PORT_TYPE {
    NONE,
    QSFP,
    SFP
};

struct i2c_switch{
    unsigned char master_bus;   // I2C bus number
    unsigned char switch_addr;  // PCA9548 device address, 0xFF if directly connect to a bus.
    unsigned char channel;      // PCA9548 channel number. If the switch_addr is 0xFF, this value is ignored.
    enum PORT_TYPE port_type;   // QSFP/SFP tranceiver port type.
    char calling_name[20];      // Calling name.
};

struct i2c_dev_data {
    int portid;
    struct i2c_switch pca9548;
};

#ifdef SEASTONE2
/* PREDEFINED I2C SWITCH DEVICE TOPOLOGY */
static struct i2c_switch fpga_i2c_bus_dev[] = {
    /* BUS2 QSFP Exported as virtual bus */
    {I2C_MASTER_CH_2,0x72,0,QSFP,"QSFP1"}, {I2C_MASTER_CH_2,0x72,1,QSFP,"QSFP2"}, {I2C_MASTER_CH_2,0x72,2,QSFP,"QSFP3"}, {I2C_MASTER_CH_2,0x72,3,QSFP,"QSFP4"},
    {I2C_MASTER_CH_2,0x72,4,QSFP,"QSFP5"}, {I2C_MASTER_CH_2,0x72,5,QSFP,"QSFP6"}, {I2C_MASTER_CH_2,0x72,6,QSFP,"QSFP7"}, {I2C_MASTER_CH_2,0x72,7,QSFP,"QSFP8"},
    {I2C_MASTER_CH_2,0x73,0,QSFP,"QSFP9"}, {I2C_MASTER_CH_2,0x73,1,QSFP,"QSFP10"},{I2C_MASTER_CH_2,0x73,2,QSFP,"QSFP11"},{I2C_MASTER_CH_2,0x73,3,QSFP,"QSFP12"},
    {I2C_MASTER_CH_2,0x73,4,QSFP,"QSFP13"},{I2C_MASTER_CH_2,0x73,5,QSFP,"QSFP14"},{I2C_MASTER_CH_2,0x73,6,QSFP,"QSFP15"},{I2C_MASTER_CH_2,0x73,7,QSFP,"QSFP16"},
    {I2C_MASTER_CH_2,0x74,0,QSFP,"QSFP17"},{I2C_MASTER_CH_2,0x74,1,QSFP,"QSFP18"},{I2C_MASTER_CH_2,0x74,2,QSFP,"QSFP19"},{I2C_MASTER_CH_2,0x74,3,QSFP,"QSFP20"},
    {I2C_MASTER_CH_2,0x74,4,QSFP,"QSFP21"},{I2C_MASTER_CH_2,0x74,5,QSFP,"QSFP22"},{I2C_MASTER_CH_2,0x74,6,QSFP,"QSFP23"},{I2C_MASTER_CH_2,0x74,7,QSFP,"QSFP24"},
    {I2C_MASTER_CH_2,0x75,0,QSFP,"QSFP25"},{I2C_MASTER_CH_2,0x75,1,QSFP,"QSFP26"},{I2C_MASTER_CH_2,0x75,2,QSFP,"QSFP27"},{I2C_MASTER_CH_2,0x75,3,QSFP,"QSFP28"},
    {I2C_MASTER_CH_2,0x75,4,QSFP,"QSFP29"},{I2C_MASTER_CH_2,0x75,5,QSFP,"QSFP30"},{I2C_MASTER_CH_2,0x75,6,QSFP,"QSFP31"},{I2C_MASTER_CH_2,0x75,7,QSFP,"QSFP32"},
    /* BUS1 SFP+ Exported as virtual bus */
    {I2C_MASTER_CH_1,0x72,0,SFP,"SFP1"},{I2C_MASTER_CH_1,0x72,1,SFP,"SFP2"},
    /* BUS3 CPLD Access via SYSFS */
    {I2C_MASTER_CH_3,0xFF,0,NONE,"CPLD"},
    /* BUS5 POWER CHIP Exported as virtual bus */
    {I2C_MASTER_CH_5,0xFF,0,NONE,"POWER"},
    /* BUS4 CPLD_B */
    {I2C_MASTER_CH_4,0xFF,0,NONE,"CPLD_B"},
    /* BUS6 PSU */
    {I2C_MASTER_CH_6,0xFF,0,NONE,"PSU"},
    /* BUS7 FAN */
    /* Channel 2 is no hardware connected */
    {I2C_MASTER_CH_7,0x77,0,NONE,"FAN5"},{I2C_MASTER_CH_7,0x77,1,NONE,"FAN4"},{I2C_MASTER_CH_7,0x77,3,NONE,"FAN2"},{I2C_MASTER_CH_7,0x77,4,NONE,"FAN1"},
    /* BUS8 POWER MONITOR */
    {I2C_MASTER_CH_8,0xFF,0,NONE,"UCD90120"},
    /* BUS9 LM75 */
    {I2C_MASTER_CH_9,0xFF,0,NONE,"LM75"},
};
#else
/* PREDEFINED I2C SWITCH DEVICE TOPOLOGY */
static struct i2c_switch fpga_i2c_bus_dev[] = {
    /* BUS1 SFP Exported as virtual bus */
    {I2C_MASTER_CH_10,0x72,0,SFP,"SFP1"}, {I2C_MASTER_CH_10,0x72,1,SFP,"SFP2"}, {I2C_MASTER_CH_10,0x72,2,SFP,"SFP3"}, {I2C_MASTER_CH_10,0x72,3,SFP,"SFP4"},
    {I2C_MASTER_CH_10,0x72,4,SFP,"SFP5"}, {I2C_MASTER_CH_10,0x72,5,SFP,"SFP6"}, {I2C_MASTER_CH_10,0x72,6,SFP,"SFP7"}, {I2C_MASTER_CH_10,0x72,7,SFP,"SFP8"},
    {I2C_MASTER_CH_10,0x73,0,SFP,"SFP9"}, {I2C_MASTER_CH_10,0x73,1,SFP,"SFP10"},{I2C_MASTER_CH_10,0x73,2,SFP,"SFP11"},{I2C_MASTER_CH_10,0x73,3,SFP,"SFP12"},
    {I2C_MASTER_CH_10,0x73,4,SFP,"SFP13"},{I2C_MASTER_CH_10,0x73,5,SFP,"SFP14"},{I2C_MASTER_CH_10,0x73,6,SFP,"SFP15"},{I2C_MASTER_CH_10,0x73,7,SFP,"SFP16"},
    {I2C_MASTER_CH_10,0x74,0,SFP,"SFP17"},{I2C_MASTER_CH_10,0x74,1,SFP,"SFP18"},{I2C_MASTER_CH_10,0x74,2,SFP,"SFP19"},{I2C_MASTER_CH_10,0x74,3,SFP,"SFP20"},
    {I2C_MASTER_CH_10,0x74,4,SFP,"SFP21"},{I2C_MASTER_CH_10,0x74,5,SFP,"SFP22"},{I2C_MASTER_CH_10,0x74,6,SFP,"SFP23"},{I2C_MASTER_CH_10,0x74,7,SFP,"SFP24"},
    {I2C_MASTER_CH_10,0x75,0,SFP,"SFP25"},{I2C_MASTER_CH_10,0x75,1,SFP,"SFP26"},{I2C_MASTER_CH_10,0x75,2,SFP,"SFP27"},{I2C_MASTER_CH_10,0x75,3,SFP,"SFP28"},
    {I2C_MASTER_CH_10,0x75,4,SFP,"SFP29"},{I2C_MASTER_CH_10,0x75,5,SFP,"SFP30"},{I2C_MASTER_CH_10,0x75,6,SFP,"SFP31"},{I2C_MASTER_CH_10,0x75,7,SFP,"SFP32"},
    {I2C_MASTER_CH_10,0x76,0,SFP,"SFP33"},{I2C_MASTER_CH_10,0x76,1,SFP,"SFP34"},{I2C_MASTER_CH_10,0x76,2,SFP,"SFP35"},{I2C_MASTER_CH_10,0x76,3,SFP,"SFP36"},
    {I2C_MASTER_CH_10,0x76,4,SFP,"SFP37"},{I2C_MASTER_CH_10,0x76,5,SFP,"SFP38"},{I2C_MASTER_CH_10,0x76,6,SFP,"SFP39"},{I2C_MASTER_CH_10,0x76,7,SFP,"SFP40"},
    {I2C_MASTER_CH_10,0x77,0,SFP,"SFP41"},{I2C_MASTER_CH_10,0x77,1,SFP,"SFP42"},{I2C_MASTER_CH_10,0x77,2,SFP,"SFP43"},{I2C_MASTER_CH_10,0x77,3,SFP,"SFP44"},
    {I2C_MASTER_CH_10,0x77,4,SFP,"SFP45"},{I2C_MASTER_CH_10,0x77,5,SFP,"SFP46"},{I2C_MASTER_CH_10,0x77,6,SFP,"SFP47"},{I2C_MASTER_CH_10,0x77,7,SFP,"SFP48"},
    /* BUS2 QSFP28 Exported as virtual bus */
    {I2C_MASTER_CH_2,0x74,4,QSFP,"QSFP1"},{I2C_MASTER_CH_2,0x74,5,QSFP,"QSFP2"},{I2C_MASTER_CH_2,0x74,6,QSFP,"QSFP3"},{I2C_MASTER_CH_2,0x74,7,QSFP,"QSFP4"},
    {I2C_MASTER_CH_2,0x74,0,QSFP,"QSFP5"},{I2C_MASTER_CH_2,0x74,1,QSFP,"QSFP6"},{I2C_MASTER_CH_2,0x74,2,QSFP,"QSFP7"},{I2C_MASTER_CH_2,0x74,3,QSFP,"QSFP8"},
    /* BUS3 CPLD Access via SYSFS */
    {I2C_MASTER_CH_3,0xFF,0,NONE,"CPLD"},
    /* BUS5 POWER CHIP Exported as virtual bus */
    {I2C_MASTER_CH_5,0xFF,0,NONE,"POWER"},
    /* BUS4 CPLD_B */
    {I2C_MASTER_CH_4,0xFF,0,NONE,"CPLD_B"},
    /* BUS6 PSU */
    {I2C_MASTER_CH_6,0xFF,0,NONE,"PSU"},
    /* BUS7 FAN */
    /* Channel 2 is no hardware connected */
    {I2C_MASTER_CH_7,0x77,0,NONE,"FAN5"},{I2C_MASTER_CH_7,0x77,1,NONE,"FAN4"},{I2C_MASTER_CH_7,0x77,3,NONE,"FAN2"},{I2C_MASTER_CH_7,0x77,4,NONE,"FAN1"},
    /* BUS8 UCD90120 */
    {I2C_MASTER_CH_8,0xFF,0,NONE,"UCD90120"},
    /* BUS9 TEMP SENSOR LM75 */
    {I2C_MASTER_CH_9,0xFF,0,NONE,"LM75"}
};
#endif
struct fpga_device{
    /* data mmio region */
    void __iomem *data_base_addr;
    resource_size_t data_mmio_start;
    resource_size_t data_mmio_len;
};

static struct fpga_device fpga_dev = {
    .data_base_addr = NULL,
    .data_mmio_start = 0,
    .data_mmio_len = 0,
};

struct seastone2_fpga_data {
    struct device *sff_devices[SFF_PORT_TOTAL];
    struct i2c_client *sff_i2c_clients[SFF_PORT_TOTAL];
    struct i2c_adapter *i2c_adapter[VIRTUAL_I2C_PORT_LENGTH];
    struct mutex fpga_lock;         // For FPGA internal lock
    void __iomem * fpga_read_addr;
    uint8_t cpld1_read_addr;
    uint8_t cpld2_read_addr;
};

struct sff_device_data {
    int portid;
    enum PORT_TYPE port_type;
};

struct seastone2_fpga_data *fpga_data;

/*
 * Kernel object for other module drivers.
 * Other module can use these kobject as a parent.
 */

static struct kobject *fpga = NULL;
static struct kobject *cpld1 = NULL;
static struct kobject *cpld2 = NULL;

/**
 * Device node in sysfs tree.
 */
static struct device *sff_dev = NULL;

/**
 * [get_fpga_reg_value description]
 * @param  dev     [description]
 * @param  devattr [description]
 * @param  buf     [description]
 * @return         [description]
 */
static ssize_t get_fpga_reg_value(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    // read data from the address
    uint32_t data;
    data = ioread32(fpga_data->fpga_read_addr);
    return sprintf(buf,"0x%8.8x\n",data);
}
/**
 * [set_fpga_reg_address description]
 * @param  dev     [description]
 * @param  devattr [description]
 * @param  buf     [description]
 * @param  count   [description]
 * @return         [description]
 */
static ssize_t set_fpga_reg_address(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    uint32_t addr;
    char *last;

    addr = (uint32_t)strtoul(buf,&last,16);
    if(addr == 0 && buf == last){
        return -EINVAL;
    }
    fpga_data->fpga_read_addr = fpga_dev.data_base_addr+addr;
    return count;
}
/**
 * [get_fpga_scratch description]
 * @param  dev     [description]
 * @param  devattr [description]
 * @param  buf     [description]
 * @return         [description]
 */
static ssize_t get_fpga_scratch(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    return sprintf(buf,"0x%8.8x\n", ioread32(fpga_dev.data_base_addr+FPGA_SCRATCH) & 0xffffffff);
}
/**
 * [set_fpga_scratch description]
 * @param  dev     [description]
 * @param  devattr [description]
 * @param  buf     [description]
 * @param  count   [description]
 * @return         [description]
 */
static ssize_t set_fpga_scratch(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    uint32_t data;
    char *last;
    data = (uint32_t)strtoul(buf,&last,16);
    if(data == 0 && buf == last){
        return -EINVAL;
    }
    iowrite32(data, fpga_dev.data_base_addr+FPGA_SCRATCH);
    return count;
}
/**
 * [set_fpga_reg_value description]
 * @param  dev     [description]
 * @param  devattr [description]
 * @param  buf     [description]
 * @return         [description]
 */
static ssize_t set_fpga_reg_value(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    //register is 4 bytes
    uint32_t addr;
    uint32_t value;
    uint32_t mode = 8;
    char *tok;
    char clone[count];
    char *pclone = clone;
    char *last;

    strscpy(clone, buf, count);

    mutex_lock(&fpga_data->fpga_lock);
    tok = strsep((char**)&pclone, " ");
    if(tok == NULL){
        mutex_unlock(&fpga_data->fpga_lock);
        return -EINVAL;
    }
    addr = (uint32_t)strtoul(tok,&last,16);
    if(addr == 0 && tok == last){
        mutex_unlock(&fpga_data->fpga_lock);
        return -EINVAL;
    }
    tok = strsep((char**)&pclone, " ");
    if(tok == NULL){
        mutex_unlock(&fpga_data->fpga_lock);
        return -EINVAL;
    }
    value = (uint32_t)strtoul(tok,&last,16);
    if(value == 0 && tok == last){
        mutex_unlock(&fpga_data->fpga_lock);
        return -EINVAL;
    }
    tok = strsep((char**)&pclone, " ");
    if(tok == NULL){
        mode = 32;
    }else{
        mode = (uint32_t)strtoul(tok,&last,10);
        if(mode == 0 && tok == last){
            mutex_unlock(&fpga_data->fpga_lock);
            return -EINVAL;
        }
    }
    if(mode == 32){
        iowrite32(value, fpga_dev.data_base_addr+addr);
    }else if(mode == 8){
        iowrite8(value, fpga_dev.data_base_addr+addr);
    }else{
        mutex_unlock(&fpga_data->fpga_lock);
        return -EINVAL;
    }
    mutex_unlock(&fpga_data->fpga_lock);
    return count;
}

/**
 * Read all FPGA XCVR register in binary mode.
 * @param  filp  [description]
 * @param  kobj  [description]
 * @param  attr  [description]
 * @param  buf   [description]
 * @param  off   [description]
 * @param  count [description]
 * @return       [description]
 */
static ssize_t dump_read(struct file *filp, struct kobject *kobj,
                struct bin_attribute *attr, char *buf,
                loff_t off, size_t count)
{
    unsigned long i=0;
    ssize_t status;
    u8 read_reg;

    if( off + count > PORT_XCVR_REGISTER_SIZE ){
        return -EINVAL;
    }
    mutex_lock(&fpga_data->fpga_lock);
    while(i < count){
        read_reg = ioread8(fpga_dev.data_base_addr + SFF_PORT_CTRL_BASE + off + i);
        buf[i++] = read_reg;
    }
    status = count;
    mutex_unlock(&fpga_data->fpga_lock);
    return status;
}

/**
 * Show FPGA port XCVR ready status
 * @param  dev  [description]
 * @param  attr [description]
 * @param  buf  [description]
 * @return      [description]
 */
static ssize_t ready_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u32 data;
    struct sff_device_data *dev_data = dev_get_drvdata(dev);
    unsigned int REGISTER = FPGA_PORT_XCVR_READY;

    mutex_lock(&fpga_data->fpga_lock);
    data = ioread32(fpga_dev.data_base_addr+REGISTER);
    mutex_unlock(&fpga_data->fpga_lock);
    return sprintf(buf, "%d\n", (data >> 0) & 1U);
}

/* FPGA attributes */
static DEVICE_ATTR( getreg, 0600, get_fpga_reg_value, set_fpga_reg_address);
static DEVICE_ATTR( scratch, 0600, get_fpga_scratch, set_fpga_scratch);
static DEVICE_ATTR( setreg, 0200, NULL , set_fpga_reg_value);
static DEVICE_ATTR_RO(ready);
static BIN_ATTR_RO( dump, PORT_XCVR_REGISTER_SIZE);

static struct bin_attribute *fpga_bin_attrs[] = {
        &bin_attr_dump,
        NULL,
};

static struct attribute *fpga_attrs[] = {
        &dev_attr_getreg.attr,
        &dev_attr_scratch.attr,
        &dev_attr_setreg.attr,
        &dev_attr_ready.attr,
        NULL,
};

static struct attribute_group fpga_attr_grp = {
        .attrs = fpga_attrs,
        .bin_attrs = fpga_bin_attrs,
};

/* SW CPLDs attributes */
static ssize_t cpld1_getreg_show(struct device *dev,
                                 struct device_attribute *attr, char *buf)
{
    // CPLD register is one byte
    uint8_t data;
    int err;

    err = fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],
                          CPLD1_SLAVE_ADDR, 0x00,
                          I2C_SMBUS_READ, fpga_data->cpld1_read_addr,
                          I2C_SMBUS_BYTE_DATA, (union i2c_smbus_data*)&data);

    if (err < 0)
        return err;

    return sprintf(buf, "0x%2.2x\n", data);
}

static ssize_t cpld1_getreg_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf, size_t count)
{
    ssize_t status;
    uint8_t addr;

    status = kstrtou8(buf, 0, &addr);
    if (status == 0) {
        fpga_data->cpld1_read_addr = addr;
        status = count;
    }
    return status;
}
struct device_attribute dev_attr_cpld1_getreg = __ATTR(getreg, 0600, cpld1_getreg_show, cpld1_getreg_store);

static ssize_t cpld1_dump_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    // CPLD register is one byte
    uint8_t data;
    fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD1_SLAVE_ADDR,0x00,I2C_SMBUS_READ,fpga_data->cpld1_read_addr,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&data);
    return sprintf(buf,"0x%2.2x\n",data);
}
static ssize_t cpld1_dump_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    uint8_t addr;
    char *last;
    addr = (uint8_t)strtoul(buf,&last,16);
    if(addr == 0 && buf == last){
        return -EINVAL;
    }
    fpga_data->cpld1_read_addr = addr;
    return size;
}
struct device_attribute dev_attr_cpld1_dump = __ATTR(dump,0600,cpld1_dump_show,cpld1_dump_store);

static ssize_t cpld1_scratch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    // CPLD register is one byte
    __u8 data;
    int err;
    err = fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD1_SLAVE_ADDR,0x00,I2C_SMBUS_READ,0x01,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&data);
    if(err < 0)
        return err;
    return sprintf(buf, "0x%2.2x\n",data);
}
static ssize_t cpld1_scratch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    // CPLD register is one byte
    __u8 data;
    char *last;
    int err;
    data = (uint8_t)strtoul(buf,&last,16);
    if(data == 0 && buf == last){
        return -EINVAL;
    }
    err = fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD1_SLAVE_ADDR,0x00,I2C_SMBUS_WRITE,0x01,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&data);
    if(err < 0)
        return err;
    return size;
}
struct device_attribute dev_attr_cpld1_scratch = __ATTR(scratch,0600,cpld1_scratch_show,cpld1_scratch_store);

static ssize_t cpld1_setreg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{

    uint8_t addr,value;
    char *tok;
    char clone[size];
    char *pclone = clone;
    int err;
    char *last;

    strscpy(clone, buf, size);

    tok = strsep((char**)&pclone, " ");
    if(tok == NULL){
        return sprintf(buf,"ERROR line %d",__LINE__);
    }
    addr = (uint8_t)strtoul(tok,&last,16);
    if(addr == 0 && tok == last){
        return -EINVAL;
    }
    tok = strsep((char**)&pclone, " ");
    if(tok == NULL){
        return sprintf(buf,"ERROR line %d",__LINE__);
    }
    value = (uint8_t)strtoul(tok,&last,16);
    if(value == 0 && tok == last){
        return -EINVAL;
    }

    err = fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD1_SLAVE_ADDR,0x00,I2C_SMBUS_WRITE,addr,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&value);
    if(err < 0)
        return sprintf(buf,"ERROR line %d",__LINE__);

    return size;
}
struct device_attribute dev_attr_cpld1_setreg = __ATTR(setreg,0200,NULL,cpld1_setreg_store);

static struct attribute *cpld1_attrs[] = {
        &dev_attr_cpld1_getreg.attr,
        &dev_attr_cpld1_dump.attr,
        &dev_attr_cpld1_scratch.attr,
        &dev_attr_cpld1_setreg.attr,
        NULL,
};

static struct attribute_group cpld1_attr_grp = {
        .attrs = cpld1_attrs,
};

static ssize_t cpld2_getreg_show(struct device *dev,
                                 struct device_attribute *attr, char *buf)
{
    // CPLD register is one byte
    uint8_t data;
    int err;

    err = fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],
                          CPLD2_SLAVE_ADDR, 0x00, I2C_SMBUS_READ, fpga_data->cpld2_read_addr,
                          I2C_SMBUS_BYTE_DATA, (union i2c_smbus_data*)&data);
    if (err < 0)
        return err;

    return sprintf(buf, "0x%2.2x\n", data);
}

static ssize_t cpld2_getreg_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf, size_t count)
{
    // CPLD register is one byte
    uint8_t addr;
    ssize_t status;

    status = kstrtou8(buf, 0, &addr);
    if (status == 0) {
        fpga_data->cpld2_read_addr = addr;
        status = count;
    }
    return status;
}
struct device_attribute dev_attr_cpld2_getreg = __ATTR(getreg, 0600, cpld2_getreg_show, cpld2_getreg_store);

static ssize_t cpld2_dump_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    // CPLD register is one byte
    uint8_t data;
    fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD2_SLAVE_ADDR,0x00,I2C_SMBUS_READ,fpga_data->cpld2_read_addr,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&data);
    return sprintf(buf,"0x%2.2x\n",data);
}
static ssize_t cpld2_dump_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    // CPLD register is one byte
    uint32_t addr;
    char *last;
    addr = (uint8_t)strtoul(buf,&last,16);
    if(addr == 0 && buf == last){
        return -EINVAL;
    }
    fpga_data->cpld2_read_addr = addr;
    return size;
}
struct device_attribute dev_attr_cpld2_dump = __ATTR(dump,0600,cpld2_dump_show,cpld2_dump_store);

static ssize_t cpld2_scratch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    // CPLD register is one byte
    __u8 data;
    int err;
    err = fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD2_SLAVE_ADDR,0x00,I2C_SMBUS_READ,0x01,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&data);
    if(err < 0)
        return err;
    return sprintf(buf, "0x%2.2x\n",data);
}
static ssize_t cpld2_scratch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    // CPLD register is one byte
    __u8 data;
    char *last;
    int err;

    data = (uint8_t)strtoul(buf,&last,16);
    if(data == 0 && buf == last){
        return -EINVAL;
    }
    err = fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD2_SLAVE_ADDR,0x00,I2C_SMBUS_WRITE,0x01,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&data);
    if(err < 0)
        return err;
    return size;
}
struct device_attribute dev_attr_cpld2_scratch = __ATTR(scratch,0600,cpld2_scratch_show,cpld2_scratch_store);

static ssize_t cpld2_setreg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    uint8_t addr,value;
    char *tok;
    char clone[size];
    char *pclone = clone;
    int err;
    char *last;

    strscpy(clone, buf, size);

    tok = strsep((char**)&pclone, " ");
    if(tok == NULL){
        return -EINVAL;
    }
    addr = (uint8_t)strtoul(tok,&last,16);
    if(addr == 0 && tok == last){
        return -EINVAL;
    }
    tok = strsep((char**)&pclone, " ");
    if(tok == NULL){
        return -EINVAL;
    }
    value = (uint8_t)strtoul(tok,&last,16);
    if(value == 0 && tok == last){
        return -EINVAL;
    }

    err = fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD2_SLAVE_ADDR,0x00,I2C_SMBUS_WRITE,addr,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&value);
    if(err < 0)
        return err;

    return size;
}
struct device_attribute dev_attr_cpld2_setreg = __ATTR(setreg,0200,NULL,cpld2_setreg_store);

static struct attribute *cpld2_attrs[] = {
        &dev_attr_cpld2_getreg.attr,
        &dev_attr_cpld2_dump.attr,
        &dev_attr_cpld2_scratch.attr,
        &dev_attr_cpld2_setreg.attr,
        NULL,
};

static struct attribute_group cpld2_attr_grp = {
        .attrs = cpld2_attrs,
};

/* QSFP/SFP+ attributes */
static ssize_t qsfp_modirq_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u32 data;
    struct sff_device_data *dev_data = dev_get_drvdata(dev);
    unsigned int portid = dev_data->portid;
    unsigned int REGISTER = SFF_PORT_STATUS_BASE + (portid-1)*0x10;

    mutex_lock(&fpga_data->fpga_lock);
    data = ioread32(fpga_dev.data_base_addr+REGISTER);
    mutex_unlock(&fpga_data->fpga_lock);
    return sprintf(buf, "%d\n", (data >> STAT_IRQ) & 1U);
}
DEVICE_ATTR_RO(qsfp_modirq);

static ssize_t qsfp_modprs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u32 data;
    struct sff_device_data *dev_data = dev_get_drvdata(dev);
    unsigned int portid = dev_data->portid;
    unsigned int REGISTER = SFF_PORT_STATUS_BASE + (portid-1)*0x10;

    mutex_lock(&fpga_data->fpga_lock);
    data = ioread32(fpga_dev.data_base_addr+REGISTER);
    mutex_unlock(&fpga_data->fpga_lock);
    return sprintf(buf, "%d\n", (data >> STAT_PRESENT) & 1U);
}
DEVICE_ATTR_RO(qsfp_modprs);

static ssize_t sfp_txfault_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u32 data;
    struct sff_device_data *dev_data = dev_get_drvdata(dev);
    unsigned int portid = dev_data->portid;
    unsigned int REGISTER = SFF_PORT_STATUS_BASE + (portid-1)*0x10;

    mutex_lock(&fpga_data->fpga_lock);
    data = ioread32(fpga_dev.data_base_addr+REGISTER);
    mutex_unlock(&fpga_data->fpga_lock);
    return sprintf(buf, "%d\n", (data >> STAT_TXFAULT) & 1U);
}
DEVICE_ATTR_RO(sfp_txfault);

static ssize_t sfp_rxlos_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u32 data;
    struct sff_device_data *dev_data = dev_get_drvdata(dev);
    unsigned int portid = dev_data->portid;
    unsigned int REGISTER = SFF_PORT_STATUS_BASE + (portid-1)*0x10;

    mutex_lock(&fpga_data->fpga_lock);
    data = ioread32(fpga_dev.data_base_addr+REGISTER);
    mutex_unlock(&fpga_data->fpga_lock);
    return sprintf(buf, "%d\n", (data >> STAT_RXLOS) & 1U);
}
DEVICE_ATTR_RO(sfp_rxlos);

static ssize_t sfp_modabs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u32 data;
    struct sff_device_data *dev_data = dev_get_drvdata(dev);
    unsigned int portid = dev_data->portid;
    unsigned int REGISTER = SFF_PORT_STATUS_BASE + (portid-1)*0x10;

    mutex_lock(&fpga_data->fpga_lock);
    data = ioread32(fpga_dev.data_base_addr+REGISTER);
    mutex_unlock(&fpga_data->fpga_lock);
    return sprintf(buf, "%d\n", (data >> STAT_MODABS) & 1U);
}
DEVICE_ATTR_RO(sfp_modabs);

static ssize_t qsfp_lpmode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u32 data;
    struct sff_device_data *dev_data = dev_get_drvdata(dev);
    unsigned int portid = dev_data->portid;
    unsigned int REGISTER = SFF_PORT_CTRL_BASE + (portid-1)*0x10;

    mutex_lock(&fpga_data->fpga_lock);
    data = ioread32(fpga_dev.data_base_addr+REGISTER);
    mutex_unlock(&fpga_data->fpga_lock);
    return sprintf(buf, "%d\n", (data >> CTRL_LPMOD) & 1U);
}
static ssize_t qsfp_lpmode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    ssize_t status;
    long value;
    u32 data;
    struct sff_device_data *dev_data = dev_get_drvdata(dev);
    unsigned int portid = dev_data->portid;
    unsigned int REGISTER = SFF_PORT_CTRL_BASE + (portid-1)*0x10;

    mutex_lock(&fpga_data->fpga_lock);
    status = kstrtol(buf, 0, &value);
    if (status == 0) {
        // check if value is 0 clear
        data = ioread32(fpga_dev.data_base_addr+REGISTER);
        if(!value)
            data = data & ~( (u32)0x1 << CTRL_LPMOD);
        else
            data = data | ((u32)0x1 << CTRL_LPMOD);
        iowrite32(data,fpga_dev.data_base_addr+REGISTER);
        status = size;
    }
    mutex_unlock(&fpga_data->fpga_lock);
    return status;
}
DEVICE_ATTR_RW(qsfp_lpmode);

static ssize_t qsfp_reset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u32 data;
    struct sff_device_data *dev_data = dev_get_drvdata(dev);
    unsigned int portid = dev_data->portid;
    unsigned int REGISTER = SFF_PORT_CTRL_BASE + (portid-1)*0x10;

    mutex_lock(&fpga_data->fpga_lock);
    data = ioread32(fpga_dev.data_base_addr+REGISTER);
    mutex_unlock(&fpga_data->fpga_lock);
    return sprintf(buf, "%d\n", (data >> CTRL_RST) & 1U);
}

static ssize_t qsfp_reset_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    ssize_t status;
    long value;
    u32 data;
    struct sff_device_data *dev_data = dev_get_drvdata(dev);
    unsigned int portid = dev_data->portid;
    unsigned int REGISTER = SFF_PORT_CTRL_BASE + (portid-1)*0x10;

    mutex_lock(&fpga_data->fpga_lock);
    status = kstrtol(buf, 0, &value);
    if (status == 0) {
        // check if value is 0 clear
        data = ioread32(fpga_dev.data_base_addr+REGISTER);
        if(!value)
            data = data & ~( (u32)0x1 << CTRL_RST);
        else
            data = data | ((u32)0x1 << CTRL_RST);
        iowrite32(data,fpga_dev.data_base_addr+REGISTER);
        status = size;
    }
    mutex_unlock(&fpga_data->fpga_lock);
    return status;
}
DEVICE_ATTR_RW(qsfp_reset);

static ssize_t sfp_txdisable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u32 data;
    struct sff_device_data *dev_data = dev_get_drvdata(dev);
    unsigned int portid = dev_data->portid;
    unsigned int REGISTER = SFF_PORT_CTRL_BASE + (portid-1)*0x10;

    mutex_lock(&fpga_data->fpga_lock);
    data = ioread32(fpga_dev.data_base_addr+REGISTER);
    mutex_unlock(&fpga_data->fpga_lock);
    return sprintf(buf, "%d\n", (data >> CTRL_TXDIS) & 1U);
}
static ssize_t sfp_txdisable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    ssize_t status;
    long value;
    u32 data;
    struct sff_device_data *dev_data = dev_get_drvdata(dev);
    unsigned int portid = dev_data->portid;
    unsigned int REGISTER = SFF_PORT_CTRL_BASE + (portid-1)*0x10;

    mutex_lock(&fpga_data->fpga_lock);
    status = kstrtol(buf, 0, &value);
    if (status == 0) {
        // check if value is 0 clear
        data = ioread32(fpga_dev.data_base_addr+REGISTER);
        if(!value)
            data = data & ~( (u32)0x1 << CTRL_TXDIS);
        else
            data = data | ((u32)0x1 << CTRL_TXDIS);
        iowrite32(data,fpga_dev.data_base_addr+REGISTER);
        status = size;
    }
    mutex_unlock(&fpga_data->fpga_lock);
    return status;
}
DEVICE_ATTR_RW(sfp_txdisable);

static struct attribute *sff_attrs[] = {
    &dev_attr_qsfp_modirq.attr,
    &dev_attr_qsfp_modprs.attr,
    &dev_attr_qsfp_lpmode.attr,
    &dev_attr_qsfp_reset.attr,
    &dev_attr_sfp_txfault.attr,
    &dev_attr_sfp_rxlos.attr,
    &dev_attr_sfp_modabs.attr,
    &dev_attr_sfp_txdisable.attr,
    NULL,
};

static struct attribute_group sff_attr_grp = {
        .attrs = sff_attrs,
};

static const struct attribute_group *sff_attr_grps[] = {
    &sff_attr_grp,
    NULL
};


static ssize_t port_led_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    // value can be "nomal", "test"
    __u8 led_mode_1,led_mode_2;
    int err;
    err = fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD1_SLAVE_ADDR,0x00,I2C_SMBUS_READ,0x09,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&led_mode_1);
    if(err < 0)
        return err;
    err = fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD2_SLAVE_ADDR,0x00,I2C_SMBUS_READ,0x09,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&led_mode_2);
    if(err < 0)
        return err;
    return sprintf(buf, "%s %s\n",
            led_mode_1 ? "test" : "normal",
            led_mode_2 ? "test" : "normal");
}
static ssize_t port_led_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int status;
    __u8 led_mode_1;
    if(sysfs_streq(buf, "test")){
        led_mode_1 = 0x01;
    }else if(sysfs_streq(buf, "normal")){
        led_mode_1 = 0x00;
    }else{
        return -EINVAL;
    }
    status = fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD1_SLAVE_ADDR,0x00,
        I2C_SMBUS_WRITE,0x09,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&led_mode_1);
    status = fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD2_SLAVE_ADDR,0x00,
        I2C_SMBUS_WRITE,0x09,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&led_mode_1);
    return size;
}
DEVICE_ATTR_RW(port_led_mode);

// Only work when port_led_mode set to 1
static ssize_t port_led_color_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    // value can be "off", "green", "amber", "both"
    __u8 led_color1,led_color2;
    int err;
    err = fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD1_SLAVE_ADDR,0x00,I2C_SMBUS_READ,0x09,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&led_color1);
    if(err < 0)
        return err;
    err = fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD2_SLAVE_ADDR,0x00,I2C_SMBUS_READ,0x09,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&led_color2);
    if(err < 0)
        return err;
    return sprintf(buf, "%s %s\n",
            led_color1 == 0x03 ? "off" : led_color1 == 0x02 ? "green" : led_color1 ==0x01 ? "amber": "both",
            led_color2 == 0x03 ? "off" : led_color2 == 0x02 ? "green" : led_color2 ==0x01 ? "amber": "both");
}

static ssize_t port_led_color_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int status;
    __u8 led_color;
    if(sysfs_streq(buf, "off")){
        led_color = 0x03;
    }else if(sysfs_streq(buf, "green")){
        led_color = 0x02;
    }else if(sysfs_streq(buf, "amber")){
        led_color = 0x01;
    }else if(sysfs_streq(buf, "both")){
        led_color = 0x00;
    }else{
        status = -EINVAL;
        return status;
    }
    status = fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD1_SLAVE_ADDR,0x00,
        I2C_SMBUS_WRITE,0x0A,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&led_color);
    status = fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD2_SLAVE_ADDR,0x00,
        I2C_SMBUS_WRITE,0x0A,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&led_color);
    return size;
}
DEVICE_ATTR_RW(port_led_color);

static struct attribute *sff_led_test[] = {
    &dev_attr_port_led_mode.attr,
    &dev_attr_port_led_color.attr,
    NULL,
};

static struct attribute_group sff_led_test_grp = {
        .attrs = sff_led_test,
};

static struct device * seastone2_sff_init(int portid){
    struct sff_device_data *new_data;
    struct device *new_device;

    new_data = kzalloc(sizeof(*new_data), GFP_KERNEL);
    if (!new_data) {
        printk(KERN_ALERT "Cannot alloc sff device data @port%d", portid);
        return NULL;
    }
    /* The QSFP port ID start from 1 */
    new_data->portid = portid+1;
    new_data->port_type = fpga_i2c_bus_dev[portid].port_type;
    new_device = device_create_with_groups(fpgafwclass, sff_dev, MKDEV(0,0), new_data, sff_attr_grps, "%s",fpga_i2c_bus_dev[portid].calling_name);
    if (IS_ERR(new_device)) {
        printk(KERN_ALERT "Cannot create sff device @port%d", portid);
        kfree(new_data);
        return NULL;
    }
    return new_device;
}

static int i2c_wait_ack(struct i2c_adapter *a,unsigned long timeout,int writing){
    int error = 0;
    int Status;

    struct i2c_dev_data *new_data = i2c_get_adapdata(a);
    void __iomem *pci_bar = fpga_dev.data_base_addr;

    unsigned int REG_FDR0;
    unsigned int REG_CR0;
    unsigned int REG_SR0;
    unsigned int REG_DR0;
    unsigned int REG_ID0;

    unsigned int master_bus = new_data->pca9548.master_bus;

    if(master_bus < I2C_MASTER_CH_1 || master_bus > I2C_MASTER_CH_TOTAL){
        error = -ENXIO;
        return error;
    }

    REG_FDR0  = I2C_MASTER_FREQ_1    + (master_bus-1)*0x0100;
    REG_CR0   = I2C_MASTER_CTRL_1    + (master_bus-1)*0x0100;
    REG_SR0   = I2C_MASTER_STATUS_1  + (master_bus-1)*0x0100;
    REG_DR0   = I2C_MASTER_DATA_1    + (master_bus-1)*0x0100;
    REG_ID0   = I2C_MASTER_PORT_ID_1 + (master_bus-1)*0x0100;

    check(pci_bar+REG_SR0);
    check(pci_bar+REG_CR0);

    timeout = jiffies + msecs_to_jiffies(timeout);
    while(1){
        Status = ioread8(pci_bar+REG_SR0);
        if(jiffies > timeout){
            info("Status %2.2X",Status);
            info("Error Timeout");
            error = -ETIMEDOUT;
            break;
        }


        if(Status & (1 << I2C_SR_BIT_MIF)){
            break;
        }

        if(writing == 0 && (Status & (1<<I2C_SR_BIT_MCF))){
            break;
        }
    }
    Status = ioread8(pci_bar+REG_SR0);
    iowrite8(0, pci_bar+REG_SR0);

    if(error<0){
        info("Status %2.2X",Status);
        return error;
    }

    if(!(Status & (1 << I2C_SR_BIT_MCF))){
        info("Error Unfinish");
        return -EIO;
    }

    if(Status & (1 <<I2C_SR_BIT_MAL)){
        info("Error MAL");
        return -EAGAIN;
    }

    if(Status & (1 << I2C_SR_BIT_RXAK)){
        info( "SL No Acknowlege");
        if(writing){
            info("Error No Acknowlege");
            iowrite8(1<<I2C_CR_BIT_MEN,pci_bar+REG_CR0);
            return -ENXIO;
        }
    }else{
        info( "SL Acknowlege");
    }

    return 0;
}

static int smbus_access(struct i2c_adapter *adapter, u16 addr,
              unsigned short flags, char rw, u8 cmd,
              int size, union i2c_smbus_data *data)
{
        int error = 0;
        struct i2c_dev_data *dev_data;
        /* Write the command register */
        dev_data = i2c_get_adapdata(adapter);

        unsigned int  portid = dev_data->portid;
        void __iomem *pci_bar = fpga_dev.data_base_addr;

#ifdef DEBUG_KERN
        printk(KERN_INFO "portid %2d|@ 0x%2.2X|f 0x%4.4X|(%d)%-5s| (%d)%-15s|CMD %2.2X "
            ,portid,addr,flags,rw,rw == 1 ? "READ ":"WRITE"
            ,size,                  size == 0 ? "QUICK" :
                                    size == 1 ? "BYTE" :
                                    size == 2 ? "BYTE_DATA" :
                                    size == 3 ? "WORD_DATA" :
                                    size == 4 ? "PROC_CALL" :
                                    size == 5 ? "BLOCK_DATA" :
                                    size == 8 ? "I2C_BLOCK_DATA" :  "ERROR"
            ,cmd);
#endif
        /* Map the size to what the chip understands */
        switch (size) {
            case I2C_SMBUS_QUICK:
            case I2C_SMBUS_BYTE:
            case I2C_SMBUS_BYTE_DATA:
            case I2C_SMBUS_WORD_DATA:
            case I2C_SMBUS_BLOCK_DATA:
            case I2C_SMBUS_I2C_BLOCK_DATA:
                break;
            default:
                printk(KERN_INFO "Unsupported transaction %d\n", size);
                error = -EOPNOTSUPP;
                goto Done;
        }

        unsigned int REG_FDR0;
        unsigned int REG_CR0;
        unsigned int REG_SR0;
        unsigned int REG_DR0;
        unsigned int REG_ID0;

        unsigned int master_bus = dev_data->pca9548.master_bus;

        if(master_bus < I2C_MASTER_CH_1 || master_bus > I2C_MASTER_CH_TOTAL){
            error = -ENXIO;
            goto Done;
        }

        REG_FDR0  = I2C_MASTER_FREQ_1    + (master_bus-1)*0x0100;
        REG_CR0   = I2C_MASTER_CTRL_1    + (master_bus-1)*0x0100;
        REG_SR0   = I2C_MASTER_STATUS_1  + (master_bus-1)*0x0100;
        REG_DR0   = I2C_MASTER_DATA_1    + (master_bus-1)*0x0100;
        REG_ID0   = I2C_MASTER_PORT_ID_1 + (master_bus-1)*0x0100;

        iowrite8(portid,pci_bar+REG_ID0);

        int cnt=0;

        ////[S][ADDR/R]
        // Clear status register
        iowrite8(0,pci_bar+REG_SR0);
        iowrite8(1 << I2C_CR_BIT_MIEN | 1 << I2C_CR_BIT_MTX | 1 << I2C_CR_BIT_MSTA ,pci_bar+REG_CR0);
        SET_REG_BIT_H(pci_bar+REG_CR0,I2C_CR_BIT_MEN);

        if(rw == I2C_SMBUS_READ &&
                (size == I2C_SMBUS_QUICK || size == I2C_SMBUS_BYTE)){
            // sent device address with Read mode
            iowrite8(addr << 1 | 0x01,pci_bar+REG_DR0);
        }else{
            // sent device address with Write mode
           iowrite8(addr << 1 | 0x00,pci_bar+REG_DR0);
        }



        info( "MS Start");

        //// Wait {A}
        error = i2c_wait_ack(adapter,12,1);
        if(error<0){
            info( "get error %d",error);
            goto Done;
        }

        //// [CMD]{A}
        if(size == I2C_SMBUS_BYTE_DATA ||
            size == I2C_SMBUS_WORD_DATA ||
            size == I2C_SMBUS_BLOCK_DATA ||
            size == I2C_SMBUS_I2C_BLOCK_DATA ||
            (size == I2C_SMBUS_BYTE && rw == I2C_SMBUS_WRITE)){

            // sent command code to data register
            iowrite8(cmd,pci_bar+REG_DR0);
            info( "MS Send CMD 0x%2.2X",cmd);

            // Wait {A}
            error = i2c_wait_ack(adapter,12,1);
            if(error<0){
                info( "get error %d",error);
                goto Done;
            }
        }

        switch(size){
            case I2C_SMBUS_BYTE_DATA:
                    cnt = 1;  break;
            case I2C_SMBUS_WORD_DATA:
                    cnt = 2;  break;
            case I2C_SMBUS_BLOCK_DATA:
            case I2C_SMBUS_I2C_BLOCK_DATA:
            /* In block data modes keep number of byte in block[0] */
                    cnt = data->block[0];
                              break;
            default:
                    cnt = 0;  break;
        }

        // [CNT]  used only block data write
        if(size == I2C_SMBUS_BLOCK_DATA && rw == I2C_SMBUS_WRITE){

            iowrite8(cnt,pci_bar+REG_DR0);
            info( "MS Send CNT 0x%2.2X",cnt);

            // Wait {A}
            error = i2c_wait_ack(adapter,12,1);
            if(error<0){
                info( "get error %d",error);
                goto Done;
            }
        }

        // [DATA]{A}
        if( rw == I2C_SMBUS_WRITE && (
                    size == I2C_SMBUS_BYTE ||
                    size == I2C_SMBUS_BYTE_DATA ||
                    size == I2C_SMBUS_WORD_DATA ||
                    size == I2C_SMBUS_BLOCK_DATA ||
                    size == I2C_SMBUS_I2C_BLOCK_DATA
            )){
            int bid=0;
            info( "MS prepare to sent [%d bytes]",cnt);
            if(size == I2C_SMBUS_BLOCK_DATA || size == I2C_SMBUS_I2C_BLOCK_DATA){
                bid=1;      // block[0] is cnt;
                cnt+=1;     // offset from block[0]
            }
            for(;bid<cnt;bid++){

                iowrite8(data->block[bid],pci_bar+REG_DR0);
                info( "   Data > %2.2X",data->block[bid]);
                // Wait {A}
                error = i2c_wait_ack(adapter,12,1);
                if(error<0){
                    goto Done;
                }
            }

        }

        // REPEATE START
        if( rw == I2C_SMBUS_READ && (
                size == I2C_SMBUS_BYTE_DATA ||
                size == I2C_SMBUS_WORD_DATA ||
                size == I2C_SMBUS_BLOCK_DATA ||
                size == I2C_SMBUS_I2C_BLOCK_DATA
            )){
            info( "MS Repeated Start");

            SET_REG_BIT_L(pci_bar+REG_CR0,I2C_CR_BIT_MEN);
            iowrite8(1 << I2C_CR_BIT_MIEN |
                1 << I2C_CR_BIT_MTX |
                1 << I2C_CR_BIT_MSTA |
                1 << I2C_CR_BIT_RSTA ,pci_bar+REG_CR0);
            SET_REG_BIT_H(pci_bar+REG_CR0,I2C_CR_BIT_MEN);

            // sent Address with Read mode
            iowrite8( addr<<1 | 0x1 ,pci_bar+REG_DR0);

            // Wait {A}
            error = i2c_wait_ack(adapter,12,1);
            if(error<0){
                goto Done;
            }

        }

        if( rw == I2C_SMBUS_READ && (
                size == I2C_SMBUS_BYTE ||
                size == I2C_SMBUS_BYTE_DATA ||
                size == I2C_SMBUS_WORD_DATA ||
                size == I2C_SMBUS_BLOCK_DATA ||
                size == I2C_SMBUS_I2C_BLOCK_DATA
            )){

            switch(size){
                case I2C_SMBUS_BYTE:
                case I2C_SMBUS_BYTE_DATA:
                        cnt = 1;  break;
                case I2C_SMBUS_WORD_DATA:
                        cnt = 2;  break;
                case I2C_SMBUS_BLOCK_DATA:
                    // will be changed after recived first data
                        cnt = 3;  break;
                case I2C_SMBUS_I2C_BLOCK_DATA:
                        cnt = data->block[0];  break;
                default:
                        cnt = 0;  break;
            }

            int bid = 0;
            info( "MS Receive");

            //set to Receive mode
            iowrite8(1 << I2C_CR_BIT_MEN |
                1 << I2C_CR_BIT_MIEN |
                1 << I2C_CR_BIT_MSTA , pci_bar+REG_CR0);

            for(bid=-1;bid<cnt;bid++){

                // Wait for byte transfer
                error = i2c_wait_ack(adapter,12,0);
                if(error<0){
                    goto Done;
                }

                if(bid == cnt-2){
                    info( "SET NAK");
                    SET_REG_BIT_H(pci_bar+REG_CR0,I2C_CR_BIT_TXAK);
                }

                if(bid<0){
                    ioread8(pci_bar+REG_DR0);
                    info( "READ Dummy Byte" );
                }else{

                    if(bid==cnt-1){
                        info ( "SET STOP in read loop");
                        SET_REG_BIT_L(pci_bar+REG_CR0,I2C_CR_BIT_MSTA);
                    }
                    if(size == I2C_SMBUS_I2C_BLOCK_DATA){
                        // block[0] is read length
                        data->block[bid+1] = ioread8(pci_bar+REG_DR0);
                    }else {
                        data->block[bid] = ioread8(pci_bar+REG_DR0);
                    }
                    info( "DATA IN [%d] %2.2X",bid,data->block[bid]);

                    if(size == I2C_SMBUS_BLOCK_DATA && bid == 0){
                        cnt = data->block[0] + 1;
                    }
                }
            }
        }

Stop:
        // [P]
        SET_REG_BIT_L(pci_bar+REG_CR0,I2C_CR_BIT_MSTA);
        info( "MS STOP");

Done:
        iowrite8(1<<I2C_CR_BIT_MEN,pci_bar+REG_CR0);
        check(pci_bar+REG_CR0);
        check(pci_bar+REG_SR0);
#ifdef DEBUG_KERN
        printk(KERN_INFO "END --- Error code  %d",error);
#endif
        return error;
}

/**
 * Wrapper of smbus_access access with PCA9548 I2C switch management.
 * This function set PCA9548 switches to the proper slave channel.
 * Only one channel among switches chip is selected during communication time.
 *
 * Note: If the bus does not have any PCA9548 on it, the switch_addr must be
 * set to 0xFF, it will use normal smbus_access function.
 */
static int fpga_i2c_access(struct i2c_adapter *adapter, u16 addr,
              unsigned short flags, char rw, u8 cmd,
              int size, union i2c_smbus_data *data)
{
    int error = 0;
    struct i2c_dev_data *dev_data;
    dev_data = i2c_get_adapdata(adapter);
    unsigned char master_bus = dev_data->pca9548.master_bus;
    unsigned char switch_addr = dev_data->pca9548.switch_addr;
    unsigned char channel = dev_data->pca9548.channel;

    // Acquire the master resource.
    mutex_lock(&fpga_i2c_master_locks[master_bus-1]);
    uint16_t prev_port = fpga_i2c_lasted_access_port[master_bus-1];

    if(switch_addr != 0xFF){
        // Check lasted access switch address on a master
        if((unsigned char)(prev_port >> 8) == switch_addr){
            // check if channel is the same
            if((unsigned char)(prev_port & 0x00FF) != channel){
                // set new PCA9548 at switch_addr to current
                error= smbus_access(adapter,switch_addr,flags,I2C_SMBUS_WRITE,1 << channel,I2C_SMBUS_BYTE,NULL);
                // update lasted port
                fpga_i2c_lasted_access_port[master_bus-1] = switch_addr << 8 | channel;
            }
        }else{
            // reset prev_port PCA9548 chip
            error= smbus_access(adapter,(u16)(prev_port >> 8),flags,I2C_SMBUS_WRITE,0x00,I2C_SMBUS_BYTE,NULL);
            // set PCA9548 to current channel
            error= smbus_access(adapter,switch_addr,flags,I2C_SMBUS_WRITE,1 << channel,I2C_SMBUS_BYTE,NULL);
            // update lasted port
            fpga_i2c_lasted_access_port[master_bus-1] = switch_addr << 8 | channel;
        }
    }

    // Do SMBus communication
    error = smbus_access(adapter,addr,flags,rw,cmd,size,data);
    // reset the channel
    mutex_unlock(&fpga_i2c_master_locks[master_bus-1]);
    return error;
}



/**
 * A callback function show available smbus functions.
 */
static u32 fpga_i2c_func(struct i2c_adapter *a)
{
    return I2C_FUNC_SMBUS_QUICK  |
        I2C_FUNC_SMBUS_BYTE      |
        I2C_FUNC_SMBUS_BYTE_DATA |
        I2C_FUNC_SMBUS_WORD_DATA |
        I2C_FUNC_SMBUS_BLOCK_DATA|
        I2C_FUNC_SMBUS_I2C_BLOCK;
}

static const struct i2c_algorithm seastone2_i2c_algorithm = {
    .smbus_xfer = fpga_i2c_access,
    .functionality  = fpga_i2c_func,
};

/**
 * Create virtual I2C bus adapter for switch devices
 * @param  pdev             platform device pointer
 * @param  portid           virtual i2c port id for switch device mapping
 * @param  bus_number_offset bus offset for virtual i2c adapter in system
 * @return                  i2c adapter.
 *
 * When bus_number_offset is -1, created adapter with dynamic bus number.
 * Otherwise create adapter at i2c bus = bus_number_offset + portid.
 */
static struct i2c_adapter * seastone2_i2c_init(struct platform_device *pdev, int portid, int bus_number_offset)
{
    int error;

    struct i2c_adapter *new_adapter;
    struct i2c_dev_data *new_data;

    new_adapter = kzalloc(sizeof(*new_adapter), GFP_KERNEL);
    if (!new_adapter){
        printk(KERN_ALERT "Cannot alloc i2c adapter for %s", fpga_i2c_bus_dev[portid].calling_name);
        return NULL;
    }

    new_adapter->owner = THIS_MODULE;
    new_adapter->class = I2C_CLASS_HWMON;
    new_adapter->algo  = &seastone2_i2c_algorithm;
    /* If the bus offset is -1, use dynamic bus number */
    if (bus_number_offset == -1){
        new_adapter->nr = -1;
    }else{
        new_adapter->nr = bus_number_offset + portid;
    }

    new_data = kzalloc(sizeof(*new_data), GFP_KERNEL);
    if (!new_data){
        printk(KERN_ALERT "Cannot alloc i2c data for %s", fpga_i2c_bus_dev[portid].calling_name);
        kfree_sensitive(new_adapter);
        return NULL;
    }

    new_data->portid = portid;
    new_data->pca9548.master_bus = fpga_i2c_bus_dev[portid].master_bus;
    new_data->pca9548.switch_addr = fpga_i2c_bus_dev[portid].switch_addr;
    new_data->pca9548.channel = fpga_i2c_bus_dev[portid].channel;
    strscpy(new_data->pca9548.calling_name, fpga_i2c_bus_dev[portid].calling_name,sizeof(new_data->pca9548.calling_name));

    snprintf(new_adapter->name, sizeof(new_adapter->name),
        "SMBus I2C Adapter PortID: %s", new_data->pca9548.calling_name);

    void __iomem *i2c_freq_base_reg = fpga_dev.data_base_addr+I2C_MASTER_FREQ_1;
    iowrite8(0x07,i2c_freq_base_reg+(new_data->pca9548.master_bus-1)*0x100); // 0x07 400kHz
    i2c_set_adapdata(new_adapter,new_data);
    error = i2c_add_numbered_adapter(new_adapter);
    if(error < 0){
        printk(KERN_ALERT "Cannot add i2c adapter %s", new_data->pca9548.calling_name);
        kfree_sensitive(new_adapter);
        kfree_sensitive(new_data);
        return NULL;
    }

    return new_adapter;
};

// I/O resource need.
static struct resource seastone2_resources[] = {
    {
        .start  = 0x10000000,
        .end    = 0x10001000,
        .flags  = IORESOURCE_MEM,
    },
};

static void seastone2_dev_release( struct device * dev)
{
    return;
}

static struct platform_device seastone2_dev = {
    .name           = DRIVER_NAME,
    .id             = -1,
    .num_resources  = ARRAY_SIZE(seastone2_resources),
    .resource       = seastone2_resources,
    .dev = {
                    .release = seastone2_dev_release,
    }
};

/**
 * Board info for QSFP/SFP+ eeprom.
 * Note: Using sff8436 as I2C eeprom driver.
 */
static struct i2c_board_info sff8436_eeprom_info[] = {
    { I2C_BOARD_INFO("optoe1", 0x50) },
    { I2C_BOARD_INFO("optoe2", 0x50) },
};

static int seastone2_drv_probe(struct platform_device *pdev)
{
    struct resource *res;
    int ret = 0;
    int portid_count;
    uint8_t cpld1_version, cpld2_version;
    uint16_t prev_i2c_switch = 0;

    /* The device class need to be instantiated before this function called */
    BUG_ON(fpgafwclass == NULL);

    fpga_data = devm_kzalloc(&pdev->dev, sizeof(struct seastone2_fpga_data),
                    GFP_KERNEL);

    if (!fpga_data)
        return -ENOMEM;

    // Set default read address to VERSION
    fpga_data->fpga_read_addr = fpga_dev.data_base_addr+FPGA_VERSION;
    fpga_data->cpld1_read_addr = 0x00;
    fpga_data->cpld2_read_addr = 0x00;

    mutex_init(&fpga_data->fpga_lock);
    for(ret=I2C_MASTER_CH_1 ;ret <= I2C_MASTER_CH_TOTAL; ret++){
        mutex_init(&fpga_i2c_master_locks[ret-1]);
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (unlikely(!res)) {
        printk(KERN_ERR "Specified Resource Not Available...\n");
        kfree_sensitive(fpga_data);
        return -1;
    }

    fpga = kobject_create_and_add("FPGA", &pdev->dev.kobj);
    if (!fpga){
        kfree_sensitive(fpga_data);
        return -ENOMEM;
    }

    ret = sysfs_create_group(fpga, &fpga_attr_grp);
    if (ret != 0) {
        printk(KERN_ERR "Cannot create FPGA sysfs attributes\n");
        kobject_put(fpga);
        kfree_sensitive(fpga_data);
        return ret;
    }

    cpld1 = kobject_create_and_add("CPLD1", &pdev->dev.kobj);
    if (!cpld1){
        sysfs_remove_group(fpga, &fpga_attr_grp);
        kobject_put(fpga);
        kfree_sensitive(fpga_data);
        return -ENOMEM;
    }
    ret = sysfs_create_group(cpld1, &cpld1_attr_grp);
    if (ret != 0) {
        printk(KERN_ERR "Cannot create CPLD1 sysfs attributes\n");
        kobject_put(cpld1);
        sysfs_remove_group(fpga, &fpga_attr_grp);
        kobject_put(fpga);
        kfree_sensitive(fpga_data);
        return ret;
    }

    cpld2 = kobject_create_and_add("CPLD2", &pdev->dev.kobj);
    if (!cpld2){
        sysfs_remove_group(cpld1, &cpld1_attr_grp);
        kobject_put(cpld1);
        sysfs_remove_group(fpga, &fpga_attr_grp);
        kobject_put(fpga);
        kfree_sensitive(fpga_data);
        return -ENOMEM;
    }
    ret = sysfs_create_group(cpld2, &cpld2_attr_grp);
    if (ret != 0) {
        printk(KERN_ERR "Cannot create CPLD2 sysfs attributes\n");
        kobject_put(cpld2);
        sysfs_remove_group(cpld1, &cpld1_attr_grp);
        kobject_put(cpld1);
        sysfs_remove_group(fpga, &fpga_attr_grp);
        kobject_put(fpga);
        kfree_sensitive(fpga_data);
        return ret;
    }

    sff_dev = device_create(fpgafwclass, NULL, MKDEV(0,0), NULL, "sff_device");
    if (IS_ERR(sff_dev)){
        printk(KERN_ERR "Failed to create sff device\n");
        sysfs_remove_group(cpld2, &cpld2_attr_grp);
        kobject_put(cpld2);
        sysfs_remove_group(cpld1, &cpld1_attr_grp);
        kobject_put(cpld1);
        sysfs_remove_group(fpga, &fpga_attr_grp);
        kobject_put(fpga);
        kfree_sensitive(fpga_data);
        return PTR_ERR(sff_dev);
    }

    ret = sysfs_create_group(&sff_dev->kobj, &sff_led_test_grp);
    if (ret != 0) {
        printk(KERN_ERR "Cannot create SFF attributes\n");
        device_destroy(fpgafwclass, MKDEV(0,0));
        sysfs_remove_group(cpld2, &cpld2_attr_grp);
        kobject_put(cpld2);
        sysfs_remove_group(cpld1, &cpld1_attr_grp);
        kobject_put(cpld1);
        sysfs_remove_group(fpga, &fpga_attr_grp);
        kobject_put(fpga);
        kfree_sensitive(fpga_data);
        return ret;
    }

    ret = sysfs_create_link(&pdev->dev.kobj,&sff_dev->kobj,"SFF");
    if (ret != 0){
        sysfs_remove_group(&sff_dev->kobj, &sff_led_test_grp);
        device_destroy(fpgafwclass, MKDEV(0,0));
        sysfs_remove_group(cpld2, &cpld2_attr_grp);
        kobject_put(cpld2);
        sysfs_remove_group(cpld1, &cpld1_attr_grp);
        kobject_put(cpld1);
        sysfs_remove_group(fpga, &fpga_attr_grp);
        kobject_put(fpga);
        kfree_sensitive(fpga_data);
        return ret;
    }

    for(portid_count=0 ; portid_count < VIRTUAL_I2C_PORT_LENGTH ; portid_count++){
        fpga_data->i2c_adapter[portid_count] = seastone2_i2c_init(pdev, portid_count, VIRTUAL_I2C_BUS_OFFSET);
    }


    /* Init SFF devices */
    for(portid_count=0; portid_count < SFF_PORT_TOTAL; portid_count++){
        struct i2c_adapter *i2c_adap = fpga_data->i2c_adapter[portid_count];
        if(i2c_adap){
            fpga_data->sff_devices[portid_count] = seastone2_sff_init(portid_count);
            struct sff_device_data *sff_data = dev_get_drvdata(fpga_data->sff_devices[portid_count]);
            BUG_ON(sff_data == NULL);
            if( sff_data->port_type == QSFP ){
                fpga_data->sff_i2c_clients[portid_count] = i2c_new_client_device(i2c_adap, &sff8436_eeprom_info[0]);
            }else{
                fpga_data->sff_i2c_clients[portid_count] = i2c_new_client_device(i2c_adap, &sff8436_eeprom_info[1]);
            }
            sff_data = NULL;
            sysfs_create_link(&fpga_data->sff_devices[portid_count]->kobj,
                &fpga_data->sff_i2c_clients[portid_count]->dev.kobj,
                "i2c");
        }

    }

    printk(KERN_INFO "Virtual I2C buses created\n");

#ifdef TEST_MODE
    return 0;
#endif
    fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD1_SLAVE_ADDR,0x00,
        I2C_SMBUS_READ,0x00,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&cpld1_version);
    fpga_i2c_access(fpga_data->i2c_adapter[VIRTUAL_I2C_CPLD_INDEX],CPLD2_SLAVE_ADDR,0x00,
        I2C_SMBUS_READ,0x00,I2C_SMBUS_BYTE_DATA,(union i2c_smbus_data*)&cpld2_version);

    printk(KERN_INFO "CPLD1 VERSON: %2.2x\n", cpld1_version);
    printk(KERN_INFO "CPLD2 VERSON: %2.2x\n", cpld2_version);

    /* Init I2C buses that has PCA9548 switch device. */
    for(portid_count = 0; portid_count < VIRTUAL_I2C_PORT_LENGTH; portid_count++){

       struct i2c_dev_data *dev_data;
       dev_data = i2c_get_adapdata(fpga_data->i2c_adapter[portid_count]);
       unsigned char master_bus = dev_data->pca9548.master_bus;
       unsigned char switch_addr = dev_data->pca9548.switch_addr;
       unsigned char channel = dev_data->pca9548.channel;

       if(switch_addr != 0xFF){

           if(prev_i2c_switch != ( (master_bus << 8) | switch_addr) ){
               // Found the bus with PCA9548, trying to clear all switch in it.
               smbus_access(fpga_data->i2c_adapter[portid_count],switch_addr,0x00,I2C_SMBUS_WRITE,0x00,I2C_SMBUS_BYTE,NULL);
               prev_i2c_switch = ( master_bus << 8 ) | switch_addr;
           }
       }
    }
    return 0;
}

static void seastone2_drv_remove(struct platform_device *pdev)
{
    int portid_count;
    struct sff_device_data *rem_data;

    for(portid_count=0; portid_count < SFF_PORT_TOTAL; portid_count++){
        sysfs_remove_link(&fpga_data->sff_devices[portid_count]->kobj,"i2c");
        i2c_unregister_device(fpga_data->sff_i2c_clients[portid_count]);
    }

    for(portid_count=0 ; portid_count < VIRTUAL_I2C_PORT_LENGTH ; portid_count++){
        if(fpga_data->i2c_adapter[portid_count] != NULL){
            info(KERN_INFO "<%x>",fpga_data->i2c_adapter[portid_count]);
            i2c_del_adapter(fpga_data->i2c_adapter[portid_count]);
        }
    }

    for (portid_count=0; portid_count < SFF_PORT_TOTAL; portid_count++){
        if(fpga_data->sff_devices[portid_count] != NULL){
            rem_data = dev_get_drvdata(fpga_data->sff_devices[portid_count]);
            device_unregister(fpga_data->sff_devices[portid_count]);
            put_device(fpga_data->sff_devices[portid_count]);
            kfree(rem_data);
        }
    }

    sysfs_remove_group(fpga, &fpga_attr_grp);
    sysfs_remove_group(cpld1, &cpld1_attr_grp);
    sysfs_remove_group(cpld2, &cpld2_attr_grp);
    sysfs_remove_group(&sff_dev->kobj, &sff_led_test_grp);
    kobject_put(fpga);
    kobject_put(cpld1);
    kobject_put(cpld2);
    device_destroy(fpgafwclass, MKDEV(0,0));
    devm_kfree(&pdev->dev, fpga_data);
}

#ifdef TEST_MODE
    #define FPGA_PCI_BAR_NUM 2
#else
    #define FPGA_PCI_BAR_NUM 0
#endif



static const struct pci_device_id fpga_id_table[] = {
    {  PCI_VDEVICE(XILINX, FPGA_PCIE_DEVICE_ID) },
    {  PCI_VDEVICE(TEST, TEST_PCIE_DEVICE_ID) },
    {0, }
};

MODULE_DEVICE_TABLE(pci, fpga_id_table);

static int fpga_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    int err;
    struct device *dev = &pdev->dev;
        if ((err = pci_enable_device(pdev))) {
        dev_err(dev, "pci_enable_device probe error %d for device %s\n",
            err, pci_name(pdev));
        return err;
    }

    if ((err = pci_request_regions(pdev, FPGA_PCI_NAME)) < 0) {
        dev_err(dev, "pci_request_regions error %d\n", err);
        goto pci_disable;
    }

    /* bar0: data mmio region */
    fpga_dev.data_mmio_start = pci_resource_start(pdev, FPGA_PCI_BAR_NUM);
    fpga_dev.data_mmio_len = pci_resource_len(pdev, FPGA_PCI_BAR_NUM);
    fpga_dev.data_base_addr = pci_iomap(pdev, FPGA_PCI_BAR_NUM, 0);
    if (!fpga_dev.data_base_addr) {
        dev_err(dev, "cannot iomap region of size %lu\n",
            (unsigned long)fpga_dev.data_mmio_len);
        goto pci_release;
    }
    dev_info(dev, "data_mmio iomap base = 0x%lx \n",
         (unsigned long)fpga_dev.data_base_addr);
    dev_info(dev, "data_mmio_start = 0x%lx data_mmio_len = %lu\n",
         (unsigned long)fpga_dev.data_mmio_start,
         (unsigned long)fpga_dev.data_mmio_len);

    printk(KERN_INFO "FPGA PCIe driver probe OK.\n");
    printk(KERN_INFO "FPGA ioremap registers of size %lu\n",(unsigned long)fpga_dev.data_mmio_len);
    printk(KERN_INFO "FPGA Virtual BAR %d at %8.8lx - %8.8lx\n",FPGA_PCI_BAR_NUM,(unsigned long)fpga_dev.data_base_addr,(unsigned long)fpga_dev.data_base_addr+ (unsigned long)fpga_dev.data_mmio_len);
    printk(KERN_INFO "");
    uint32_t buff = ioread32(fpga_dev.data_base_addr);
    printk(KERN_INFO "FPGA VERSION : %8.8x\n", buff);
    fpgafw_init();
    return 0;

reg_release:
    pci_iounmap(pdev, fpga_dev.data_base_addr);
pci_release:
    pci_release_regions(pdev);
pci_disable:
    pci_disable_device(pdev);
    return -EBUSY;
}

static void fpga_pci_remove(struct pci_dev *pdev)
{
    fpgafw_exit();
    pci_iounmap(pdev, fpga_dev.data_base_addr);
    pci_release_regions(pdev);
    pci_disable_device(pdev);
    printk(KERN_INFO "FPGA PCIe driver remove OK.\n");
};

static struct pci_driver pci_dev_ops = {
    .name       = FPGA_PCI_NAME,
    .probe      = fpga_pci_probe,
    .remove     = fpga_pci_remove,
    .id_table   = fpga_id_table,
};


static struct platform_driver seastone2_drv = {
    .probe  = seastone2_drv_probe,
    .remove = __exit_p(seastone2_drv_remove),
    .driver = {
        .name   = DRIVER_NAME,
    },
};

enum{
    READREG,
    WRITEREG
};

struct fpga_reg_data {
    uint32_t addr;
    uint32_t value;
};

static long fpgafw_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
    int ret = 0;
    struct fpga_reg_data data;
    mutex_lock(&fpga_data->fpga_lock);

#ifdef TEST_MODE
    static  uint32_t status_reg;
#endif
    // Switch function to read and write.
    switch (cmd){
        case READREG:
            if (copy_from_user(&data, (void __user*)arg, sizeof(data)) != 0){
                mutex_unlock(&fpga_data->fpga_lock);
                return -EFAULT;
            }
            data.value = ioread32(fpga_dev.data_base_addr+data.addr);
            if (copy_to_user((void __user*)arg ,&data, sizeof(data)) != 0){
                mutex_unlock(&fpga_data->fpga_lock);
                return -EFAULT;
            }
#ifdef TEST_MODE
            if(data.addr == 0x1210){
                switch (status_reg){
                    case 0x0000 : status_reg=0x8000;
                    break;

                    case 0x8080 : status_reg=0x80C0;
                    break;
                    case 0x80C0 : status_reg=0x80F0;
                    break;
                    case 0x80F0 : status_reg=0x80F8;
                    break;

                }
                iowrite32(status_reg,fpga_dev.data_base_addr+0x1210);
            }
#endif


            break;
        case WRITEREG:
            if (copy_from_user(&data, (void __user*)arg, sizeof(data)) != 0){
                mutex_unlock(&fpga_data->fpga_lock);
                return -EFAULT;
            }
            iowrite32(data.value,fpga_dev.data_base_addr+data.addr);

#ifdef TEST_MODE
            if(data.addr == 0x1204){
                status_reg=0x8080;
                iowrite32(status_reg,fpga_dev.data_base_addr+0x1210);
            }
#endif

            break;
        default:
            mutex_unlock(&fpga_data->fpga_lock);
            return -EINVAL;
    }
    mutex_unlock(&fpga_data->fpga_lock);
    return ret;
}


const struct file_operations fpgafw_fops = {
    .owner      = THIS_MODULE,
    .unlocked_ioctl = fpgafw_unlocked_ioctl,
};


static int fpgafw_init(void){
   printk(KERN_INFO "Initializing the switchboard driver\n");
   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fpgafw_fops);
   if (majorNumber<0){
      printk(KERN_ALERT "Failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "Device registered correctly with major number %d\n", majorNumber);

   // Register the device class
   fpgafwclass = class_create(CLASS_NAME);
   if (IS_ERR(fpgafwclass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(fpgafwclass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "Device class registered correctly\n");

   // Register the device driver
   fpgafwdev = device_create(fpgafwclass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(fpgafwdev)){               // Clean up if there is an error
      class_destroy(fpgafwclass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the FW upgrade device node\n");
      return PTR_ERR(fpgafwdev);
   }
   printk(KERN_INFO "FPGA fw upgrade device node created correctly\n"); // Made it! device was initialized
   return 0;
}

static void fpgafw_exit(void){
   device_destroy(fpgafwclass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(fpgafwclass);                          // unregister the device class
   class_destroy(fpgafwclass);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);            // unregister the major number
   printk(KERN_INFO "Goodbye!\n");
}

int seastone2_init(void)
{
    int rc;
    rc = pci_register_driver(&pci_dev_ops);
    if (rc)
        return rc;
    if(fpga_dev.data_base_addr == NULL){
        printk(KERN_ALERT "FPGA PCIe device not found!\n");
        return -ENODEV;
    }
    platform_device_register(&seastone2_dev);
    platform_driver_register(&seastone2_drv);
    return 0;
}

void seastone2_exit(void)
{
    platform_driver_unregister(&seastone2_drv);
    platform_device_unregister(&seastone2_dev);
    pci_unregister_driver(&pci_dev_ops);
}

module_init(seastone2_init);
module_exit(seastone2_exit);

MODULE_AUTHOR("Pradchaya P. pphuhcar@celestica.com");
#ifdef SEASTONE2
MODULE_DESCRIPTION("Celestica seastone2 platform driver");
#else
MODULE_DESCRIPTION("Celestica questone2 platform driver");
#endif
MODULE_VERSION(MOD_VERSION);
MODULE_LICENSE("GPL");

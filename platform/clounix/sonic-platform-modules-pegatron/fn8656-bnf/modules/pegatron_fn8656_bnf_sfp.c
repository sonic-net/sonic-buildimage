/*
 * pegatron_fn8656_bnf_sfp.c - A driver to read and write the EEPROM on optical transceivers
 * (SFP and QSFP)
 *
 * Copyright (C) 2014 Cumulus networks Inc.
 * Copyright (C) 2017 Finisar Corp.
 * Copyright (C) 2019 Pegatron Corp. <Peter5_Lin@pegatroncorp.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Freeoftware Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 *  Description:
 *  a) Optical transceiver EEPROM read/write transactions are just like
 *      the at24 eeproms managed by the at24.c i2c driver
 *  b) The register/memory layout is up to 256 128 byte pages defined by
 *      a "pages valid" register and switched via a "page select"
 *      register as explained in below diagram.
 *  c) 256 bytes are mapped at a time. 'Lower page 00h' is the first 128
 *          bytes of address space, and always references the same
 *          location, independent of the page select register.
 *          All mapped pages are mapped into the upper 128 bytes
 *          (offset 128-255) of the i2c address.
 *  d) Devices with one I2C address (eg QSFP) use I2C address 0x50
 *      (A0h in the spec), and map all pages in the upper 128 bytes
 *      of that address.
 *  e) Devices with two I2C addresses (eg SFP) have 256 bytes of data
 *      at I2C address 0x50, and 256 bytes of data at I2C address
 *      0x51 (A2h in the spec).  Page selection and paged access
 *      only apply to this second I2C address (0x51).
 *  e) The address space is presented, by the driver, as a linear
 *          address space.  For devices with one I2C client at address
 *          0x50 (eg QSFP), offset 0-127 are in the lower
 *          half of address 50/A0h/client[0].  Offset 128-255 are in
 *          page 0, 256-383 are page 1, etc.  More generally, offset
 *          'n' resides in page (n/128)-1.  ('page -1' is the lower
 *          half, offset 0-127).
 *  f) For devices with two I2C clients at address 0x50 and 0x51 (eg SFP),
 *      the address space places offset 0-127 in the lower
 *          half of 50/A0/client[0], offset 128-255 in the upper
 *          half.  Offset 256-383 is in the lower half of 51/A2/client[1].
 *          Offset 384-511 is in page 0, in the upper half of 51/A2/...
 *          Offset 512-639 is in page 1, in the upper half of 51/A2/...
 *          Offset 'n' is in page (n/128)-3 (for n > 383)
 *
 *                      One I2c addressed (eg QSFP) Memory Map
 *
 *                      2-Wire Serial Address: 1010000x
 *
 *                      Lower Page 00h (128 bytes)
 *                      =====================
 *                     |                     |
 *                     |                     |
 *                     |                     |
 *                     |                     |
 *                     |                     |
 *                     |                     |
 *                     |                     |
 *                     |                     |
 *                     |                     |
 *                     |                     |
 *                     |Page Select Byte(127)|
 *                      =====================
 *                                |
 *                                |
 *                                |
 *                                |
 *                                V
 *       ------------------------------------------------------------
 *      |                 |                  |                       |
 *      |                 |                  |                       |
 *      |                 |                  |                       |
 *      |                 |                  |                       |
 *      |                 |                  |                       |
 *      |                 |                  |                       |
 *      |                 |                  |                       |
 *      |                 |                  |                       |
 *      |                 |                  |                       |
 *      V                 V                  V                       V
 *   ------------   --------------      ---------------     --------------
 *  |            | |              |    |               |   |              |
 *  |   Upper    | |     Upper    |    |     Upper     |   |    Upper     |
 *  |  Page 00h  | |    Page 01h  |    |    Page 02h   |   |   Page 03h   |
 *  |            | |   (Optional) |    |   (Optional)  |   |  (Optional   |
 *  |            | |              |    |               |   |   for Cable  |
 *  |            | |              |    |               |   |  Assemblies) |
 *  |    ID      | |     AST      |    |      User     |   |              |
 *  |  Fields    | |    Table     |    |   EEPROM Data |   |              |
 *  |            | |              |    |               |   |              |
 *  |            | |              |    |               |   |              |
 *  |            | |              |    |               |   |              |
 *   ------------   --------------      ---------------     --------------
 *
 * The SFF 8436 (QSFP) spec only defines the 4 pages described above.
 * In anticipation of future applications and devices, this driver
 * supports access to the full architected range, 256 pages.
 *
 * The CMIS (Common Management Interface Specification) defines use of
 * considerably more pages (at least to page 0xAF), which this driver
 * supports.
 *
 * NOTE: This version of the driver ONLY SUPPORTS BANK 0 PAGES on CMIS
 * devices.
 *
 **/

#define PEGA_DEBUG 1
/*#define pega_DEBUG*/
#ifdef PEGA_DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif /* DEBUG */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/types.h>
#include <linux/version.h>
#include "pegatron_pub.h"
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#define NUM_ADDRESS 2

/* The maximum length of a port name */
#define MAX_PORT_NAME_LEN 20

/* fundamental unit of addressing for EEPROM */
#define SFP_PAGE_SIZE 128

/*
 * Single address devices (eg QSFP) have 256 pages, plus the unpaged
 * low 128 bytes.  If the device does not support paging, it is
 * only 2 'pages' long.
 */
#define SFP_ARCH_PAGES 256
#define ONE_ADDR_EEPROM_SIZE ((1 + SFP_ARCH_PAGES) * SFP_PAGE_SIZE)
#define ONE_ADDR_EEPROM_UNPAGED_SIZE (2 * SFP_PAGE_SIZE)

/*
 * Dual address devices (eg SFP) have 256 pages, plus the unpaged
 * low 128 bytes, plus 256 bytes at 0x50.  If the device does not
 * support paging, it is 4 'pages' long.
 */
#define TWO_ADDR_EEPROM_SIZE ((3 + SFP_ARCH_PAGES) * SFP_PAGE_SIZE)
#define TWO_ADDR_EEPROM_UNPAGED_SIZE (4 * SFP_PAGE_SIZE)
#define TWO_ADDR_NO_0X51_SIZE (2 * SFP_PAGE_SIZE)

/*
 * flags to distinguish one-address (QSFP family) from two-address (SFP family)
 * If the family is not known, figure it out when the device is accessed
 */
#define ONE_ADDR 1
#define TWO_ADDR 2
#define CMIS_ADDR 3

/* a few constants to find our way around the EEPROM */\
#define SFP_EEPROM_A0_ADDR  0xA0
#define SFP_EEPROM_A2_ADDR  0xA2
#define SFP_PAGE_SELECT_REG   0x7F
#define ONE_ADDR_PAGEABLE_REG 0x02
#define QSFP_NOT_PAGEABLE (1<<2)
#define CMIS_NOT_PAGEABLE (1<<7)
#define TWO_ADDR_PAGEABLE_REG 0x40
#define TWO_ADDR_PAGEABLE (1<<4)
#define TWO_ADDR_0X51_REG 92
#define TWO_ADDR_0X51_SUPP (1<<6)
#define SFP_ID_REG 0
#define SFP_READ_OP 0
#define SFP_WRITE_OP 1
#define SFP_EOF 0  /* used for access beyond end of device */
#define MAX_PORT_NUM                (56)
#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)

#define SFP_FAST_BYTE_LENGTH 4

#define SFP_LOWER_PAGE              (0x0)
#define SFP_CACHE_PAGE_NUM          (0x11)
#define MAX_CACHE_LEN               (SFP_PAGE_SIZE*(SFP_CACHE_PAGE_NUM + 1)) //SFP_CACHE_PAGE_NUM upper pages and 1 lower page

#define FPGA_I2C_RD_MODE_BLOCK 1
#define FPGA_I2C_RD_MODE_BYTE 0

static struct task_struct *sfp_task;
static char sfp_eeprom_cache[MAX_PORT_NUM][MAX_CACHE_LEN];
static int enable_sfp_eeprom_cache = FALSE;
static int enable_sfp_three_batch =TRUE;

struct fn8656_bnf_sfp_platform_data {
    u32     byte_len;       /* size (sum of all addr) */
    u16     page_size;      /* for writes */
    u8      flags;
    void        *dummy1;        /* backward compatibility */
    void        *dummy2;        /* backward compatibility */

    /* dev_class: ONE_ADDR (QSFP) or TWO_ADDR (SFP) */
    int          dev_class;
    int          slave_addr;
    unsigned int write_max;
};

struct kwai_fpga{
    struct pci_dev *pdev;
    struct mutex lock;
    void __iomem *mmio;
    unsigned int i2c_mode;
    struct fn8656_bnf_sfp_platform_data chip[MAX_PORT_NUM+1];
};

/*
 * This parameter is to help this driver avoid blocking other drivers out
 * of I2C for potentially troublesome amounts of time. With a 100 kHz I2C
 * clock, one 256 byte read takes about 1/43 second which is excessive;
 * but the 1/170 second it takes at 400 kHz may be quite reasonable; and
 * at 1 MHz (Fm+) a 1/430 second delay could easily be invisible.
 *
 * This value is forced to be a power of two so that writes align on pages.
 */
//static unsigned int io_limit = SFP_PAGE_SIZE;

/*
 * specs often allow 5 msec for a page write, sometimes 20 msec;
 * it's important to recover from write timeouts.
 */
static unsigned int write_timeout = 250;

static uint loglevel = LOG_INFO | LOG_WARNING | LOG_ERR;
static char fpga_debug[MAX_DEBUG_INFO_LEN] = "fpga debug info:            \n";
/*-------------------------------------------------------------------------*/
/*
 * This routine computes the addressing information to be used for
 * a given r/w request.
 *
 * Task is to calculate the client (0 = i2c addr 50, 1 = i2c addr 51),
 * the page, and the offset.
 *
 * Handles both single address (eg QSFP) and two address (eg SFP).
 *     For SFP, offset 0-255 are on client[0], >255 is on client[1]
 *     Offset 256-383 are on the lower half of client[1]
 *     Pages are accessible on the upper half of client[1].
 *     Offset >383 are in 128 byte pages mapped into the upper half
 *
 *     For QSFP, all offsets are on client[0]
 *     offset 0-127 are on the lower half of client[0] (no paging)
 *     Pages are accessible on the upper half of client[1].
 *     Offset >127 are in 128 byte pages mapped into the upper half
 *
 *     Callers must not read/write beyond the end of a client or a page
 *     without recomputing the client/page.  Hence offset (within page)
 *     plus length must be less than or equal to 128.  (Note that this
 *     routine does not have access to the length of the call, hence
 *     cannot do the validity check.)
 *
 * Offset within Lower Page 00h and Upper Page 00h are not recomputed
 */

#define FPGA_HW_VERSION     (0)
#define FPGA_SW_VERSION     (4)

#define FPGA_NUM   1

static ssize_t get_fpga_chip_num(struct device *dev, struct device_attribute *da,
             char *buf)
{
    return sprintf(buf, "%d\n", FPGA_NUM);
}
static ssize_t get_fpga_type(struct device *dev, struct device_attribute *da,
             char *buf)
{
    return sprintf(buf, "xilinx artix-7:xc7a75tfgg484-2\n");
}
static ssize_t get_fpga_alias(struct device *dev, struct device_attribute *da,
             char *buf)
{
    return sprintf(buf, "fpga\n");
}

static ssize_t read_fpga_HWversion(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct kwai_fpga *fpga = pdev->sysdata;

    uint8_t data = readl(fpga->mmio + FPGA_HW_VERSION);
    pega_print(DEBUG,"read fpga addr:0x%x,data:0x%x\n",fpga->mmio + FPGA_HW_VERSION,data);

    return sprintf(buf, "%02d\n", data);
}

static ssize_t read_fpga_SWversion(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct pci_dev *pci_dev = to_pci_dev(dev);
    struct kwai_fpga *fpga = pci_dev->sysdata;

    uint8_t data = readl(fpga->mmio + FPGA_SW_VERSION);
    pega_print(DEBUG,"read fpga addr:0x%x,data:0x%x\n",fpga->mmio + FPGA_SW_VERSION,data);

    return sprintf(buf, "%02d\n", data);
}

struct fpga_device_attribute{
        struct device_attribute dev_attr;
        int index;
};
#define to_fpga_dev_attr(_dev_attr) \
        container_of(_dev_attr, struct fpga_device_attribute, dev_attr)

#define FPGA_ATTR(_name, _mode, _show, _store, _index)        \
        { .dev_attr = __ATTR(_name, _mode, _show, _store),      \
          .index = _index }

#define FPGA_DEVICE_ATTR(_name, _mode, _show, _store, _index) \
struct fpga_device_attribute fpga_dev_attr_##_name          \
        = FPGA_ATTR(_name, _mode, _show, _store, _index)

static FPGA_DEVICE_ATTR(fpga_chip_num,  S_IRUGO, get_fpga_chip_num, NULL, 0);
static FPGA_DEVICE_ATTR(fpga_alias,  S_IRUGO, get_fpga_alias, NULL, 0);
static FPGA_DEVICE_ATTR(fpga_type,  S_IRUGO, get_fpga_type, NULL, 0);
static FPGA_DEVICE_ATTR(fpga_hw_version,  S_IRUGO, read_fpga_HWversion, NULL, 0);
static FPGA_DEVICE_ATTR(fpga_sw_version,  S_IRUGO, read_fpga_SWversion, NULL, 0);

static struct attribute *fn8656_bnf_fpga_attributes[] = {
    &fpga_dev_attr_fpga_type.dev_attr.attr,
    &fpga_dev_attr_fpga_alias.dev_attr.attr,
    &fpga_dev_attr_fpga_chip_num.dev_attr.attr,
    &fpga_dev_attr_fpga_hw_version.dev_attr.attr,
    &fpga_dev_attr_fpga_sw_version.dev_attr.attr,
    NULL
};
static const struct attribute_group fn8656_bnf_fpga_group = { .attrs = fn8656_bnf_fpga_attributes};

static uint8_t fn8656_bnf_sfp_translate_offset(struct kwai_fpga *sfp,
        loff_t *offset, int num)
{
    unsigned int page = 0;

    /* if SFP style, offset > 255, shift to i2c addr 0x51 */
    if (sfp->chip[num].dev_class == TWO_ADDR) {
        if (*offset > 255) {
            sfp->chip[num].slave_addr = SFP_EEPROM_A2_ADDR;
            *offset -= 256;
        } else {
            sfp->chip[num].slave_addr = SFP_EEPROM_A0_ADDR;
        }
    }

    /*
     * if offset is in the range 0-128...
     * page doesn't matter (using lower half), return 0.
     * offset is already correct (don't add 128 to get to paged area)
     */
    if (*offset < SFP_PAGE_SIZE)
        return page;

    /* note, page will always be positive since *offset >= 128 */
    page = (*offset >> 7)-1;
    /* 0x80 places the offset in the top half, offset is last 7 bits */
    *offset = SFP_PAGE_SIZE + (*offset & 0x7f);

    return page;  /* note also returning clifn8656_bnf_sfp_translate_offsetent and offset */
}

#define FPGA_PORT_MGR_CFG  (0x1000)
#define FPGA_PORT_MGR_CTRL (0x1004)
#define FPGA_PORT_MGR_STAT (0x1008)
#define FPGA_PORT_MGR_MUX  (0x1010)
#define FPGA_PORT_BATCH_DATA (0x100000)
/*
 *i2c controller 0 is used for port 0~24
 *i2c controller 1 is used for port 24~48
 *i2c controller 2 is used for port 48~56
*/
#define i2c_dev(mode, port, idx) \
do { \
 if (((port) < 24 ) || (!mode)) \
        idx = 0; \
 else if ((port) < 48) \
    idx = 1;  \
 else \
    idx = 2;  \
}while(0)

#define port_mgr_cfg_reg(base_addr, idx) \
(base_addr + FPGA_PORT_MGR_CFG + 0x20 * (idx))

#define port_mgr_ctrl_reg(base_addr, idx) \
(base_addr + FPGA_PORT_MGR_CTRL + 0x20 * (idx))

#define port_mgr_stat_reg(base_addr, idx) \
(base_addr + FPGA_PORT_MGR_STAT + 0x20 * (idx))

#define port_mgr_mux_reg(base_addr, idx) \
(base_addr + FPGA_PORT_MGR_MUX + 0x20 * (idx))

#define port_mgr_batch_reg(base_addr, idx, offset) \
(base_addr + FPGA_PORT_BATCH_DATA + 0x10000 * (idx) + offset)

#define port2data(mode, port, data) \
do { \
 if (((port) < 24) || (!mode)) \
        data = (port); \
 else if ((port) < 48 ) \
    data = (port) % 24;  \
 else \
    data = (port) % 48;  \
}while(0)


static int fpga_i2c_byte_read(struct kwai_fpga *sfp,
            int port,
            char *buf, unsigned int offset)
{
    uint32_t data;
    uint32_t idx = 0;
    int i = 0;

    i2c_dev(sfp->i2c_mode, (port - 1), idx);
    data = 0x80000000;
    writel(data, port_mgr_cfg_reg(sfp->mmio, idx));

    data = 0x40fa0100 | ((sfp->chip[port].slave_addr & 0xFF) << 8) | (sfp->chip[port].slave_addr & 0xFF);
    writel(data, port_mgr_cfg_reg(sfp->mmio, idx));

    port2data(sfp->i2c_mode, (port - 1), data);
    writel(data, port_mgr_mux_reg(sfp->mmio, idx));

    data = 0x81000000 | ((offset&0xFF) << 16);
    writel(data, port_mgr_ctrl_reg(sfp->mmio, idx));
    pega_print(DEBUG, "fpga_i2c_byte_read port %d, offset:0x%x, idx:%d,cfg:%x@%x mux:%x@%x ctrl:%x@%x stat:%x@%x\n", port, offset, idx,
                       port_mgr_cfg_reg(sfp->mmio, idx), readl(port_mgr_cfg_reg(sfp->mmio, idx)),
                       port_mgr_mux_reg(sfp->mmio, idx), readl(port_mgr_mux_reg(sfp->mmio, idx)),
                       port_mgr_ctrl_reg(sfp->mmio, idx), readl(port_mgr_ctrl_reg(sfp->mmio, idx)),
                       port_mgr_stat_reg(sfp->mmio, idx), readl(port_mgr_stat_reg(sfp->mmio, idx))
                       );
    do {
        data = readl(port_mgr_stat_reg(sfp->mmio, idx));
        if ((data & 0xC0000000) == 0x80000000) {
            *buf = data & 0xFF;
            return 0;
        }
        usleep_range(100,200);
    }while(i++ < 1000);
    return -ENXIO;
}
static int fpga_i2c_block_read(struct kwai_fpga *sfp,
            int port,
            char *buf, unsigned int offset,size_t count)
{
    uint32_t data;
    uint32_t idx = 0;
    int i = 0;
    int buf_offset = 0;

    i2c_dev(sfp->i2c_mode, (port - 1), idx);
    data = 0x80000000;
    writel(data, port_mgr_cfg_reg(sfp->mmio, idx));

    data = 0x40fa0100 | ((sfp->chip[port].slave_addr & 0xFF) << 8) | (sfp->chip[port].slave_addr & 0xFF);
    writel(data, port_mgr_cfg_reg(sfp->mmio, idx));

    port2data(sfp->i2c_mode, (port - 1), data);
    writel(data, port_mgr_mux_reg(sfp->mmio, idx));

    data = 0x82000000 | ((offset&0xFF) << 16) | ((count & 0xFF) << 8);
    writel(data, port_mgr_ctrl_reg(sfp->mmio, idx));
    pega_print(DEBUG, "fpga_i2c_block_read port %d, offset:0x%x, idx:%d,cfg:%x@%x mux:%x@%x ctrl:%x@%x stat:%x@%x\n", port, offset, idx,
                       port_mgr_cfg_reg(sfp->mmio, idx), readl(port_mgr_cfg_reg(sfp->mmio, idx)),
                       port_mgr_mux_reg(sfp->mmio, idx), readl(port_mgr_mux_reg(sfp->mmio, idx)),
                       port_mgr_ctrl_reg(sfp->mmio, idx), readl(port_mgr_ctrl_reg(sfp->mmio, idx)),
                       port_mgr_stat_reg(sfp->mmio, idx), readl(port_mgr_stat_reg(sfp->mmio, idx))
                       );
    do {
        data = readl(port_mgr_stat_reg(sfp->mmio, idx));
        if ((data & 0xC0000000) == 0x80000000) {
            for(buf_offset = 0; buf_offset < count;buf_offset++){
            if ((enable_sfp_three_batch == FALSE) && (idx))
            {
                pega_print(DEBUG, "fpga_i2c_block_read bach:@%x\n", port_mgr_batch_reg(sfp->mmio, idx, buf_offset));
            }
else
                *buf++ = readb(port_mgr_batch_reg(sfp->mmio, idx, buf_offset));
            }
            return buf_offset;
        }
        usleep_range(100,200);
    } while(i++ < 1000);
    return -ENXIO;
}

static int fpga_i2c_byte_write(struct kwai_fpga *sfp,
            int port,
            char *buf, unsigned int offset)
{
    uint32_t data;
    uint32_t idx = 0;
    int i = 0;

    i2c_dev(sfp->i2c_mode, (port - 1), idx);
    data = 0x80000000;
    writel(data, port_mgr_cfg_reg(sfp->mmio, idx));

    data = 0x40fa0100 | ((sfp->chip[port].slave_addr & 0xFF) << 8) | (sfp->chip[port].slave_addr & 0xFF);
    writel(data, port_mgr_cfg_reg(sfp->mmio, idx));

    port2data(sfp->i2c_mode, (port - 1), data);
    writel(data, port_mgr_mux_reg(sfp->mmio, idx));

    data = 0x84000000 | ((offset & 0xFF) << 16) | (*buf & 0xFF);
    writel(data, port_mgr_ctrl_reg(sfp->mmio, idx));
    pega_print(DEBUG, "fpga_i2c_byte_write port %d, offset:0x%x, idx:%d,cfg:%x@%x mux:%x@%x ctrl:%x@%x stat:%x@%x\n", port, offset, idx,
                       port_mgr_cfg_reg(sfp->mmio, idx), readl(port_mgr_cfg_reg(sfp->mmio, idx)),
                       port_mgr_mux_reg(sfp->mmio, idx), readl(port_mgr_mux_reg(sfp->mmio, idx)),
                       port_mgr_ctrl_reg(sfp->mmio, idx), readl(port_mgr_ctrl_reg(sfp->mmio, idx)),
                       port_mgr_stat_reg(sfp->mmio, idx), readl(port_mgr_stat_reg(sfp->mmio, idx))
                       );
    do {
        data = readl(port_mgr_stat_reg(sfp->mmio, idx));
        if (data & 0x80000000) {
            return 0;
        }
    }while(i++ < 1000);
    return -ENXIO;
}

static ssize_t fn8656_bnf_sfp_eeprom_read_byte_by_byte(struct kwai_fpga *sfp,
            int port,
            char *buf, unsigned int offset, size_t count)
{
    unsigned long timeout, write_time;
    int ret;
    int i = 0;

    timeout = jiffies + msecs_to_jiffies(write_timeout);
    do {
        write_time = jiffies;
        ret = fpga_i2c_byte_read(sfp, port, buf + i, (offset + i));
        pega_print(DEBUG, "read register byte by byte %d, offset:0x%x, data :0x%x ret:%d\n", i, offset + i, *(buf + i), ret);
        if (ret == -ENXIO) /* no module present */
            return ret;
	i++;
    } while (time_before(write_time, timeout) && (i < count));

    if (i == count)
        return count;

    return -ETIMEDOUT;
}

static int fpga_i2c_read(struct kwai_fpga *sfp,
            int port,
            char *buf, unsigned int offset,size_t count)
{
    if (sfp->i2c_mode == FPGA_I2C_RD_MODE_BLOCK)
        return fpga_i2c_block_read(sfp, port, buf, offset, count);
    else
        return fn8656_bnf_sfp_eeprom_read_byte_by_byte(sfp, port, buf, offset, count);
}

static ssize_t fn8656_bnf_sfp_eeprom_read(struct kwai_fpga *sfp,
            int port,
            char *buf, unsigned int offset, size_t count)
{
    unsigned long timeout, read_time;
    int status;
	if (count < SFP_FAST_BYTE_LENGTH)
	{
		return fn8656_bnf_sfp_eeprom_read_byte_by_byte(sfp, port, buf, offset, count);
	}
    /*smaller eeproms can work given some SMBus extension calls */
    if (count > SFP_PAGE_SIZE)
        count = SFP_PAGE_SIZE;

    /*
     * Reads fail if the previous write didn't complete yet. We may
     * loop a few times until this one succeeds, waiting at least
     * long enough for one entire page write to work.
     */
    timeout = jiffies + msecs_to_jiffies(write_timeout);
    do {
        read_time = jiffies;

        status = fpga_i2c_read(sfp, port, buf, offset, count);

        pega_print(DEBUG, "eeprom read %zu@%d --> %d (%ld)\n",
                count, offset, status, jiffies);

        if (status == count)  /* happy path */
            return count;

        if (status == -ENXIO) /* no module present */
            return status;

    } while (time_before(read_time, timeout));

    return -ETIMEDOUT;
}

static ssize_t fn8656_bnf_sfp_eeprom_write_byte_by_byte(
            struct kwai_fpga *sfp,
            int port,
            const char *buf, unsigned int offset, size_t count)
{
    unsigned long timeout, write_time;
    int ret;
    int i = 0;

    timeout = jiffies + msecs_to_jiffies(write_timeout);
    do {
        write_time = jiffies;
        ret = fpga_i2c_byte_write(sfp, port, (char *)buf + i, (offset + i));
        pega_print(DEBUG, "Write register byte by byte %zu, offset:0x%x, data :0x%x ret:%d\n",
                       count, offset + i, *(buf + i), ret);
        if (ret == -ENXIO)
            return ret;
	i++;
	//usleep_range(10000, 15000);
    } while (time_before(write_time, timeout) && (i < count));

    if (i == count) {
        return count;
    }

    return -ETIMEDOUT;
}


static ssize_t fn8656_bnf_sfp_eeprom_write(struct kwai_fpga *sfp,
                int port,
                const char *buf,
                unsigned int offset, size_t count)
{
    ssize_t status;
    unsigned long timeout, write_time;
    unsigned int next_page_start;

    /* write max is at most a page
     * (In this driver, write_max is actually one byte!)
     */
    if (count > sfp->chip[port].write_max)
        count = sfp->chip[port].write_max;

    /* shorten count if necessary to avoid crossing page boundary */
    next_page_start = roundup(offset + 1, SFP_PAGE_SIZE);
    if ((offset + count) > next_page_start)
        count = next_page_start - offset;

    if (count > I2C_SMBUS_BLOCK_MAX)
        count = I2C_SMBUS_BLOCK_MAX;

    /*
     * Reads fail if the previous write didn't complete yet. We may
     * loop a few times until this one succeeds, waiting at least
     * long enough for one entire page write to work.
     */
    timeout = jiffies + msecs_to_jiffies(write_timeout);
    do {
        write_time = jiffies;

        status = fn8656_bnf_sfp_eeprom_write_byte_by_byte(sfp, port, buf, offset, count);
        if (status == count)  /* happy path */
            return count;

        if (status == -ENXIO) /* no module present */
            return status;

    } while (time_before(write_time, timeout));

    return -ETIMEDOUT;
}

static ssize_t fn8656_bnf_sfp_eeprom_update_client(struct kwai_fpga *sfp,
                char *buf, loff_t off, size_t count, int num)
{
    ssize_t retval = 0;
    uint8_t page = 0;
    uint8_t page_check = 0;
    loff_t phy_offset = off;
    int ret = 0;

    page = fn8656_bnf_sfp_translate_offset(sfp, &phy_offset, num);

    pega_print(DEBUG,"off %lld  page:%d phy_offset:%lld, count:%ld\n",
        off, page, phy_offset, (long int) count);
    if (page > 0) {
        ret = fn8656_bnf_sfp_eeprom_write(sfp, num, &page,
            SFP_PAGE_SELECT_REG, 1);
        if (ret < 0) {
            pega_print(WARNING,"Write page register for page %d failed ret:%d!\n",page, ret);
            return ret;
        }
    }
    wmb();

    ret =  fn8656_bnf_sfp_eeprom_read(sfp, num,
            &page_check , SFP_PAGE_SELECT_REG, 1);
    if (ret < 0) {
            pega_print(DEBUG,"Read page register for page %d failed ret:%d!\n",page, ret);
            return ret;
    }
    pega_print(DEBUG,"read page register %d checked %d, ret:%d\n",page, page_check, ret);

    while (count) {
        ssize_t status;

        status =  fn8656_bnf_sfp_eeprom_read(sfp, num,
            buf, phy_offset, count);

        if (status <= 0) {
            if (retval == 0)
                retval = status;
            break;
        }
        buf += status;
        phy_offset += status;
        count -= status;
        retval += status;
    }


    if (page > 0) {
        /* return the page register to page 0 (why?) */
        page = 0;
        ret = fn8656_bnf_sfp_eeprom_write(sfp, num, &page,
            SFP_PAGE_SELECT_REG, 1);
        if (ret < 0) {
            pega_print(ERR,"Restore page register to 0 failed:%d!\n", ret);
            /* error only if nothing has been transferred */
            if (retval == 0)
                retval = ret;
        }
    }
    return retval;
}

static ssize_t fn8656_bnf_sfp_eeprom_write_client(struct kwai_fpga *sfp,
                char *buf, loff_t off, size_t count, int num)
{
    ssize_t retval = 0;
    uint8_t page = 0;
    uint8_t page_check = 0;
    loff_t phy_offset = off;
    int ret = 0;

    page = fn8656_bnf_sfp_translate_offset(sfp, &phy_offset, num);

    pega_print(DEBUG,"off %lld  page:%d phy_offset:%lld, count:%ld\n",
        off, page, phy_offset, (long int) count);
    if (page > 0) {
        ret = fn8656_bnf_sfp_eeprom_write(sfp, num, &page,
            SFP_PAGE_SELECT_REG, 1);
        if (ret < 0) {
            pega_print(WARNING,"Write page register for page %d failed ret:%d!\n",page, ret);
            return ret;
        }
    }
    wmb();

    ret =  fn8656_bnf_sfp_eeprom_read(sfp, num,
            &page_check , SFP_PAGE_SELECT_REG, 1);
    if (ret < 0) {
            pega_print(WARNING,"Read page register for page %d failed ret:%d!\n",page, ret);
            return ret;
    }
    pega_print(DEBUG,"read page register %d checked %d, ret:%d\n",page, page_check, ret);

    ret =  fn8656_bnf_sfp_eeprom_write(sfp, num,
		    buf, phy_offset, count);

    if (ret < 0) {
	    retval = ret;
	    pega_print(ERR,"write eeprom failed:offset:0x%x bytes:%d ret:%d!\n", phy_offset, num, ret);
    }
    /*update sfp eeprom cache buffer after write*/
    if(enable_sfp_eeprom_cache == TRUE)
        fn8656_bnf_sfp_eeprom_read(sfp,num, &sfp_eeprom_cache[num-1][page*SFP_PAGE_SIZE+phy_offset], phy_offset, count);

    if (page > 0) {
        /* return the page register to page 0 (why?) */
        page = 0;
        ret = fn8656_bnf_sfp_eeprom_write(sfp, num, &page,
            SFP_PAGE_SELECT_REG, 1);
        if (ret < 0) {
            pega_print(ERR,"Restore page register to 0 failed:%d!\n", ret);
            /* error only if nothing has been transferred */
            if (retval == 0)
                retval = ret;
        }
    }
    return retval;
}

/*
 * Figure out if this access is within the range of supported pages.
 * Note this is called on every access because we don't know if the
 * module has been replaced since the last call.
 * If/when modules support more pages, this is the routine to update
 * to validate and allow access to additional pages.
 *
 * Returns updated len for this access:
 *     - entire access is legal, original len is returned.
 *     - access begins legal but is too long, len is truncated to fit.
 *     - initial offset exceeds supported pages, return OPTOE_EOF (zero)
 */
static ssize_t fn8656_bnf_sfp_page_legal(struct kwai_fpga *sfp,
        loff_t off, size_t len, int num)
{
    u8 regval;
    int not_pageable;
    int status;
    size_t maxlen;

    if (off < 0)
        return -EINVAL;

    if (sfp->chip[num].dev_class == TWO_ADDR) {
        /* SFP case */
        /* if only using addr 0x50 (first 256 bytes) we're good */
        if ((off + len) <= TWO_ADDR_NO_0X51_SIZE)
            return len;
        /* if offset exceeds possible pages, we're not good */
        if (off >= TWO_ADDR_EEPROM_SIZE)
            return SFP_EOF;
        /* in between, are pages supported? */
        status = fn8656_bnf_sfp_eeprom_read(sfp, num, &regval,
                TWO_ADDR_PAGEABLE_REG, 1);
        if (status < 0)
            return status;  /* error out (no module?) */
        if (regval & TWO_ADDR_PAGEABLE) {
            /* Pages supported, trim len to the end of pages */
            maxlen = TWO_ADDR_EEPROM_SIZE - off;
        } else {
            /* pages not supported, trim len to unpaged size */
            if (off >= TWO_ADDR_EEPROM_UNPAGED_SIZE)
                return SFP_EOF;

            /* will be accessing addr 0x51, is that supported? */
            /* byte 92, bit 6 implies DDM support, 0x51 support */
            status = fn8656_bnf_sfp_eeprom_read(sfp, num, &regval,
                        TWO_ADDR_0X51_REG, 1);
            if (status < 0)
                return status;
            if (regval & TWO_ADDR_0X51_SUPP) {
                /* addr 0x51 is OK */
                maxlen = TWO_ADDR_EEPROM_UNPAGED_SIZE - off;
            } else {
                /* addr 0x51 NOT supported, trim to 256 max */
                if (off >= TWO_ADDR_NO_0X51_SIZE)
                    return SFP_EOF;
                maxlen = TWO_ADDR_NO_0X51_SIZE - off;
            }
        }
        len = (len > maxlen) ? maxlen : len;
        pega_print(DEBUG,"page_legal, SFP, off %lld len %ld\n",
            off, (long int) len);
    } else {
        /* QSFP case, CMIS case */
        /* if no pages needed, we're good */
        if ((off + len) <= ONE_ADDR_EEPROM_UNPAGED_SIZE)
            return len;
        /* if offset exceeds possible pages, we're not good */
        if (off >= ONE_ADDR_EEPROM_SIZE)
            return SFP_EOF;
        /* in between, are pages supported? */
        status = fn8656_bnf_sfp_eeprom_read(sfp, num, &regval,
                ONE_ADDR_PAGEABLE_REG, 1);
        if (status < 0)
            return status;  /* error out (no module?) */

        if (sfp->chip[num].dev_class == ONE_ADDR) {
            not_pageable = QSFP_NOT_PAGEABLE;
        } else {
            not_pageable = CMIS_NOT_PAGEABLE;
        }
        pega_print(DEBUG,"Paging Register: 0x%x; not_pageable mask: 0x%x\n",
            regval, not_pageable);

        if (regval & not_pageable) {
            /* pages not supported, trim len to unpaged size */
            if (off >= ONE_ADDR_EEPROM_UNPAGED_SIZE)
                return SFP_EOF;
            maxlen = ONE_ADDR_EEPROM_UNPAGED_SIZE - off;
        } else {
            /* Pages supported, trim len to the end of pages */
            maxlen = ONE_ADDR_EEPROM_SIZE - off;
        }
        len = (len > maxlen) ? maxlen : len;
        pega_print(DEBUG,"page_legal, QSFP, off %lld len %ld\n",
            off, (long int) len);
    }
    return len;
}

static ssize_t fn8656_bnf_sfp_read(struct kwai_fpga *sfp,
        char *buf, loff_t off, size_t len, int num)
{
    int chunk;
    int status = 0;
    ssize_t retval;
    size_t pending_len = 0, chunk_len = 0;
    loff_t chunk_offset = 0, chunk_start_offset = 0;
    loff_t chunk_end_offset = 0;

    if (unlikely(!len))
        return len;

    /*
     * Read data from chip, protecting against concurrent updates
     * from this host, but not from other I2C masters.
     */
    mutex_lock(&sfp->lock);

    /*
     * Confirm this access fits within the device suppored addr range
     */
    status = fn8656_bnf_sfp_page_legal(sfp, off, len, num);
    if ((status == SFP_EOF) || (status < 0)) {
        mutex_unlock(&sfp->lock);
        return status;
    }
    len = status;

    /*
     * For each (128 byte) chunk involved in this request, issue a
     * separate call to sff_eeprom_update_client(), to
     * ensure that each access recalculates the client/page
     * and writes the page register as needed.
     * Note that chunk to page mapping is confusing, is different for
     * QSFP and SFP, and never needs to be done.  Don't try!
     */
    pending_len = len; /* amount remaining to transfer */
    retval = 0;  /* amount transferred */
    for (chunk = off >> 7; chunk <= (off + len - 1) >> 7; chunk++) {

        /*
         * Compute the offset and number of bytes to be read/write
         *
         * 1. start at an offset not equal to 0 (within the chunk)
         *    and read/write less than the rest of the chunk
         * 2. start at an offset not equal to 0 and read/write the rest
         *    of the chunk
         * 3. start at offset 0 (within the chunk) and read/write less
         *    than entire chunk
         * 4. start at offset 0 (within the chunk), and read/write
         *    the entire chunk
         */
        chunk_start_offset = chunk * SFP_PAGE_SIZE;
        chunk_end_offset = chunk_start_offset + SFP_PAGE_SIZE;

        if (chunk_start_offset < off) {
            chunk_offset = off;
            if ((off + pending_len) < chunk_end_offset)
                chunk_len = pending_len;
            else
                chunk_len = chunk_end_offset - off;
        } else {
            chunk_offset = chunk_start_offset;
            if (pending_len < SFP_PAGE_SIZE)
                chunk_len = pending_len;
            else
                chunk_len = SFP_PAGE_SIZE;
        }

        /*
         * note: chunk_offset is from the start of the EEPROM,
         * not the start of the chunk
         */
        status = fn8656_bnf_sfp_eeprom_update_client(sfp, buf,
                chunk_offset, chunk_len, num);
        if (status != chunk_len) {
            /* This is another 'no device present' path */
            if (status > 0)
                retval += status;
            if (retval == 0)
                retval = status;
            break;
        }
        buf += status;
        pending_len -= status;
        retval += status;
    }
    mutex_unlock(&sfp->lock);

    return retval;
}

static ssize_t fn8656_bnf_sfp_write(struct kwai_fpga *sfp,
        char *buf, loff_t off, size_t len, int num)
{
    int chunk;
    int status = 0;
    ssize_t retval;
    size_t pending_len = 0, chunk_len = 0;
    loff_t chunk_offset = 0, chunk_start_offset = 0;
    loff_t chunk_end_offset = 0;

    if (unlikely(!len))
        return len;

    /*
     * Read data from chip, protecting against concurrent updates
     * from this host, but not from other I2C masters.
     */
    mutex_lock(&sfp->lock);

    /*
     * Confirm this access fits within the device suppored addr range
     */
    status = fn8656_bnf_sfp_page_legal(sfp, off, len, num);
    if ((status == SFP_EOF) || (status < 0)) {
        mutex_unlock(&sfp->lock);
        return status;
    }
    len = status;

    /*
     * For each (128 byte) chunk involved in this request, issue a
     * separate call to sff_eeprom_update_client(), to
     * ensure that each access recalculates the client/page
     * and writes the page register as needed.
     * Note that chunk to page mapping is confusing, is different for
     * QSFP and SFP, and never needs to be done.  Don't try!
     */
    pending_len = len; /* amount remaining to transfer */
    retval = 0;  /* amount transferred */
    for (chunk = off >> 7; chunk <= (off + len - 1) >> 7; chunk++) {

        /*
         * Compute the offset and number of bytes to be read/write
         *
         * 1. start at an offset not equal to 0 (within the chunk)
         *    and read/write less than the rest of the chunk
         * 2. start at an offset not equal to 0 and read/write the rest
         *    of the chunk
         * 3. start at offset 0 (within the chunk) and read/write less
         *    than entire chunk
         * 4. start at offset 0 (within the chunk), and read/write
         *    the entire chunk
         */
        chunk_start_offset = chunk * SFP_PAGE_SIZE;
        chunk_end_offset = chunk_start_offset + SFP_PAGE_SIZE;

        if (chunk_start_offset < off) {
            chunk_offset = off;
            if ((off + pending_len) < chunk_end_offset)
                chunk_len = pending_len;
            else
                chunk_len = chunk_end_offset - off;
        } else {
            chunk_offset = chunk_start_offset;
            if (pending_len < SFP_PAGE_SIZE)
                chunk_len = pending_len;
            else
                chunk_len = SFP_PAGE_SIZE;
        }

        /*
         * note: chunk_offset is from the start of the EEPROM,
         * not the start of the chunk
         */
        status = fn8656_bnf_sfp_eeprom_write_client(sfp, buf,
                chunk_offset, chunk_len, num);
        if (status != chunk_len) {
            /* This is another 'no device present' path */
            if (status > 0)
                retval += status;
            if (retval == 0)
                retval = status;
            break;
        }
        buf += status;
        pending_len -= status;
        retval += status;
    }
    mutex_unlock(&sfp->lock);

    return retval;
}


int sfp_poll(void *data)
{
    struct kwai_fpga * sfp = (struct kwai_fpga *)data;
    int port = 0;
    unsigned int offset = 0 ;
    int ret = 0;

    while(!kthread_should_stop())
    {
        if(enable_sfp_eeprom_cache == FALSE)
        {
            usleep_range(1000000, 1500000);
            continue;
        }

        for(port = 0; port < MAX_PORT_NUM; port ++)
        {
            if(kthread_should_stop())
                return 0;
            ret = fn8656_bnf_sfp_read(sfp, sfp_eeprom_cache[port], 0,MAX_CACHE_LEN, port+1);
            if(ret != MAX_CACHE_LEN)
            {
                memset(sfp_eeprom_cache[port],0x0,MAX_CACHE_LEN);
                sfp_eeprom_cache[port][0] = ret;
                pega_print(DEBUG,"port:%d,ret:%d\n",port+1,sfp_eeprom_cache[port][0]);
            }
            usleep_range(1000000, 1500000);
        }
    }
    return 0;
}

static ssize_t
fn8656_bnf_sfp_bin_read(struct file *filp, struct kobject *kobj,
        struct bin_attribute *attr,
        char *buf, loff_t off, size_t count)
{
    struct pci_dev *dev = to_pci_dev(container_of(kobj, struct device, kobj));
    struct kwai_fpga *fpga = dev->sysdata;
    int port = (char *)(attr->private) - (char *)NULL;

    if(enable_sfp_eeprom_cache == TRUE)
    {
        memcpy(buf,&sfp_eeprom_cache[port-1][off],count);
        if(sfp_eeprom_cache[port-1][0] < 0)
            count = buf[0];
        return count;
    }

    return fn8656_bnf_sfp_read(fpga, buf, off, count, port);
}

static ssize_t
fn8656_bnf_sfp_bin_write(struct file *filp, struct kobject *kobj,
        struct bin_attribute *attr,
        char *buf, loff_t off, size_t count)
{
    struct pci_dev *dev = to_pci_dev(container_of(kobj, struct device, kobj));
    struct kwai_fpga *fpga = dev->sysdata;

    return fn8656_bnf_sfp_write(fpga, buf, off, count, (char *)(attr->private) - (char *)NULL);
}

#define SFP_EEPROM_ATTR(_num)    \
        static struct bin_attribute sfp##_num##_eeprom_attr = {   \
                .attr = {   \
                        .name =  __stringify(sfp##_num##_eeprom),  \
                        .mode = S_IRUGO|S_IWUSR    \
                },                                 \
                .private = (void *)_num, \
                .size = TWO_ADDR_EEPROM_SIZE,      \
                .read = fn8656_bnf_sfp_bin_read,   \
                .write = fn8656_bnf_sfp_bin_write,   \
                }

SFP_EEPROM_ATTR(1);SFP_EEPROM_ATTR(2);SFP_EEPROM_ATTR(3);SFP_EEPROM_ATTR(4);SFP_EEPROM_ATTR(5);
SFP_EEPROM_ATTR(6);SFP_EEPROM_ATTR(7);SFP_EEPROM_ATTR(8);SFP_EEPROM_ATTR(9);SFP_EEPROM_ATTR(10);
SFP_EEPROM_ATTR(11);SFP_EEPROM_ATTR(12);SFP_EEPROM_ATTR(13);SFP_EEPROM_ATTR(14);SFP_EEPROM_ATTR(15);
SFP_EEPROM_ATTR(16);SFP_EEPROM_ATTR(17);SFP_EEPROM_ATTR(18);SFP_EEPROM_ATTR(19);SFP_EEPROM_ATTR(20);
SFP_EEPROM_ATTR(21);SFP_EEPROM_ATTR(22);SFP_EEPROM_ATTR(23);SFP_EEPROM_ATTR(24);SFP_EEPROM_ATTR(25);
SFP_EEPROM_ATTR(26);SFP_EEPROM_ATTR(27);SFP_EEPROM_ATTR(28);SFP_EEPROM_ATTR(29);SFP_EEPROM_ATTR(30);
SFP_EEPROM_ATTR(31);SFP_EEPROM_ATTR(32);SFP_EEPROM_ATTR(33);SFP_EEPROM_ATTR(34);SFP_EEPROM_ATTR(35);
SFP_EEPROM_ATTR(36);SFP_EEPROM_ATTR(37);SFP_EEPROM_ATTR(38);SFP_EEPROM_ATTR(39);SFP_EEPROM_ATTR(40);
SFP_EEPROM_ATTR(41);SFP_EEPROM_ATTR(42);SFP_EEPROM_ATTR(43);SFP_EEPROM_ATTR(44);SFP_EEPROM_ATTR(45);
SFP_EEPROM_ATTR(46);SFP_EEPROM_ATTR(47);SFP_EEPROM_ATTR(48);SFP_EEPROM_ATTR(49);SFP_EEPROM_ATTR(50);
SFP_EEPROM_ATTR(51);SFP_EEPROM_ATTR(52);SFP_EEPROM_ATTR(53);SFP_EEPROM_ATTR(54);SFP_EEPROM_ATTR(55);
SFP_EEPROM_ATTR(56);

static struct bin_attribute *fn8656_bnf_sfp_epprom_attributes[] = {
    &sfp1_eeprom_attr, &sfp2_eeprom_attr, &sfp3_eeprom_attr, &sfp4_eeprom_attr, &sfp5_eeprom_attr,
    &sfp6_eeprom_attr, &sfp7_eeprom_attr, &sfp8_eeprom_attr, &sfp9_eeprom_attr, &sfp10_eeprom_attr,
    &sfp11_eeprom_attr, &sfp12_eeprom_attr, &sfp13_eeprom_attr, &sfp14_eeprom_attr, &sfp15_eeprom_attr,
    &sfp16_eeprom_attr, &sfp17_eeprom_attr, &sfp18_eeprom_attr, &sfp19_eeprom_attr, &sfp20_eeprom_attr,
    &sfp21_eeprom_attr, &sfp22_eeprom_attr, &sfp23_eeprom_attr, &sfp24_eeprom_attr, &sfp25_eeprom_attr,
    &sfp26_eeprom_attr, &sfp27_eeprom_attr, &sfp28_eeprom_attr, &sfp29_eeprom_attr, &sfp30_eeprom_attr,
    &sfp31_eeprom_attr, &sfp32_eeprom_attr, &sfp33_eeprom_attr, &sfp34_eeprom_attr, &sfp35_eeprom_attr,
    &sfp36_eeprom_attr, &sfp37_eeprom_attr, &sfp38_eeprom_attr, &sfp39_eeprom_attr, &sfp40_eeprom_attr,
    &sfp41_eeprom_attr, &sfp42_eeprom_attr, &sfp43_eeprom_attr, &sfp44_eeprom_attr, &sfp45_eeprom_attr,
    &sfp46_eeprom_attr, &sfp47_eeprom_attr, &sfp48_eeprom_attr, &sfp49_eeprom_attr, &sfp50_eeprom_attr,
    &sfp51_eeprom_attr, &sfp52_eeprom_attr, &sfp53_eeprom_attr, &sfp54_eeprom_attr, &sfp55_eeprom_attr,
    &sfp56_eeprom_attr,
    NULL
};

static const struct attribute_group fn8656_bnf_sfp_group = { .bin_attrs = fn8656_bnf_sfp_epprom_attributes};

static int kwai_fpga_probe(struct pci_dev *pdev,
        const struct pci_device_id *pci_id)
{
    struct kwai_fpga *fpga;
    int err, i = 0;
    int status = 0;
    uint8_t sw_ver = 0;


    fpga = kzalloc(sizeof(struct kwai_fpga), GFP_KERNEL);
    if (!fpga)
        return -ENOMEM;

    fpga->pdev = pdev;
    pdev->sysdata = (void *)fpga;
    mutex_init(&fpga->lock);

    err = pci_enable_device(pdev);
    if (err) {
        printk(KERN_ERR "kwai_fpga: Can't enable device.\n");
        goto err_dev;
    }
    if (!devm_request_mem_region(&pdev->dev, pci_resource_start(pdev, 0),
                pci_resource_len(pdev, 0),
                "kwai_fpga")) {
        printk(KERN_WARNING "kwai_fpga: Can't request iomem (0x%llx).\n",
               (unsigned long long)pci_resource_start(pdev, 0));
        err = -EBUSY;
        goto err_disable;
    }
    pci_set_master(pdev);
    pci_set_drvdata(pdev, fpga);

    fpga->mmio = devm_ioremap(&pdev->dev, pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
    if (!fpga->mmio) {
        printk(KERN_ERR "kwai_fpga: ioremap() failed\n");
        err = -EIO;
        goto err_disable;
    }

    for(i = 1; i <= MAX_PORT_NUM; i++)
    {
        fpga->chip[i].dev_class = CMIS_ADDR;
        fpga->chip[i].byte_len  = ONE_ADDR_EEPROM_SIZE;
        fpga->chip[i].write_max = I2C_SMBUS_BLOCK_MAX;
        fpga->chip[i].slave_addr = SFP_EEPROM_A0_ADDR;
    }
    status = sysfs_create_group(&pdev->dev.kobj, &fn8656_bnf_sfp_group);
    if (status) {
        goto err_disable;
    }
    status = sysfs_create_group(&pdev->dev.kobj, &fn8656_bnf_fpga_group);
    if (status) {
        goto err_disable;
    }

    sfp_task = kthread_run(sfp_poll, (void*)fpga, "sfpd");
    if(!sfp_task){
        pega_print(ERR,"Unable to start kernel thread.\n");
        return -ECHILD;
    }

    sw_ver = readl(fpga->mmio + FPGA_SW_VERSION);
    if(sw_ver > 0x6) {
        fpga->i2c_mode = FPGA_I2C_RD_MODE_BLOCK;
    }
    else {
        fpga->i2c_mode = FPGA_I2C_RD_MODE_BYTE;
    }

    return 0;

err_disable:
    pci_disable_device(pdev);
err_dev:
    kfree(fpga);

    return err;
}

static void kwai_fpga_remove(struct pci_dev *pdev)
{
    struct kwai_fpga *fpga = pdev->sysdata;

    if(sfp_task)
    {
        kthread_stop(sfp_task);
        sfp_task = NULL;
    }
    sysfs_remove_group(&pdev->dev.kobj, &fn8656_bnf_sfp_group);
    sysfs_remove_group(&pdev->dev.kobj, &fn8656_bnf_fpga_group);
    devm_iounmap(&pdev->dev, fpga->mmio);
    pci_disable_device(pdev);
    kfree(fpga);
}

static const struct pci_device_id kwai_fpga_pci_tbl[] = {
    { PCI_DEVICE(0x10EE, 0x7021) },
    { 0, },
};
MODULE_DEVICE_TABLE(pci, kwai_fpga_pci_tbl);

static struct pci_driver kwai_fpga_pci_driver = {
    .name       = "kwai_fpga",
    .id_table   = kwai_fpga_pci_tbl,
    .probe      = kwai_fpga_probe,
    .remove     = kwai_fpga_remove,
    .suspend    = NULL,
    .resume     = NULL,
};

module_pci_driver(kwai_fpga_pci_driver);

module_param(loglevel, uint, 0664);
module_param(enable_sfp_eeprom_cache, uint, 0664);
module_param(enable_sfp_three_batch, uint, 0664);
module_param_string(fpga_debug, fpga_debug, MAX_DEBUG_INFO_LEN, 0664);

MODULE_AUTHOR("Bao Hengxi <hengxibao@clounix.com>");
MODULE_DESCRIPTION("fn8656_bnf_sfp driver");
MODULE_LICENSE("GPL");

/*
*
* Licensed under the GNU General Public License Version 2
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
*/

/*
* pddf_xilinx_device_7021_algo.c
* Description:
*   A sample i2c driver algorithms for Xilinx Corporation Device 7021 FPGA adapters
*
*********************************************************************************/
#define __STDC_WANT_LIB_EXT1__ 1
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include "pddf_i2c_algo.h"

#define DEBUG 0

enum {
    STATE_DONE = 0,
    STATE_INIT,
    STATE_ADDR,
    STATE_ADDR10,
    STATE_START,
    STATE_WRITE,
    STATE_READ,
    STATE_STOP,
    STATE_ERROR,
};

/* registers */
#define FPGAI2C_REG_PRELOW      0
#define FPGAI2C_REG_PREHIGH     1
#define FPGAI2C_REG_CONTROL     2
#define FPGAI2C_REG_DATA            3
#define FPGAI2C_REG_CMD         4 /* write only */
#define FPGAI2C_REG_STATUS      4 /* read only, same address as FPGAI2C_REG_CMD */
#define FPGAI2C_REG_VER         5

#define FPGAI2C_REG_CTRL_IEN        0x40
#define FPGAI2C_REG_CTRL_EN     0x80

#define FPGAI2C_REG_CMD_START       0x91
#define FPGAI2C_REG_CMD_STOP        0x41
#define FPGAI2C_REG_CMD_READ        0x21
#define FPGAI2C_REG_CMD_WRITE       0x11
#define FPGAI2C_REG_CMD_READ_ACK    0x21
#define FPGAI2C_REG_CMD_READ_NACK   0x29
#define FPGAI2C_REG_CMD_IACK        0x01

#define FPGAI2C_REG_STAT_IF     0x01
#define FPGAI2C_REG_STAT_TIP        0x02
#define FPGAI2C_REG_STAT_ARBLOST    0x20
#define FPGAI2C_REG_STAT_BUSY       0x40
#define FPGAI2C_REG_STAT_NACK       0x80

struct fpgalogic_i2c {
    void __iomem *base;
    u32 reg_shift;
    u32 reg_io_width;
    wait_queue_head_t wait;
    struct i2c_msg *msg;
    int pos;
    int nmsgs;
    int state; /* see STATE_ */
    int ip_clock_khz;
    int bus_clock_khz;
    void (*reg_set)(struct fpgalogic_i2c *i2c, int reg, u8 value);
    u8 (*reg_get)(struct fpgalogic_i2c *i2c, int reg);
    u32 timeout;
    struct mutex lock;
};
static struct fpgalogic_i2c fpgalogic_i2c[I2C_PCI_MAX_BUS];
extern void __iomem * fpga_ctl_addr;
extern int (*ptr_fpgapci_read)(uint32_t);
extern int (*ptr_fpgapci_write)(uint32_t, uint32_t);
extern int (*pddf_i2c_pci_add_numbered_bus)(struct i2c_adapter *, int);

void i2c_get_mutex(struct fpgalogic_i2c *i2c)
{
    mutex_lock(&i2c->lock);
}

/**
 * i2c_release_mutex - release mutex
 */
void i2c_release_mutex(struct fpgalogic_i2c *i2c)
{
    mutex_unlock(&i2c->lock);
}

static void fpgai2c_reg_set_8(struct fpgalogic_i2c *i2c, int reg, u8 value)
{
    iowrite8(value, i2c->base + (reg << i2c->reg_shift));
}

static inline u8 fpgai2c_reg_get_8(struct fpgalogic_i2c *i2c, int reg)
{
    return ioread8(i2c->base + (reg << i2c->reg_shift));
}

static inline void fpgai2c_reg_set(struct fpgalogic_i2c *i2c, int reg, u8 value)
{
    i2c->reg_set(i2c, reg, value);
    udelay(100);
}

static inline u8 fpgai2c_reg_get(struct fpgalogic_i2c *i2c, int reg)
{
    udelay(100);
    return i2c->reg_get(i2c, reg);
}


/*
 * i2c_get_mutex must be called prior to calling this function.
 */
static int fpgai2c_poll(struct fpgalogic_i2c *i2c)
{
    u8 stat = fpgai2c_reg_get(i2c, FPGAI2C_REG_STATUS);
    struct i2c_msg *msg = i2c->msg;
    u8 addr;

    /* Ready? */
    if (stat & FPGAI2C_REG_STAT_TIP)
        return -EBUSY;

    if (i2c->state == STATE_DONE || i2c->state == STATE_ERROR) {
        /* Stop has been sent */
        fpgai2c_reg_set(i2c, FPGAI2C_REG_CMD, FPGAI2C_REG_CMD_IACK);
        if (i2c->state == STATE_ERROR)
            return -EIO;
        return 0;
    }

    /* Error? */
    if (stat & FPGAI2C_REG_STAT_ARBLOST) {
        i2c->state = STATE_ERROR;
        fpgai2c_reg_set(i2c, FPGAI2C_REG_CMD, FPGAI2C_REG_CMD_STOP);
        return -EAGAIN;
    }

    if (i2c->state == STATE_INIT) {
        if (stat & FPGAI2C_REG_STAT_BUSY)
            return -EBUSY;

        i2c->state = STATE_ADDR;
    }

    if (i2c->state == STATE_ADDR) {
        /* 10 bit address? */
        if (i2c->msg->flags & I2C_M_TEN) {
            addr = 0xf0 | ((i2c->msg->addr >> 7) & 0x6);
            i2c->state = STATE_ADDR10;
        } else {
            addr = (i2c->msg->addr << 1);
            i2c->state = STATE_START;
        }

        /* Set read bit if necessary */
        addr |= (i2c->msg->flags & I2C_M_RD) ? 1 : 0;

        fpgai2c_reg_set(i2c, FPGAI2C_REG_DATA, addr);
        fpgai2c_reg_set(i2c, FPGAI2C_REG_CMD, FPGAI2C_REG_CMD_START);

        return 0;
    }

    /* Second part of 10 bit addressing */
    if (i2c->state == STATE_ADDR10) {
        fpgai2c_reg_set(i2c, FPGAI2C_REG_DATA, i2c->msg->addr & 0xff);
        fpgai2c_reg_set(i2c, FPGAI2C_REG_CMD, FPGAI2C_REG_CMD_WRITE);

        i2c->state = STATE_START;
        return 0;
    }

    if (i2c->state == STATE_START || i2c->state == STATE_WRITE) {
        i2c->state = (msg->flags & I2C_M_RD) ? STATE_READ : STATE_WRITE;

        if (stat & FPGAI2C_REG_STAT_NACK) {
            i2c->state = STATE_ERROR;
            fpgai2c_reg_set(i2c, FPGAI2C_REG_CMD, FPGAI2C_REG_CMD_STOP);
            return -ENXIO;
        }
    } else {
        msg->buf[i2c->pos++] = fpgai2c_reg_get(i2c, FPGAI2C_REG_DATA);
    }
    if (i2c->pos >= msg->len) {
        i2c->nmsgs--;
        i2c->msg++;
        i2c->pos = 0;
        msg = i2c->msg;

        if (i2c->nmsgs) {
            if (!(msg->flags & I2C_M_NOSTART)) {
                i2c->state = STATE_ADDR;
                return 0;
            } else {
                i2c->state = (msg->flags & I2C_M_RD)
                    ? STATE_READ : STATE_WRITE;
            }
        } else {
            i2c->state = STATE_DONE;
            fpgai2c_reg_set(i2c, FPGAI2C_REG_CMD, FPGAI2C_REG_CMD_STOP);
            return 0;
        }
    }

    if (i2c->state == STATE_READ) {
        fpgai2c_reg_set(i2c, FPGAI2C_REG_CMD, i2c->pos == (msg->len - 1) ?
                  FPGAI2C_REG_CMD_READ_NACK : FPGAI2C_REG_CMD_READ_ACK);
    } else {
        fpgai2c_reg_set(i2c, FPGAI2C_REG_DATA, msg->buf[i2c->pos++]);
        fpgai2c_reg_set(i2c, FPGAI2C_REG_CMD, FPGAI2C_REG_CMD_WRITE);
    }

    return 0;
}

static int fpgai2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    struct fpgalogic_i2c *i2c = i2c_get_adapdata(adap);
    int ret;
    unsigned long timeout = jiffies + msecs_to_jiffies(1000);

    i2c->msg = msgs;
    i2c->pos = 0;
    i2c->nmsgs = num;
    i2c->state = STATE_INIT;

     /* Handle the transfer */
     while (time_before(jiffies, timeout)) {
         i2c_get_mutex(i2c);
         ret = fpgai2c_poll(i2c);
         i2c_release_mutex(i2c);

         if (i2c->state == STATE_DONE || i2c->state == STATE_ERROR)
              return (i2c->state == STATE_DONE) ? num : ret;

         if (ret == 0)
              timeout = jiffies + HZ;

         usleep_range(5, 15);
     }
     printk("[%s] ERROR STATE_ERROR\n", __FUNCTION__);

     i2c->state = STATE_ERROR;

     return -ETIMEDOUT;

}

static u32 fpgai2c_func(struct i2c_adapter *adap)
{
/* a typical full-I2C adapter would use the following  */
    return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm fpgai2c_algorithm= {
    .master_xfer = fpgai2c_xfer, /*write I2C messages */
    .functionality = fpgai2c_func, /* what the adapter supports */
};

static int fpgai2c_init(struct fpgalogic_i2c *i2c)
{
    int prescale;
    int diff;
    u8 ctrl;

    i2c->reg_set = fpgai2c_reg_set_8;
    i2c->reg_get = fpgai2c_reg_get_8;

    ctrl = fpgai2c_reg_get(i2c, FPGAI2C_REG_CONTROL);
    /* make sure the device is disabled */
    fpgai2c_reg_set(i2c, FPGAI2C_REG_CONTROL, ctrl & ~(FPGAI2C_REG_CTRL_EN|FPGAI2C_REG_CTRL_IEN));

    /*
    *  I2C Frequency depends on host clock
    *  input clock of 100MHz
    *  prescale to 100MHz / ( 5*100kHz) -1 = 199 = 0x4F 100000/(5*100)-1=199=0xc7
    */
    prescale = (i2c->ip_clock_khz / (5 * i2c->bus_clock_khz)) - 1;
    prescale = clamp(prescale, 0, 0xffff);

    diff = i2c->ip_clock_khz / (5 * (prescale + 1)) - i2c->bus_clock_khz;
    if (abs(diff) > i2c->bus_clock_khz / 10) {
        printk("[%s] ERROR Unsupported clock settings: core: %d KHz, bus: %d KHz\n",
            __FUNCTION__, i2c->ip_clock_khz, i2c->bus_clock_khz);
        return -EINVAL;
    }

    fpgai2c_reg_set(i2c, FPGAI2C_REG_PRELOW, prescale & 0xff);
    fpgai2c_reg_set(i2c, FPGAI2C_REG_PREHIGH, prescale >> 8);

    /* Init the device */
    fpgai2c_reg_set(i2c, FPGAI2C_REG_CMD, FPGAI2C_REG_CMD_IACK);
    fpgai2c_reg_set(i2c, FPGAI2C_REG_CONTROL, ctrl | FPGAI2C_REG_CTRL_EN);

    /* Initialize interrupt handlers if not already done */
    init_waitqueue_head(&i2c->wait);
    return 0;
}

static int adap_data_init(struct i2c_adapter *adap, int i2c_ch_index)
{
    struct fpgapci_devdata *pci_privdata = 0;
    pci_privdata = (struct fpgapci_devdata*) dev_get_drvdata(adap->dev.parent);

    if (pci_privdata == 0) {
        printk("[%s]: ERROR pci_privdata is 0\n", __FUNCTION__);
        return -1;
    }
#if DEBUG
    pddf_dbg(FPGA, KERN_INFO "[%s] index: [%d] fpga_data__base_addr:0x%0x8lx"
        " fpgapci_bar_len:0x%08lx fpga_i2c_ch_base_addr:0x%08lx ch_size=0x%x supported_i2c_ch=%d",
             __FUNCTION__, i2c_ch_index, pci_privdata->fpga_data_base_addr,
            pci_privdata->bar_length, pci_privdata->fpga_i2c_ch_base_addr,
            pci_privdata->fpga_i2c_ch_size, pci_privdata->max_fpga_i2c_ch);
#endif
    if (i2c_ch_index >= pci_privdata->max_fpga_i2c_ch || pci_privdata->max_fpga_i2c_ch > I2C_PCI_MAX_BUS) {
        printk("[%s]: ERROR i2c_ch_index=%d max_ch_index=%d out of range: %d\n",
             __FUNCTION__, i2c_ch_index, pci_privdata->max_fpga_i2c_ch, I2C_PCI_MAX_BUS);
        return -1;
    }
#ifdef __STDC_LIB_EXT1__
    memset_s(&fpgalogic_i2c[i2c_ch_index], sizeof(fpgalogic_i2c[0]), 0, sizeof(fpgalogic_i2c[0]));
#else
    memset(&fpgalogic_i2c[i2c_ch_index], 0, sizeof(fpgalogic_i2c[0]));
#endif
    /* Initialize driver's itnernal data structures */
    fpgalogic_i2c[i2c_ch_index].reg_shift = 0; /* 8 bit registers */
    fpgalogic_i2c[i2c_ch_index].reg_io_width = 1; /* 8 bit read/write */
    fpgalogic_i2c[i2c_ch_index].timeout = 500;//1000;//1ms
    fpgalogic_i2c[i2c_ch_index].ip_clock_khz = 100000;//100000;/* input clock of 100MHz */
    fpgalogic_i2c[i2c_ch_index].bus_clock_khz = 100;
    fpgalogic_i2c[i2c_ch_index].base = pci_privdata->fpga_i2c_ch_base_addr +
                          i2c_ch_index* pci_privdata->fpga_i2c_ch_size;
    mutex_init(&fpgalogic_i2c[i2c_ch_index].lock);
    fpgai2c_init(&fpgalogic_i2c[i2c_ch_index]);


    adap->algo_data = &fpgalogic_i2c[i2c_ch_index];
    i2c_set_adapdata(adap, &fpgalogic_i2c[i2c_ch_index]);
    return 0;
}

static int pddf_i2c_pci_add_numbered_bus_default (struct i2c_adapter *adap, int i2c_ch_index)
{
    int ret = 0;

    adap_data_init(adap, i2c_ch_index);
    adap->algo = &fpgai2c_algorithm;

    ret = i2c_add_numbered_adapter(adap);
    return ret;
}

/*
 * FPGAPCI APIs
 */
int board_i2c_fpgapci_read(uint32_t offset)
{
	int data;
	data=ioread32(fpga_ctl_addr+offset);
	return data;
}


int board_i2c_fpgapci_write(uint32_t offset, uint32_t value)
{
	iowrite32(value, fpga_ctl_addr+offset);
	return (0);
}


static int __init pddf_xilinx_device_7021_algo_init(void)
{
    pddf_dbg(FPGA, KERN_INFO "[%s]\n", __FUNCTION__);
    pddf_i2c_pci_add_numbered_bus = pddf_i2c_pci_add_numbered_bus_default;
    ptr_fpgapci_read = board_i2c_fpgapci_read;
    ptr_fpgapci_write = board_i2c_fpgapci_write;
    return 0;
}

static void __exit pddf_xilinx_device_7021_algo_exit(void)
{
    pddf_dbg(FPGA, KERN_INFO "[%s]\n", __FUNCTION__);

    pddf_i2c_pci_add_numbered_bus = NULL;
    ptr_fpgapci_read = NULL;
    ptr_fpgapci_write = NULL;
    return;
}


module_init (pddf_xilinx_device_7021_algo_init);
module_exit (pddf_xilinx_device_7021_algo_exit);
MODULE_DESCRIPTION("Xilinx Corporation Device 7021 FPGAPCIe I2C-Bus algorithm");
MODULE_LICENSE("GPL");

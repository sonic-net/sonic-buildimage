/*
 * Juniper Networks TMC I2C Accelerator driver
 *
 * Copyright (C) 2020 Juniper Networks
 * Author: Ashish Bhensdadia <bashish@juniper.net>
 *
 * This driver implement the I2C functionality
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/i2c-mux.h>
#include <linux/interrupt.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/mfd/core.h>
#include "jnx-tmc.h"


#define TMC_I2C_MASTER_I2C_SCAN_RESET_BIT   BIT(31)

#define TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET     0x0
#define TMC_I2C_MSTR_AUTOMATION_I2C_DPMEM_OFFSET    0x10

#define TMC_I2C_MSTR_AUTOMATION_I2C(adap, offset)   \
	    ((adap)->membase + offset)

#define TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_0    0x0
#define TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_8    0x8
#define TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_4    0x4
#define TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_C    0xC
#define TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_10   0x10
#define TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_14   0x14
#define TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_18   0x18
#define TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_1C   0x1C
#define TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_20   0x20
#define TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_24   0x24

#define TMC_I2C_MSTR_I2C_DPMEM(adap, offset) \
	((adap)->dpmbase + offset)

#define TMC_I2C_TRANS_LEN		2
#define tmc_iowrite(val, addr)		iowrite32((val), (addr))

#define TMC_I2C_CTRL_GROUP(g)		(((g) & 0xFF) << 8)
#define TMC_I2C_CTRL_WRCNT(w)		(((w) & 0x3F) << 16)
#define TMC_I2C_CTRL_RDCNT(r)		(((r) & 0x3F) << 16)
#define TMC_I2C_CTRL_DEVADDR(d)		(((d) & 0xFF) << 8)
#define TMC_I2C_CTRL_OFFSET(o)		((o) & 0xFF)


#define TMC_I2C_MEM_CTRL_VLD		BIT(31)

#define TMC_I2C_CTRL_ERR(s)		((s) & 0x0F000000)
#define TMC_I2C_CTRL_DONE_BIT(s)	((s) & BIT(31))
#define TMC_I2C_CTRL_STATUS_OK(s)	(TMC_I2C_CTRL_DONE_BIT(s) & \
					TMC_I2C_CTRL_ERR(s))
#define TMC_I2C_CTRL_DONE(s)		(TMC_I2C_CTRL_DONE_BIT(s) == BIT(31))

#define TMC_I2C_STAT_INC(adap, s)	(((adap)->stat.s)++)
#define TMC_I2C_STAT_INCN(adap, s, n)	(((adap)->stat.s) += (n))
#define TMC_I2C_GET_MASTER(tadap)	((tadap)->tctrl)

#define TMC_I2C_READ   0x1
#define TMC_I2C_WRITE  0x2

#define TMC_I2C_MASTER_LOCK(s, flags) \
do { \
	spin_lock_irqsave(&(s)->lock, flags); \
} while (0)

#define TMC_I2C_MASTER_UNLOCK(s, flags) \
do { \
	spin_unlock_irqrestore(&(s)->lock, flags); \
} while (0)

#define tmc_i2c_dbg(dev, fmt, args...) \
do { \
	if (tmc_i2c_debug >= 1) \
		dev_err(dev, fmt, ## args); \
} while (0)


/* pfe TMC i2c channel */
static int pfe_channel = 32;
module_param(pfe_channel, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(pfe_channel, "Maximum number of channel for PFE TMC");

/* chassid TMC i2c channel */
static int chd_channel = 11;
module_param(chd_channel, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(chd_channel, "Maximum number of channel for CHASSID TMC");

static u32 wr_index_to_oper[] = {0x01000000, 0x02000000,
				0x84000000, 0x85000000, 0x83000000};
static u32 rd_index_to_oper[] = {0x08000000, 0x09000000, 0x0A000000,
				0x8B000000, 0x8C000000, 0x8D000000, 0x83000000};

struct tmc_i2c_adapter_stats {
	u32 abort;
	u32 go;
	u32 mstr_rdy;
	u32 mstr_busy;
	u32 trans_compl;
	u32 msg_cnt;
	u32 rd_cnt;
	u32 wr_cnt;
	u32 byte_cnt;
	u32 slave_timeo;
	u32 scl_bus_loss;
	u32 sda_bus_loss;
	u32 ack_ptimeo;
	u32 rd_cnt_0;
	u32 rd_cnt_gt32;
	u32 rst_tgt;
	u32 rst_mstr;
};

struct tmc_i2c_adapter {
	void __iomem *membase;
	void __iomem *dpmbase;
	struct i2c_adapter adap;
	struct i2c_mux_core *muxc;
	struct tmc_i2c_ctrl *tctrl;
	int mux_channels;
	int mux_select;
	u32 i2c_delay;
	int entries;
	int master;
	u32 control;
	u32 speed;
	bool done;
	bool polling;
	bool use_block;
	wait_queue_head_t wait;
	struct tmc_i2c_adapter_stats stat;
};

struct tmc_i2c_ctrl {
	void __iomem *membase;
	void __iomem *dpmbase;
	struct i2c_adapter **adap;
	struct device *dev;
	int num_masters;
	int mux_channels;
	u32 i2c_delay;
	u32 master_mask;
	spinlock_t lock;	    /* master lock */
};

/*
 * Reset the Tmc I2C master
 */
static void tmc_i2c_reset_master(struct i2c_adapter *adap)
{
	struct tmc_i2c_adapter *tadap = i2c_get_adapdata(adap);
	struct tmc_i2c_ctrl *tmc = TMC_I2C_GET_MASTER(tadap);
	u32 val, master = tadap->master;
	unsigned long flags;
	void __iomem *addr;

	dev_warn(&adap->dev, "Re-setting i2c master: %d\n", master);

	TMC_I2C_MASTER_LOCK(tmc, flags);

	addr = tmc->membase + TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET;
	val = ioread32(addr);
	tmc_iowrite(val | (TMC_I2C_MASTER_I2C_SCAN_RESET_BIT), addr);
	tmc_iowrite(val & ~(TMC_I2C_MASTER_I2C_SCAN_RESET_BIT), addr);
	TMC_I2C_STAT_INC(tadap, rst_mstr);

	TMC_I2C_MASTER_UNLOCK(tmc, flags);
}

/*
 * check if the Tmc I2C master is ready
 */
static int tmc_i2c_mstr_wait_rdy(struct i2c_adapter *adap, u8 rw, u32 delay)
{
	struct tmc_i2c_adapter *tadap = i2c_get_adapdata(adap);
	unsigned long timeout;
	u32 val;

	val = ioread32(TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
			TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET));
	if (val) {
		tmc_iowrite(0x80000000, TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
				TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET));
		mdelay(5);
		tmc_iowrite(0x00000000, TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
				TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET));
		val = ioread32(TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
				TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET));
	} else {
		tmc_iowrite(0x00000000, TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
			TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET));
	}

	if ((rw == TMC_I2C_READ) && (delay)) {
		tmc_iowrite(0x80000000, TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
				TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET));
		tmc_iowrite(0x00000000, TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
				TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET));
		return 0;
	}

	timeout = jiffies + adap->timeout;
	do {
		val = ioread32(TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
				TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET));
		if (!val)
			return 0;

		if (tadap->polling) {
			usleep_range(50, 100);
		} else {
			tadap->done = false;
			wait_event_timeout(tadap->wait, tadap->done,
					   adap->timeout);
		}
	} while (time_before(jiffies, timeout));

	TMC_I2C_STAT_INC(tadap, mstr_busy);

	return -EBUSY;
}

/*
 * Wait for master completion
 */
static u32 tmc_i2c_mstr_wait_completion(struct i2c_adapter *adap,
					u32 dp_entry_offset)
{
	struct tmc_i2c_adapter *tadap = i2c_get_adapdata(adap);
	u32 val;

	if (tadap->polling) {
		/* Poll for the results */
		unsigned long timeout = jiffies + adap->timeout;

		do {
			usleep_range(1000, 1200);
			val = ioread32(TMC_I2C_MSTR_I2C_DPMEM(tadap,
					dp_entry_offset));
			if (TMC_I2C_CTRL_DONE(val))
				break;
		} while (time_before(jiffies, timeout));
	} else {
		wait_event_timeout(tadap->wait, tadap->done, adap->timeout);
		val = ioread32(TMC_I2C_MSTR_I2C_DPMEM(tadap,
					dp_entry_offset));
	}

	return TMC_I2C_CTRL_STATUS_OK(val);
}

/*
 * TMC I2C delay read/write operation
 */
static int tmc_i2c_delay_rw_op(struct i2c_adapter *adap, u8 rw, u32 mux,
			u32 addr, u32 offset, u32 len, u32 delay, u8 *buf)
{
	struct tmc_i2c_adapter *tadap = i2c_get_adapdata(adap);
	struct device *dev = &adap->dev;
	int err, n;
	u32 control = 0, data = 0;
	u32 val;

	err = tmc_i2c_mstr_wait_rdy(adap, rw, 0);
	if (err < 0) {
		tmc_i2c_reset_master(adap);
		return err;
	}

	TMC_I2C_STAT_INC(tadap, mstr_rdy);

	/* initialize the start address and mux */
	tmc_iowrite(0x00000000, TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
		    TMC_I2C_MSTR_AUTOMATION_I2C_DPMEM_OFFSET));

	tmc_iowrite(mux, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_0));
	tmc_iowrite(0x84400000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_4));

	/* populate delay */
	if (delay) {
		if (delay > 1000) {
			delay = delay/1000;
			delay |= (1 << 16);
		}
	}
	tmc_iowrite(delay, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_8));
	tmc_iowrite(0x86000000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_C));

	/* prepare control command */
	control |= TMC_I2C_CTRL_DEVADDR(addr);
	control |= TMC_I2C_CTRL_OFFSET(offset);

	if (rw == TMC_I2C_WRITE) {
		for (n = 0; n < len; n++)
			data |= (buf[n] << ((len - 1 - n) * 8));
		tmc_iowrite(data, TMC_I2C_MSTR_I2C_DPMEM(tadap,
				TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_10));
		control |= TMC_I2C_CTRL_WRCNT(len);
		control |= wr_index_to_oper[len-1];
		dev_dbg(dev, "WR Data: [%#04x, %#04x, %#04x, %#04x]\n",
			((data >> 24) & 0xff), ((data >> 16) & 0xff),
			((data >> 8) & 0xff), (data & 0xff));

	} else {
		/* read */
		tmc_iowrite(0x00000000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
				TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_10));
		control |= TMC_I2C_CTRL_RDCNT(len);
		control |= rd_index_to_oper[len-1];
	}

	/*
	 * valid this transaction as well
	 */
	control |= TMC_I2C_MEM_CTRL_VLD;

	tadap->control = control;

	/*
	 * operation control command
	 */
	tmc_iowrite(control, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_14));

	/*
	 * End commands
	 */
	tmc_iowrite(0x00000000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_18));
	tmc_iowrite(0x8E000000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_1C));
	tmc_iowrite(0x00000000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_20));
	tmc_iowrite(0x8F000000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_24));

	dev_dbg(dev, "Control (%#x): RD_WR_TYPE:%#02x, RD_WR_LEN:%d,"
		"Addr:%#01x, Offset:%#02x\n", control,
		((control >> 24) & 0x3F), ((control >> 16) & 0x3F),
		((control >> 8) & 0xff), ((control) & 0xff));

	tadap->done = false;

	/* fire the transaction */
	tmc_iowrite(0x00000001, TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
			TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET));

	TMC_I2C_STAT_INC(tadap, go);

	val = tmc_i2c_mstr_wait_completion(adap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_10);
	if (val) {
		dev_err(&adap->dev,
			"i2c transaction error (0x%08x)\n", val);

		/* stop the transaction */
		tmc_iowrite(0x00000000, TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
				TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET));
		return -EIO;
	}

	/*
	 * read a word of data
	 */
	if (rw == TMC_I2C_READ) {
		data = ioread32(TMC_I2C_MSTR_I2C_DPMEM(tadap,
				TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_10));
		for (n = 0; n < len; n++)
			buf[n] = (data >> (n * 8)) & 0xff;

		dev_dbg(dev, "RD Data: [%#04x, %#04x, %#04x, %#04x]\n",
			((data >> 24) & 0xff), ((data >> 16) & 0xff),
			((data >> 8) & 0xff), (data & 0xff));

		TMC_I2C_STAT_INC(tadap, rd_cnt);
	} else {
		/* write */
		TMC_I2C_STAT_INC(tadap, wr_cnt);
	}

	/* stop the transaction */
	tmc_iowrite(0x00000000, TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
			TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET));

	return 0;

}

/*
 *TMC I2C none delay Read/write opertion
 */
static int tmc_i2c_none_delay_rw_op(struct i2c_adapter *adap, u8 rw, u32 mux,
					u32 addr, u32 offset, u32 len, u8 *buf)
{
	struct tmc_i2c_adapter *tadap = i2c_get_adapdata(adap);
	struct device *dev = &adap->dev;
	int err, n;
	u32 control = 0, data = 0;
	u32 val;

	err = tmc_i2c_mstr_wait_rdy(adap, rw, 0);
	if (err < 0) {
		tmc_i2c_reset_master(adap);
		return err;
	}

	TMC_I2C_STAT_INC(tadap, mstr_rdy);

	/* initialize the start address and mux */
	tmc_iowrite(0x00000000, TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
		    TMC_I2C_MSTR_AUTOMATION_I2C_DPMEM_OFFSET));

	tmc_iowrite(mux, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_0));
	tmc_iowrite(0x84400000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_4));


	/* prepare control command */
	control |= TMC_I2C_CTRL_DEVADDR(addr);
	control |= TMC_I2C_CTRL_OFFSET(offset);

	if (rw == TMC_I2C_WRITE) {
		for (n = 0; n < len; n++)
			data |= (buf[n] << (n * 8));
		tmc_iowrite(data, TMC_I2C_MSTR_I2C_DPMEM(tadap,
				TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_8));
		control |= wr_index_to_oper[len-1];
		dev_dbg(dev, "WR Data: [%#04x, %#04x, %#04x, %#04x]\n",
			((data >> 24) & 0xff), ((data >> 16) & 0xff),
			((data >> 8) & 0xff), (data & 0xff));

	} else {
		/* read */
		tmc_iowrite(0x00000000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
				TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_8));
		control |= rd_index_to_oper[len-1];
	}

	/*
	 * valid this transaction as well
	 */
	control |= TMC_I2C_MEM_CTRL_VLD;

	tadap->control = control;

	/*
	 * operation control command
	 */
	tmc_iowrite(control, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_C));

	/*
	 * End commands
	 */
	tmc_iowrite(0x00000000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_10));
	tmc_iowrite(0x8E000000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_14));
	tmc_iowrite(0x00000000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_18));
	tmc_iowrite(0x8F000000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_1C));

	dev_dbg(dev, "Control (%#x): RD_WR_TYPE:%#02x, RD_WR_LEN:%d,"
		"Addr:%#01x, Offset:%#02x\n", control,
		((control >> 24) & 0x3F), ((control >> 16) & 0x3F),
		((control >> 8) & 0xff), ((control) & 0xff));

	tadap->done = false;

	/* fire the transaction */
	tmc_iowrite(0x00000001, TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
			TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET));

	TMC_I2C_STAT_INC(tadap, go);

	/* wait till transaction complete */
	val = tmc_i2c_mstr_wait_completion(adap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_8);
	if (val) {
		dev_err(&adap->dev,
			"i2c transaction error (0x%08x)\n", val);

		/* stop the transaction */
		tmc_iowrite(0x00000000, TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
				TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET));
		return -EIO;
	}

	/*
	 * read a word of data
	 */
	if (rw == TMC_I2C_READ) {
		data = ioread32(TMC_I2C_MSTR_I2C_DPMEM(tadap,
				TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_8));
		for (n = 0; n < len; n++)
			buf[n] = (data >> (n * 8)) & 0xff;

		dev_dbg(dev, "RD Data: [%#04x, %#04x, %#04x, %#04x]\n",
			((data >> 24) & 0xff), ((data >> 16) & 0xff),
			((data >> 8) & 0xff), (data & 0xff));
		TMC_I2C_STAT_INC(tadap, rd_cnt);
	} else {
		/* write */
		TMC_I2C_STAT_INC(tadap, wr_cnt);
	}

	/* stop the transaction */
	tmc_iowrite(0x00000000, TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
			TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET));

	return 0;
}

/*
 * TMC I2C read/write operation
 */
static int tmc_i2c_rw_op(struct i2c_adapter *adap, u8 rw, u32 mux,
			 u32 addr, u32 offset, u32 len, u8 *buf)
{
	struct tmc_i2c_adapter *tadap = i2c_get_adapdata(adap);
	u32 i2c_delay = tadap->i2c_delay;

	if (i2c_delay) {
		return tmc_i2c_delay_rw_op(adap, rw, mux, addr, offset,
						len, i2c_delay, buf);
	} else {
		return tmc_i2c_none_delay_rw_op(adap, rw, mux, addr, offset,
						len, buf);
	}
}

static int tmc_i2c_calc_entries(int msglen)
{
	int entries = msglen / TMC_I2C_TRANS_LEN;

	return (entries += (msglen % TMC_I2C_TRANS_LEN) ? 1 : 0);
}

static int tmc_i2c_block_read(struct i2c_adapter *adap,
				   struct i2c_msg *msgs, int num)
{
	struct tmc_i2c_adapter *tadap = i2c_get_adapdata(adap);
	struct device *dev = &adap->dev;
	int curmsg, entries, len;
	int offset = 0;
	struct i2c_msg *msg;
	int err, n = 0;
	u8 rwbuf[4] = {0};

	dev_dbg(dev, "Read i2c Block\n");

	for (curmsg = 0, offset = 0; curmsg < num; curmsg++) {
		msg = &msgs[curmsg];
		len = msg->len;

		if (msg->flags & I2C_M_RECV_LEN)
			len = (I2C_SMBUS_BLOCK_MAX + 1);

		entries = tmc_i2c_calc_entries(len);

		if (msg->flags & I2C_M_RD) {
			if (curmsg == 1 && ((msg->flags & I2C_M_RECV_LEN &&
				!(msgs[0].flags & I2C_M_RD)) ||
				(msg->len > TMC_I2C_TRANS_LEN))) {
				offset = msgs[0].buf[0];
			}

			while (entries) {
				err = tmc_i2c_rw_op(adap, TMC_I2C_READ,
					tadap->mux_select,
					msgs[0].addr, offset,
					TMC_I2C_TRANS_LEN, rwbuf);
				if (err < 0)
					return err;
				msg = &msgs[num - 1];
				msg->buf[n] = rwbuf[0];
				msg->buf[n+1] = rwbuf[1];
				n = n + TMC_I2C_TRANS_LEN;
				offset = offset + TMC_I2C_TRANS_LEN;
				entries--;
			}
		}
	}

	return 0;
}

/*
 *TMC I2C SMB Read opertion
 */
static int tmc_i2c_smb_block_read_op(struct i2c_adapter *adap, u8 rw, u32 mux,
					u32 addr, u32 offset, u32 len, u8 *buf)
{
	struct tmc_i2c_adapter *tadap = i2c_get_adapdata(adap);
	int err, i = 0;
	u32 control = 0, data = 0;
	u32 start_add;

	err = tmc_i2c_mstr_wait_rdy(adap, rw, 0);
	if (err < 0) {
		tmc_i2c_reset_master(adap);
		return err;
	}

	TMC_I2C_STAT_INC(tadap, mstr_rdy);

	/* initialize the start address and mux */
	tmc_iowrite(0x00000000, TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
		    TMC_I2C_MSTR_AUTOMATION_I2C_DPMEM_OFFSET));

	tmc_iowrite(mux, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_0));
	tmc_iowrite(0x84400000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_4));


	/* prepare control command */
	control |= TMC_I2C_CTRL_DEVADDR(addr);
	control |= TMC_I2C_CTRL_OFFSET(offset);

	/* read */
	tmc_iowrite(0x00000000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_8));
	control |= TMC_I2C_CTRL_RDCNT(len);;
	control |= rd_index_to_oper[6];

	tadap->control = control;

	/*
	 * operation control command
	 */
	tmc_iowrite(control, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_C));

	/*
	 * End commands
	 */
	tmc_iowrite(0x00000000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_10));
	tmc_iowrite(0x8E000000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_14));
	tmc_iowrite(0x00000000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_18));
	tmc_iowrite(0x8F000000, TMC_I2C_MSTR_I2C_DPMEM(tadap,
			TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_1C));

	tadap->done = false;

	/* fire the transaction */
	tmc_iowrite(0x00000001, TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
			TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET));

	TMC_I2C_STAT_INC(tadap, go);

	/*
	 * read a block of data
	 */
	start_add = TMC_I2C_MSTR_I2C_DPMEM_ENTRY_OFFSET_8;
	while (len > 0) {
		usleep_range(10000, 12000);
		data = ioread32(TMC_I2C_MSTR_I2C_DPMEM(tadap, start_add));
		buf[i] = data & 0xff;
		buf[i + 1] = (data >> 8) & 0xff;
		start_add = start_add + 8;
		i = i + 2;
		len = len - 2;

		TMC_I2C_STAT_INC(tadap, rd_cnt);
	}

	/* stop the transaction */
	tmc_iowrite(0x00000000, TMC_I2C_MSTR_AUTOMATION_I2C(tadap,
			TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET));

	return 0;
}

static int tmc_i2c_smb_block_read(struct i2c_adapter *adap,
				   struct i2c_msg *msgs, int num)
{
	struct tmc_i2c_adapter *tadap = i2c_get_adapdata(adap);
	int curmsg, len;
	int offset = 0;
	struct i2c_msg *msg;
	int err, i = 0;
	u8 rwbuf[32] = {0};

	for (curmsg = 0, offset = 0; curmsg < num; curmsg++) {
		msg = &msgs[curmsg];
		len = msg->len;

		if (msg->flags & I2C_M_RECV_LEN)
			len = (I2C_SMBUS_BLOCK_MAX + 1);

		if (msg->flags & I2C_M_RD) {
			if ((curmsg == 1) && (msg->flags & I2C_M_RECV_LEN) &&
				!(msgs[0].flags & I2C_M_RD)) {
				offset = msgs[0].buf[0];
			}

			err = tmc_i2c_smb_block_read_op(adap, TMC_I2C_READ,
				tadap->mux_select,
				msgs[0].addr, offset,
				32, rwbuf);
			if (err < 0) {
				return err;
			}
			msg = &msgs[num - 1];
			for (i = 0; i < len - 1; i++) {
				msg->buf[i] = rwbuf[i];
			}
		}
	}

	return 0;
}

static int tmc_i2c_mstr_xfer(struct i2c_adapter *adap,
				  struct i2c_msg *msgs, int num)
{
	struct tmc_i2c_adapter *tadap = i2c_get_adapdata(adap);
	struct device *dev = &adap->dev;
	int n, curmsg, len = 0;
	int err = 0;
	struct i2c_msg *msg;
	bool read;
	u8 rwbuf[4] = {0};

	dev_dbg(dev, "Num messages -> %d\n", num);

	/*
	 * Initialize all vars
	 */
	tadap->entries   = 0;
	tadap->use_block = false;

	for (curmsg = 0; curmsg < num; curmsg++) {
		msg = &msgs[curmsg];
		dev_dbg(dev, "[%02d] %d bytes, flag %#02x buf %#02x\n",
			curmsg, msg->len, msg->flags, msg->buf[0]);
		if ((msg->len > TMC_I2C_TRANS_LEN) ||
		    ((msg->flags & I2C_M_RD) &&
		     (msg->flags & I2C_M_RECV_LEN))) {
			/* If PEC is enabled len will come as 3 for a word read.
			 * We don't want to use block read for this case.
			 */
			if ((msg->flags & I2C_CLIENT_PEC) &&
				(msg->len == TMC_I2C_TRANS_LEN+1)) {
				tadap->use_block = false;
			} else {
				tadap->use_block = true;
			}
			break;
		}
	}

	if (tadap->use_block) {
		/* Read Block */
		if ((msg->flags & I2C_M_RD) && (msg->flags & I2C_M_RECV_LEN)) {
		    err = tmc_i2c_smb_block_read(adap, msgs, num);
		} else {
		    err = tmc_i2c_block_read(adap, msgs, num);
		}
		if (err < 0)
			return err;
	} else {
		read = msgs[num - 1].flags & I2C_M_RD;
		for (curmsg = 0; curmsg < num; curmsg++) {
			msg = &msgs[curmsg];
			len = msg->len;

			dev_dbg(dev, "  [%02d] %s %d bytes addr %#02x, "
				"flag %#02x, buf[0] %#02x\n", curmsg,
				read ? "RD" : "WR", len, msg->addr,
				msg->flags, msg->buf[0]);

			/* SMBus quick read/write command */
			if (len == 0 && curmsg == 0 && num == 1) {
				if (read) {
					len = 1;
				}
				break;
			}

			if (curmsg == 0) {
				if (!read) {
					for (n = 1; n < len; n++)
						rwbuf[n-1] = (msg->buf[n]);
					len--;
				} else {
					/* read operation */
					continue;
				}
			}
		}

		if (!read) {
			/* write */
			err = tmc_i2c_rw_op(adap, TMC_I2C_WRITE,
					tadap->mux_select, msgs[0].addr,
					msgs[0].buf[0], len, rwbuf);
		} else {
			/* read */
				/*
				 * If PEC is enabled read only 2 bytes as expected in
				 * case of a word read instead of 3 to make it compatible
				 * with word write implementation.
				 */
				if (msg->flags & I2C_CLIENT_PEC && (len == TMC_I2C_TRANS_LEN + 1)) {
					len--;
				}

			err = tmc_i2c_rw_op(adap, TMC_I2C_READ,
					tadap->mux_select,
					msgs[0].addr, msgs[0].buf[0],
					len, rwbuf);
			msg = &msgs[num - 1];
			len = msg->len;
			/*
			 * To avoid failure in PEC enabled case clear flag.
			 */
			if (len == TMC_I2C_TRANS_LEN + 1) {
				msgs[num - 1].flags &= ~I2C_M_RD;
			}
			for (n = 0; n < len; n++)
				msg->buf[n] = rwbuf[n];
		}
		if (err < 0)
			return err;
	}

	TMC_I2C_STAT_INCN(tadap, msg_cnt, num);

	return num;
}

static u32 tmc_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL
		| I2C_FUNC_SMBUS_READ_BLOCK_DATA;
}

static const struct i2c_algorithm tmc_i2c_algo = {
	.master_xfer	= tmc_i2c_mstr_xfer,
	.functionality	= tmc_i2c_func,
};

static int tmc_i2c_mux_group_sel(struct i2c_mux_core *muxc, u32 chan)
{
	struct tmc_i2c_adapter *tadap = i2c_mux_priv(muxc);

	dev_dbg(muxc->dev, "chan = %d\n", chan);

	if (!tadap || chan > TMC_I2C_MSTR_MAX_GROUPS)
		return -ENODEV;
	tadap->mux_select = chan;

	return 0;
}

static int tmc_i2c_mux_init(struct i2c_adapter *adap)
{
	struct tmc_i2c_adapter *tadap = i2c_get_adapdata(adap);
	int chan, ret;

	tadap->muxc = i2c_mux_alloc(adap, &adap->dev, tadap->mux_channels,
				    0, 0, tmc_i2c_mux_group_sel, NULL);

	if (!tadap->muxc)
		return -ENOMEM;

	tadap->muxc->priv = tadap;
	for (chan = 0; chan < tadap->mux_channels; chan++) {
		ret = i2c_mux_add_adapter(tadap->muxc, 0, chan, 0);
		if (ret) {
			dev_err(&adap->dev, "Failed to add adapter %d\n", chan);
			i2c_mux_del_adapters(tadap->muxc);
			return ret;
		}
	}

	return 0;
}

static struct i2c_adapter *
tmc_i2c_init_one(struct tmc_i2c_ctrl *tmc,
		int master, int id)
{
	struct tmc_i2c_adapter *adapter;
	struct device *dev = tmc->dev;
	int err;

	adapter = devm_kzalloc(dev, sizeof(*adapter), GFP_KERNEL);
	if (!adapter)
		return ERR_PTR(-ENOMEM);

	init_waitqueue_head(&adapter->wait);
	adapter->adap.owner	= THIS_MODULE;
	adapter->adap.algo	= &tmc_i2c_algo;
	adapter->adap.nr	= -1;
	adapter->adap.timeout	= HZ / 5;
	adapter->master		= master;
	adapter->mux_channels	= tmc->mux_channels;
	adapter->i2c_delay	= tmc->i2c_delay;
	adapter->membase	= tmc->membase;
	adapter->dpmbase	= tmc->dpmbase;
	adapter->polling	= 1;
	adapter->tctrl		= tmc;

	i2c_set_adapdata(&adapter->adap, adapter);
	snprintf(adapter->adap.name, sizeof(adapter->adap.name),
		 "%s:%d", dev_name(dev), master);

	adapter->adap.dev.parent = dev;
	err = i2c_add_numbered_adapter(&adapter->adap);
	if (err)
		goto error;

	err = tmc_i2c_mux_init(&adapter->adap);
	if (err)
		goto err_remove;

	dev_dbg(dev, "Adapter[%02d-%02d]: "
		"dpmbase: 0x%lx\n", id, master,
		(unsigned long)adapter->dpmbase);
	return &adapter->adap;

err_remove:
	i2c_del_adapter(&adapter->adap);
error:
	return ERR_PTR(err);
}

static void tmc_i2c_cleanup_one(struct i2c_adapter *adap)
{
	struct tmc_i2c_adapter *tadap = i2c_get_adapdata(adap);

	i2c_mux_del_adapters(tadap->muxc);
	i2c_del_adapter(adap);

}


static int tmc_i2c_of_init(struct device *dev,
				struct tmc_i2c_ctrl *tmc, int id)
{
	u32 mux_channels, master, num_masters = 0, master_mask = 0;
	u32 i2c_delay = 0;

        if (!(master_mask & BIT(master)))
	    num_masters++;
        master_mask |= BIT(master);

        if (id == 0) {
            /* chassisd */
	    mux_channels = chd_channel;
            num_masters = 1;
	    i2c_delay = 0;
        } else if (id == 1) {
            /* pfe */
	    mux_channels = pfe_channel;
            num_masters = 1;
	    i2c_delay = 20;
        } else {
            return -EINVAL;
        }

	tmc->adap = devm_kcalloc(dev, num_masters,
				      sizeof(struct i2c_adapter *),
				      GFP_KERNEL);
	if (!tmc->adap)
		return -ENOMEM;

	tmc->num_masters	= num_masters;
	tmc->master_mask	= master_mask;
	tmc->mux_channels	= mux_channels;
	tmc->i2c_delay		= i2c_delay;

	return 0;
}

static int tmc_i2c_probe(struct platform_device *pdev)
{
	int i, n, err;
	struct resource *res;
	struct i2c_adapter *adap;
	struct device *dev = &pdev->dev;
	struct tmc_i2c_ctrl *tmc;

        const struct mfd_cell *cell = mfd_get_cell(pdev);

	/*
	 * Allocate memory for the Tmc FPGA
	 */
	tmc = devm_kzalloc(dev, sizeof(*tmc), GFP_KERNEL);
	if (!tmc)
		return -ENOMEM;

	platform_set_drvdata(pdev, tmc);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	dev_info(dev, "Tmc I2C Accel resource 0x%llx, %llu\n",
		res->start, resource_size(res));

	tmc->membase = devm_ioremap(dev, res->start,
						 resource_size(res));
	if (!tmc->membase)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res)
		return -ENODEV;

	dev_info(dev, "Tmc I2C Mem resource 0x%llx, %llu\n",
		 res->start, resource_size(res));

	tmc->dpmbase = devm_ioremap(dev, res->start,
						 resource_size(res));
	if (!tmc->dpmbase)
		return -ENOMEM;

	tmc->dev = dev;
	spin_lock_init(&tmc->lock);

	err = tmc_i2c_of_init(dev, tmc, cell->id);
	if (err)
		return err;

	dev_info(dev, "Tmc I2C Masters: %d\n", tmc->num_masters);
	dev_info(dev, "Tmc I2C Delay: %d\n", tmc->i2c_delay);

	for (n = 0, i = 0; i < TMC_I2C_MASTER_NR_MSTRS; i++) {
		if (tmc->master_mask & BIT(i)) {
			adap = tmc_i2c_init_one(tmc, i, n);
			if (IS_ERR(adap)) {
				err = PTR_ERR(adap);
				dev_err(dev, "Failed to initialize master "
					"adapter %d: %d\n", i, err);
				goto err_remove;
			}
			tmc->adap[n++] = adap;
		}
	}

	return 0;

err_remove:
	for (n--; n >= 0; n--)
		tmc_i2c_cleanup_one(tmc->adap[n]);
	return err;
}

static int tmc_i2c_remove(struct platform_device *pdev)
{
	struct tmc_i2c_ctrl *tmc = platform_get_drvdata(pdev);
	int i;

	/* Disable all masters */
	tmc_iowrite(0, tmc->membase + TMC_I2C_MSTR_AUTOMATION_I2C_SCAN_OFFSET);

	for (i = 0; i < tmc->num_masters; i++)
		tmc_i2c_cleanup_one(tmc->adap[i]);

	return 0;
}

static struct platform_driver tmc_i2c_driver = {
	.driver = {
		.name   = "i2c-tmc",
		.owner  = THIS_MODULE,
	},
	.probe  = tmc_i2c_probe,
	.remove = tmc_i2c_remove,
};

module_platform_driver(tmc_i2c_driver);

MODULE_DESCRIPTION("Juniper Networks TMC FPGA I2C Accelerator driver");
MODULE_AUTHOR("Ashish Bhensdadia <bashish@juniper.net>");
MODULE_LICENSE("GPL");

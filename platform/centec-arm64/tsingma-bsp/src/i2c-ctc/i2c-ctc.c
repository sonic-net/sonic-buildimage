/*
 * Centec I2C controller driver
 *
 * Author: Wangyb <wangyb@centecnetworks.com>
 *
 * Copyright 2005-2018, Centec Networks (Suzhou) Co., Ltd.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/export.h>
#include "i2c-ctc.h"
#include "../pinctrl-ctc/pinctrl-ctc.h"
#include "../include/sysctl.h"
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>

#define IC_ICK_NS(f) (1000000000 / f)

static char *abort_sources[] = {
	[ABRT_7B_ADDR_NOACK] = "slave address not acknowledged (7bit mode)",
	[ABRT_10ADDR1_NOACK] =
	    "first address byte not acknowledged (10bit mode)",
	[ABRT_10ADDR2_NOACK] =
	    "second address byte not acknowledged (10bit mode)",
	[ABRT_TXDATA_NOACK] = "data not acknowledged",
	[ABRT_GCALL_NOACK] = "no acknowledgement for a general call",
	[ABRT_GCALL_READ] = "read after general call",
	[ABRT_SBYTE_ACKDET] = "start byte acknowledged",
	[ABRT_SBYTE_NORSTRT] =
	    "trying to send start byte when restart is disabled",
	[ABRT_10B_RD_NORSTRT] =
	    "trying to read when restart is disabled (10bit mode)",
	[ABRT_MASTER_DIS] = "trying to use disabled adapter",
	[ARB_LOST] = "lost arbitration",
};

static u32 ctc_readl(struct ctc_i2c_dev *dev, int offset)
{
	u32 value;

	value = readl_relaxed(dev->base + offset);
	return value;
}

static void ctc_writel(struct ctc_i2c_dev *dev, u32 b, int offset)
{
	writel_relaxed(b, dev->base + offset);
}

static void __i2c_ctc_enable(struct ctc_i2c_dev *dev, bool enable)
{
	int timeout = 100;

	do {
		ctc_writel(dev, enable, CTC_IC_ENABLE);
		if ((ctc_readl(dev, CTC_IC_ENABLE_STATUS) & 1) == enable)
			return;
		usleep_range(25, 250);
	} while (timeout--);

	dev_warn(dev->dev, "timeout in %sabling adapter\n",
		 enable ? "en" : "dis");
}

/* Conditional expression:
 *	SCL high level time = (high level cnt + (14 + 4)cnt) * 20ns
 *  SCL low level time = (low level cnt + (1 + 2)cnt) * 20ns
 */
static u32 __ctc_calc_ss_cnt(u32 clk_freq)
{
	return CTC_SCL_Timing(clk_freq) / 2 / 20;
}

static u32 __ctc_calc_fs_cnt(u32 clk_freq)
{
	return CTC_SCL_Timing(clk_freq) / 2 / 20 + 1;
}

int i2c_ctc_init(struct ctc_i2c_dev *dev)
{
	u32 hcnt, lcnt;
	u32 reg, comp_param1;

	reg = ctc_readl(dev, CTC_IC_COMP_TYPE);
	if (reg != CTC_IC_COMP_TYPE_VALUE) {
		dev_err(dev->dev, "Unknown Centec component type: 0x%08x\n",
			reg);
		return -ENODEV;
	}

	/* Disable the adapter */
	__i2c_ctc_enable(dev, false);

	/* Set SCL timing parameters */
	if ((dev->master_cfg & CTC_IC_CON_SPEED_MASK) == CTC_IC_CON_SPEED_FAST) {
		hcnt = __ctc_calc_fs_cnt(dev->clk_freq) - 14 - 4;
		lcnt = __ctc_calc_fs_cnt(dev->clk_freq) - 1 - 2;

		ctc_writel(dev, hcnt, CTC_IC_FS_SCL_HCNT);
		ctc_writel(dev, lcnt, CTC_IC_FS_SCL_LCNT);
		dev_info(dev->dev,
			 "Fast-mode HCNT:LCNT = %d:%d, clk_freq = %d\n", hcnt,
			 lcnt, dev->clk_freq);

	} else {
		hcnt = __ctc_calc_ss_cnt(dev->clk_freq) - 14 - 2;
		lcnt = __ctc_calc_ss_cnt(dev->clk_freq) - 1;

		ctc_writel(dev, hcnt, CTC_IC_SS_SCL_HCNT);
		ctc_writel(dev, lcnt, CTC_IC_SS_SCL_LCNT);
		dev_info(dev->dev,
			 "Standard-mode HCNT:LCNT = %d:%d, clk_freq = %d\n",
			 hcnt, lcnt, dev->clk_freq);
	}

	/* Configure SDA Hold Time if required */
	if (dev->sda_hold_time) {
		ctc_writel(dev, dev->sda_hold_time, CTC_IC_SDA_HOLD);
	}

	/* Configure Tx/Rx FIFO threshold levels */
	comp_param1 = ctc_readl(dev, CTC_IC_COMP_PARAM_1);
	dev->tx_fifo_depth = ((comp_param1 >> 16) & 0xff) + 1;
	dev->rx_fifo_depth = ((comp_param1 >> 8) & 0xff) + 1;

	ctc_writel(dev, dev->tx_fifo_depth / 2, CTC_IC_TX_TL);
	ctc_writel(dev, 0, CTC_IC_RX_TL);

	/* Configure the i2c master */
	ctc_writel(dev, dev->master_cfg, CTC_IC_CON);

	return 0;
}

int i2c_ctc_recover_bus(struct i2c_adapter *adap)
{
	struct ctc_i2c_dev *dev = i2c_get_adapdata(adap);
	u32 val = 0;

	dev_info(dev->dev, "Trying i2c bus recovery\n");

	if (dev->i2c_num == 0)
		val = 0x1;
	if (dev->i2c_num == 1)
		val = 0x2;

	regmap_write(dev->regmap_base,
		     offsetof(struct SysCtl_regs, SysI2CResetCtl), val);
	val = 0x0;
	regmap_write(dev->regmap_base,
		     offsetof(struct SysCtl_regs, SysI2CResetCtl), val);

	if (dev->soc_ver == CTC_REV_TM_1_1) {
		ctc_writel(dev, 0x1, CTC_IC_BUS_CLEAR_EN);
	}

	i2c_ctc_init(dev);

	return 0;
}

static int i2c_ctc_wait_bus_not_busy(struct ctc_i2c_dev *dev)
{
	int timeout = 20;

	while (ctc_readl(dev, CTC_IC_STATUS) & CTC_IC_STATUS_ACTIVITY) {
		if (timeout <= 0) {
			dev_warn(dev->dev, "timeout waiting for bus ready\n");
			i2c_recover_bus(&dev->adapter);

			if (ctc_readl(dev, CTC_IC_STATUS) &
			    CTC_IC_STATUS_ACTIVITY) {
				dev_warn(dev->dev,
					 "timeout waiting for bus ready again\n");
				return -ETIMEDOUT;
			}
			return 0;
		}
		timeout--;
		usleep_range(1000, 1100);
	}
	return 0;
}

void i2c_ctc_disable(struct ctc_i2c_dev *dev)
{
	/* Disable controller */
	__i2c_ctc_enable(dev, false);

	/* Disable all interupts */
	ctc_writel(dev, 0, CTC_IC_INTR_MASK);
	ctc_readl(dev, CTC_IC_CLR_INTR);
}

void i2c_ctc_disable_intr(struct ctc_i2c_dev *dev)
{
	ctc_writel(dev, 0, CTC_IC_INTR_MASK);
}

static void i2c_ctc_xfer_init(struct ctc_i2c_dev *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	u32 ic_con, ic_tar = 0;

	/* Disable the adapter */
	__i2c_ctc_enable(dev, false);

	/* if the slave address is ten bit address, enable 10BITADDR */
	ic_con = ctc_readl(dev, CTC_IC_CON);
	if (msgs[dev->msg_write_idx].flags & I2C_M_TEN) {
		ic_con |= CTC_IC_CON_10BITADDR_MASTER;
		ic_tar = CTC_IC_TAR_10BITADDR_MASTER;
	} else {
		ic_con &= ~CTC_IC_CON_10BITADDR_MASTER;
	}

	ctc_writel(dev, ic_con, CTC_IC_CON);

	/* Set the slave (target) address */
	ctc_writel(dev, msgs[dev->msg_write_idx].addr | ic_tar, CTC_IC_TAR);

	/* Enforce disabled interrupts (due to HW issues) */
	i2c_ctc_disable_intr(dev);

	/* Enable the adapter */
	__i2c_ctc_enable(dev, true);

	/* Clear and enable interrupts */
	ctc_readl(dev, CTC_IC_CLR_INTR);
	ctc_writel(dev, CTC_IC_INTR_MASTER_MASK, CTC_IC_INTR_MASK);
}

static int i2c_ctc_handle_tx_abort(struct ctc_i2c_dev *dev)
{
	unsigned long abort_source = dev->abort_source;
	int i;

	if (abort_source & CTC_IC_TX_ABRT_NOACK) {
		for_each_set_bit(i, &abort_source, ARRAY_SIZE(abort_sources))
		    dev_dbg(dev->dev, "%s: %s\n", __func__, abort_sources[i]);
		return -EREMOTEIO;
	}

	for_each_set_bit(i, &abort_source, ARRAY_SIZE(abort_sources))
	    dev_err(dev->dev, "%s: %s\n", __func__, abort_sources[i]);

	if (abort_source & CTC_IC_TX_ARB_LOST)
		return -EAGAIN;
	else if (abort_source & CTC_IC_TX_ABRT_GCALL_READ)
		return -EINVAL;
	else
		return -EIO;
}

static int i2c_ctc_interrupt_transfer(struct ctc_i2c_dev *dev)
{
	int ret;

	mutex_lock(&dev->lock);

	reinit_completion(&dev->cmd_complete);

	dev->cmd_err = 0;
	dev->msg_write_idx = 0;
	dev->msg_read_idx = 0;
	dev->msg_err = 0;
	dev->status = STATUS_IDLE;
	dev->abort_source = 0;
	dev->rx_outstanding = 0;

	ret = i2c_ctc_wait_bus_not_busy(dev);
	if (ret < 0)
		goto done;

	/* start the transfers */
	i2c_ctc_xfer_init(dev);

	/* wait for tx to complete */
	if (!wait_for_completion_timeout(&dev->cmd_complete, HZ)) {
		dev_err(dev->dev, "controller timed out\n");
		i2c_recover_bus(&dev->adapter);
		ret = -ETIMEDOUT;
		goto done;
	}

	__i2c_ctc_enable(dev, false);

	if (dev->msg_err) {
		ret = dev->msg_err;
		goto done;
	}

	/* no error */
	if (likely(!dev->cmd_err)) {
		ret = dev->msgs_num;
		goto done;
	}

	/* We have an error */
	if (dev->cmd_err == CTC_IC_ERR_TX_ABRT) {
		ret = i2c_ctc_handle_tx_abort(dev);
		goto done;
	}
	ret = -EIO;

done:
	mutex_unlock(&dev->lock);
	return ret;
}

static void ctc_i2c_flush_rxfifo(struct ctc_i2c_dev *dev)
{
	while (ctc_readl(dev, CTC_IC_STATUS) & CTC_IC_STATUS_RFNE)
		ctc_readl(dev, CTC_IC_DATA_CMD);
}

static int ctc_i2c_xfer_finish(struct ctc_i2c_dev *dev)
{
	int ret;
	ulong start_stop_det = jiffies;

	while (1) {
		if ((ctc_readl(dev, CTC_IC_RAW_INTR_STAT) &
		     CTC_IC_INTR_STOP_DET)) {
			ctc_readl(dev, CTC_IC_CLR_STOP_DET);
			break;
		} else if (time_after(jiffies, start_stop_det + I2C_STOPDET_TO)) {
			break;
		}
	}

	ret = i2c_ctc_wait_bus_not_busy(dev);
	if (ret < 0)
		return -1;

	ctc_i2c_flush_rxfifo(dev);

	return 0;
}

static int __ctc_i2c_read(struct ctc_i2c_dev *dev, __u16 chip_addr, u8 * offset,
			  __u16 olen, u8 * data, __u16 dlen)
{
	unsigned int active = 0;
	unsigned int flag = 0;
	int ret;
	unsigned long start_time_rx;

	ret = i2c_ctc_wait_bus_not_busy(dev);
	if (ret < 0)
		return -1;

	/* Disable the adapter */
	__i2c_ctc_enable(dev, false);

	/* Set the slave (target) address */
	ctc_writel(dev, chip_addr, CTC_IC_TAR);

	/* Enable the adapter */
	__i2c_ctc_enable(dev, true);

	if (olen > 0) {
		ctc_writel(dev, *offset | CTC_RESTART, CTC_IC_DATA_CMD);
		olen--;
		while (olen) {
			offset++;
			olen--;
			ctc_writel(dev, *offset, CTC_IC_DATA_CMD);
		}
	}

	start_time_rx = jiffies;
	while (dlen) {
		if (!active) {
			if (flag == 0) {
				if (dlen == 1) {
					ctc_writel(dev,
						   CTC_CMD_READ | CTC_STOP |
						   CTC_RESTART,
						   CTC_IC_DATA_CMD);
				} else {
					ctc_writel(dev,
						   CTC_CMD_READ | CTC_RESTART,
						   CTC_IC_DATA_CMD);
				}
				flag = 1;
			} else if (dlen != 1) {
				ctc_writel(dev, CTC_CMD_READ, CTC_IC_DATA_CMD);
			} else {
				ctc_writel(dev, CTC_CMD_READ | CTC_STOP,
					   CTC_IC_DATA_CMD);
			}
			active = 1;
		}
		if (ctc_readl(dev, CTC_IC_STATUS) & CTC_IC_STATUS_RFNE) {
			*data++ =
			    (unsigned char)ctc_readl(dev, CTC_IC_DATA_CMD);
			dlen--;
			start_time_rx = jiffies;
			active = 0;
		} else if (time_after(jiffies, start_time_rx + I2C_BYTE_TO)) {
			return -ETIMEDOUT;
		}
	}

	return ctc_i2c_xfer_finish(dev);
}

static int __ctc_i2c_write(struct ctc_i2c_dev *dev, __u16 chip_addr,
			   u8 * offset, __u16 olen, u8 * data, __u16 dlen)
{
	int ret;
	unsigned long start_time_tx;
	int nb = dlen;

	ret = i2c_ctc_wait_bus_not_busy(dev);
	if (ret < 0)
		return -1;

	/* Disable the adapter */
	__i2c_ctc_enable(dev, false);

	/* Set the slave (target) address */
	ctc_writel(dev, chip_addr, CTC_IC_TAR);

	/* Enable the adapter */
	__i2c_ctc_enable(dev, true);

	start_time_tx = jiffies;
	while (dlen) {
		if (ctc_readl(dev, CTC_IC_STATUS) & CTC_IC_STATUS_TFNF) {
			if (--dlen == 0) {
				ctc_writel(dev, *data | CTC_STOP,
					   CTC_IC_DATA_CMD);
			} else {
				ctc_writel(dev, *data, CTC_IC_DATA_CMD);
			}
			data++;
			start_time_tx = jiffies;
		} else
		    if (time_after(jiffies, start_time_tx + (nb * I2C_BYTE_TO)))
		{
			dev_err(dev->dev, "Timed out. i2c write Failed\n");
			return -ETIMEDOUT;
		}
	}
	return ctc_i2c_xfer_finish(dev);
}

static int i2c_ctc_polling_transfer(struct ctc_i2c_dev *dev)
{
	struct i2c_msg *dmsg, *omsg, dummy;
	int ret;

	memset(&dummy, 0, sizeof(struct i2c_msg));

	/* We expect either two messages (one with an offset and one with the
	 * actucal data) or one message (just data) */
	if (dev->msgs_num > 2 || dev->msgs_num == 0) {
		dev_err(dev->dev, "%s: Only one or two messages are supported.",
			__func__);
		return -1;
	}

	omsg = dev->msgs_num == 1 ? &dummy : dev->msgs;
	dmsg = dev->msgs_num == 1 ? dev->msgs : (dev->msgs + 1);

	if (dmsg->flags & I2C_M_RD) {
		ret = __ctc_i2c_read(dev, dmsg->addr, omsg->buf, omsg->len,
				     dmsg->buf, dmsg->len);
	} else
		ret = __ctc_i2c_write(dev, dmsg->addr, omsg->buf, omsg->len,
				      dmsg->buf, dmsg->len);
	if (!ret)
		return dev->msgs_num;
	else
		return -EIO;
}

static int i2c_ctc_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[],
			int num)
{
	struct ctc_i2c_dev *dev = i2c_get_adapdata(adap);
	int ret;

	dev_dbg(dev->dev, "%s: msgs: %d\n", __func__, num);
	dev->msgs = msgs;
	dev->msgs_num = num;

	if (dev->xfer_type == CTC_IC_INTERRUPT_TRANSFER)
		ret = i2c_ctc_interrupt_transfer(dev);
	else
		ret = i2c_ctc_polling_transfer(dev);

	return ret;
}

static void i2c_ctc_xfer_msg(struct ctc_i2c_dev *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	u32 intr_mask;
	int tx_limit, rx_limit;
	u32 addr = msgs[dev->msg_write_idx].addr;
	u32 buf_len = dev->tx_buf_len;
	u8 *buf = dev->tx_buf;
	bool need_restart = false;

	intr_mask = CTC_IC_INTR_MASTER_MASK;

	/* msg_write_idx */
	for (; dev->msg_write_idx < dev->msgs_num; dev->msg_write_idx++) {

		if (msgs[dev->msg_write_idx].addr != addr) {
			dev_err(dev->dev, "%s: invalid target address\n",
				__func__);
			dev->msg_err = -EINVAL;
			break;
		}

		if (msgs[dev->msg_write_idx].len == 0) {
			dev_err(dev->dev, "%s: invalid message length\n",
				__func__);
			dev->msg_err = -EINVAL;
			break;
		}

		if (!(dev->status & STATUS_WRITE_IN_PROGRESS)) {
			/* new i2c_msg */
			buf = msgs[dev->msg_write_idx].buf;
			buf_len = msgs[dev->msg_write_idx].len;

			if ((dev->master_cfg & CTC_IC_CON_RESTART_EN)
			    && (dev->msg_write_idx > 0))
				need_restart = true;
		}

		tx_limit = dev->tx_fifo_depth - ctc_readl(dev, CTC_IC_TXFLR);
		rx_limit = dev->rx_fifo_depth - ctc_readl(dev, CTC_IC_RXFLR);

		while (buf_len > 0 && tx_limit > 0 && rx_limit > 0) {
			u32 cmd = 0;

			/* set the stop bit */
			if (dev->msg_write_idx == dev->msgs_num - 1
			    && buf_len == 1)
				cmd |= BIT(9);

			/* set the restart bit */
			if (need_restart) {
				cmd |= BIT(10);
				need_restart = false;
			}

			if (msgs[dev->msg_write_idx].flags & I2C_M_RD) {
				if (rx_limit - dev->rx_outstanding <= 0)
					break;

				ctc_writel(dev, cmd | CTC_CMD_READ, CTC_IC_DATA_CMD);	/* 1 = Read */
				rx_limit--;
				dev->rx_outstanding++;
			} else
				ctc_writel(dev, cmd | *buf++, CTC_IC_DATA_CMD);	/* 0 = Write */
			tx_limit--;
			buf_len--;
		}

		dev->tx_buf = buf;
		dev->tx_buf_len = buf_len;

		if (buf_len > 0) {
			/* more bytes to be written */
			dev->status |= STATUS_WRITE_IN_PROGRESS;
			break;
		} else
			dev->status &= ~STATUS_WRITE_IN_PROGRESS;
	}

	if (dev->msg_write_idx == dev->msgs_num)
		intr_mask &= ~CTC_IC_INTR_TX_EMPTY;

	if (dev->msg_err)
		intr_mask = 0;

	ctc_writel(dev, intr_mask, CTC_IC_INTR_MASK);
}

static void i2c_ctc_read(struct ctc_i2c_dev *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	int rx_valid;
	u32 len;
	u8 *buf;

	/* msg_read_idx */
	for (; dev->msg_read_idx < dev->msgs_num; dev->msg_read_idx++) {

		if (!(msgs[dev->msg_read_idx].flags & I2C_M_RD))
			continue;

		if (!(dev->status & STATUS_READ_IN_PROGRESS)) {
			len = msgs[dev->msg_read_idx].len;
			buf = msgs[dev->msg_read_idx].buf;
		} else {
			len = dev->rx_buf_len;
			buf = dev->rx_buf;
		}

		rx_valid = ctc_readl(dev, CTC_IC_RXFLR);

		for (; len > 0 && rx_valid > 0; len--, rx_valid--) {
			*buf++ = ctc_readl(dev, CTC_IC_DATA_CMD);
			dev->rx_outstanding--;
		}

		if (len > 0) {
			dev->status |= STATUS_READ_IN_PROGRESS;
			dev->rx_buf_len = len;
			dev->rx_buf = buf;
			return;
		} else
			dev->status &= ~STATUS_READ_IN_PROGRESS;
	}
}

static u32 i2c_ctc_read_clear_intrbits(struct ctc_i2c_dev *dev)
{
	u32 stat;

	stat = ctc_readl(dev, CTC_IC_INTR_STAT);
	if (stat & CTC_IC_INTR_RX_UNDER)
		ctc_readl(dev, CTC_IC_CLR_RX_UNDER);
	if (stat & CTC_IC_INTR_RX_OVER)
		ctc_readl(dev, CTC_IC_CLR_RX_OVER);
	if (stat & CTC_IC_INTR_TX_OVER)
		ctc_readl(dev, CTC_IC_CLR_TX_OVER);
	if (stat & CTC_IC_INTR_RD_REQ)
		ctc_readl(dev, CTC_IC_CLR_RD_REQ);
	if (stat & CTC_IC_INTR_TX_ABRT) {
		dev->abort_source = ctc_readl(dev, CTC_IC_TX_ABRT_SOURCE);
		ctc_readl(dev, CTC_IC_CLR_TX_ABRT);
	}
	if (stat & CTC_IC_INTR_RX_DONE)
		ctc_readl(dev, CTC_IC_CLR_RX_DONE);
	if (stat & CTC_IC_INTR_ACTIVITY)
		ctc_readl(dev, CTC_IC_CLR_ACTIVITY);
	if (stat & CTC_IC_INTR_STOP_DET)
		ctc_readl(dev, CTC_IC_CLR_STOP_DET);
	if (stat & CTC_IC_INTR_START_DET)
		ctc_readl(dev, CTC_IC_CLR_START_DET);
	if (stat & CTC_IC_INTR_GEN_CALL)
		ctc_readl(dev, CTC_IC_CLR_GEN_CALL);

	return stat;
}

static irqreturn_t i2c_ctc_isr(int this_irq, void *dev_id)
{
	struct ctc_i2c_dev *dev = dev_id;
	u32 stat, enabled;

	enabled = ctc_readl(dev, CTC_IC_ENABLE);
	stat = ctc_readl(dev, CTC_IC_RAW_INTR_STAT);

	dev_dbg(dev->dev, "%s: enabled=%#x stat=%#x\n", __func__, enabled,
		stat);
	if (!enabled || !(stat & ~CTC_IC_INTR_ACTIVITY))
		return IRQ_NONE;

	stat = i2c_ctc_read_clear_intrbits(dev);

	if (stat & CTC_IC_INTR_TX_ABRT) {
		dev->cmd_err |= CTC_IC_ERR_TX_ABRT;
		dev->status = STATUS_IDLE;

		ctc_writel(dev, 0, CTC_IC_INTR_MASK);
		goto tx_aborted;
	}

	if (stat & CTC_IC_INTR_RX_FULL)
		i2c_ctc_read(dev);

	if (stat & CTC_IC_INTR_TX_EMPTY)
		i2c_ctc_xfer_msg(dev);

tx_aborted:
	if ((stat & (CTC_IC_INTR_TX_ABRT | CTC_IC_INTR_STOP_DET))
	    || dev->msg_err)
		complete(&dev->cmd_complete);
	return IRQ_HANDLED;
}

static u32 i2c_ctc_func(struct i2c_adapter *adap)
{
	struct ctc_i2c_dev *dev = i2c_get_adapdata(adap);
	return dev->functionality;
}

static struct i2c_algorithm i2c_ctc_algo = {
	.master_xfer = i2c_ctc_xfer,
	.functionality = i2c_ctc_func,
};

static struct i2c_bus_recovery_info i2c_ctc_recovery_info = {
	.recover_bus = i2c_ctc_recover_bus,
};

int i2c_ctc_probe(struct ctc_i2c_dev *dev)
{
	struct i2c_adapter *adap = &dev->adapter;
	int ret;

	init_completion(&dev->cmd_complete);
	mutex_init(&dev->lock);

	i2c_ctc_init(dev);

	snprintf(adap->name, sizeof(adap->name),
		 "Centec TsingMa SoC's I2C adapter");
	adap->algo = &i2c_ctc_algo;
	adap->bus_recovery_info = &i2c_ctc_recovery_info;
	adap->dev.parent = dev->dev;
	i2c_set_adapdata(adap, dev);

	i2c_ctc_disable_intr(dev);
	if (dev->xfer_type == CTC_IC_INTERRUPT_TRANSFER) {
		ret = devm_request_irq(dev->dev, dev->irq, i2c_ctc_isr,
				       IRQF_SHARED | IRQF_COND_SUSPEND,
				       dev_name(dev->dev), dev);
		if (ret) {
			dev_err(dev->dev, "failure requesting irq %i: %d\n",
				dev->irq, ret);
			return ret;
		}
	}
	ret = i2c_add_numbered_adapter(adap);
	if (ret)
		dev_err(dev->dev, "failure adding adapter: %d\n", ret);

	return ret;
}

static int ctc_i2c_plat_probe(struct platform_device *pdev)
{
	struct ctc_i2c_dev *dev;
	struct i2c_adapter *adap;
	struct resource *mem;
	int irq, ret;
	u32 clk_freq, ht;
	u32 val, i2c_num;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	dev = devm_kzalloc(&pdev->dev, sizeof(struct ctc_i2c_dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dev->base = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(dev->base))
		return PTR_ERR(dev->base);

	dev->regmap_base =
	    syscon_regmap_lookup_by_phandle(pdev->dev.of_node, "ctc,sysctrl");
	if (IS_ERR(dev->regmap_base))
		return PTR_ERR(dev->regmap_base);

	regmap_read(dev->regmap_base,
		    offsetof(struct SysCtl_regs, SysCtlSysRev), &val);
	dev->soc_ver = ((val == 0x1) ? CTC_REV_TM_1_1 : CTC_REV_TM_1_0);

	dev->dev = &pdev->dev;
	dev->irq = irq;
	platform_set_drvdata(pdev, dev);

	of_property_read_u32(pdev->dev.of_node, "clock-frequency", &clk_freq);

	dev->clk_freq = clk_freq;

	dev->functionality = I2C_FUNC_10BIT_ADDR | CTC_IC_DEFAULT_FUNCTIONALITY;
	dev->master_cfg = CTC_IC_CON_MASTER | CTC_IC_CON_SLAVE_DISABLE |
		CTC_IC_CON_RESTART_EN;

	if (dev->clk_freq <= 100000)
		dev->master_cfg |= CTC_IC_CON_SPEED_STD;
	else if (dev->clk_freq <= 400000)
		dev->master_cfg |= CTC_IC_CON_SPEED_FAST;
	else {
		dev_err(&pdev->dev, "Unsupported this frequency\n");
		return -EINVAL;
	}

	dev->clk = devm_clk_get(&pdev->dev, NULL);
	clk_prepare_enable(dev->clk);

	if (!of_property_read_u32
	    (pdev->dev.of_node, "i2c-sda-hold-time-ns", &ht)) {
		dev->sda_hold_time = ht / IC_ICK_NS(clk_get_rate(dev->clk));
	}

	of_property_read_u32(pdev->dev.of_node, "i2c-num", &i2c_num);
	dev->i2c_num = i2c_num;

	dev->adapter.nr = pdev->id;
	adap = &dev->adapter;
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_DEPRECATED;
	adap->dev.of_node = pdev->dev.of_node;

	ret = i2c_ctc_probe(dev);

	return ret;
}

static int ctc_i2c_plat_remove(struct platform_device *pdev)
{
	struct ctc_i2c_dev *dev = platform_get_drvdata(pdev);

	i2c_del_adapter(&dev->adapter);

	i2c_ctc_disable(dev);

	return 0;
}

static const struct of_device_id ctc_i2c_of_match[] = {
	{.compatible = "ctc,i2c",},
	{},
};

MODULE_DEVICE_TABLE(of, ctc_i2c_of_match);

static struct platform_driver ctc_i2c_driver = {
	.probe = ctc_i2c_plat_probe,
	.remove = ctc_i2c_plat_remove,
	.driver = {
		   .name = "i2c_centec",
		   .of_match_table = of_match_ptr(ctc_i2c_of_match),
		   },
};

static int __init ctc_i2c_init_driver(void)
{
	return platform_driver_register(&ctc_i2c_driver);
}

subsys_initcall(ctc_i2c_init_driver);

static void __exit ctc_i2c_exit_driver(void)
{
	platform_driver_unregister(&ctc_i2c_driver);
}

module_exit(ctc_i2c_exit_driver);

MODULE_AUTHOR("Centec, Inc.");
MODULE_LICENSE("GPL");

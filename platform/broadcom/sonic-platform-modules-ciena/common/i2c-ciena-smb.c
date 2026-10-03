/*
 * This file is part of Cienaâs i2c adapter
 *
 * Copyright (C) 2020 Ciena Corporation
 * Author: Marc St-Amand <mstamand@ciena.com>
 *
 * i2c adapter is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 2 of the License.
 *
 * i2c adapter is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/export.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/ratelimit.h>
#include <linux/regmap.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#include "generic_cic_dumper.h"

#include "kcompat.h"
#include "i2c-ciena-err.h"
#include "i2c-ciena-smb.h"
#define CREATE_TRACE_POINTS
#include "i2c-ciena-smb-tp.h"

/* extra noisy dev_dbg to track what is happening in the chip */
static bool ciena_smb_noisy = false;
module_param(ciena_smb_noisy, bool, 0644);
#define DEV_DBG_NOISY if (ciena_smb_noisy) dev_dbg

/* Watch completion interrupts on i2c-over-i2c technology. */
static bool force_watch_completion = false;
module_param(force_watch_completion, bool, 0644);

#define CIENA_SMB_POLL_DELAY_US 70000

struct ciena_smb_adap {
	struct device               *dev;
	struct i2c_adapter           adapter;
	struct i2c_adapter_quirks    quirks;
	struct ciena_i2c_err_state **err_state;
	struct ciena_i2c_err_state  *local_err_state;
	struct ciena_i2c_err_state   adap_err_state;
	unsigned                     pre_start_gap;
	ktime_t                      pre_start_tick;
	unsigned                     timeout_usecs;
	int                          irq;
	unsigned                     missed_irq_count;
	unsigned                     missed_irq_track;
	const unsigned              *offsets;
	struct regmap               *parent_regmap;
	struct completion            cmd_complete;
	int                          data_addr_cache;
	struct delayed_work          children_work;
	unsigned                     deferred_children_delay_ms;
	bool                         da_16_bit;
	bool                         reset_needed;
};

#define I2C_DONE_BIT (1 << 0)
#define START_BIT    (1 << 0)
#define FINISH_BIT   (1 << 1)
#define ABORT_BIT    (1 << 2)
#define RD_WRn_BIT   (1 << 3)
#define DADDR_16_BIT (1 << 4)
#define NO_DADDR_BIT (1 << 5)
#define DEV_NACK_BIT (1 << 6)
#define RST_MSTR_BIT (1 << 7)

#define CTRL_REG_MASK (DADDR_16_BIT | RD_WRn_BIT | START_BIT)

/*----------------------------------------------------------------------------*/
static const char* ciena_smb_reg_name(enum ciena_smb_register reg)
{
	switch (reg) {
		case CIENA_SMB_REG_CTRL:     return "ctrl";
		case CIENA_SMB_REG_4X:       return "4x";
		case CIENA_SMB_REG_DEVADDR:  return "devaddr";
		case CIENA_SMB_REG_DATAADDR: return "dataaddr";
		case CIENA_SMB_REG_DATA_VLD: return "data_vld";
		case CIENA_SMB_REG_DEBUG:    return "debug";
		case CIENA_SMB_REG_DATA_WR:  return "data_wr";
		case CIENA_SMB_REG_DATA_RD:  return "data_rd";
		default:           return "unknown";
	}
}

/*----------------------------------------------------------------------------*/
static inline int ciena_smb_id(struct ciena_smb_adap *adap)
{
	if (adap->err_state && *adap->err_state)
		return (*adap->err_state)->adapter_id;

	return i2c_adapter_id(&adap->adapter);
}

/*----------------------------------------------------------------------------*/
static unsigned __ciena_smb_reg_rd(struct ciena_smb_adap *adap, unsigned offset)
{
	unsigned val = ~0U;
	int rc;

	rc = regmap_read(adap->parent_regmap, offset, &val);

	DEV_DBG_NOISY(adap->dev, "%s offset=0x%x val=0x%x rc=%d\n",
		      __func__, offset, val, rc);

	return val;
}

#define ciena_smb_reg_rd(_priv, _reg)					\
	__ciena_smb_reg_rd(_priv, _priv->offsets[_reg])

/*----------------------------------------------------------------------------*/
static void __ciena_smb_reg_wr_mask(struct ciena_smb_adap *adap,
				    unsigned offset, unsigned mask,
				    unsigned data)
{
	int rc;

	rc = regmap_write_bits(adap->parent_regmap, offset, mask, data);

	DEV_DBG_NOISY(adap->dev, "%s offset=0x%x mask= 0x%x data=0x%x rc=%d\n",
		      __func__, offset, mask, data, rc);
}

#define ciena_smb_reg_wr_mask(_priv, _reg, mask, data)			\
	__ciena_smb_reg_wr_mask(_priv, _priv->offsets[_reg], mask, data)

/*----------------------------------------------------------------------------*/
static void __ciena_smb_reg_wr(struct ciena_smb_adap *adap, unsigned offset,
			       unsigned data)
{
	int rc;

	rc = regmap_write(adap->parent_regmap, offset, data);

	DEV_DBG_NOISY(adap->dev, "%s offset=0x%x data=0x%x rc=%d\n",
		      __func__, offset, data, rc);
}

#define ciena_smb_reg_wr(_priv, _reg, data)			\
	__ciena_smb_reg_wr(_priv, _priv->offsets[_reg], data)

/*----------------------------------------------------------------------------*/
static void ciena_smb_reset_master(struct ciena_smb_adap *adap)
{
	unsigned wait_msec = 10;
	unsigned reg;

	ciena_smb_reg_wr_mask(adap, CIENA_SMB_REG_CTRL,
			      RST_MSTR_BIT, RST_MSTR_BIT);

	while (wait_msec--) {
		msleep(1);
		reg = ciena_smb_reg_rd(adap, CIENA_SMB_REG_CTRL);
		if (0 == (reg & RST_MSTR_BIT)) break;
	}

	if (reg & RST_MSTR_BIT)
		dev_warn(adap->dev, "master reset bit did not self-clear\n");
}

/*----------------------------------------------------------------------------*/
static inline int ciena_smb_status_complete(struct ciena_smb_adap *adap)
{
	unsigned reg      = ciena_smb_reg_rd(adap, CIENA_SMB_REG_CTRL);
	bool     complete = ((reg & (FINISH_BIT | ABORT_BIT)) != 0);
	int      rc       = -ETIMEDOUT;

	DEV_DBG_NOISY(adap->dev,
		      "read %s 0x%x -- %s%s%s%s%s%s\n",
		      ciena_smb_reg_name(CIENA_SMB_REG_CTRL), reg,
		      (reg & DEV_NACK_BIT ) ? "DEV_NACK " : "",
		      (reg & DADDR_16_BIT ) ? "DADDR_16 " : "",
		      (reg & RD_WRn_BIT ) ? "RD_WRn " : "",
		      (reg & ABORT_BIT ) ? "ABORT " : "",
		      (reg & FINISH_BIT ) ? "FINISH " : "",
		      (reg & START_BIT ) ? "START " : "");

	if (complete) {
		bool dev_nack = ((reg & DEV_NACK_BIT) != 0);
		bool aborted  = ((reg & ABORT_BIT) != 0);
		rc = dev_nack ? -ENODEV : (aborted ? -EIO : 0);
	}

	return rc;
}

/*----------------------------------------------------------------------------*/
static irqreturn_t ciena_smb_i2c_irq_handler(int irq, void *data)
{
	struct ciena_smb_adap *adap = data;
	complete(&adap->cmd_complete);
	return IRQ_HANDLED;
}

/*----------------------------------------------------------------------------*/
#define CIENA_I2C_MAX_MISS 10
static int ciena_smb_wait(struct ciena_smb_adap *adap)
{
	unsigned long      poll_timeout = adap->timeout_usecs;
	unsigned long      jiffs        = 1 + usecs_to_jiffies(poll_timeout);
	const char        *err_str      = "";
	ktime_t            tstart       = ktime_get_raw();
	ktime_t            interval;
	bool               irq_tmo      = false;
	int                rc           = -ETIMEDOUT;

	do {
		if (adap->offsets[CIENA_SMB_REG_I2C_DONE]) {
			struct completion *cmd;
			unsigned           reg;
			bool               done;

			poll_timeout = 0;
			cmd          = &adap->cmd_complete;

			rc = wait_for_completion_timeout(cmd, jiffs);
			dev_dbg(adap->dev, "completion rc=%d\n", rc);

			reg = ciena_smb_reg_rd(adap, CIENA_SMB_REG_I2C_DONE);
			done = !!(I2C_DONE_BIT & reg);

			if (0 == rc) irq_tmo = done;
			else if (!done){
				/* insane controller: done interrupt
				 * fired, but done bit is not set */
				rc = -ECOMM;
				err_str = "incomplete ";
				goto wait_err;
			}

			/* even if the completion timed out, fall out
			 * and check the status */
		}
		else usleep_range(50, 100);

		rc = ciena_smb_status_complete(adap);

		if (irq_tmo && (-ETIMEDOUT != rc)) {
			adap->missed_irq_count += adap->missed_irq_track;
			/* dump the cic state on the first interrupt miss */
			if (1 == adap->missed_irq_count) {
				CIENA_I2C_ERR_REPORT(adap->dev, -ETIMEDOUT,
						     *adap->err_state,
						     "missed completion "
						     "interrupt\n");
				ciena_cic_dump(adap->irq);
			}
			/* ten strikes: we are out, something is broken */
			else if (CIENA_I2C_MAX_MISS == adap->missed_irq_count)
				ciena_cic_panic("%s: missed %u completion "
						"interrupts in a row\n",
						dev_name(adap->dev),
						adap->missed_irq_count);
		}
		else adap->missed_irq_count = 0;

		switch (rc) {
		case 0:
		case -ENODEV:
			return rc;
		case -ETIMEDOUT:
			err_str = "timeout ";
			break;
		case -EIO:
			err_str = "abort ";
			fallthrough;
		default:
			goto wait_err;
		}

		interval = ktime_us_delta(ktime_get_raw(), tstart);
	}
	while (poll_timeout > interval);

wait_err:
	CIENA_I2C_ERR_REPORT(adap->dev, rc, *adap->err_state,
			     "%s%s=0x%x, %s=0x%x\n", err_str,
			     ciena_smb_reg_name(CIENA_SMB_REG_CTRL),
			     ciena_smb_reg_rd(adap, CIENA_SMB_REG_CTRL),
			     ciena_smb_reg_name(CIENA_SMB_REG_DEBUG),
			     ciena_smb_reg_rd(adap, CIENA_SMB_REG_DEBUG));

	return rc;
}

/*----------------------------------------------------------------------------*/
static void ciena_smb_write_data(struct ciena_smb_adap *adap, uint8_t *buf,
				 size_t len)
{
	unsigned stride = regmap_get_reg_stride(adap->parent_regmap);
	unsigned offset = adap->offsets[CIENA_SMB_REG_DATA_WR];

	DEV_DBG_NOISY(adap->dev, "data write len:%x buf:%*phN\n",
		      (unsigned) len, (int) len, buf);

	while (len) {
		size_t   wlen = (stride > len) ? len : stride;
		unsigned wr   = 0;

		switch (wlen) {
		default: wr |= buf[3] << 24; fallthrough;
		case 3:  wr |= buf[2] << 16; fallthrough;
		case 2:  wr |= buf[1] << 8;  fallthrough;
		case 1:  wr |= buf[0] << 0;
		}

		__ciena_smb_reg_wr(adap, offset, wr);

		buf    += wlen;
		len    -= wlen;
		offset += stride;
	}
}

/*----------------------------------------------------------------------------*/
static void ciena_smb_read_data(struct ciena_smb_adap *adap, uint8_t *buf,
				size_t len)
{
	unsigned stride = regmap_get_reg_stride(adap->parent_regmap);
	unsigned offset = adap->offsets[CIENA_SMB_REG_DATA_RD];
	size_t   olen   = len;

	while (len) {
		unsigned rd   = __ciena_smb_reg_rd(adap, offset);
		size_t   rlen = (stride > len) ? len : stride;

		switch (rlen) {
		default: buf[3] = (rd >> 24) & 0xff; fallthrough;
		case 3:  buf[2] = (rd >> 16) & 0xff; fallthrough;
		case 2:  buf[1] = (rd >>  8) & 0xff; fallthrough;
		case 1:  buf[0] = (rd >>  0) & 0xff;
		}

		buf    += rlen;
		len    -= rlen;
		offset += stride;
	}

	DEV_DBG_NOISY(adap->dev, "data read len:%x buf:%*phN\n",
		      (unsigned) olen, (int) olen, buf - olen);
}

/*----------------------------------------------------------------------------*/
static void ciena_smb_write_fake_addr(struct ciena_smb_adap *adap,
				      uint16_t addr)
{
	unsigned stride = regmap_get_reg_stride(adap->parent_regmap);
	unsigned offset = adap->offsets[CIENA_SMB_REG_DATAADDR];

	if (sizeof(addr) > stride) {
		__ciena_smb_reg_wr(adap, offset, addr & 0xff);
		__ciena_smb_reg_wr(adap, offset + stride, addr >> 8);
	}
	else __ciena_smb_reg_wr(adap, offset, addr);
}

/*----------------------------------------------------------------------------*/
static int ciena_smb_run_command(struct ciena_smb_adap *adap,
				 struct i2c_msg        *msg,
				 unsigned               command)
{
	unsigned ack_poll_cnt = 40;
	int      rc;

	while (true) {
		if (adap->offsets[CIENA_SMB_REG_I2C_DONE]) {
			ciena_smb_reg_wr(adap, CIENA_SMB_REG_I2C_DONE,
					 I2C_DONE_BIT);
			reinit_completion(&adap->cmd_complete);
		}

		ciena_smb_reg_wr_mask(adap, CIENA_SMB_REG_CTRL,
				      CTRL_REG_MASK,
				      command | START_BIT);

		rc = ciena_smb_wait(adap);

		switch (rc) {
		case -ENODEV:
			/* ENODEV means the slave device did not ACK
                         * its own address: keep polling */
			break;
		case -EIO:
		case -ETIMEDOUT:
			ciena_smb_reset_master(adap);
			fallthrough;
		default:
			return rc;
		}

		if (ack_poll_cnt--) {
			msleep(1);
			trace_ciena_i2c_smb_ack_poll(ciena_smb_id(adap),
						     msg->addr, command);
		}
		/* 40 strikes: you are out */
		else return -ECOMM;
	}
}

/*----------------------------------------------------------------------------*
 * Writes are always handled as if the "data address" was a single
 * byte: it should not matter whether the second byte is clocked out of
 * the FPGA as an address or as plain data.
 */
static int ciena_smb_do_write(struct ciena_smb_adap *adap, struct i2c_msg *msg,
			      unsigned next_flags)
{
	uint16_t fake_addr;
	unsigned addr_16b = 0;
	uint8_t *buf;
	size_t   len;
	int      rc;

	if (msg->len == 0) return -EIO;

	/* Default to one-byte 'data' address. */
	fake_addr = *msg->buf << 8;
	buf       = msg->buf + 1;
	len       = msg->len - 1;

	if (I2C_M_NOSTART & msg->flags) {
		if (0 > adap->data_addr_cache) {
			/* Someone is trying to access this i2c bus in a way
			 * that this controller cannot support. How sad. */
			dev_warn_ratelimited(adap->dev,
					     "invalid address for no-start "
					     "write dev:%x len:%x buf:%*phN\n",
					     msg->addr, (unsigned) msg->len,
					     (int) msg->len, msg->buf);
			return -EIO;
		}
		fake_addr = adap->data_addr_cache;
		addr_16b  = (adap->da_16_bit) ? DADDR_16_BIT: 0;
		buf       = msg->buf;
		len       = msg->len;
	}
	else if (msg->len <= 2) {
		/* If the next message is a read or a no-start write,
		 * and the cached address is valid, then skip this
		 * write. It will be morphed into the 'data address'
		 * of the next message. */
		if ((I2C_M_RD & next_flags) ||
		    (I2C_M_NOSTART & next_flags)) {
			/* Cache the "data address" in the format
			 * expected by the controller. */
			adap->da_16_bit = (2 == msg->len);
			adap->data_addr_cache = (*msg->buf) << 8;
			if (msg->len == 2)
				adap->data_addr_cache |= *(msg->buf + 1);
			return 0;
		}
	}

	ciena_smb_write_fake_addr(adap, fake_addr);
	ciena_smb_reg_wr(adap, CIENA_SMB_REG_DEVADDR, msg->addr);

	if (len) ciena_smb_write_data(adap, buf, len);
	else dev_dbg(adap->dev, "no data");

	ciena_smb_reg_wr(adap, CIENA_SMB_REG_DATA_VLD, len);

	rc = ciena_smb_run_command(adap, msg, addr_16b);

	adap->pre_start_tick = ktime_get_raw();

	/* The fake cached address is used up now. */
	adap->data_addr_cache = -1;

	trace_ciena_i2c_smb_write(ciena_smb_id(adap), msg->addr, fake_addr,
				  !!addr_16b + 1, len, buf, rc);

	return rc;
}

/*----------------------------------------------------------------------------*/
static int ciena_smb_do_read(struct ciena_smb_adap *adap, struct i2c_msg *msg)
{
	unsigned addr_16b = 0;
	bool     no_data_addr = false;
	int      rc;

	if (msg->len == 0) return -EIO;

	if (0 > adap->data_addr_cache) {
		if (adap->offsets[CIENA_SMB_REG_NO_DATAADDR]) {
			/* No fake address read: by invitation only. */
			ciena_smb_reg_wr_mask(adap, CIENA_SMB_REG_NO_DATAADDR,
					      NO_DADDR_BIT, NO_DADDR_BIT);
			no_data_addr = true;
		}
		else {
			/* Someone is trying to access this i2c bus in a way
			 * that this controller cannot support. Fail in a
			 * noisy way, but go easy on the errors just in case
			 * this ever happens in the field. */
			dev_warn_ratelimited(adap->dev, "invalid address for "
					     "read dev:%x len:%x buf:%*phN\n",
					     msg->addr, (unsigned) msg->len,
					     (int) msg->len, msg->buf);
			return -EIO;
		}
	}
	else {
		if (adap->da_16_bit) addr_16b = DADDR_16_BIT;
		ciena_smb_write_fake_addr(adap, adap->data_addr_cache);
	}
	ciena_smb_reg_wr(adap, CIENA_SMB_REG_DEVADDR, msg->addr);
	ciena_smb_reg_wr(adap, CIENA_SMB_REG_DATA_VLD, msg->len);

	rc = ciena_smb_run_command(adap, msg, RD_WRn_BIT | addr_16b);

	adap->pre_start_tick = ktime_get_raw();

	if (!rc) ciena_smb_read_data(adap, msg->buf, msg->len);

	trace_ciena_i2c_smb_read(ciena_smb_id(adap), msg->addr,
				 adap->data_addr_cache,
				 no_data_addr ? 0 : (adap->da_16_bit + 1),
				 msg->len, msg->buf, rc);


	if (no_data_addr)
		ciena_smb_reg_wr_mask(adap, CIENA_SMB_REG_NO_DATAADDR,
				      NO_DADDR_BIT, 0);
	else
		/* We are done with the cached fake address now. */
		adap->data_addr_cache = -1;

	return rc;
}

/*----------------------------------------------------------------------------*
 * Limp through something that vaguely looks like smbus exchanges,
 * with variable length write commands interpreted as data addresses.
 */
static int ciena_smb_master_xfer(struct i2c_adapter *i2c_adap,
                                 struct i2c_msg *msg,
                                 int num)
{
	struct ciena_smb_adap *adap = i2c_get_adapdata(i2c_adap);
	s64                    usecs_since_stop;
	s64                    gap;
	int                    remaining = num;
	int                    rc = 0;

	ciena_i2c_err_set_state(adap->err_state, &adap->adap_err_state);

	if ((-ETIMEDOUT == ciena_smb_status_complete(adap)) ||
	    adap->reset_needed) {
		ciena_smb_reset_master(adap);
		if (-ETIMEDOUT == ciena_smb_status_complete(adap)) {
			rc = -EBUSY;
			remaining = 0;
			CIENA_I2C_ERR_REPORT(adap->dev, rc, *adap->err_state,
					     "unready after reset\n");
		}
	}

	while (remaining) {
		ciena_i2c_err_set_devaddr(*adap->err_state, msg->addr);

		if (signal_pending_state(TASK_INTERRUPTIBLE, current)) {
			rc = -ERESTARTSYS;
			CIENA_I2C_ERR_REPORT(adap->dev, rc, *adap->err_state,
					     "interrupted\n");
			break;
		}

		usecs_since_stop = ktime_us_delta(ktime_get_raw(),
						  adap->pre_start_tick);
		gap = adap->pre_start_gap - usecs_since_stop;

		if (0 < gap) usleep_range(gap, adap->pre_start_gap);

		if (msg->flags & I2C_M_RD) {
			rc = ciena_smb_do_read(adap, msg);
		}
		else {
			unsigned next_flags = 0;

			if ((remaining > 1) && ((msg + 1)->addr == msg->addr))
				next_flags = (msg + 1)->flags;

			rc = ciena_smb_do_write(adap, msg, next_flags);
		}

		dev_dbg(adap->dev,
			"msg:%d %s dev:%x flags:%x len:%x buf:%*phN rc:%d\n",
			num - remaining, (msg->flags & I2C_M_RD) ? "rd" : "wr",
			msg->addr, msg->flags, (unsigned) msg->len,
			(int) msg->len, msg->buf, rc);

		if (rc) break;
		msg++;
		remaining--;
	}

	ciena_i2c_err_reset_rc(*adap->err_state, rc);
	ciena_i2c_err_clr_state(adap->err_state, &adap->adap_err_state);

	/* if the transfer failed, always reset the adapter before
	 * launching the next one */
	adap->reset_needed = !!rc;

	return rc ? rc : num;
}

/*----------------------------------------------------------------------------*/
static u32 ciena_smb_functionality(struct i2c_adapter *adap)
{
	/* This adapter can handle plain smbus transfers (all of
	 * I2C_FUNC_SMBUS_EMUL minus I2C_FUNC_SMBUS_QUICK). It can
	 * also do eeprom smbus-style transfers with 16-bit data
	 * addresses. With only a .master_xfer, the SMBus protocols
	 * are emulated by the kernel.
	 *
	 * Throwing a non-esstw16bda i2c transfer at this driver
	 * will fail in a horrible manner.
	 */
	return (I2C_FUNC_I2C |
		I2C_FUNC_NOSTART |
		I2C_FUNC_SMBUS_BYTE |
		I2C_FUNC_SMBUS_BYTE_DATA |
		I2C_FUNC_SMBUS_WORD_DATA |
		I2C_FUNC_SMBUS_PROC_CALL |
		I2C_FUNC_SMBUS_WRITE_BLOCK_DATA |
		I2C_FUNC_SMBUS_I2C_BLOCK |
		I2C_FUNC_SMBUS_PEC);
}

/*----------------------------------------------------------------------------*/
static const struct i2c_algorithm ciena_smb_algorithm = {
	.master_xfer   = ciena_smb_master_xfer,
	.functionality = ciena_smb_functionality,
};

/*----------------------------------------------------------------------------*/
static int ciena_smb_create_children(struct ciena_smb_adap       *adap,
				     const struct i2c_board_info *child,
				     int                          n_children)
{
	struct i2c_client *cl;

	dev_info(adap->dev, "n_children=%d\n", n_children);

	while (n_children--) {
		request_module("%s", child->type);
		cl = i2c_new_client_device(&adap->adapter, child);
		if (!cl) {
			dev_err(adap->dev, "cannot create client %s\n",
				child->type);
			return -EINVAL;
		}
		dev_dbg(adap->dev, "client %s @ %p\n", child->type, cl);
		child++;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
static void ciena_smb_child_work(struct work_struct *work)
{
	struct ciena_smb_adap       *adap;
	struct i2c_ciena_smb_config *conf;

	adap = container_of(to_delayed_work(work),
			    struct ciena_smb_adap, children_work);
	conf = (struct i2c_ciena_smb_config *) dev_get_platdata(adap->dev);

	(void) ciena_smb_create_children(adap, conf->deferred_children,
					 conf->num_deferred_children);
}

/*----------------------------------------------------------------------------*/
static int ciena_smb_create_all_children(struct ciena_smb_adap       *adap,
					 struct i2c_ciena_smb_config *config)
{
	int rc = 0;

	if (config->children)
		rc = ciena_smb_create_children(adap, config->children,
					       config->num_children);

	if (!rc && config->deferred_children) {
		INIT_DELAYED_WORK(&adap->children_work, ciena_smb_child_work);
		if (adap->deferred_children_delay_ms)
			schedule_delayed_work(&adap->children_work,
				msecs_to_jiffies(adap->deferred_children_delay_ms));
		else {
			/* schedule_work() fails if the work is already
			 * started, which means something is very wrong in
			 * this driver */
			BUG_ON(!schedule_delayed_work(&adap->children_work, 0));
		}
	}

	return rc;
}

/*----------------------------------------------------------------------------*/
static int ciena_smb_i2c_init_irq(struct platform_device *pdev,
				  struct ciena_smb_adap  *adap)
{
	struct device   *dev = &pdev->dev;
	struct resource *irq_res;
	int              irq;
	int              rc;

	if (!adap->offsets[CIENA_SMB_REG_I2C_DONE])
		return 0;

	irq_res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (irq_res) irq = irq_res->start;
	else if (dev->of_node &&
		 of_find_property(dev->of_node, "interrupts", NULL)) {
		irq = of_irq_get(dev->of_node, 0);
		if (0 >= irq) return irq ? irq : -EPROBE_DEFER;
	}
	else return 0;

	rc = devm_request_any_context_irq(dev, irq, ciena_smb_i2c_irq_handler,
					  0, dev_name(dev), adap);

	if (0 > rc) dev_err(dev, "cannot request irq %d (%d)\n", irq, rc);
	else {
		adap->irq = irq;
		dev_info(dev, "using IRQ #%d (%s)\n", irq,
			 (IRQC_IS_NESTED == rc) ? "nested" : "hard irq");
		rc = 0;
	}

	return rc;
}

/*----------------------------------------------------------------------------*/
static void *ciena_smb_of_pdata(struct platform_device       *pdev,
				struct i2c_ciena_smb_config  *of_config)
{
	struct device *dev   = &pdev->dev;
	int            index = 0;
	const __be32  *offp;
	unsigned      *offsets;

	if (!dev->of_node) return NULL;

	offsets = devm_kzalloc(dev, CIENA_SMB_REG_MAX * sizeof(*offsets),
			       GFP_KERNEL);
	if (!offsets) {
		dev_err(dev, "no memory for OF offsets\n");
		return NULL;
	}

	while (NULL != (offp = of_get_address(dev->of_node, index,
					      NULL, NULL))) {
		offsets[index++] = be32_to_cpu(*offp);
		if (CIENA_SMB_REG_MAX == index) break;
	}

	/* to work properly all offsets must be present (except the
	 * interrupt related ones) */
	if (CIENA_SMB_REG_I2C_DONE > index) {
		dev_err(dev, "missing OF regs (index = %d)\n", index);
		return NULL;
	}

	of_config->offsets = offsets;

	of_property_read_u32(dev->of_node, "pre-start-gap",
			     &of_config->pre_start_gap);

	return of_config;
}

/*----------------------------------------------------------------------------*/
static int ciena_smb_probe(struct platform_device *pdev)
{
	struct device               *dev    = &pdev->dev;
	void                        *pdata  = dev_get_platdata(dev);
	struct ciena_smb_adap       *adap;
	struct i2c_ciena_smb_config  of_config = {};
	struct i2c_ciena_smb_config *config;
	CIENA_BUS_TYPE_PTR          regmap_bus;
	struct device               *regmap_dev;
	unsigned                     regsize;
	int                          busn;
	int                          rc;

	if (!pdata) pdata = ciena_smb_of_pdata(pdev, &of_config);

	if (!pdata) {
		dev_err(dev, "missing platform data\n");
		return -EINVAL;
	}

	config = (struct i2c_ciena_smb_config *) pdata;
	busn   = config->bus_number;

	adap = devm_kzalloc(dev, sizeof(*adap), GFP_KERNEL);
	if (!adap) {
		dev_err(dev, "failed to alloc adapter private data\n");
		return -ENOMEM;
	}

	init_completion(&adap->cmd_complete);

	adap->dev             = dev;
	adap->adapter.nr      = pdev->id;
	adap->offsets         = config->offsets;
	adap->parent_regmap   = dev_get_regmap(dev->parent, NULL);
	adap->data_addr_cache = -1;
	adap->pre_start_gap   = config->pre_start_gap;
	adap->deferred_children_delay_ms = config->deferred_children_delay_ms;

	if (config->timeout_usecs)
		adap->timeout_usecs = config->timeout_usecs;
	else
		adap->timeout_usecs = CIENA_SMB_POLL_DELAY_US;

	if (!adap->parent_regmap || !adap->offsets) {
		dev_err(dev, "missing %s\n",
			adap->parent_regmap ? "offsets" : "parent_regmap");
		return -EINVAL;
	}

	/* Our i2c-over-i2c technology is inherently slow. Do not
	 * bother watching i2c-over-i2c interrupt completions. */
	regmap_dev = regmap_get_device(adap->parent_regmap);
	regmap_bus = regmap_dev ? regmap_dev->bus : NULL;
	adap->missed_irq_track = (&i2c_bus_type != regmap_bus);
	if (force_watch_completion && !adap->missed_irq_track) {
		adap->missed_irq_track = true;
		dev_info(dev, "watching i2c-over-i2c completion interrupts\n");
	}

	dev_set_drvdata(dev, adap);

	regsize = (adap->offsets[CIENA_SMB_REG_DATA_RD] -
		   adap->offsets[CIENA_SMB_REG_DATA_WR]);

	adap->quirks.max_write_len = regsize;

	/* If the user provided a CIENA_SMB_REG_DATA_RD_MAX offset,
	 * use it. Otherwise use the same regsize for read. */
	if (adap->offsets[CIENA_SMB_REG_DATA_RD_MAX])
		regsize = (adap->offsets[CIENA_SMB_REG_DATA_RD_MAX] -
			   adap->offsets[CIENA_SMB_REG_DATA_RD]);

	adap->quirks.max_read_len = regsize;

	/* Create the I2C adapter. */
	adap->adapter.dev.parent  = dev;
	adap->adapter.dev.of_node = of_node_get(dev->of_node);
	adap->adapter.owner       = THIS_MODULE;
	adap->adapter.algo        = &ciena_smb_algorithm;
	adap->adapter.timeout     = HZ;
	adap->adapter.nr          = busn ? busn : -1;
	adap->adapter.quirks      = &adap->quirks;

	i2c_set_adapdata(&adap->adapter, adap);

	if (config->adap_name)
		snprintf(adap->adapter.name, sizeof(adap->adapter.name) - 1,
			 config->adap_name);
	else
		snprintf(adap->adapter.name, sizeof(adap->adapter.name) - 1,
			 "ciena-i2c-smbus@0x%x",
			 adap->offsets[CIENA_SMB_REG_CTRL]);

	if (config->err_state) adap->err_state = config->err_state;
	else                   adap->err_state = &adap->local_err_state;

	rc = ciena_smb_i2c_init_irq(pdev, adap);
	if (rc) goto on_fail;

	rc = i2c_add_numbered_adapter(&adap->adapter);
	if (rc != 0) {
		dev_err(dev, "failed to add adapter [%d]\n", rc);
		goto on_fail;
	}

	adap->adap_err_state.adapter_id = i2c_adapter_id(&adap->adapter);

	ciena_smb_reset_master(adap);

	rc = ciena_smb_create_all_children(adap, config);
	if (rc != 0) {
		dev_err(dev, "failed to create child devices [%d]\n", rc);
		goto del_adap;
	}

	return 0;

del_adap:
	i2c_del_adapter(&adap->adapter);
on_fail:
	of_node_put(dev->of_node);
	dev_set_drvdata(dev, NULL);
	return rc;
}

/*----------------------------------------------------------------------------*/
static void ciena_smb_remove(struct platform_device *pdev)
{
	struct ciena_smb_adap       *adap = dev_get_drvdata(&pdev->dev);
	struct i2c_ciena_smb_config *conf = dev_get_platdata(&pdev->dev);

	dev_dbg(&pdev->dev, "%s\n", __func__);

	if (conf && conf->deferred_children)
		cancel_delayed_work_sync(&adap->children_work);

	i2c_del_adapter(&adap->adapter);
	of_node_put(pdev->dev.of_node);

	dev_set_drvdata(&pdev->dev, NULL);
}

/*----------------------------------------------------------------------------*/
static const struct platform_device_id ciena_smb_platform_ids[] = {
	{ .name = CIENA_I2C_SMBUS_DRIVER_NAME },
	{}
};
MODULE_DEVICE_TABLE(platform, ciena_smb_platform_ids);

static const struct of_device_id ciena_smb_device_id_table[] = {
	{ .compatible = "ciena,i2c-smb", },
	{},
};
MODULE_DEVICE_TABLE(of, ciena_smb_device_id_table);

/*----------------------------------------------------------------------------*/
static struct platform_driver ciena_smb_driver = {
	.driver = {
		.name           = CIENA_I2C_SMBUS_DRIVER_NAME,
		.owner          = THIS_MODULE,
		.of_match_table = ciena_smb_device_id_table,
	},
	.probe    = ciena_smb_probe,
	.remove   = ciena_smb_remove,
	.id_table = ciena_smb_platform_ids,
};
module_platform_driver(ciena_smb_driver);

/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Dell Drummond <ddrummon@ciena.com>");
MODULE_DESCRIPTION("Ciena SMBus Adapter");
MODULE_LICENSE("GPL");

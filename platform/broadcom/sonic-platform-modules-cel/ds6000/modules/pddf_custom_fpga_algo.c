// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2026 Celestica Inc.
 *
 * A sample i2c driver algorithms for Celestica Inc device 100a FPGA adapters
 *
 */
#define __STDC_WANT_LIB_EXT1__ 1
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/string.h>
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

#define XIIC_MSB_OFFSET 0
#define XIIC_REG_OFFSET (0x100 + XIIC_MSB_OFFSET)

/*
 * Register offsets in bytes from RegisterBase. Three is added to the
 * base offset to access LSB (IBM style) of the word
 */
#define XIIC_CR_REG_OFFSET (0x00 + XIIC_REG_OFFSET) /* Control Register   */
#define XIIC_SR_REG_OFFSET (0x04 + XIIC_REG_OFFSET) /* Status Register    */
#define XIIC_DTR_REG_OFFSET (0x08 + XIIC_REG_OFFSET) /* Data Tx Register   */
#define XIIC_DRR_REG_OFFSET (0x0C + XIIC_REG_OFFSET) /* Data Rx Register   */
#define XIIC_ADR_REG_OFFSET (0x10 + XIIC_REG_OFFSET) /* Address Register   */
#define XIIC_TFO_REG_OFFSET (0x14 + XIIC_REG_OFFSET) /* Tx FIFO Occupancy  */
#define XIIC_RFO_REG_OFFSET (0x18 + XIIC_REG_OFFSET) /* Rx FIFO Occupancy  */
#define XIIC_TBA_REG_OFFSET (0x1C + XIIC_REG_OFFSET) /* 10 Bit Address reg */
#define XIIC_RFD_REG_OFFSET (0x20 + XIIC_REG_OFFSET) /* Rx FIFO Depth reg  */
#define XIIC_GPO_REG_OFFSET (0x24 + XIIC_REG_OFFSET) /* Output Register    */

/* Control Register masks */
#define XIIC_CR_ENABLE_DEVICE_MASK 0x01 /* Device enable = 1      */
#define XIIC_CR_TX_FIFO_RESET_MASK 0x02 /* Transmit FIFO reset=1  */
#define XIIC_CR_MSMS_MASK 0x04 /* Master starts Txing=1  */
#define XIIC_CR_DIR_IS_TX_MASK 0x08 /* Dir of tx. Txing=1     */
#define XIIC_CR_NO_ACK_MASK 0x10 /* Tx Ack. NO ack = 1     */
#define XIIC_CR_REPEATED_START_MASK 0x20 /* Repeated start = 1     */
#define XIIC_CR_GENERAL_CALL_MASK 0x40 /* Gen Call enabled = 1   */

/* Status Register masks */
#define XIIC_SR_GEN_CALL_MASK 0x01 /* 1=a mstr issued a GC   */
#define XIIC_SR_ADDR_AS_SLAVE_MASK 0x02 /* 1=when addr as slave   */
#define XIIC_SR_BUS_BUSY_MASK 0x04 /* 1 = bus is busy        */
#define XIIC_SR_MSTR_RDING_SLAVE_MASK 0x08 /* 1=Dir: mstr <-- slave  */
#define XIIC_SR_TX_FIFO_FULL_MASK 0x10 /* 1 = Tx FIFO full       */
#define XIIC_SR_RX_FIFO_FULL_MASK 0x20 /* 1 = Rx FIFO full       */
#define XIIC_SR_RX_FIFO_EMPTY_MASK 0x40 /* 1 = Rx FIFO empty      */
#define XIIC_SR_TX_FIFO_EMPTY_MASK 0x80 /* 1 = Tx FIFO empty      */

/* Interrupt Status Register masks    Interrupt occurs when...       */
#define XIIC_INTR_ARB_LOST_MASK 0x01 /* 1 = arbitration lost   */
#define XIIC_INTR_TX_ERROR_MASK 0x02 /* 1=Tx error/msg complete */
#define XIIC_INTR_TX_EMPTY_MASK 0x04 /* 1 = Tx FIFO/reg empty  */
#define XIIC_INTR_RX_FULL_MASK 0x08 /* 1=Rx FIFO/reg=OCY level */
#define XIIC_INTR_BNB_MASK 0x10 /* 1 = Bus not busy       */
#define XIIC_INTR_AAS_MASK 0x20 /* 1 = when addr as slave */
#define XIIC_INTR_NAAS_MASK 0x40 /* 1 = not addr as slave  */
#define XIIC_INTR_TX_HALF_MASK 0x80 /* 1 = TX FIFO half empty */

/* The following constants specify the depth of the FIFOs */
#define IIC_RX_FIFO_DEPTH 16 /* Rx fifo capacity               */
#define IIC_TX_FIFO_DEPTH 16 /* Tx fifo capacity               */

/*
 * Tx Fifo upper bit masks.
 */
#define XIIC_TX_DYN_START_MASK 0x0100 /* 1 = Set dynamic start */
#define XIIC_TX_DYN_STOP_MASK 0x0200 /* 1 = Set dynamic stop */

/*
 * The following constants define the register offsets for the Interrupt
 * registers. There are some holes in the memory map for reserved addresses
 * to allow other registers to be added and still match the memory map of the
 * interrupt controller registers
 */
#define XIIC_IISR_OFFSET 0x20 /* Interrupt Status Register */
#define XIIC_RESETR_OFFSET 0x40 /* Reset Register */

#define XIIC_RESET_MASK 0xAUL

#define XIIC_PM_TIMEOUT 1000 /* ms */
/* timeout waiting for the controller to respond */
#define XIIC_I2C_TIMEOUT (msecs_to_jiffies(1000))

struct fpgalogic_i2c {
	struct device *dev;
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
extern void __iomem *fpga_ctl_addr;
extern int (*ptr_fpgapci_read)(uint32_t);
extern int (*ptr_fpgapci_write)(uint32_t, uint32_t);
extern int (*pddf_i2c_pci_add_numbered_bus)(struct i2c_adapter *, int);
static int xiic_reinit(struct fpgalogic_i2c *i2c);

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

static inline void xiic_setreg32(struct fpgalogic_i2c *i2c, int reg, int value)
{
	(void)iowrite32(value, i2c->base + reg);
}

static inline int xiic_getreg32(struct fpgalogic_i2c *i2c, int reg)
{
	u32 ret;

	ret = ioread32(i2c->base + reg);

	return ret;
}

static inline void xiic_irq_clr(struct fpgalogic_i2c *i2c, u32 mask)
{
	u32 isr = xiic_getreg32(i2c, XIIC_IISR_OFFSET);

	xiic_setreg32(i2c, XIIC_IISR_OFFSET, isr & mask);
}

static int xiic_clear_rx_fifo(struct fpgalogic_i2c *i2c)
{
	u8 sr;
	unsigned long timeout;

	timeout = jiffies + XIIC_I2C_TIMEOUT;
	for (sr = xiic_getreg32(i2c, XIIC_SR_REG_OFFSET);
	     !(sr & XIIC_SR_RX_FIFO_EMPTY_MASK);
	     sr = xiic_getreg32(i2c, XIIC_SR_REG_OFFSET)) {
		xiic_getreg32(i2c, XIIC_DRR_REG_OFFSET);
		if (time_after(jiffies, timeout)) {
			pr_info("Failed to clear rx fifo\n");
			return -ETIMEDOUT;
		}
	}

	return 0;
}

/**
 * Wait until something change in a given register
 * @i2c: ocores I2C device instance
 * @reg: register to query
 * @mask: bitmask to apply on register value
 * @val: expected result
 * @timeout: timeout in jiffies
 *
 * Timeout is necessary to avoid to stay here forever when the chip
 * does not answer correctly.
 *
 * Return: 0 on success, -ETIMEDOUT on timeout
 */
static int poll_wait(struct fpgalogic_i2c *i2c, int reg, u8 mask, u8 val,
		     const unsigned long timeout)
{
	unsigned long j;
	u8 status = 0;

	j = jiffies + timeout;
	while (1) {
		mutex_lock(&i2c->lock);
		status = xiic_getreg32(i2c, reg);
		mutex_unlock(&i2c->lock);
		if ((status & mask) == val)
			break;
		if (time_after(jiffies, j))
			return -ETIMEDOUT;
		cpu_relax();
		cond_resched();
	}
	return 0;
}

/**
 * Wait until is possible to process some data
 * @i2c: ocores I2C device instance
 *
 * Used when the device is in polling mode (interrupts disabled).
 *
 * Return: 0 on success, -ETIMEDOUT on timeout
 */
static int ocores_poll_wait(struct fpgalogic_i2c *i2c)
{
	u8 mask = 0, status = 0;
	int err = 0;
	int val = 0;
	int tmp = 0;

	mutex_lock(&i2c->lock);

	if (i2c->state == STATE_DONE) {
		/* transfer is over */
		mask = XIIC_SR_BUS_BUSY_MASK;
	} else if (i2c->state == STATE_WRITE || i2c->state == STATE_START) {
		/* on going transfer */
		if (i2c->msg->len == 0)
			mask = XIIC_INTR_TX_ERROR_MASK;
		else
			mask = XIIC_SR_TX_FIFO_FULL_MASK;

	} else if (i2c->state == STATE_READ) {
		/* on going receive */
		mask = XIIC_SR_TX_FIFO_EMPTY_MASK | XIIC_SR_RX_FIFO_EMPTY_MASK;
	}
	mutex_unlock(&i2c->lock);
	// printk("Wait for: 0x%x\n", mask);

	/*
	 * once we are here we expect to get the expected result immediately
	 * so if after 50ms we timeout then something is broken.
	 */
	if (i2c->nmsgs == 1 && i2c->msg->len == 0 &&
	    i2c->state == STATE_START &&
	    !(i2c->msg->flags & I2C_M_RD)) { /* for i2cdetect I2C_SMBUS_QUICK mode*/
		err = poll_wait(i2c, XIIC_IISR_OFFSET, mask, mask,
				msecs_to_jiffies(50));
		mutex_lock(&i2c->lock);
		status = xiic_getreg32(i2c, XIIC_SR_REG_OFFSET);
		mutex_unlock(&i2c->lock);

		/* AXI IIC as an transceiver , if ever an XIIC_INTR_TX_ERROR_MASK interrupt happens, means no such i2c device */
		if (err != 0)
			err = 0;
		else
			err = -ETIMEDOUT;
	} else {
		if (mask & XIIC_SR_TX_FIFO_EMPTY_MASK) {
			err = poll_wait(i2c, XIIC_SR_REG_OFFSET, mask,
					XIIC_SR_TX_FIFO_EMPTY_MASK,
					msecs_to_jiffies(50));
			mask &= ~XIIC_SR_TX_FIFO_EMPTY_MASK;
		}
		if (err == 0) {
			err = poll_wait(i2c, XIIC_SR_REG_OFFSET, mask, 0,
					msecs_to_jiffies(50));
		}
		mutex_lock(&i2c->lock);
		status = xiic_getreg32(i2c, XIIC_IISR_OFFSET);

		if ((status & XIIC_INTR_ARB_LOST_MASK) ||
		    ((status & XIIC_INTR_TX_ERROR_MASK) &&
		     !(status & XIIC_INTR_RX_FULL_MASK) &&
		     !(i2c->msg->flags & I2C_M_RD))) { /* AXI IIC as an transceiver , if ever an XIIC_INTR_TX_ERROR_MASK interrupt happens, return */
			err = -ETIMEDOUT;

			if (status & XIIC_INTR_ARB_LOST_MASK) {
				val = xiic_getreg32(i2c, XIIC_CR_REG_OFFSET);
				tmp = XIIC_CR_MSMS_MASK;
				val &= (~tmp);
				xiic_setreg32(i2c, XIIC_CR_REG_OFFSET, val);
				xiic_setreg32(i2c, XIIC_IISR_OFFSET,
					      XIIC_INTR_ARB_LOST_MASK);
#if DEBUG
				pddf_dbg(
					FPGA,
					KERN_INFO
					"%s: TRANSFER STATUS ERROR, ISR: bit 0x%x happens\n",
					__func__, XIIC_INTR_ARB_LOST_MASK);
#endif
			}
			if (status & XIIC_INTR_TX_ERROR_MASK) {
				int sta = 0;
				int cr = 0;

				sta = xiic_getreg32(i2c, XIIC_SR_REG_OFFSET);
				cr = xiic_getreg32(i2c, XIIC_CR_REG_OFFSET);
				xiic_setreg32(i2c, XIIC_IISR_OFFSET,
					      XIIC_INTR_TX_ERROR_MASK);
#if DEBUG
				pddf_dbg(
					FPGA,
					KERN_INFO
					"%s: TRANSFER STATUS ERROR, ISR: bit 0x%x happens; "
					"SR: bit 0x%x; CR: bit 0x%x\n",
					__func__, status, sta, cr);
#endif
			}
			/* Soft reset IIC controller. */
			xiic_setreg32(i2c, XIIC_RESETR_OFFSET, XIIC_RESET_MASK);
			(void)xiic_reinit(i2c);
			mutex_unlock(&i2c->lock);
			return err;
		}
		mutex_unlock(&i2c->lock);
	}
#if DEBUG
	if (err)
		pddf_dbg(FPGA,
			 KERN_INFO
			 "%s: STATUS timeout, bit 0x%x did not clear in 50ms\n",
			 __func__, status);
#endif
	return err;
}

static void ocores_process(struct fpgalogic_i2c *i2c)
{
	struct i2c_msg *msg = i2c->msg;
	u16 val;

	mutex_lock(&i2c->lock);
	dev_dbg(i2c->dev, "STATE: %d\n", i2c->state);

	if (i2c->state == STATE_START) {
		i2c->state = (msg->flags & I2C_M_RD) ? STATE_READ : STATE_WRITE;
		if (i2c->state == STATE_READ) {
			if (msg->flags & I2C_M_RECV_LEN) {
				/*
				 * For SMBus block reads, we don't know the length yet.
				 * Tell the hardware to read the maximum possible number of bytes.
				 * We will parse the actual length from the first received byte later.
				 * The I2C core provides a buffer of I2C_SMBUS_BLOCK_MAX + 2.
				 */
				msg->len = I2C_SMBUS_BLOCK_MAX + 2;
			}
			val = msg->len | XIIC_TX_DYN_STOP_MASK;
			xiic_setreg32(i2c, XIIC_DTR_REG_OFFSET, val);
			goto out;
		}
	}
	if (i2c->state == STATE_READ) {
		/* Read all available bytes from the RX FIFO into the message buffer */
		while (!(xiic_getreg32(i2c, XIIC_SR_REG_OFFSET) &
			 XIIC_SR_RX_FIFO_EMPTY_MASK)) {
			if (i2c->pos < msg->len) {
				msg->buf[i2c->pos++] =
					xiic_getreg32(i2c, XIIC_DRR_REG_OFFSET);
			} else {
				/* This shouldn't happen if msg->len is sized correctly, but as a safeguard,
				 * read and discard the byte to prevent stalling the FIFO.
				 */
				(void)xiic_getreg32(i2c, XIIC_DRR_REG_OFFSET);
			}
		}
	} else if (i2c->state == STATE_WRITE) {
		/* if it reaches the last byte data to be sent */
		if ((i2c->pos == msg->len - 1) && (i2c->nmsgs == 1)) {
			val = msg->buf[i2c->pos++] | XIIC_TX_DYN_STOP_MASK;
			xiic_setreg32(i2c, XIIC_DTR_REG_OFFSET, val);
			i2c->state = STATE_DONE;
			goto out;
			/* if it is not the last byte data to be sent */
		} else if (i2c->pos < msg->len) {
			xiic_setreg32(i2c, XIIC_DTR_REG_OFFSET,
				      msg->buf[i2c->pos++]);
			goto out;
		}
	}

	/* end of msg? */
	if (i2c->pos >= msg->len) {
		/* adjust the length for block reads */
		if (i2c->state == STATE_READ && (msg->flags & I2C_M_RECV_LEN)) {
			u8 block_len = msg->buf[0];

			if (block_len == 0 || block_len > I2C_SMBUS_BLOCK_MAX) {
				dev_dbg(i2c->dev,
					"Invalid SMBUS block length %d\n",
					block_len);
				goto out;
			}
			/* The final message length is the block length + the length byte itself. */
			msg->len = block_len + 1;
		}

		i2c->nmsgs--;
		i2c->pos = 0;
		if (i2c->nmsgs) {
			i2c->msg++;
			msg = i2c->msg;
			if (!(msg->flags & I2C_M_NOSTART)) /* send start? */ {
				i2c->state = STATE_START;
				xiic_setreg32(i2c, XIIC_DTR_REG_OFFSET,
					      i2c_8bit_addr_from_msg(msg) |
						      XIIC_TX_DYN_START_MASK);
				goto out;
			}
		} else { /* end? */
			i2c->state = STATE_DONE;
			goto out;
		}
	}

out:
	mutex_unlock(&i2c->lock);
}

static int fpgai2c_poll(struct fpgalogic_i2c *i2c, struct i2c_msg *msgs,
			int num)
{
	int ret = 0;
	// u8 ctrl;

	mutex_lock(&i2c->lock);
	/* Soft reset IIC controller. */
	xiic_setreg32(i2c, XIIC_RESETR_OFFSET, XIIC_RESET_MASK);
	/* Set receive Fifo depth to maximum (zero based). */
	xiic_setreg32(i2c, XIIC_RFD_REG_OFFSET, IIC_RX_FIFO_DEPTH - 1);

	/* Reset Tx Fifo. */
	xiic_setreg32(i2c, XIIC_CR_REG_OFFSET, XIIC_CR_TX_FIFO_RESET_MASK);

	/* Enable IIC Device, remove Tx Fifo reset & disable general call. */
	xiic_setreg32(i2c, XIIC_CR_REG_OFFSET, XIIC_CR_ENABLE_DEVICE_MASK);

	/* set i2c clock as 100Hz. */
	// xiic_setreg32(i2c, 0x13c, 0x7C);

	/* make sure RX fifo is empty */
	ret = xiic_clear_rx_fifo(i2c);
	if (ret) {
		mutex_unlock(&i2c->lock);
		return ret;
	}

	i2c->msg = msgs;
	i2c->pos = 0;
	i2c->nmsgs = num;
	i2c->state = STATE_START;

	// printk("STATE: %d\n", i2c->state);

	if (msgs->len == 0 && num == 1) { /* suit for i2cdetect time sequence */
		u8 status = xiic_getreg32(i2c, XIIC_IISR_OFFSET);

		xiic_irq_clr(i2c, status);
		/* send out the 1st byte data and stop bit */
		xiic_setreg32(i2c, XIIC_DTR_REG_OFFSET,
			      i2c_8bit_addr_from_msg(msgs) |
				      XIIC_TX_DYN_START_MASK |
				      XIIC_TX_DYN_STOP_MASK);
	} else {
		/* send out the 1st byte data */
		xiic_setreg32(i2c, XIIC_DTR_REG_OFFSET,
			      i2c_8bit_addr_from_msg(msgs) |
				      XIIC_TX_DYN_START_MASK);
	}
	mutex_unlock(&i2c->lock);
	while (1) {
		int err;

		err = ocores_poll_wait(i2c);
		if (err) {
			i2c->state = STATE_ERROR;
			break;
		} else if (i2c->state == STATE_DONE) {
			break;
		}
		ocores_process(i2c);
	}

	return (i2c->state == STATE_DONE) ? num : -EIO;
}

static int fpgai2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	struct fpgalogic_i2c *i2c = i2c_get_adapdata(adap);
	int err = -EIO;
	u8 retry = 0, max_retry = 0;

	if (((msgs->len == 1 && (msgs->flags & I2C_M_RD)) ||
	     (msgs->len == 0 && !(msgs->flags & I2C_M_RD))) && num == 1) /* I2C_SMBUS_QUICK or I2C_SMBUS_BYTE */
		max_retry = 1;
	else
		max_retry =
			5; // retry 5 times if receive a NACK or other errors
	while ((err == -EIO) && (retry < max_retry)) {
		err = fpgai2c_poll(i2c, msgs, num);
		retry++;
	}

	return err;
}

static u32 fpgai2c_func(struct i2c_adapter *adap)
{
	/* a typical full-I2C adapter would use the following  */
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_SMBUS_BLOCK_DATA;
}

static const struct i2c_algorithm fpgai2c_algorithm = {
	.master_xfer = fpgai2c_xfer, /*write I2C messages */
	.functionality = fpgai2c_func, /* what the adapter supports */
};

static int xiic_reinit(struct fpgalogic_i2c *i2c)
{
	int ret;
	int val = 0;

	/* Soft reset IIC controller. */
	xiic_setreg32(i2c, XIIC_RESETR_OFFSET, XIIC_RESET_MASK);

	/* Set receive Fifo depth to maximum (zero based). */
	xiic_setreg32(i2c, XIIC_RFD_REG_OFFSET, IIC_RX_FIFO_DEPTH - 1);

	/* Reset Tx Fifo. */
	xiic_setreg32(i2c, XIIC_CR_REG_OFFSET, XIIC_CR_TX_FIFO_RESET_MASK);

	/* Enable IIC Device, remove Tx Fifo reset & disable general call. */
	val |= XIIC_CR_ENABLE_DEVICE_MASK;
	// val |= XIIC_CR_TX_FIFO_RESET_MASK;
	// val |= XIIC_CR_MSMS_MASK;
	val |= XIIC_CR_DIR_IS_TX_MASK;
	xiic_setreg32(i2c, XIIC_CR_REG_OFFSET, val);

	/* make sure RX fifo is empty */
	ret = xiic_clear_rx_fifo(i2c);
	if (ret)
		return ret;

	return 0;
}

static int fpgai2c_init(struct fpgalogic_i2c *i2c)
{
	// int prescale;
	// int diff;
	// u8 ctrl;
	int ret;

	// i2c->reg_set = xiic_setreg32;
	// i2c->reg_get = xiic_getreg32;

	ret = xiic_reinit(i2c);
	if (ret < 0) {
		pr_info("Cannot xiic_reinit\n");
		return ret;
	}

	/* Initialize interrupt handlers if not already done */
	init_waitqueue_head(&i2c->wait);
	return 0;
}

/**
 * i2c_ch_iomem_offset - adjust the index and iomem offset for each channel
 *
 * Platform specific i2c adapter number mapping is as follows:
 * ----------------------------------------------------------------------------
 * index      core_name    resource_address          purpose
 * --------+------------+-------------------------+----------------------------
 *   0       AXI_IIC 1    0x0001_0000 0x0001_0FFF   Access to OSFP(1-8)
 *   1       AXI_IIC 18   0x0001_1000 0x0001_1FFF   Access to OSFP(9-16)
 *   2       AXI_IIC 19   0x0001_2000 0x0001_2FFF   Access to OSFP(17-24)
 *   3       AXI_IIC 20   0x0001_3000 0x0001_3FFF   Access to OSFP(25-32)
 *   4       AXI_IIC 2    0x0001_4000 0x0001_4FFF   Access to OSFP(33-40)
 *   5       AXI_IIC 21   0x0001_5000 0x0001_5FFF   Access to OSFP(41-48)
 *   6       AXI_IIC 22   0x0001_6000 0x0001_6FFF   Access to OSFP(49-56)
 *   7       AXI_IIC 23   0x0001_7000 0x0001_7FFF   Access to OSFP(57-64)
 *   8       AXI_IIC 4    0x0001_A000 0x0001_AFFF   Access to SFP28-1
 *   9       AXI_IIC 5    0x0001_B000 0x0001_BFFF   Access to SFP28-2
 *   10      AXI_IIC 0    0x0001_8000 0x0001_8FFF   Access to RC38112
 *   11      AXI_IIC 3    0x0001_9000 0x0001_9FFF   Access to Port CPLDs
 *   12      AXI_IIC 6    0x0001_C000 0x0001_CFFF   Access to: UCD90320
 *   13      AXI_IIC 7    0x0001_D000 0x0001_DFFF   Access to: LM75
 *   14      AXI_IIC 8    0x0002_0000 0x0002_0FFF   Access to PSU
 *   15      AXI_IIC 9    0x0002_1000 0x0002_1FFF   Access to Baseboard CPLD
 *   16      AXI_IIC 10   0x0002_2000 0x0002_2FFF   Access to: ROT
 *   17      AXI_IIC 11   0x0002_3000 0x0002_3FFF   Access to: SYS EEPROMs
 *   18      AXI_IIC 12   0x0002_5000 0x0002_5FFF   Access to: ROT
 *   19      AXI_IIC 13   0x0002_6000 0x0002_6FFF   BMC I2C slave, not used
 *   20      AXI_IIC 14   0x0002_7000 0x0002_7FFF   Access to:TMP432A
 *   21      AXI_IIC 15   0x0002_8000 0x0002_8FFF   Access to:TMP432A
 *   22      AXI_IIC 16   0x0002_9000 0x0002_9FFF   Access to: Front IO
 *   23      AXI_IIC 17   0x0002_A000 0x0002_AFFF   Access to: FAN board
 *   24      AXI_IIC 24   0x0002_B000 0x0002_BFFF   Access to: TH6
 *
 * @i2c_ch_index: The i2c adaptor index, zero based.
 * Return: The adjusted iomem index for mapping.
 */
static int i2c_ch_iomem_offset(int i2c_ch_index)
{
	int ch_iomem_offset = 0;

	switch (i2c_ch_index) {
	case 8:
	case 9:
		ch_iomem_offset = i2c_ch_index + 2;
		break;
	case 10:
	case 11:
		ch_iomem_offset = i2c_ch_index - 2;
		break;
	case 14:
	case 15:
	case 16:
	case 17:
		ch_iomem_offset = i2c_ch_index + 2;
		break;
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
		ch_iomem_offset = i2c_ch_index + 3;
		break;
	default:
		ch_iomem_offset = i2c_ch_index;
		break;
	}
	return ch_iomem_offset;
}

static int adap_data_init(struct i2c_adapter *adap, int i2c_ch_index)
{
	struct fpgapci_devdata *pci_privdata = 0;

	pci_privdata = (struct fpgapci_devdata *)dev_get_drvdata(adap->dev.parent);

	if (pci_privdata == 0) {
		pr_info("[%s]: ERROR pci_privdata is 0\n", __func__);
		return -ENOMEM;
	}

	if (i2c_ch_index >= pci_privdata->max_fpga_i2c_ch ||
	    pci_privdata->max_fpga_i2c_ch > I2C_PCI_MAX_BUS) {
		pr_info("[%s]: ERROR i2c_ch_index=%d max_ch_index=%d out of range: %d\n",
			__func__, i2c_ch_index,
			pci_privdata->max_fpga_i2c_ch, I2C_PCI_MAX_BUS);
		return -EINVAL;
	}
#ifdef __STDC_LIB_EXT1__
	memset_s(&fpgalogic_i2c[i2c_ch_index], sizeof(fpgalogic_i2c[0]), 0,
		 sizeof(fpgalogic_i2c[0]));
#else
	memset(&fpgalogic_i2c[i2c_ch_index], 0, sizeof(fpgalogic_i2c[0]));
#endif

	fpgalogic_i2c[i2c_ch_index].base =
		pci_privdata->fpga_i2c_ch_base_addr +
		i2c_ch_iomem_offset(i2c_ch_index) * pci_privdata->fpga_i2c_ch_size;

#if DEBUG
	pddf_dbg(FPGA,
		 KERN_INFO "[%s] i2c_ch=%d fpga_i2c_ch_base_addr:0x%08lx ch_size=0x%x",
		 __func__,
		 i2c_ch_index,
		 fpgalogic_i2c[i2c_ch_index].base,
		 pci_privdata->fpga_i2c_ch_size);
#endif

	mutex_init(&fpgalogic_i2c[i2c_ch_index].lock);
	fpgai2c_init(&fpgalogic_i2c[i2c_ch_index]);

	adap->algo_data = &fpgalogic_i2c[i2c_ch_index];
	i2c_set_adapdata(adap, &fpgalogic_i2c[i2c_ch_index]);
	fpgalogic_i2c[i2c_ch_index].dev = &adap->dev;
	return 0;
}

/**
 * pddf_i2c_pci_add_numbered_bus_ds6000 - Init and create numbred i2c adapter
 *
 * See &i2c_ch_iomem_offset
 *
 * @adap: The i2c adapter to be init.
 * @i2c_ch_index: Logical adaptor location on FPGA with zero based.
 * Return: result of i2c_add_numbered_adapter
 */
static int pddf_i2c_pci_add_numbered_bus_ds6000(struct i2c_adapter *adap,
						 int i2c_ch_index)
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
	return ioread32(fpga_ctl_addr + offset);
}

int board_i2c_fpgapci_write(uint32_t offset, uint32_t value)
{
	iowrite32(value, fpga_ctl_addr + offset);
	return 0;
}

static int __init pddf_celestica_device_100a_algo_init(void)
{
	pddf_dbg(FPGA, KERN_INFO "[%s]\n", __func__);
	pddf_i2c_pci_add_numbered_bus = pddf_i2c_pci_add_numbered_bus_ds6000;
	ptr_fpgapci_read = board_i2c_fpgapci_read;
	ptr_fpgapci_write = board_i2c_fpgapci_write;
	return 0;
}

static void __exit pddf_celestica_device_100a_algo_exit(void)
{
	pddf_dbg(FPGA, KERN_INFO "[%s]\n", __func__);

	pddf_i2c_pci_add_numbered_bus = NULL;
	ptr_fpgapci_read = NULL;
	ptr_fpgapci_write = NULL;
}

module_init(pddf_celestica_device_100a_algo_init);
module_exit(pddf_celestica_device_100a_algo_exit);
MODULE_DESCRIPTION("celestica Corporation Device 100a FPGAPCIe I2C-Bus algorithm");
MODULE_LICENSE("GPL");

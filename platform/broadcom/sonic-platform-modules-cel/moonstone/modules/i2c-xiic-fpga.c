// SPDX-License-Identifier: GPL-2.0
/*
 * fpga-i2c-xiic.c
 * Copyright (c) 2002-2007 Xilinx Inc.
 * Copyright (c) 2009-2010 Intel Corporation
 * Copyright (c) 2022-2024 Celestica Corporation
 *
 * This code was implemented by nicwu@celestica.com
 * to port linux xiic driver to suit Celestica ds4101 platform.
 * The copyright holder as seen in the header is Celestica corporation.
 *
 */

/* Supports:
 * Xilinx IIC
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>

#define DRIVER_NAME "fpga-xiic-i2c"

enum xilinx_i2c_state {
	STATE_DONE,
	STATE_ERROR,
	STATE_START,
	STATE_WRITE,
	STATE_READ,
	STATE_BB,
};

enum xiic_endian {
	LITTLE,
	BIG
};

enum xiic_mode {
	POLLING,
	INT
};
/**
 * struct xiic_i2c - Internal representation of the XIIC I2C bus
 * @dev:	Pointer to device structure
 * @base:   Memory base of the HW registers
 * @wait:   Wait queue for callers
 * @adap:   Kernel adapter representation
 * @tx_msg: Messages from above to be sent
 * @lock:   Mutual exclusion
 * @tx_pos: Current pos in TX message
 * @nmsgs:  Number of messages in tx_msg
 * @state:  See STATE_
 * @rx_msg: Current RX message
 * @rx_pos: Position within current RX message
 * @endianness: big/little-endian byte order
 * @clk:	Pointer to struct clk
 * @dynamic: Mode of controller
 * @prev_msg_tx: Previous message is Tx
 * @smbus_block_read: Flag to handle block read
 */
struct xiic_i2c {
	struct device *dev;
	int id;
	void __iomem *base;
	void __iomem *gl_int_base;
	wait_queue_head_t wait;
	struct i2c_adapter adap;
	struct i2c_msg *tx_msg;
	struct mutex lock;
	unsigned int tx_pos;
	unsigned int nmsgs;
	enum xilinx_i2c_state state;
	struct i2c_msg *rx_msg;
	int rx_pos;

	enum xiic_endian endianness;
	struct clk *clk;
	bool dynamic;
	bool prev_msg_tx;
	bool smbus_block_read;

	/*polling variable*/
	unsigned int pos;
	spinlock_t process_lock;
	unsigned int flag;
	int nack_retry;
	struct i2c_msg *msg;
};

#define XIIC_MSB_OFFSET 0
#define XIIC_REG_OFFSET (0x100 + XIIC_MSB_OFFSET)

/*
 * Register offsets in bytes from RegisterBase. Three is added to the
 * base offset to access LSB (IBM style) of the word
 */
#define XIIC_CR_REG_OFFSET   (0x00 + XIIC_REG_OFFSET) /* Control Register */
#define XIIC_SR_REG_OFFSET   (0x04 + XIIC_REG_OFFSET) /* Status Register */
#define XIIC_DTR_REG_OFFSET  (0x08 + XIIC_REG_OFFSET) /* Data Tx Register */
#define XIIC_DRR_REG_OFFSET  (0x0C + XIIC_REG_OFFSET) /* Data Rx Register */
#define XIIC_ADR_REG_OFFSET  (0x10 + XIIC_REG_OFFSET) /* Address Register */
#define XIIC_TFO_REG_OFFSET  (0x14 + XIIC_REG_OFFSET) /* Tx FIFO Occupancy */
#define XIIC_RFO_REG_OFFSET  (0x18 + XIIC_REG_OFFSET) /* Rx FIFO Occupancy */
#define XIIC_TBA_REG_OFFSET  (0x1C + XIIC_REG_OFFSET) /* 10 Bit Address reg */
#define XIIC_RFD_REG_OFFSET  (0x20 + XIIC_REG_OFFSET) /* Rx FIFO Depth reg */
#define XIIC_GPO_REG_OFFSET  (0x24 + XIIC_REG_OFFSET) /* Output Register */

/* Control Register masks */
#define XIIC_CR_ENABLE_DEVICE_MASK 0x01	 /* Device enable = 1 */
#define XIIC_CR_TX_FIFO_RESET_MASK 0x02	 /* Transmit FIFO reset=1 */
#define XIIC_CR_MSMS_MASK 0x04		 /* Master starts Txing=1 */
#define XIIC_CR_DIR_IS_TX_MASK 0x08	 /* Dir of tx. Txing=1 */
#define XIIC_CR_NO_ACK_MASK 0x10	 /* Tx Ack. NO ack = 1 */
#define XIIC_CR_REPEATED_START_MASK 0x20 /* Repeated start = 1 */
#define XIIC_CR_GENERAL_CALL_MASK 0x40	 /* Gen Call enabled = 1 */

/* Status Register masks */
#define XIIC_SR_GEN_CALL_MASK 0x01	   /* 1=a mstr issued a GC */
#define XIIC_SR_ADDR_AS_SLAVE_MASK 0x02	   /* 1=when addr as slave */
#define XIIC_SR_BUS_BUSY_MASK 0x04	   /* 1 = bus is busy */
#define XIIC_SR_MSTR_RDING_SLAVE_MASK 0x08 /* 1=Dir: mstr <-- slave */
#define XIIC_SR_TX_FIFO_FULL_MASK 0x10	   /* 1 = Tx FIFO full */
#define XIIC_SR_RX_FIFO_FULL_MASK 0x20	   /* 1 = Rx FIFO full */
#define XIIC_SR_RX_FIFO_EMPTY_MASK 0x40	   /* 1 = Rx FIFO empty	  */
#define XIIC_SR_TX_FIFO_EMPTY_MASK 0x80	   /* 1 = Tx FIFO empty	  */

/* Interrupt Status Register masks */
#define XIIC_INTR_ARB_LOST_MASK 0x01 /* 1 = arbitration lost   */
#define XIIC_INTR_TX_ERROR_MASK 0x02 /* 1=Tx error/msg complete */
#define XIIC_INTR_TX_EMPTY_MASK 0x04 /* 1 = Tx FIFO/reg empty  */
#define XIIC_INTR_RX_FULL_MASK 0x08  /* 1=Rx FIFO/reg=OCY level */
#define XIIC_INTR_BNB_MASK 0x10	     /* 1 = Bus not busy	   */
#define XIIC_INTR_AAS_MASK 0x20	     /* 1 = when addr as slave */
#define XIIC_INTR_NAAS_MASK 0x40     /* 1 = not addr as slave  */
#define XIIC_INTR_TX_HALF_MASK 0x80  /* 1 = TX FIFO half empty */

/* The following constants specify the depth of the FIFOs */
#define IIC_RX_FIFO_DEPTH 16 /* Rx fifo capacity */
#define IIC_TX_FIFO_DEPTH 16 /* Tx fifo capacity */

/*
 * Tx Fifo upper bit masks.
 */
#define XIIC_TX_DYN_START_MASK 0x0100 /* 1 = Set dynamic start */
#define XIIC_TX_DYN_STOP_MASK 0x0200  /* 1 = Set dynamic stop */

/*
 * The following constants define the register offsets for the Interrupt
 * registers. There are some holes in the memory map for reserved addresses
 * to allow other registers to be added and still match the memory map of the
 * interrupt controller registers
 */
#define XIIC_IISR_OFFSET 0x20	/* Interrupt Status Register */
#define XIIC_RESETR_OFFSET 0x40 /* Reset Register */

#define XIIC_RESET_MASK 0xAUL

#define XIIC_PM_TIMEOUT 1000 /* ms */
/* timeout waiting for the controller to respond */
#define XIIC_I2C_TIMEOUT (msecs_to_jiffies(1000))

/*
 * For the register read and write functions, a little-endian and big-endian
 * version are necessary. Endianness is detected during the probe function.
 * Only the least significant byte [doublet] of the register are ever
 * accessed. This requires an offset of 3 [2] from the base address for
 * big-endian systems.
 */
static inline void xiic_setreg8(struct xiic_i2c *i2c, int reg, u8 value)
{
	if (i2c->endianness == LITTLE)
		iowrite8(value, i2c->base + reg);
	else
		iowrite8(value, i2c->base + reg + 3);
}

static inline u8 xiic_getreg8(struct xiic_i2c *i2c, int reg)
{
	u8 ret;

	if (i2c->endianness == LITTLE)
		ret = ioread8(i2c->base + reg);
	else
		ret = ioread8(i2c->base + reg + 3);
	return ret;
}

static inline void xiic_setreg16(struct xiic_i2c *i2c, int reg, u16 value)
{
	if (i2c->endianness == LITTLE)
		iowrite16(value, i2c->base + reg);
	else
		iowrite16be(value, i2c->base + reg + 2);
}

static inline void xiic_setreg32(struct xiic_i2c *i2c, int reg, int value)
{
	if (i2c->endianness == LITTLE)
		iowrite32(value, i2c->base + reg);
	else
		iowrite32be(value, i2c->base + reg);
}

static inline int xiic_getreg32(struct xiic_i2c *i2c, int reg)
{
	u32 ret;

	if (i2c->endianness == LITTLE)
		ret = ioread32(i2c->base + reg);
	else
		ret = ioread32be(i2c->base + reg);

	return ret;
}

static inline void xiic_irq_clr(struct xiic_i2c *i2c, u32 mask)
{
	u32 isr = xiic_getreg32(i2c, XIIC_IISR_OFFSET);

	xiic_setreg32(i2c, XIIC_IISR_OFFSET, isr & mask);
}

static int xiic_clear_rx_fifo(struct xiic_i2c *i2c)
{
	u8 sr;
	unsigned long timeout;

	timeout = jiffies + XIIC_I2C_TIMEOUT;
	for (sr = xiic_getreg8(i2c, XIIC_SR_REG_OFFSET);
		!(sr & XIIC_SR_RX_FIFO_EMPTY_MASK);
		sr = xiic_getreg8(i2c, XIIC_SR_REG_OFFSET)) {
		xiic_getreg8(i2c, XIIC_DRR_REG_OFFSET);
		if (time_after(jiffies, timeout)) {
			dev_err(i2c->dev, "Failed to clear rx fifo\n");
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int xiic_reinit(struct xiic_i2c *i2c)
{
	int ret;

	/* Soft reset IIC controller. */
	xiic_setreg32(i2c, XIIC_RESETR_OFFSET, XIIC_RESET_MASK);

	/* Set receive Fifo depth to maximum (zero based). */
	xiic_setreg8(i2c, XIIC_RFD_REG_OFFSET, IIC_RX_FIFO_DEPTH - 1);

	/* Reset Tx Fifo. */
	xiic_setreg8(i2c, XIIC_CR_REG_OFFSET, XIIC_CR_TX_FIFO_RESET_MASK);

	/* Enable IIC Device, remove Tx Fifo reset & disable general call. */
	xiic_setreg8(i2c, XIIC_CR_REG_OFFSET, XIIC_CR_ENABLE_DEVICE_MASK);

	/* make sure RX fifo is empty */
	ret = xiic_clear_rx_fifo(i2c);
	if (ret)
		return ret;

	return 0;
}

static void xiic_deinit(struct xiic_i2c *i2c)
{
	u8 cr;

	xiic_setreg32(i2c, XIIC_RESETR_OFFSET, XIIC_RESET_MASK);

	/* Disable IIC Device. */
	cr = xiic_getreg8(i2c, XIIC_CR_REG_OFFSET);
	xiic_setreg8(i2c, XIIC_CR_REG_OFFSET, cr & ~XIIC_CR_ENABLE_DEVICE_MASK);
}

static void ocores_process(struct xiic_i2c *i2c)
{
	struct i2c_msg *msg = i2c->msg;
	u16 val;

	/*
	 * If we spin here because we are in timeout, so we are going
	 * to be in STATE_ERROR.
	 */
	mutex_lock(&i2c->lock);
	dev_dbg(&i2c->adap.dev, "STATE: %d\n", i2c->state);

	if (i2c->state == STATE_START) {
		i2c->state = (msg->flags & I2C_M_RD) ? STATE_READ : STATE_WRITE;
		/* if it's the time sequence is
		 * 'start bit + address + read bit + stop bit'
		 */
		if (i2c->state == STATE_READ) {
			/* it's the last message so we include dynamic stop bit
			 * with length.
			 */
			val = msg->len | XIIC_TX_DYN_STOP_MASK;
			xiic_setreg16(i2c, XIIC_DTR_REG_OFFSET, val);
			goto out;
		}
	}
	if (i2c->state == STATE_READ) {
		/* suit for I2C_FUNC_SMBUS_BLOCK_DATA */
		if (msg->flags & I2C_M_RECV_LEN) {
			val = xiic_getreg32(i2c, XIIC_DRR_REG_OFFSET);
			msg->buf[i2c->pos++] = val;
			/* msg len includes 1 byte smbus block count
			 * and data bytes.
			 */
			msg->len = val + 1;
			if (msg->len > I2C_SMBUS_BLOCK_MAX + 2) {
				dev_err(&i2c->adap.dev,
					"Invalid block write size %d\n",
					val);
				goto out;
			}
			msg->flags &= ~I2C_M_RECV_LEN;
		} else {
			msg->buf[i2c->pos++] = xiic_getreg32(i2c,
							XIIC_DRR_REG_OFFSET);
		}
	} else if (i2c->state == STATE_WRITE) {
		/* if it reaches the last byte data to be sent */
		if ((i2c->pos == msg->len - 1) && (i2c->nmsgs == 1)) {
			val = msg->buf[i2c->pos++] | XIIC_TX_DYN_STOP_MASK;
			xiic_setreg16(i2c, XIIC_DTR_REG_OFFSET, val);
			i2c->state = STATE_DONE;
			goto out;
		/* if it is not the last byte data to be sent */
		} else if (i2c->pos < msg->len) {
			xiic_setreg16(i2c, XIIC_DTR_REG_OFFSET,
				      msg->buf[i2c->pos++]);
			goto out;
		}
	}

	/* end of msg? */
	if (i2c->pos == msg->len) {
		i2c->nmsgs--;
		i2c->pos = 0;
		if (i2c->nmsgs) {
			i2c->msg++;
			msg = i2c->msg;
			if (!(msg->flags & I2C_M_NOSTART)) {  /* send start? */
				i2c->state = STATE_START;
				xiic_setreg16(i2c, XIIC_DTR_REG_OFFSET,
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
static int ocores_wait(struct xiic_i2c *i2c,
			   int reg, u8 mask, u8 val,
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
static int ocores_poll_wait(struct xiic_i2c *i2c)
{
	u8 mask = 0, status = 0;
	int err = 0;

	if (i2c->state == STATE_DONE) {
		/* transfer is over */
		mask = XIIC_SR_BUS_BUSY_MASK;
	} else if (i2c->state == STATE_WRITE || i2c->state == STATE_START) {
		/* on going transfer */
		if (i2c->msg->len == 0)
			mask = XIIC_INTR_TX_ERROR_MASK;
		else
			mask = XIIC_SR_TX_FIFO_FULL_MASK;
	} else if (i2c->state == STATE_READ)
		/* on going receive */
		mask = XIIC_SR_TX_FIFO_EMPTY_MASK | XIIC_SR_RX_FIFO_EMPTY_MASK;

	dev_dbg(&i2c->adap.dev, "Wait for: 0x%x\n", mask);

	/*
	 * once we are here we expect to get the expected result immediately
	 * so if after 50ms we timeout then something is broken.
	 */
	/* for i2cdetect I2C_SMBUS_QUICK mode*/
	if (i2c->nmsgs == 1 && i2c->msg->len == 0 &&
	    i2c->state == STATE_START && !(i2c->msg->flags & I2C_M_RD)) {

		err = ocores_wait(i2c, XIIC_IISR_OFFSET, mask, mask,
				  msecs_to_jiffies(50));
		mutex_lock(&i2c->lock);
		status = xiic_getreg32(i2c, XIIC_SR_REG_OFFSET);
		mutex_unlock(&i2c->lock);
		/* AXI IIC as an transceiver
		 * if ever an XIIC_INTR_TX_ERROR_MASK interrupt happens, means
		 * no such i2c device
		 */
		if (err != 0)
			err = 0;
		else
			err = -ETIMEDOUT;
	} else {
		if (mask & XIIC_SR_TX_FIFO_EMPTY_MASK) {
			err = ocores_wait(i2c, XIIC_SR_REG_OFFSET, mask,
					  XIIC_SR_TX_FIFO_EMPTY_MASK,
					  msecs_to_jiffies(50));
			mask &= ~XIIC_SR_TX_FIFO_EMPTY_MASK;
		}
		if (err == 0)
			err = ocores_wait(i2c, XIIC_SR_REG_OFFSET, mask, 0,
					  msecs_to_jiffies(50));

		mutex_lock(&i2c->lock);
		status = xiic_getreg32(i2c, XIIC_IISR_OFFSET);
		mutex_unlock(&i2c->lock);

		/* AXI IIC as an transceiver,
		 * if ever an XIIC_INTR_TX_ERROR_MASK interrupt happens, return
		 */
		if ((status & XIIC_INTR_ARB_LOST_MASK) ||
		    ((status & XIIC_INTR_TX_ERROR_MASK) &&
		    !(i2c->msg->flags & I2C_M_RD))) {

			err = -ETIMEDOUT;
			/* Soft reset IIC controller. */
			xiic_setreg32(i2c, XIIC_RESETR_OFFSET, XIIC_RESET_MASK);
			if (status & XIIC_INTR_ARB_LOST_MASK)
				dev_dbg(i2c->adap.dev.parent,
					 "%s: TRANSFER STATUS WARNING, ISR: bit 0x%x happens\n",
					 __func__, XIIC_INTR_ARB_LOST_MASK);
			if (status & XIIC_INTR_TX_ERROR_MASK)
				dev_dbg(i2c->adap.dev.parent,
					 "%s: TRANSFER STATUS WARNING, ISR: bit 0x%x happens\n",
					 __func__, XIIC_INTR_TX_ERROR_MASK);
			return err;
		}
	}

	if (err)
		dev_dbg(i2c->adap.dev.parent,
			 "%s: STATUS timeout, bit 0x%x did not clear in 50ms\n",
			 __func__, mask);

	return err;
}

static void ocores_process_polling(struct xiic_i2c *i2c)
{
	while (1) {
		int err;

		err = ocores_poll_wait(i2c);
		if (err) {
			i2c->state = STATE_ERROR;
			break;
		} else if (i2c->state == STATE_DONE)
			break;

		ocores_process(i2c);
	}
}

static int ocores_xfer_core(struct xiic_i2c *i2c,
				struct i2c_msg *msgs, int num)
{
	int ret = 0;

	mutex_lock(&i2c->lock);
	/* Soft reset IIC controller. */
	xiic_setreg32(i2c, XIIC_RESETR_OFFSET, XIIC_RESET_MASK);
	/* Set receive Fifo depth to maximum (zero based). */
	xiic_setreg8(i2c, XIIC_RFD_REG_OFFSET, IIC_RX_FIFO_DEPTH - 1);

	/* Reset Tx Fifo. */
	xiic_setreg8(i2c, XIIC_CR_REG_OFFSET, XIIC_CR_TX_FIFO_RESET_MASK);

	/* Enable IIC Device, remove Tx Fifo reset & disable general call. */
	xiic_setreg8(i2c, XIIC_CR_REG_OFFSET, XIIC_CR_ENABLE_DEVICE_MASK);

	/* set i2c clock as 100Hz. */
	//xiic_setreg8(i2c, 0x13c, 0x7C);

	/* make sure RX fifo is empty */
	ret = xiic_clear_rx_fifo(i2c);
	if (ret){
		mutex_unlock(&i2c->lock);
		return ret;
	}

	i2c->msg = msgs;
	i2c->pos = 0;
	i2c->nmsgs = num;
	i2c->state = STATE_START;

	dev_dbg(&i2c->adap.dev, "STATE: %d\n", i2c->state);

	if (msgs->len == 0 && num == 1) { /* suit for i2cdetect time sequence */
		u8 status = xiic_getreg32(i2c, XIIC_IISR_OFFSET);

		xiic_irq_clr(i2c, status);
		/* send out the 1st byte data and stop bit */
		xiic_setreg16(i2c, XIIC_DTR_REG_OFFSET,
						i2c_8bit_addr_from_msg(msgs)
						| XIIC_TX_DYN_START_MASK
						| XIIC_TX_DYN_STOP_MASK);
	} else
		/* send out the 1st byte data */
		xiic_setreg16(i2c, XIIC_DTR_REG_OFFSET,
						i2c_8bit_addr_from_msg(msgs)
						| XIIC_TX_DYN_START_MASK);

	mutex_unlock(&i2c->lock);
	ocores_process_polling(i2c);

	return (i2c->state == STATE_DONE) ? num : -EIO;
}

/**
 * interface for host computer calling on
 * @adapter: ocores I2C adapter
 * @msgs   : messages to send out
 * @num	: numbers of messages to send out
 * set iic working mode, call xiic_start_xfer to receive and send messages,
 * wait for interrupt process waiting up queue and return process status.
 *
 * Return: 0 on bus idle, non 0 on bus busy
 */

static int xiic_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	struct xiic_i2c *i2c = i2c_get_adapdata(adap);
	int err = -EIO;
	int retry = 0;
	int max_retry = 5;

	dev_dbg(adap->dev.parent, "%s entry SR: 0x%x\n", __func__,
		xiic_getreg8(i2c, XIIC_SR_REG_OFFSET));

	/*
	 * quick to respond i2cdetect command, so not retry here,
	 * I2C_SMBUS_QUICK or I2C_SMBUS_BYTE
	 */
	if (num == 1 && msgs->len == 0 && !(msgs->flags & I2C_M_RD))
		max_retry = 1;
	else if (num == 1 && msgs->len == 1 && (msgs->flags & I2C_M_RD))
		max_retry = 1;

	while ((err == -EIO) && (retry < max_retry)) {
		err = ocores_xfer_core(i2c, msgs, num);
		retry++;
	}

	return err;
}

static u32 xiic_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C |
	       I2C_FUNC_SMBUS_EMUL |
	       I2C_FUNC_SMBUS_BLOCK_DATA;
}

static const struct i2c_algorithm xiic_algorithm = {
	.master_xfer = xiic_xfer,
	.functionality = xiic_func,
};

static const struct i2c_adapter xiic_adapter = {
	.owner = THIS_MODULE,
	.name = DRIVER_NAME,
	.class = I2C_CLASS_DEPRECATED,
	.algo = &xiic_algorithm,
};

static int xiic_i2c_probe(struct platform_device *pdev)
{
	struct xiic_i2c *i2c;
	struct resource *res;
	int ret;
	u32 sr;

	dev_dbg(&pdev->dev, "probe xiic i2c\n");
	i2c = devm_kzalloc(&pdev->dev, sizeof(*i2c), GFP_KERNEL);
	if (!i2c)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	i2c->base = devm_ioremap_resource(&pdev->dev, res);
	dev_dbg(&pdev->dev, "bus id: %d, i2c->base: %p\n", pdev->id, i2c->base);

	if (IS_ERR(i2c->base))
		return PTR_ERR(i2c->base);

	/* hook up driver to tree */
	platform_set_drvdata(pdev, i2c);
	i2c->adap = xiic_adapter;

	/*
	 *set pdev id and transfer it to interrupt base
	 *using specific formula in isr function
	 */
	i2c->id = pdev->id;

	/* pass some flags or parameters to xiic_xfer to deal with*/
	i2c_set_adapdata(&i2c->adap, i2c);

	i2c->adap.dev.parent = &pdev->dev;
	i2c->adap.dev.of_node = pdev->dev.of_node;

	spin_lock_init(&i2c->process_lock);
	mutex_init(&i2c->lock);
	init_waitqueue_head(&i2c->wait);

	i2c->dev = &pdev->dev;
	pm_runtime_set_autosuspend_delay(i2c->dev, XIIC_PM_TIMEOUT);
	pm_runtime_use_autosuspend(i2c->dev);
	pm_runtime_set_active(i2c->dev);
	pm_runtime_enable(i2c->dev);

	ret = xiic_reinit(i2c);
	if (ret < 0) {
		dev_err(&pdev->dev, "Cannot xiic_reinit\n");
		goto err_clk_dis;
	}
	/*
	 * Detect endianness
	 * Try to reset the TX FIFO. Then check the EMPTY flag. If it is not
	 * set, assume that the endianness was wrong and swap.
	 */
	i2c->endianness = LITTLE;
	xiic_setreg32(i2c, XIIC_CR_REG_OFFSET, XIIC_CR_TX_FIFO_RESET_MASK);
	/* Reset is cleared in xiic_reinit */
	sr = xiic_getreg32(i2c, XIIC_SR_REG_OFFSET);
	if (!(sr & XIIC_SR_TX_FIFO_EMPTY_MASK))
		i2c->endianness = BIG;

	/* add i2c adapter to i2c tree */
	i2c->adap.nr = (pdev->id >= 100)? pdev->id:-1;
	ret = i2c_add_numbered_adapter(&i2c->adap);
	if (ret) {
		xiic_deinit(i2c);
		goto err_clk_dis;
	}

	return 0;

err_clk_dis:
	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	return ret;
}

static int xiic_i2c_remove(struct platform_device *pdev)
{
	struct xiic_i2c *i2c = platform_get_drvdata(pdev);
	int ret;

	/* remove adapter & data */
	i2c_del_adapter(&i2c->adap);

	ret = pm_runtime_get_sync(i2c->dev);
	if (ret < 0)
		return ret;

	xiic_deinit(i2c);
	pm_runtime_put_sync(i2c->dev);
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_dont_use_autosuspend(&pdev->dev);

	return 0;
}

#if defined(CONFIG_OF)
static const struct of_device_id xiic_of_match[] = {
	{ .compatible = "xlnx,xps-iic-2.00.a", },
	{},
};
MODULE_DEVICE_TABLE(of, xiic_of_match);
#endif

static struct platform_driver xiic_i2c_driver = {
	.probe   = xiic_i2c_probe,
	.remove  = xiic_i2c_remove,
	.driver  = {
		.name = DRIVER_NAME,
		.of_match_table = of_match_ptr(xiic_of_match),
	},
};

module_platform_driver(xiic_i2c_driver);

MODULE_AUTHOR("info@mocean-labs.com; nicwu@celestica.com");
MODULE_VERSION("0.0.2");
MODULE_DESCRIPTION("Celestica I2C bus driver based on Xilinx I2C bus driver");
MODULE_LICENSE("GPL v2");

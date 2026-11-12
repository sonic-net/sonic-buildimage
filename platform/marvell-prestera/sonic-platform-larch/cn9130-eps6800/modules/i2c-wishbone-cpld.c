/*
 * I2C adapter driver for accton CPLD wishbone
 *
 * Copyright (C) 2023 Accton Technology Corporation.
 * Ang Liu <ang_liu@accton.com.cn>
 *
 * Based on:
 *  i2c-pca-platform.c
 *  i2c-imx.c
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>

#include "accton-cpld.h"
#include "i2c-wishbone-cpld.h"

/* ------ Definitions for i2c master's registers ------ */

#define OC_CPLD_I2C_PRER_HIGH   0x8
#define OC_CPLD_I2C_PRER_LOW    0x9
#define OC_CPLD_I2C_CTR         0xA       /*cpld i2c master control register*/
#define OC_CPLD_I2C_TXR         0xB       /*cpld i2c master Transmit register*/
#define OC_CPLD_I2C_CR          0xC       /*cpld i2c master command register*/
#define OC_CPLD_I2C_CR_EN       0xD       /*cpld i2c master CR en register*/
#define OC_CPLD_I2C_RXR         0xE       /*cpld i2c master revice register*/
#define OC_CPLD_I2C_SR          0xF       /*cpld i2c master status register*/

/* Read-write access */
#define OC_I2C_PRER_LO 0x00                /* Low byte clock prescaler register  */
#define OC_I2C_PRER_HI 0x01                /* High byte clock prescaler register */
#define OC_I2C_CTR     OC_CPLD_I2C_CTR     /* Control register                   */

/* Write-only registers */
#define OC_I2C_TXR     OC_CPLD_I2C_TXR     /* Transmit byte register             */
#define OC_I2C_CR      OC_CPLD_I2C_CR      /* Command register                   */

/* Read-only registers */
#define OC_I2C_RXR     OC_CPLD_I2C_RXR     /* Receive byte register              */
#define OC_I2C_SR      OC_CPLD_I2C_SR      /* Status register                    */

/* ---Bits definition --- */
/* Control register */
#define OC_I2C_EN (1<<7)        /* Core enable bit:                    */
								/*      1 - core is enabled            */
								/*      0 - core is disabled           */
#define OC_I2C_IEN (1<<6)       /* Interrupt enable bit                */
								/*      1 - Interrupt enabled          */
								/*      0 - Interrupt disabled         */
								/* Other bits in CR are reserved       */

/* Command register bits */
#define OC_I2C_STA (1<<7)       /* Generate (repeated) start condition */
#define OC_I2C_STO (1<<6)       /* Generate stop condition             */
#define OC_I2C_RD  (1<<5)       /* Read from slave                     */
#define OC_I2C_WR  (1<<4)       /* Write to slave                      */
#define OC_I2C_ACK (1<<3)       /* when a receiver, sent ACK or NACK   */
								/*      0 - ACK                        */
								/*      1 - NACK                       */
#define OC_I2C_IACK (1<<0)      /* Interrupt acknowledge               */

/* Status register bits */
#define OC_I2C_RXACK (1<<7)     /* Received acknowledge from slave     */
								/*      0 - ACK                        */
								/*      1 - NACK                       */
#define OC_I2C_BUSY  (1<<6)     /* Bus busy                            */
#define OC_I2C_AL    (1<<5)     /* Arbitration lost                    */
#define OC_I2C_TIP   (1<<1)     /* Transfer in progress                */
#define OC_I2C_IF    (1<<0)     /* Interrupt flag                      */

/* bit testing and setting macros */
#define OC_ISSET(reg,bitmask)       ((reg)&(bitmask))
#define OC_ISCLEAR(reg,bitmask)     (!(OC_ISSET(reg,bitmask)))
#define OC_BITSET(reg,bitmask)      ((reg)|(bitmask))
#define OC_BITCLEAR(reg,bitmask)    ((reg)|(~(bitmask)))
#define OC_BITTOGGLE(reg,bitmask)   ((reg)^(bitmask))
#define OC_REGMOVE(reg,value)       ((reg)=(value))

#define WISHBONE_DELAY	usleep_range(50, 100)
#define TRY_COUNT_MAX		1000	/* 50~100 ms */

struct wishbone_i2c {
	u32					reg_base;
	u32					cpld_type;
	u32 				base_nr;
	int					stopped;
	struct i2c_adapter	adap;
};

static int wishbone_i2c_cpld_read(struct wishbone_i2c *wishbone, u8 reg)
{
	int ret;
	struct i2c_adapter *adap = &wishbone->adap;

	reg += wishbone->reg_base;
	ret = accton_cpld_read(wishbone->cpld_type,  reg);
	dev_dbg(&adap->dev, "cpld_type:0x%x, reg:0x%x, ret:%d\n",
		wishbone->cpld_type, reg, ret);

	return ret;
}

static int wishbone_i2c_cpld_write(struct wishbone_i2c *wishbone, u8 reg, u8 val)
{
	int ret;
	struct i2c_adapter *adap = &wishbone->adap;

	reg += wishbone->reg_base;
	ret = accton_cpld_write(wishbone->cpld_type, reg, val);
	dev_dbg(&adap->dev, "cpld_type:0x%x, reg:0x%x, val:0x%x, ret:%d\n",
		wishbone->cpld_type, reg, val, ret);

	return ret;
}

static int wishbone_i2c_init(struct wishbone_i2c *wishbone)
{
	uint32_t val = 0;

	val = OC_I2C_EN;
	return wishbone_i2c_cpld_write(wishbone, OC_I2C_CTR, val);
}

static void wishbone_i2c_deinit(struct wishbone_i2c *wishbone)
{
	uint32_t val = 0;

	val = OC_I2C_STO;
	wishbone_i2c_cpld_write(wishbone, OC_I2C_CR, val);
	val = 0x0;
	wishbone_i2c_cpld_write(wishbone, OC_I2C_CTR, val);
}

static int wishbone_i2c_trx_complete(struct wishbone_i2c *wishbone)
{
	int val = 0;
	uint32_t try_count = 0;

	do
	{
		WISHBONE_DELAY;
		val = wishbone_i2c_cpld_read(wishbone, OC_CPLD_I2C_SR);
		try_count++;
	} while (((val < 0) || ((val & OC_I2C_TIP) != 0x0)) && (try_count < TRY_COUNT_MAX));

	if (try_count >= TRY_COUNT_MAX) {
		dev_dbg(&wishbone->adap.dev, "Timeout\n");
		return -ETIMEDOUT;
	}

	if (val & OC_I2C_AL) {
		dev_dbg(&wishbone->adap.dev, "Arbitration lost\n");
		return -EAGAIN;
	}

	dev_dbg(&wishbone->adap.dev, "TRX complete\n");

	return 0;
}

static int wishbone_i2c_acked(struct wishbone_i2c *wishbone)
{
	int val = 0;

	val = wishbone_i2c_cpld_read(wishbone, OC_CPLD_I2C_SR);
	if ((val & OC_I2C_RXACK) != 0x0) {
		dev_dbg(&wishbone->adap.dev, "No ACK\n");
		return -ENXIO;  /* No ACK */
	}

	dev_dbg(&wishbone->adap.dev, "ACK received\n");
	return 0;
}

static int wishbone_i2c_start(struct wishbone_i2c *wishbone)
{
	int result = 0;

	dev_dbg(&wishbone->adap.dev, "start\n");
	wishbone->stopped = 0;

	return result;
}

static void wishbone_i2c_stop(struct wishbone_i2c *wishbone)
{
	int val = 0;

	if (!wishbone->stopped) {
		/* Stop I2C transaction */
		dev_dbg(&wishbone->adap.dev, "stop\n");
		val = OC_I2C_STO;
		wishbone_i2c_cpld_write(wishbone, OC_CPLD_I2C_CR, val);
		wishbone->stopped = 1;
	}
}

static int wishbone_i2c_write(struct wishbone_i2c *wishbone, struct i2c_msg *msgs)
{
	int i, result, val;

	dev_dbg(&wishbone->adap.dev, 
		"write slave address: addr=0x%x\n", msgs->addr);

	//1.Write slave address
	val = msgs->addr << 1;
	wishbone_i2c_cpld_write(wishbone, OC_CPLD_I2C_TXR, val);

	//2.Generate write start
	val = OC_I2C_WR | OC_I2C_STA;
	wishbone_i2c_cpld_write(wishbone, OC_CPLD_I2C_CR, val);

	//3.Wait for transaction to complete
	result = wishbone_i2c_trx_complete(wishbone);
	if (result)
		return result;
	result = wishbone_i2c_acked(wishbone);
	if (result)
		return result;

	//4.Read clear ACK RXR
	val = wishbone_i2c_cpld_read(wishbone, OC_CPLD_I2C_RXR);

	//5.Write data
	dev_dbg(&wishbone->adap.dev, "write data\n");
	for (i = 0; i < msgs->len; i++) {
		dev_dbg(&wishbone->adap.dev,
			"write byte: B%d=0x%X\n", i, msgs->buf[i]);
		wishbone_i2c_cpld_write(wishbone, OC_CPLD_I2C_TXR, msgs->buf[i]);

		val = OC_I2C_WR;
		wishbone_i2c_cpld_write(wishbone, OC_CPLD_I2C_CR, val);
		result = wishbone_i2c_trx_complete(wishbone);
		if (result)
			return result;
		result = wishbone_i2c_acked(wishbone);
		if (result)
			return result;
		//Read clean
		val = wishbone_i2c_cpld_read(wishbone, OC_CPLD_I2C_RXR);
	}
	return 0;
}

static int wishbone_i2c_read(struct wishbone_i2c *wishbone, struct i2c_msg *msgs, bool is_lastmsg)
{
	int i, result;
	int val;
	int block_data = msgs->flags & I2C_M_RECV_LEN;

	dev_dbg(&wishbone->adap.dev,
		"write slave address: addr=0x%x\n", msgs->addr);

	//1.Write slave address
	val = (msgs->addr << 1) | 0x1;
	wishbone_i2c_cpld_write(wishbone, OC_CPLD_I2C_TXR, val);

	//2.Generate write start
	val = OC_I2C_WR | OC_I2C_STA;
	wishbone_i2c_cpld_write(wishbone, OC_CPLD_I2C_CR, val);

	//3.Wait for transaction to complete
	result = wishbone_i2c_trx_complete(wishbone);
	if (result)
		return result;
	result = wishbone_i2c_acked(wishbone);
	if (result)
		return result;

	//4.Read clean
	val = wishbone_i2c_cpld_read(wishbone, OC_CPLD_I2C_RXR);

	//5.Read data
	dev_dbg(&wishbone->adap.dev, "read data, msgs len=%d\n", msgs->len);
	for (i = 0; i < msgs->len; i++) {
		int len = 0;

		/*
		 * First byte is the length of remaining packet
		 * in the SMBus block data read. Add it to
		 * msgs->len.
		 */
		if ((!i) && block_data) {
			val = OC_I2C_RD;
			wishbone_i2c_cpld_write(wishbone, OC_CPLD_I2C_CR, val);
			result = wishbone_i2c_trx_complete(wishbone);
			if (result)
				return result;
			len = wishbone_i2c_cpld_read(wishbone, OC_CPLD_I2C_RXR);
			if ((len == 0) || (len > I2C_SMBUS_BLOCK_MAX))
				return -EPROTO;

			dev_dbg(&wishbone->adap.dev, "read length: 0x%X\n", len);
			msgs->len += len;
			msgs->buf[0] = len;
		} else {
			if (i == (msgs->len - 1)) {
				if (is_lastmsg) {
					dev_dbg(&wishbone->adap.dev, "is_lastmsg_lastbyte\n");
					val = OC_I2C_RD | OC_I2C_STO | OC_I2C_ACK;
					wishbone->stopped = 1;
				} else {
					dev_dbg(&wishbone->adap.dev, "is_lastbyte\n");
					val = OC_I2C_RD | OC_I2C_ACK;
				}
			} else {
				val = OC_I2C_RD;
			}

			wishbone_i2c_cpld_write(wishbone, OC_CPLD_I2C_CR, val);
			result = wishbone_i2c_trx_complete(wishbone);
			if (result)
				return result;
			msgs->buf[i] = wishbone_i2c_cpld_read(wishbone, OC_CPLD_I2C_RXR);
		}
		dev_dbg(&wishbone->adap.dev,
			"read byte: B%d=0x%X\n", i, msgs->buf[i]);
	}
	return 0;
}

static int wishbone_i2c_xfer(struct i2c_adapter *adap,
						struct i2c_msg *msgs, int num)
{
	unsigned int i;
	int result;
	bool is_lastmsg = false;
	struct wishbone_i2c *wishbone = i2c_get_adapdata(adap);

	dev_dbg(&wishbone->adap.dev, "xfer\n");

	/* Start I2C transfer */
	wishbone_i2c_start(wishbone);

	/* read/write data */
	for (i = 0; i < num; i++) {
		if (i == num - 1)
			is_lastmsg = true;

		dev_dbg(&wishbone->adap.dev, "transfer message: %d\n", i);
		/* write/read data */
#ifdef CONFIG_I2C_DEBUG_BUS
		int val;
		val = wishbone_i2c_cpld_read(wishbone, OC_CPLD_I2C_CR);
		dev_dbg(&wishbone->adap.dev,
			"COMMAND: STA=%d, STO=%d, RD=%d, WR=%d, ACK=%d, IACK=%d\n",
			(val & OC_I2C_STA ? 1 : 0), (val & OC_I2C_STO ? 1 : 0),
			(val & OC_I2C_RD ? 1 : 0), (val & OC_I2C_WR ? 1 : 0),
			(val & OC_I2C_ACK ? 1 : 0), (val & OC_I2C_IACK ? 1 : 0));
		val = wishbone_i2c_cpld_read(wishbone, OC_CPLD_I2C_SR);
		dev_dbg(&wishbone->adap.dev,
			"STATUS: RXACK=%d, BUSY=%d, AL=%d, TIP=%d, IF=%d\n",
			(val & OC_I2C_RXACK ? 1 : 0), (val & OC_I2C_BUSY ? 1 : 0),
			(val & OC_I2C_AL ? 1 : 0), (val & OC_I2C_TIP ? 1 : 0),
			(val & OC_I2C_IF ? 1 : 0));
#endif
		if (msgs[i].flags & I2C_M_RD)
			result = wishbone_i2c_read(wishbone, &msgs[i], is_lastmsg);
		else {
			result = wishbone_i2c_write(wishbone, &msgs[i]);
		}
		if (result)
			goto fail0;
	}

fail0:
	/* Stop I2C transfer */
	wishbone_i2c_stop(wishbone);

	dev_dbg(&wishbone->adap.dev, "exit with: %s: %d\n",
		(result < 0) ? "error" : "success msg",
			(result < 0) ? result : num);
	return (result < 0) ? result : num;
}

static u32 wishbone_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_SMBUS_READ_BLOCK_DATA;
}

static const struct i2c_algorithm wishbone_i2c_algorithm = {
	.master_xfer	= wishbone_i2c_xfer,
	.functionality	= wishbone_i2c_func,
};

static int wishbone_i2c_probe(struct platform_device *pdev)
{
	struct wishbone_i2c *wishbone;
	struct wishbone_i2c_platform_data *platform_data =
				dev_get_platdata(&pdev->dev);
	struct device_node *of_node = pdev->dev.of_node;
	int ret = 0;

	wishbone = devm_kzalloc(&pdev->dev, sizeof(*wishbone), GFP_KERNEL);
	if (!wishbone)
		return -ENOMEM;

	ret = of_property_read_u32(of_node, "reg", &wishbone->reg_base);
	if (ret < 0) {
		dev_err(&pdev->dev, "no reg for node '%s'\n",
			of_node->name);
		goto fail;
	}

	ret = of_property_read_u32(of_node, "base-nr", &wishbone->base_nr);
	if (ret < 0) {
		dev_dbg(&pdev->dev, "no base-nr for node '%s'\n", of_node->name);
	}

	ret = of_property_read_u32(of_node, "cpld-type", &wishbone->cpld_type);
	if (ret < 0) {
		dev_err(&pdev->dev, "no cpld-type for node '%s'\n", of_node->name);
		goto fail;
	}

	i2c_set_adapdata(&wishbone->adap, wishbone);
	wishbone->adap.nr = wishbone->base_nr ? wishbone->base_nr : pdev->id;
	wishbone->adap.owner = THIS_MODULE;
	snprintf(wishbone->adap.name, sizeof(wishbone->adap.name),
		 "%s", pdev->name);
	wishbone->adap.dev.parent = &pdev->dev;
	wishbone->adap.dev.of_node = of_node;
	wishbone->adap.class = I2C_CLASS_DEPRECATED;
	wishbone->adap.algo = &wishbone_i2c_algorithm;
	wishbone->adap.retries = 3;

	if (platform_data) {
		wishbone->adap.timeout = platform_data->timeout;
	} else {
		wishbone->adap.timeout = HZ / 5;
	}

	ret = wishbone_i2c_init(wishbone);
	if (ret) {
		dev_err(&pdev->dev, "init wishbone i2c fail\n");
		ret = -EBUSY;
		goto fail;
	}

	ret = i2c_add_numbered_adapter(&wishbone->adap);
	if (ret) {
		dev_err(&pdev->dev, "add wishbone i2c adapter fail\n");
		ret = -EINVAL;
		goto fail;
	}

	platform_set_drvdata(pdev, wishbone);

	dev_info(&pdev->dev, "wishbone i2c registered.\n");
	return 0;

fail:
	return ret;
}

static void wishbone_i2c_remove(struct platform_device *pdev)
{
	struct wishbone_i2c *wishbone = platform_get_drvdata(pdev);

	wishbone_i2c_deinit(wishbone);
	i2c_del_adapter(&wishbone->adap);
}

#ifdef CONFIG_OF
static const struct of_device_id i2c_pca_of_match_table[] = {
	{ .compatible = "accton,wishbone-i2c" },
	{},
};
MODULE_DEVICE_TABLE(of, i2c_pca_of_match_table);
#endif

static struct platform_driver wishbone_i2c_driver = {
	.probe = wishbone_i2c_probe,
	.remove = wishbone_i2c_remove,
	.driver = {
		.name = "wishbone-i2c",
		.of_match_table = of_match_ptr(i2c_pca_of_match_table),
	},
};

module_platform_driver(wishbone_i2c_driver);

MODULE_AUTHOR("Ang Liu <ang_liu@accton.com.cn>");
MODULE_DESCRIPTION("Accton CPLD wishbone I2C adapter driver");
MODULE_LICENSE("GPL v2");

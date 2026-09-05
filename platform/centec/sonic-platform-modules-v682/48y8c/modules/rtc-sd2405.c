/*
 * rtc class driver for the SD2405 chip
 *
 * Author: Dale Farnsworth <dale@farnsworth.org>
 *
 * based on previously existing rtc class drivers
 *
 * 2007 (c) MontaVista, Software, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/bcd.h>
#include <linux/rtc.h>
#include <linux/delay.h>

#define DRV_VERSION "0.1"

/*
 * register indices
 */
#define SD2405_REG_SC			0x0	    /* seconds      00-59 */
#define SD2405_REG_MN			0x1	    /* minutes      00-59 */
#define SD2405_REG_HR			0x2	    /* hours        00-23 */
#define SD2405_REG_DW			0x3	    /* day of week   1-7  */
#define SD2405_REG_DT			0x4	    /* day of month 00-31 */
#define SD2405_REG_MO			0x5	    /* month        01-12 */
#define SD2405_REG_YR			0x6	    /* year         00-99 */

#define SD2405_REG_CTRL1		0xf	    /* control 1 */
#define SD2405_REG_CTRL2		0x10	/* control 2 */
#define SD2405_REG_CTRL3		0x11	/* control 3 ARST*/

#define SD2405_REG_LEN			7

/*
 * register write protect
 */
#define SD2405_REG_CONTROL1_WRITE	0x80
#define SD2405_REG_CONTROL2_WRITE	0x84


#define SD2405_IDLE_TIME_AFTER_WRITE	3	/* specification says 2.5 mS */

static struct i2c_driver sd2405_driver;

#if 1
/* modified by shil, for bug 46153 */
static int sd2405_i2c_read_regs(struct i2c_client *client, u8 *buf)
{
	int i;

	for (i = 0; i < SD2405_REG_LEN; i++) {
		buf[i] = i2c_smbus_read_byte_data(client, SD2405_REG_SC+i);
	}
	return 0;
}

/* modified by shil, for bug 46153 */
static int sd2405_i2c_write_regs(struct i2c_client *client, u8 const *buf)
{
	int i;

	for (i = 0; i < SD2405_REG_LEN; i++) { 
		i2c_smbus_write_byte_data(client, SD2405_REG_SC+i, buf[i]);
		msleep(SD2405_IDLE_TIME_AFTER_WRITE);
	}
	return 0;
}

#else
static int sd2405_i2c_read_regs(struct i2c_client *client, u8 *buf)
{
	struct i2c_msg msgs[1] = {
		{
		 .addr = client->addr,
		 .flags = I2C_M_RD,	/* read */
		 .len = SD2405_REG_LEN,
		 .buf = buf}
	};
	int rc;

	rc = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (rc != ARRAY_SIZE(msgs)) {
		dev_err(&client->dev, "%s: register read failed\n", __func__);
		return -EIO;
	}
	return 0;
}

static int sd2405_i2c_write_regs(struct i2c_client *client, u8 const *buf)
{
	int rc;
   
    u8 temp_reg[SD2405_REG_LEN+1] = {0};
    memcpy(&temp_reg[1], buf, SD2405_REG_LEN);
    
	struct i2c_msg msgs[1] = {
		{
		 .addr = client->addr,
		 .flags = 0,	/* write */
		 .len = SD2405_REG_LEN+1,
		 .buf = temp_reg}
	};
   
	rc = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (rc != ARRAY_SIZE(msgs))
		goto write_failed;
	msleep(SD2405_IDLE_TIME_AFTER_WRITE);

	return 0;

 write_failed:
	dev_err(&client->dev, "%s: register write failed\n", __func__);
	return -EIO;
}
#endif

static int sd2405_i2c_read_time(struct i2c_client *client, struct rtc_time *tm)
{
	int rc;
	u8 regs[SD2405_REG_LEN];

	rc = sd2405_i2c_read_regs(client, regs);
	if (rc < 0)
		return rc;

	tm->tm_sec = bcd2bin(regs[SD2405_REG_SC]);
	tm->tm_min = bcd2bin(regs[SD2405_REG_MN]);
	tm->tm_hour = bcd2bin(regs[SD2405_REG_HR] & 0x3f);
	tm->tm_wday = bcd2bin(regs[SD2405_REG_DW]);
	tm->tm_mday = bcd2bin(regs[SD2405_REG_DT]);
	tm->tm_mon = bcd2bin(regs[SD2405_REG_MO]) - 1;
	tm->tm_year = bcd2bin(regs[SD2405_REG_YR]) + 100;

	return 0;
}

static int sd2405_i2c_set_write_protect(struct i2c_client *client)
{
	int rc;
	rc = i2c_smbus_write_byte_data(client, SD2405_REG_CTRL1, 0);
	rc += i2c_smbus_write_byte_data(client, SD2405_REG_CTRL2, 0);    
	if (rc < 0) {
		dev_err(&client->dev, "%s: control register write failed\n",
			__func__);
		return -EIO;
	}
	return 0;
}

static int sd2405_i2c_clear_write_protect(struct i2c_client *client)
{
	int rc;
	rc = i2c_smbus_write_byte_data(client, SD2405_REG_CTRL2, SD2405_REG_CONTROL1_WRITE);    
	rc += i2c_smbus_write_byte_data(client, SD2405_REG_CTRL1, SD2405_REG_CONTROL2_WRITE);
	if (rc < 0) {
		dev_err(&client->dev, "%s: control register write failed\n",
			__func__);
		return -EIO;
	}
	return 0;
}

static int
sd2405_i2c_set_time(struct i2c_client *client, struct rtc_time const *tm)
{
	u8 regs[SD2405_REG_LEN];
	int rc;

	rc = sd2405_i2c_clear_write_protect(client);
	if (rc < 0)
		return rc;

	regs[SD2405_REG_SC] = bin2bcd(tm->tm_sec);
	regs[SD2405_REG_MN] = bin2bcd(tm->tm_min);
	regs[SD2405_REG_HR] = bin2bcd(tm->tm_hour)|0x80;
	regs[SD2405_REG_DW] = bin2bcd(tm->tm_wday);
	regs[SD2405_REG_DT] = bin2bcd(tm->tm_mday);
	regs[SD2405_REG_MO] = bin2bcd(tm->tm_mon + 1);
	regs[SD2405_REG_YR] = bin2bcd(tm->tm_year - 100);

	rc = sd2405_i2c_write_regs(client, regs);
	if (rc < 0)
		return rc;
    
    rc = sd2405_i2c_set_write_protect(client);
	if (rc < 0)
		return rc;
    
	return 0;
}

static int sd2405_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	return sd2405_i2c_read_time(to_i2c_client(dev), tm);
}

static int sd2405_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	return sd2405_i2c_set_time(to_i2c_client(dev), tm);
}

static const struct rtc_class_ops sd2405_rtc_ops = {
	.read_time = sd2405_rtc_read_time,
	.set_time = sd2405_rtc_set_time,
};

static int
sd2405_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct rtc_device *rtc;

/* modified by shil, for bug 46153 */
#if 0
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -ENODEV;
#endif

	dev_info(&client->dev, "chip found, driver version " DRV_VERSION "\n");

	rtc = devm_rtc_allocate_device(&client->dev);
	if (IS_ERR(rtc))
		return PTR_ERR(rtc);

	i2c_set_clientdata(client, rtc);
	rtc->ops = &sd2405_rtc_ops;

	return devm_rtc_register_device(rtc);
}

static struct i2c_device_id sd2405_id[] = {
	{ "sd2405", 0 },
	{ }
};

static struct i2c_driver sd2405_driver = {
	.driver = {
		   .name = "rtc-sd2405",
		   },
	.probe = sd2405_probe,
	.id_table = sd2405_id,
};

static int __init sd2405_init(void)
{
	return i2c_add_driver(&sd2405_driver);
}

static void __exit sd2405_exit(void)
{
	i2c_del_driver(&sd2405_driver);
}

MODULE_DESCRIPTION("Maxim SD2405 RTC driver");
MODULE_AUTHOR("Dale Farnsworth <dale@farnsworth.org>");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

module_init(sd2405_init);
module_exit(sd2405_exit);

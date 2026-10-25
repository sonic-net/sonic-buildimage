#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include "pddf_psu_defs.h"

#define PSU_REG_VOUT_MODE 0x20
#define PSU_REG_READ_VOUT 0x8b

ssize_t pddf_show_custom_psu_v_out(struct device *dev, struct device_attribute *da, char *buf);
extern PSU_SYSFS_ATTR_DATA access_psu_v_out;

static int two_complement_to_int(u16 data, u8 valid_bit, int mask)
{
	u16  valid_data  = data & mask;
	bool is_negative = valid_data >> (valid_bit - 1);

	return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static u8 psu_get_vout_mode(struct i2c_client *client)
{
	u8 status = 0, retry = 10;
	uint8_t offset = PSU_REG_VOUT_MODE;

	while (retry) {
		status = i2c_smbus_read_byte_data((struct i2c_client *)client, offset);
		if (unlikely(status < 0)) {
			msleep(60);
			retry--;
			continue;
		}
		break;
	}

	if (status < 0)
	{
		printk(KERN_ERR "%s: Get PSU Vout mode failed\n", __func__);
		return 0;
	}
	else
	{
		/*printk(KERN_ERR "%s: vout_mode reg value 0x%x\n", __func__, status);*/
		return status;
	}
}

static u16 psu_get_v_out(struct i2c_client *client)
{
	u16 status = 0, retry = 10;
	uint8_t offset = PSU_REG_READ_VOUT;

	while (retry) {
		status = i2c_smbus_read_word_data((struct i2c_client *)client, offset);
		if (unlikely(status < 0)) {
			msleep(60);
			retry--;
			continue;
		}
		break;
	}

	if (status < 0)
	{
		printk(KERN_ERR "%s: Get PSU Vout failed\n", __func__);
		return 0;
	}
	else
	{
		/*printk(KERN_ERR "%s: vout reg value 0x%x\n", __func__, status);*/
		return status;
	}
}

ssize_t pddf_show_custom_psu_v_out(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	int exponent, mantissa;
	int multiplier = 1000;

	u16 value = psu_get_v_out(client);
	u8 vout_mode = psu_get_vout_mode(client);

	if ((vout_mode >> 5) == 0)
		exponent = two_complement_to_int(vout_mode & 0x1f, 5, 0x1f);
	else
	{
        /*printk(KERN_ERR "%s: Only support linear mode for vout mode\n", __func__);*/
        exponent = 0;
	}
	mantissa = value;
	if (exponent >= 0)
		return sprintf(buf, "%d\n", (mantissa << exponent) * multiplier);
	else
		return sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}



static int __init pddf_custom_psu_init(void)
{
	access_psu_v_out.show = pddf_show_custom_psu_v_out;
	access_psu_v_out.do_get = NULL;
    return 0;
}

static void __exit pddf_custom_psu_exit(void)
{
    return;
}

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("pddf custom psu api");
MODULE_LICENSE("GPL");

module_init(pddf_custom_psu_init);
module_exit(pddf_custom_psu_exit);

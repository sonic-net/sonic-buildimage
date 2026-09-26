// SPDX-License-Identifier: GPL-2.0-or-later

/* Copyright (C) 2026 Nexthop Systems Inc.
 *
 * Driver for the TDK FS160x family of uPOL DC-DC converters with integrated
 * inductor. Supported models: FS1603, FS1604, FS1606, FS1006.
 *
 * The parts expose voltage, current and temperature telemetry over a
 * proprietary I2C register map. They are not PMBus devices, and their
 * register numbers actively collide with PMBus command codes -- 0x21 is a
 * status byte here where PMBus defines VOUT_COMMAND -- so a generic PMBus
 * driver must never be bound to them.
 *
 * Read-only. VOUT_SET (0x12/0x13) is an atomic write-protected pair: writing
 * the high byte arms an update that takes effect only once the low byte is
 * written. A partial write leaves the part armed, after which it acknowledges
 * but silently ignores every register write until its input power is cycled,
 * so this driver does not expose it.
 *
 * The reported output voltage is the converter's own output. Where a board uses
 * a feedback divider to reach a higher rail voltage, TDK directs readers to the
 * Fb register (0x22) scaled by target VOUT/0.6 instead, and notes that current
 * reporting does not apply in that configuration. Neither is implemented here.
 */

#include <linux/bitfield.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/regmap.h>

/* clang-format off */
#define FS160X_REG_PVIN			0x0C
#define FS160X_REG_VOUT			0x0D
#define FS160X_REG_IOUT			0x0E
#define FS160X_REG_TEMP			0x0F
#define FS160X_REG_OCSET		0x15
#define FS160X_REG_IOUT_GAIN		0x1A
#define FS160X_REG_STATUS		0x21

/* OCSet[2:0] encodes which member of the family the part is. It is the only
 * register that identifies the model, so it doubles as a presence check.
 */
#define FS160X_OCSET_MODEL		GENMASK(2, 0)
#define FS160X_MODEL_FS1603		0
#define FS160X_MODEL_FS1604		1
#define FS160X_MODEL_FS1606		2
#define FS160X_MODEL_FS1006		3

/* Bits in FS160X_REG_STATUS. */
#define FS160X_STATUS_POWER_GOOD	BIT(7)
#define FS160X_STATUS_OVER_VOLTAGE	BIT(6)
#define FS160X_STATUS_OVER_CURRENT	BIT(5)
#define FS160X_STATUS_OVER_TEMP		BIT(4)
/* clang-format on */

/* TEMP_REPORT is the temperature in degC directly, 1 degC per LSB and no
 * offset (TDK FS160x register map: Temperature = decimal(Temp_report[7:0])).
 */

/* Voltage telemetry is a linear step and base, both fixed per variant.
 * Current is raw/32 A except on the FS1606-0600, which applies a calibration
 * correction derived from FS160X_REG_IOUT_GAIN and the output voltage.
 */
struct fs160x_info {
	unsigned int vout_step_mv;
	unsigned int vout_base_mv;
	u8 model;
	bool iout_gain_corrected;
};

enum fs160x_model {
	fs1603,
	fs1604,
	fs1606,
	fs1606_0600,
	fs1606_2500,
	fs1006,
};

/* The FS1606 reports at 10mV/300mV except for the -2500 preset, which uses the
 * same 20mV/600mV encoding as the higher-voltage family members.
 */
/* clang-format off */
static const struct fs160x_info fs160x_models[] = {
	[fs1603]      = { .vout_step_mv = 20, .vout_base_mv = 600,
			  .model = FS160X_MODEL_FS1603 },
	[fs1604]      = { .vout_step_mv = 20, .vout_base_mv = 600,
			  .model = FS160X_MODEL_FS1604 },
	[fs1606]      = { .vout_step_mv = 10, .vout_base_mv = 300,
			  .model = FS160X_MODEL_FS1606 },
	[fs1606_0600] = { .vout_step_mv = 10, .vout_base_mv = 300,
			  .model = FS160X_MODEL_FS1606,
			  .iout_gain_corrected = true },
	[fs1606_2500] = { .vout_step_mv = 20, .vout_base_mv = 600,
			  .model = FS160X_MODEL_FS1606 },
	[fs1006]      = { .vout_step_mv = 20, .vout_base_mv = 600,
			  .model = FS160X_MODEL_FS1006 },
};

/* clang-format on */

/* Every register is a byte, and every read is live telemetry, so nothing is
 * cached. regmap is used for the uniform accessors and the debugfs dump.
 */
static const struct regmap_config fs160x_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = FS160X_REG_STATUS,
	.cache_type = REGCACHE_NONE,
};

struct fs160x_data {
	struct regmap *regmap;
	const struct fs160x_info *info;
};

static int fs160x_read_vout_mv(struct fs160x_data *data, long *val)
{
	unsigned int raw;
	int err = regmap_read(data->regmap, FS160X_REG_VOUT, &raw);

	if (err)
		return err;

	*val = raw * data->info->vout_step_mv + data->info->vout_base_mv;
	return 0;
}

/* Output current in mA.
 *
 * Every variant but the FS1606-0600 reports raw/32 A directly. The -0600
 * additionally applies, with G taken from FS160X_REG_IOUT_GAIN bits [7:2]:
 *
 *   I = raw/32 - (9.05 - 0.24 * G) * Vout - 0.356 * G + 13.1
 *
 * Scaled to milliamps and millivolts so it stays in integer arithmetic:
 *
 *   I_mA = raw * 125 / 4
 *          - ((9050 - 240 * G) * Vout_mV) / 1000
 *          - 356 * G
 *          + 13100
 */
static int fs160x_read_iout_ma(struct fs160x_data *data, long *val)
{
	unsigned int raw, gain;
	long vout_mv;
	int err;

	err = regmap_read(data->regmap, FS160X_REG_IOUT, &raw);
	if (err)
		return err;

	*val = raw * 125 / 4;

	if (!data->info->iout_gain_corrected)
		return 0;

	err = regmap_read(data->regmap, FS160X_REG_IOUT_GAIN, &gain);
	if (err)
		return err;
	gain = (gain >> 2) & 0x3F;

	err = fs160x_read_vout_mv(data, &vout_mv);
	if (err)
		return err;

	*val -= ((9050 - 240 * (long)gain) * vout_mv) / 1000;
	*val -= 356 * (long)gain;
	*val += 13100;

	if (*val < 0)
		*val = 0;

	return 0;
}

static int fs160x_read_status_bit(struct fs160x_data *data, u8 mask,
				  bool active_low, long *val)
{
	unsigned int status;
	int err = regmap_read(data->regmap, FS160X_REG_STATUS, &status);

	if (err)
		return err;

	*val = !!(status & mask);
	if (active_low)
		*val = !*val;

	return 0;
}

static int fs160x_read_in(struct fs160x_data *data, u32 attr, int channel,
			  long *val)
{
	unsigned int raw;
	int err;

	switch (attr) {
	case hwmon_in_input:
		if (channel == 0) {
			err = regmap_read(data->regmap, FS160X_REG_PVIN, &raw);
			if (err)
				return err;
			/* PVIN reports in 1/16 V steps. */
			*val = raw * 1000 / 16;
			return 0;
		}
		return fs160x_read_vout_mv(data, val);
	case hwmon_in_alarm:
		return fs160x_read_status_bit(data, FS160X_STATUS_OVER_VOLTAGE,
					      false, val);
	case hwmon_in_lcrit_alarm:
		/* Power good is asserted high, so the alarm is its inverse. It
		 * reflects real-time status rather than latching, and also
		 * deasserts while the part is disabled or soft-starting, so a
		 * deliberately disabled rail reads as a fault.
		 */
		return fs160x_read_status_bit(data, FS160X_STATUS_POWER_GOOD,
					      true, val);
	default:
		return -EOPNOTSUPP;
	}
}

static int fs160x_read_curr(struct fs160x_data *data, u32 attr, long *val)
{
	switch (attr) {
	case hwmon_curr_input:
		return fs160x_read_iout_ma(data, val);
	case hwmon_curr_alarm:
		return fs160x_read_status_bit(data, FS160X_STATUS_OVER_CURRENT,
					      false, val);
	default:
		return -EOPNOTSUPP;
	}
}

static int fs160x_read_temp(struct fs160x_data *data, u32 attr, long *val)
{
	unsigned int raw;
	int err;

	switch (attr) {
	case hwmon_temp_input:
		err = regmap_read(data->regmap, FS160X_REG_TEMP, &raw);
		if (err)
			return err;
		*val = raw * 1000;
		return 0;
	case hwmon_temp_alarm:
		return fs160x_read_status_bit(data, FS160X_STATUS_OVER_TEMP,
					      false, val);
	default:
		return -EOPNOTSUPP;
	}
}

static int fs160x_read(struct device *dev, enum hwmon_sensor_types type,
		       u32 attr, int channel, long *val)
{
	struct fs160x_data *data = dev_get_drvdata(dev);
	int err;

	switch (type) {
	case hwmon_in:
		err = fs160x_read_in(data, attr, channel, val);
		break;
	case hwmon_curr:
		err = fs160x_read_curr(data, attr, val);
		break;
	case hwmon_temp:
		err = fs160x_read_temp(data, attr, val);
		break;
	default:
		err = -EOPNOTSUPP;
		break;
	}

	return err;
}

static int fs160x_read_string(struct device *dev, enum hwmon_sensor_types type,
			      u32 attr, int channel, const char **str)
{
	if (type != hwmon_in || attr != hwmon_in_label)
		return -EOPNOTSUPP;

	*str = channel == 0 ? "vin" : "vout";
	return 0;
}

static umode_t fs160x_is_visible(const void *data, enum hwmon_sensor_types type,
				 u32 attr, int channel)
{
	switch (type) {
	case hwmon_in:
		switch (attr) {
		case hwmon_in_input:
		case hwmon_in_label:
			return 0444;
		case hwmon_in_alarm:
		case hwmon_in_lcrit_alarm:
			/* Both alarms describe the regulated output only. */
			return channel == 1 ? 0444 : 0;
		default:
			return 0;
		}
	case hwmon_curr:
	case hwmon_temp:
		return 0444;
	default:
		return 0;
	}
}

static const struct hwmon_ops fs160x_ops = {
	.is_visible = fs160x_is_visible,
	.read = fs160x_read,
	.read_string = fs160x_read_string,
};

/* clang-format off */
static const struct hwmon_channel_info *fs160x_info[] = {
	HWMON_CHANNEL_INFO(in, HWMON_I_INPUT | HWMON_I_LABEL,
			   HWMON_I_INPUT | HWMON_I_LABEL | HWMON_I_ALARM |
				   HWMON_I_LCRIT_ALARM),
	HWMON_CHANNEL_INFO(curr, HWMON_C_INPUT | HWMON_C_ALARM),
	HWMON_CHANNEL_INFO(temp, HWMON_T_INPUT | HWMON_T_ALARM),
	NULL
};

/* clang-format on */

static const struct hwmon_chip_info fs160x_chip_info = {
	.ops = &fs160x_ops,
	.info = fs160x_info,
};

static int fs160x_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct fs160x_data *data;
	struct device *hwmon;
	unsigned int ocset;
	u8 model;
	int err;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return dev_err_probe(dev, -ENODEV,
				     "SMBus byte data not supported\n");

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->regmap = devm_regmap_init_i2c(client, &fs160x_regmap_config);
	if (IS_ERR(data->regmap))
		return dev_err_probe(dev, PTR_ERR(data->regmap),
				     "failed to allocate regmap\n");

	data->info = i2c_get_match_data(client);
	if (!data->info)
		return dev_err_probe(dev, -ENODEV, "no match data for %s\n",
				     client->name);

	err = regmap_read(data->regmap, FS160X_REG_OCSET, &ocset);
	if (err) {
		if (err == -EIO || err == -EAGAIN)
			err = -EPROBE_DEFER;
		return dev_err_probe(dev, err, "failed to read OCSet\n");
	}

	model = FIELD_GET(FS160X_OCSET_MODEL, ocset);
	if (model != data->info->model)
		return dev_err_probe(dev, -ENODEV,
				     "declared %s, reports model %u not %u\n",
				     client->name, model, data->info->model);

	hwmon = devm_hwmon_device_register_with_info(dev, "fs160x", data,
						     &fs160x_chip_info, NULL);
	return PTR_ERR_OR_ZERO(hwmon);
}

static const struct i2c_device_id fs160x_id[] = {
	{ "fs1603", (kernel_ulong_t)&fs160x_models[fs1603] },
	{ "fs1604", (kernel_ulong_t)&fs160x_models[fs1604] },
	{ "fs1606", (kernel_ulong_t)&fs160x_models[fs1606] },
	{ "fs1606_0600", (kernel_ulong_t)&fs160x_models[fs1606_0600] },
	{ "fs1606_2500", (kernel_ulong_t)&fs160x_models[fs1606_2500] },
	{ "fs1006_5000", (kernel_ulong_t)&fs160x_models[fs1006] },
	{ "fs1006_3300", (kernel_ulong_t)&fs160x_models[fs1006] },
	{}
};
MODULE_DEVICE_TABLE(i2c, fs160x_id);

static const struct of_device_id fs160x_of_match[] = {
	{ .compatible = "tdk,fs1603", .data = &fs160x_models[fs1603] },
	{ .compatible = "tdk,fs1604", .data = &fs160x_models[fs1604] },
	{ .compatible = "tdk,fs1606", .data = &fs160x_models[fs1606] },
	{ .compatible = "tdk,fs1606-0600",
	  .data = &fs160x_models[fs1606_0600] },
	{ .compatible = "tdk,fs1606-2500",
	  .data = &fs160x_models[fs1606_2500] },
	{ .compatible = "tdk,fs1006-5000",
	  .data = &fs160x_models[fs1006] },
	{ .compatible = "tdk,fs1006-3300",
	  .data = &fs160x_models[fs1006] },
	{}
};
MODULE_DEVICE_TABLE(of, fs160x_of_match);

static struct i2c_driver fs160x_driver = {
	.driver = {
		.name	= "fs160x",
		.of_match_table = fs160x_of_match,
	},
	.probe = fs160x_probe,
	.id_table = fs160x_id,
};
module_i2c_driver(fs160x_driver);

MODULE_AUTHOR("Kevin Jia <kjia@nexthop.ai>");
MODULE_DESCRIPTION("TDK FS160x uPOL DC-DC converter telemetry driver");
MODULE_LICENSE("GPL");

#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/log2.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>


#define READ_VIN		0x88
#define READ_IIN		0x89
#define READ_TEMPERATURE_PEAK	0xd6
#define MFR_WRITE_PROTECT	0xF8
#define DISABLE_PROTECT		0xA2
#define ENABLE_PROTECT		0x00
#define EFUSE_OUTPUT_DISABLE	0x00
#define EFUSE_OUTPUT_ENABLE	0x80

enum pmbus_regs {
	PMBUS_PAGE                      = 0x00,
	PMBUS_OPERATION                 = 0x01,
	PMBUS_ON_OFF_CONFIG             = 0x02,
	PMBUS_CLEAR_FAULTS              = 0x03,
	PMBUS_PHASE                     = 0x04,

	PMBUS_CAPABILITY                = 0x19,
	PMBUS_QUERY                     = 0x1A,

	PMBUS_VOUT_MODE                 = 0x20,
	PMBUS_VOUT_COMMAND              = 0x21,
	PMBUS_VOUT_TRIM                 = 0x22,
	PMBUS_VOUT_CAL_OFFSET           = 0x23,
	PMBUS_VOUT_MAX                  = 0x24,
	PMBUS_VOUT_MARGIN_HIGH          = 0x25,
	PMBUS_VOUT_MARGIN_LOW           = 0x26,
	PMBUS_VOUT_TRANSITION_RATE      = 0x27,
	PMBUS_VOUT_DROOP                = 0x28,
	PMBUS_VOUT_SCALE_LOOP           = 0x29,
	PMBUS_VOUT_SCALE_MONITOR        = 0x2A,

	PMBUS_COEFFICIENTS              = 0x30,
	PMBUS_POUT_MAX                  = 0x31,

	PMBUS_FAN_CONFIG_12             = 0x3A,
	PMBUS_FAN_COMMAND_1             = 0x3B,
	PMBUS_FAN_COMMAND_2             = 0x3C,
	PMBUS_FAN_CONFIG_34             = 0x3D,
	PMBUS_FAN_COMMAND_3             = 0x3E,
	PMBUS_FAN_COMMAND_4             = 0x3F,

	PMBUS_VOUT_OV_FAULT_LIMIT       = 0x40,
	PMBUS_VOUT_OV_FAULT_RESPONSE    = 0x41,
	PMBUS_VOUT_OV_WARN_LIMIT        = 0x42,
	PMBUS_VOUT_UV_WARN_LIMIT        = 0x43,
	PMBUS_VOUT_UV_FAULT_LIMIT       = 0x44,
	PMBUS_VOUT_UV_FAULT_RESPONSE    = 0x45,
	PMBUS_IOUT_OC_FAULT_LIMIT       = 0x46,
	PMBUS_IOUT_OC_FAULT_RESPONSE    = 0x47,
	PMBUS_IOUT_OC_LV_FAULT_LIMIT    = 0x48,
	PMBUS_IOUT_OC_LV_FAULT_RESPONSE = 0x49,
	PMBUS_IOUT_OC_WARN_LIMIT        = 0x4A,
	PMBUS_IOUT_UC_FAULT_LIMIT       = 0x4B,
	PMBUS_IOUT_UC_FAULT_RESPONSE    = 0x4C,
	PMBUS_OT_FAULT_LIMIT            = 0x4F,
	PMBUS_OT_FAULT_RESPONSE         = 0x50,
	PMBUS_OT_WARN_LIMIT             = 0x51,
	PMBUS_UT_WARN_LIMIT             = 0x52,
	PMBUS_UT_FAULT_LIMIT            = 0x53,
	PMBUS_UT_FAULT_RESPONSE         = 0x54,
	PMBUS_VIN_OV_FAULT_LIMIT        = 0x55,
	PMBUS_VIN_OV_FAULT_RESPONSE     = 0x56,
	PMBUS_VIN_OV_WARN_LIMIT         = 0x57,
	PMBUS_VIN_UV_WARN_LIMIT         = 0x58,
	PMBUS_VIN_UV_FAULT_LIMIT        = 0x59,

	PMBUS_IIN_OC_FAULT_LIMIT        = 0x5B,
	PMBUS_IIN_OC_WARN_LIMIT         = 0x5D,

	PMBUS_POUT_OP_FAULT_LIMIT       = 0x68,
	PMBUS_POUT_OP_WARN_LIMIT        = 0x6A,
	PMBUS_PIN_OP_WARN_LIMIT         = 0x6B,

	PMBUS_STATUS_BYTE               = 0x78,
	PMBUS_STATUS_WORD               = 0x79,
	PMBUS_STATUS_VOUT               = 0x7A,
	PMBUS_STATUS_IOUT               = 0x7B,
	PMBUS_STATUS_INPUT              = 0x7C,
	PMBUS_STATUS_TEMPERATURE        = 0x7D,
	PMBUS_STATUS_CML                = 0x7E,
	PMBUS_STATUS_OTHER              = 0x7F,
	PMBUS_STATUS_MFR_SPECIFIC       = 0x80,
	PMBUS_STATUS_FAN_12             = 0x81,
	PMBUS_STATUS_FAN_34             = 0x82,

	PMBUS_READ_VIN                  = 0x88,
	PMBUS_READ_IIN                  = 0x89,
	PMBUS_READ_VCAP                 = 0x8A,
	PMBUS_READ_VOUT                 = 0x8B,
	PMBUS_READ_IOUT                 = 0x8C,
	PMBUS_READ_TEMPERATURE_1        = 0x8D,
	PMBUS_READ_TEMPERATURE_2        = 0x8E,
	PMBUS_READ_TEMPERATURE_3        = 0x8F,
	PMBUS_READ_FAN_SPEED_1          = 0x90,
	PMBUS_READ_FAN_SPEED_2          = 0x91,
	PMBUS_READ_FAN_SPEED_3          = 0x92,
	PMBUS_READ_FAN_SPEED_4          = 0x93,
	PMBUS_READ_DUTY_CYCLE           = 0x94,
	PMBUS_READ_FREQUENCY            = 0x95,
	PMBUS_READ_POUT                 = 0x96,
	PMBUS_READ_PIN                  = 0x97,

	PMBUS_REVISION                  = 0x98,
	PMBUS_MFR_ID                    = 0x99,
	PMBUS_MFR_MODEL                 = 0x9A,
	PMBUS_MFR_REVISION              = 0x9B,
	PMBUS_MFR_LOCATION              = 0x9C,
	PMBUS_MFR_DATE                  = 0x9D,
	PMBUS_MFR_SERIAL                = 0x9E,
	/*
	 * Virtual registers.
	 * Useful to support attributes which are not supported by standard PMBus
	 * registers but exist as manufacturer specific registers on individual chips.
	 * Must be mapped to real registers in device specific code.
	 *
	 * Semantics:
	 * Virtual registers are all word size.
	 * READ registers are read-only; writes are either ignored or return an error.
	 * RESET registers are read/write. Reading reset registers returns zero
	 * (used for detection), writing any value causes the associated history to be
	 * reset.
	 * Virtual registers have to be handled in device specific driver code. Chip
	 * driver code returns non-negative register values if a virtual register is
	 * supported, or a negative error code if not. The chip driver may return
	 * -ENODATA or any other error code in this case, though an error code other
	 * than -ENODATA is handled more efficiently and thus preferred. Either case,
	 * the calling PMBus core code will abort if the chip driver returns an error
	 * code when reading or writing virtual registers.
	 */
	PMBUS_VIRT_BASE                 = 0x100,
	PMBUS_VIRT_READ_TEMP_AVG,
	PMBUS_VIRT_READ_TEMP_MIN,
	PMBUS_VIRT_READ_TEMP_MAX,
	PMBUS_VIRT_RESET_TEMP_HISTORY,
	PMBUS_VIRT_READ_VIN_AVG,
	PMBUS_VIRT_READ_VIN_MIN,
	PMBUS_VIRT_READ_VIN_MAX,
	PMBUS_VIRT_RESET_VIN_HISTORY,
	PMBUS_VIRT_READ_IIN_AVG,
	PMBUS_VIRT_READ_IIN_MIN,
	PMBUS_VIRT_READ_IIN_MAX,
	PMBUS_VIRT_RESET_IIN_HISTORY,
	PMBUS_VIRT_READ_PIN_AVG,
	PMBUS_VIRT_READ_PIN_MIN,
	PMBUS_VIRT_READ_PIN_MAX,
	PMBUS_VIRT_RESET_PIN_HISTORY,
	PMBUS_VIRT_READ_POUT_AVG,
	PMBUS_VIRT_READ_POUT_MIN,
	PMBUS_VIRT_READ_POUT_MAX,
	PMBUS_VIRT_RESET_POUT_HISTORY,
	PMBUS_VIRT_READ_VOUT_AVG,
	PMBUS_VIRT_READ_VOUT_MIN,
	PMBUS_VIRT_READ_VOUT_MAX,
	PMBUS_VIRT_RESET_VOUT_HISTORY,
	PMBUS_VIRT_READ_IOUT_AVG,
	PMBUS_VIRT_READ_IOUT_MIN,
	PMBUS_VIRT_READ_IOUT_MAX,
	PMBUS_VIRT_RESET_IOUT_HISTORY,
	PMBUS_VIRT_READ_TEMP2_AVG,
	PMBUS_VIRT_READ_TEMP2_MIN,
	PMBUS_VIRT_READ_TEMP2_MAX,
	PMBUS_VIRT_RESET_TEMP2_HISTORY,
	PMBUS_VIRT_READ_VMON,
	PMBUS_VIRT_VMON_UV_WARN_LIMIT,
	PMBUS_VIRT_VMON_OV_WARN_LIMIT,
	PMBUS_VIRT_VMON_UV_FAULT_LIMIT,
	PMBUS_VIRT_VMON_OV_FAULT_LIMIT,
	PMBUS_VIRT_STATUS_VMON,
};

enum pmbus_sensor_classes {
	PSC_VOLTAGE_IN = 0,
	PSC_VOLTAGE_OUT,
	PSC_CURRENT_IN,
	PSC_CURRENT_OUT,
	PSC_POWER,
	PSC_TEMPERATURE,
	PSC_FAN,
	PSC_NUM_CLASSES         /* Number of power sensor classes */
};

/* Functionality bit mask */
#define PMBUS_HAVE_VIN          BIT(0)
#define PMBUS_HAVE_VCAP         BIT(1)
#define PMBUS_HAVE_VOUT         BIT(2)
#define PMBUS_HAVE_IIN          BIT(3)
#define PMBUS_HAVE_IOUT         BIT(4)
#define PMBUS_HAVE_PIN          BIT(5)
#define PMBUS_HAVE_POUT         BIT(6)
#define PMBUS_HAVE_FAN12        BIT(7)
#define PMBUS_HAVE_FAN34        BIT(8)
#define PMBUS_HAVE_TEMP         BIT(9)
#define PMBUS_HAVE_TEMP2        BIT(10)
#define PMBUS_HAVE_TEMP3        BIT(11)
#define PMBUS_HAVE_STATUS_VOUT  BIT(12)
#define PMBUS_HAVE_STATUS_IOUT  BIT(13)
#define PMBUS_HAVE_STATUS_INPUT BIT(14)
#define PMBUS_HAVE_STATUS_TEMP  BIT(15)
#define PMBUS_HAVE_STATUS_FAN12 BIT(16)
#define PMBUS_HAVE_STATUS_FAN34 BIT(17)
#define PMBUS_HAVE_VMON         BIT(18)
#define PMBUS_HAVE_STATUS_VMON  BIT(19)

enum pmbus_data_format { linear = 0, direct, vid };
enum vrm_version { vr11 = 0, vr12, vr13 };

struct pmbus_data {
	struct device *dev;
	struct device *hwmon_dev;

	u32 flags;              /* from platform data */
	struct mutex update_lock;
};

/*static ssize_t show_pec(struct device *dev, struct device_attribute *dummy, char *buf)
  {
  struct i2c_client *client = to_i2c_client(dev);
  return sprintf(buf, "%d\n", !!(client->flags & I2C_CLIENT_PEC));
  }*/
/*
   static ssize_t set_pec(struct device *dev, struct device_attribute *dummy, const char *buf, size_t count)
   {
   struct i2c_client *client = to_i2c_client(dev);
   long val;
   int err;

   err = kstrtol(buf, 10, &val);
   if (err < 0)
   return err;

   if (val != 0)
   client->flags |= I2C_CLIENT_PEC;
   else
   client->flags &= ~I2C_CLIENT_PEC;

   return count;
   }
 */



static ssize_t show_mfr_id(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pmbus_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);
	ret = i2c_smbus_read_block_data(client, PMBUS_MFR_ID, buf);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to read PMBUS_MFR_ID\n");
	}
	mutex_unlock(&data->update_lock);

	if (strncmp("TI", buf, 2) == 0)
	{
		return sprintf(buf, "%s\n", "TI" );
	}
	return -1;
}

static ssize_t show_mfr_model(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pmbus_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);
	ret = i2c_smbus_read_block_data(client, PMBUS_MFR_MODEL, buf);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to read PMBUS_MFR_MODEL\n");
	}
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%s\n", buf);
}

static ssize_t show_capability(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pmbus_data *data = i2c_get_clientdata(client);
	int val;

	mutex_lock(&data->update_lock);
	val = i2c_smbus_read_byte_data(client, PMBUS_CAPABILITY);
	mutex_unlock(&data->update_lock);
	if (val < 0) {
		return val;
	}
	return sprintf(buf, "0x%x\n", val);
}

static ssize_t show_status_word(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pmbus_data *data = i2c_get_clientdata(client);
	int val;

	mutex_lock(&data->update_lock);
	val = i2c_smbus_read_word_data(client, PMBUS_STATUS_WORD);
	mutex_unlock(&data->update_lock);
	if (val < 0) {
		return val;
	}
	return sprintf(buf, "0x%x\n", val);
}

static ssize_t show_status_vout(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pmbus_data *data = i2c_get_clientdata(client);
	int val;

	mutex_lock(&data->update_lock);
	val = i2c_smbus_read_byte_data(client, PMBUS_STATUS_VOUT);
	mutex_unlock(&data->update_lock);
	if (val < 0) {
		return val;
	}
	return sprintf(buf, "0x%x\n", val);
}

static ssize_t show_status_iout(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pmbus_data *data = i2c_get_clientdata(client);
	int val;

	mutex_lock(&data->update_lock);
	val = i2c_smbus_read_byte_data(client, PMBUS_STATUS_IOUT);
	mutex_unlock(&data->update_lock);
	if (val < 0) {
		return val;
	}
	return sprintf(buf, "0x%x\n", val);
}

static ssize_t show_status_input(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pmbus_data *data = i2c_get_clientdata(client);
	int val;

	mutex_lock(&data->update_lock);
	val = i2c_smbus_read_byte_data(client, PMBUS_STATUS_INPUT);
	mutex_unlock(&data->update_lock);
	if (val < 0) {
		return val;
	}
	return sprintf(buf, "0x%x\n", val);
}

static ssize_t show_status_temp(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pmbus_data *data = i2c_get_clientdata(client);
	int val;

	mutex_lock(&data->update_lock);
	val = i2c_smbus_read_byte_data(client, PMBUS_STATUS_TEMPERATURE);
	mutex_unlock(&data->update_lock);
	if (val < 0) {
		return val;
	}
	return sprintf(buf, "0x%x\n", val);
}

static ssize_t show_read_vin(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pmbus_data *data = i2c_get_clientdata(client);
	int val;

	mutex_lock(&data->update_lock);
	val = i2c_smbus_read_word_data(client, PMBUS_READ_VIN);
	mutex_unlock(&data->update_lock);
	if (val < 0) {
		return val;
	}
	return sprintf(buf, "%d\n", (val * 100000) / 5251);
}

static ssize_t show_read_vout(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pmbus_data *data = i2c_get_clientdata(client);
	int val;

	mutex_lock(&data->update_lock);
	val = i2c_smbus_read_word_data(client, PMBUS_READ_VOUT);
	mutex_unlock(&data->update_lock);
	if (val < 0) {
		return val;
	}
	return sprintf(buf, "%d\n", (val * 100000) / 5251);
}

static ssize_t show_read_iin(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pmbus_data *data = i2c_get_clientdata(client);
	int val;

	mutex_lock(&data->update_lock);
	val = i2c_smbus_read_word_data(client, PMBUS_READ_IIN);
	mutex_unlock(&data->update_lock);
	if (val < 0) {
		return val;
	}
	return sprintf(buf, "%d\n", (val * 1000000) / 9538);
}

static ssize_t show_read_temp1(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pmbus_data *data = i2c_get_clientdata(client);
	int val;

	mutex_lock(&data->update_lock);
	val = i2c_smbus_read_word_data(client, PMBUS_READ_TEMPERATURE_1);
	mutex_unlock(&data->update_lock);
	if (val < 0) {
		return val;
	}
	return sprintf(buf, "%d\n", (val * 100 - 32100)*1000 / 140);
}

static ssize_t set_efuse_reenable(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pmbus_data *data = i2c_get_clientdata(client);
	long val;

	int ret;
	ret = kstrtoul(buf, 10, &val);
	if (ret)
	{
		return ret;
	}
	if (val != 1)
	{
		printk("Set %ld to re-enable EFuse\n", val);
		return -EINVAL;
	}

	printk("Reenable EFuse: %ld\n", val);

	mutex_lock(&data->update_lock);
	val = i2c_smbus_write_byte_data(client, MFR_WRITE_PROTECT, DISABLE_PROTECT);
	val = i2c_smbus_write_byte_data(client, PMBUS_OPERATION, EFUSE_OUTPUT_DISABLE);
	val = i2c_smbus_write_byte_data(client, PMBUS_OPERATION, EFUSE_OUTPUT_ENABLE);
	val = i2c_smbus_write_byte_data(client, MFR_WRITE_PROTECT, ENABLE_PROTECT);

	mutex_unlock(&data->update_lock);
	return count;
}


//static SENSOR_DEVICE_ATTR(pec, S_IWUSR | S_IRUGO, show_pec, set_pec, 0);
//static SENSOR_DEVICE_ATTR(pec, S_IRUGO, show_pec, NULL, 0);
static SENSOR_DEVICE_ATTR(capability, S_IRUGO, show_capability, NULL, 0);
static SENSOR_DEVICE_ATTR(status_word, S_IRUGO, show_status_word, NULL, 0);
static SENSOR_DEVICE_ATTR(status_vout, S_IRUGO, show_status_vout, NULL, 0);
static SENSOR_DEVICE_ATTR(status_iout, S_IRUGO, show_status_iout, NULL, 0);
static SENSOR_DEVICE_ATTR(status_input, S_IRUGO, show_status_input, NULL, 0);
static SENSOR_DEVICE_ATTR(status_temp, S_IRUGO, show_status_temp, NULL, 0);
static SENSOR_DEVICE_ATTR(read_vin, S_IRUGO, show_read_vin, NULL, 0);
static SENSOR_DEVICE_ATTR(read_vout, S_IRUGO, show_read_vout, NULL, 0);
static SENSOR_DEVICE_ATTR(read_iin, S_IRUGO, show_read_iin, NULL, 0);
static SENSOR_DEVICE_ATTR(mfr_id, S_IRUGO, show_mfr_id, NULL, 0);
static SENSOR_DEVICE_ATTR(mfr_model, S_IRUGO, show_mfr_model, NULL, 0);
static SENSOR_DEVICE_ATTR(read_temp1, S_IRUGO, show_read_temp1, NULL, 0);
static SENSOR_DEVICE_ATTR(reenable_efuse, S_IWUSR, NULL, set_efuse_reenable, 0);


static struct attribute *efuse_attrs[] = {
	//	&sensor_dev_attr_pec.dev_attr.attr,
	&sensor_dev_attr_capability.dev_attr.attr,
	&sensor_dev_attr_status_word.dev_attr.attr,
	&sensor_dev_attr_status_vout.dev_attr.attr,
	&sensor_dev_attr_status_iout.dev_attr.attr,
	&sensor_dev_attr_status_input.dev_attr.attr,
	&sensor_dev_attr_status_temp.dev_attr.attr,
	&sensor_dev_attr_read_vin.dev_attr.attr,
	&sensor_dev_attr_read_vout.dev_attr.attr,
	&sensor_dev_attr_read_iin.dev_attr.attr,
	&sensor_dev_attr_mfr_id.dev_attr.attr,
	&sensor_dev_attr_mfr_model.dev_attr.attr,
	&sensor_dev_attr_read_temp1.dev_attr.attr,
	&sensor_dev_attr_reenable_efuse.dev_attr.attr,
	NULL
};

static struct attribute_group efuse_attr_grp = {
	.attrs = efuse_attrs,
};

static int efuse_probe(struct i2c_client *client)
{

	int ret, status;
	struct pmbus_data *data;

	data = devm_kzalloc(&client->dev, sizeof(struct pmbus_data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto exit;
	}
	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_WORD_DATA | I2C_FUNC_SMBUS_READ_BYTE_DATA | I2C_FUNC_SMBUS_READ_BLOCK_DATA)) {
		ret = -ENOMEM;
		goto exit;
	}

	ret = sysfs_create_group(&client->dev.kobj, &efuse_attr_grp);
	if (ret)
		goto exit_free;

	data->hwmon_dev = hwmon_device_register_with_info(&client->dev, client->name, NULL, NULL, NULL);
	if (IS_ERR(data->hwmon_dev)) {
		status = PTR_ERR(data->hwmon_dev);
		goto exit_remove;
	}

	return 0;

exit_remove:
	sysfs_remove_group(&client->dev.kobj, &efuse_attr_grp);
exit_free:
	i2c_set_clientdata(client, NULL);
	kfree(data);
exit:


	return ret;

}

static void efuse_remove(struct i2c_client *client)
{
	struct pmbus_data *data = i2c_get_clientdata(client);

	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&client->dev.kobj, &efuse_attr_grp);
	i2c_set_clientdata(client, NULL);
}

static const struct i2c_device_id efuse_id[] = {
	{"efuse", 0},
	{ }
};

MODULE_DEVICE_TABLE(i2c, efuse_id);

/* This is the driver that will be inserted */
static struct i2c_driver efuse_driver = {
	.driver = {
		.name = "efuse",
	},
	.probe = efuse_probe,
	.remove = efuse_remove,
	.id_table = efuse_id,
};

module_i2c_driver(efuse_driver);
MODULE_AUTHOR("SMCI");
MODULE_DESCRIPTION("SMCI PMBus core driver");
MODULE_LICENSE("GPL");


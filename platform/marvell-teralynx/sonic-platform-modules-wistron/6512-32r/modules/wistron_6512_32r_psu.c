#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dmi.h>

static ssize_t get_status(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_status(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t get_value(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count);

/* Addresses scanned */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

#define MFR_VENDOR_NAME_LENGTH	16
#define MFR_MODEL_NAME_LENGTH	16
#define MFR_SERIAL_NUM_LENGTH	32
#define MFR_REV_LENGTH	10

/* Each client has this additional data */
struct wistron_psu_data {
	struct device    *hwmon_dev;
	struct mutex     lock;
	int              index;                      /* PSU index */
	int              present;                    /* PSU present */
	int              pwr_good;                   /* PSU power good */
	int              v_in;
	int              v_out;
	int              i_in;
	int              i_out;
	long             p_in;
	long             p_out;
	int              temp_input;
	int              pwm;
	int              fault;
	u8               mfr_id[MFR_VENDOR_NAME_LENGTH];
	u8               mfr_model[MFR_MODEL_NAME_LENGTH];
	u8               mfr_serial[MFR_SERIAL_NUM_LENGTH];
	u8               mfr_rev[MFR_REV_LENGTH];
};

enum psu_index {
	wistron_psu1,
	wistron_psu2
};

enum wistron_psu_sysfs_attributes {
	PSU_PRESENT,
	PSU_POWER_GOOD,
	PSU_V_IN,
	PSU_V_OUT,
	PSU_I_IN,
	PSU_I_OUT,
	PSU_P_IN,
	PSU_P_OUT,
	PSU_TEMP1_INPUT,
	PSU_PWM,
	PSU_FAULT,
	PSU_MFR_ID,
	PSU_MFR_MODEL,
	PSU_MFR_SERIAL,
	PSU_MFR_REV,
};

/* sysfs attributes for hwmon */
static SENSOR_DEVICE_ATTR(present,       S_IWUSR | S_IRUGO, get_status, set_status, PSU_PRESENT);
static SENSOR_DEVICE_ATTR(power_good,    S_IWUSR | S_IRUGO, get_status, set_status, PSU_POWER_GOOD);
static SENSOR_DEVICE_ATTR(in1_input,     S_IWUSR | S_IRUGO, get_value,  set_value, PSU_V_IN);
static SENSOR_DEVICE_ATTR(in2_input,     S_IWUSR | S_IRUGO, get_value,  set_value, PSU_V_OUT);
static SENSOR_DEVICE_ATTR(curr1_input,   S_IWUSR | S_IRUGO, get_value,  set_value, PSU_I_IN);
static SENSOR_DEVICE_ATTR(curr2_input,   S_IWUSR | S_IRUGO, get_value,  set_value, PSU_I_OUT);
static SENSOR_DEVICE_ATTR(power1_input,  S_IWUSR | S_IRUGO, get_value,  set_value, PSU_P_IN);
static SENSOR_DEVICE_ATTR(power2_input,  S_IWUSR | S_IRUGO, get_value,  set_value, PSU_P_OUT);
static SENSOR_DEVICE_ATTR(temp1_input,   S_IWUSR | S_IRUGO, get_value,  set_value, PSU_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(pwm,   S_IWUSR | S_IRUGO, get_value,  set_value, PSU_PWM);
static SENSOR_DEVICE_ATTR(fault,		 S_IWUSR | S_IRUGO, get_value,  set_value, PSU_FAULT);
static SENSOR_DEVICE_ATTR(vendor,        S_IWUSR | S_IRUGO, get_value,  set_value, PSU_MFR_ID);
static SENSOR_DEVICE_ATTR(model,     	 S_IWUSR | S_IRUGO, get_value,  set_value, PSU_MFR_MODEL);
static SENSOR_DEVICE_ATTR(sn,   		 S_IWUSR | S_IRUGO, get_value,  set_value, PSU_MFR_SERIAL);
static SENSOR_DEVICE_ATTR(rev,   		 S_IWUSR | S_IRUGO, get_value,  set_value, PSU_MFR_REV);


static struct attribute *wistron_psu_attributes[] = {
	&sensor_dev_attr_present.dev_attr.attr,
	&sensor_dev_attr_power_good.dev_attr.attr,
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_in2_input.dev_attr.attr,
	&sensor_dev_attr_curr1_input.dev_attr.attr,
	&sensor_dev_attr_curr2_input.dev_attr.attr,
	&sensor_dev_attr_power1_input.dev_attr.attr,
	&sensor_dev_attr_power2_input.dev_attr.attr,
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_pwm.dev_attr.attr,
	&sensor_dev_attr_fault.dev_attr.attr,
	&sensor_dev_attr_vendor.dev_attr.attr,
	&sensor_dev_attr_model.dev_attr.attr,
	&sensor_dev_attr_sn.dev_attr.attr,
	&sensor_dev_attr_rev.dev_attr.attr,
	NULL
};

static ssize_t get_status(struct device *dev, struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
	struct i2c_client               *client = to_i2c_client(dev);
	struct wistron_psu_data      *data = i2c_get_clientdata(client);
	int                             status = 0;

	mutex_lock(&data->lock);
	if (attr->index == PSU_PRESENT) {
		status = data->present;
	} else if (attr->index == PSU_POWER_GOOD) {
		status = data->pwr_good;
	}

	mutex_unlock(&data->lock);
	return sprintf(buf, "%d", status);
}

static ssize_t set_status(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
	struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
	struct i2c_client               *client = to_i2c_client(dev);
	struct wistron_psu_data      *data = i2c_get_clientdata(client);
	int                             error, status;

	error = kstrtoint(buf, 10, &status);
	if (error) {
		printk("psu_set_status error=%d\n", error);
		return error;
	}

	mutex_lock(&data->lock);
	if (attr->index == PSU_PRESENT) {
		data->present = status;
	} else if (attr->index == PSU_POWER_GOOD) {
		data->pwr_good = status;
	}

	mutex_unlock(&data->lock);
	return count;
}

static ssize_t get_value(struct device *dev, struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
	struct i2c_client               *client = to_i2c_client(dev);
	struct wistron_psu_data     	*data = i2c_get_clientdata(client);
	size_t                          ret_count = 0;

	mutex_lock(&data->lock);

	switch (attr->index)
	{
		case PSU_V_IN:
			ret_count = sprintf(buf, "%d", data->v_in);
			break;
		case PSU_V_OUT:
			ret_count = sprintf(buf, "%d", data->v_out);
			break;
		case PSU_I_IN:
			ret_count = sprintf(buf, "%d", data->i_in);
			break;
		case PSU_I_OUT:
			ret_count = sprintf(buf, "%d", data->i_out);
			break;
		case PSU_P_IN:
			ret_count = sprintf(buf, "%ld", data->p_in);
			break;
		case PSU_P_OUT:
			ret_count = sprintf(buf, "%ld", data->p_out);
			break;
		case PSU_TEMP1_INPUT:
			ret_count = sprintf(buf, "%d", data->temp_input);
			break;
		case PSU_PWM:
			ret_count = sprintf(buf, "%d", data->pwm);
			break;
		case PSU_FAULT:
			ret_count = sprintf(buf, "%d", data->fault);
			break;
		case PSU_MFR_ID:
			ret_count = sprintf(buf, "%s", data->mfr_id);
			break;
		case PSU_MFR_MODEL:
			ret_count = sprintf(buf, "%s", data->mfr_model);
			break;
		case PSU_MFR_SERIAL:
			ret_count = sprintf(buf, "%s", data->mfr_serial);
			break;
		case PSU_MFR_REV:
			ret_count = sprintf(buf, "%s", data->mfr_rev);
			break;
		default:
			break;
	}

	mutex_unlock(&data->lock);
	return ret_count;
}

static ssize_t set_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
	struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
	struct i2c_client               *client = to_i2c_client(dev);
	struct wistron_psu_data     	*data = i2c_get_clientdata(client);
	int                             error;

	mutex_lock(&data->lock);

	switch (attr->index)
	{
		case PSU_V_IN:
			error = kstrtoint(buf, 10, &data->v_in);
			if (error)
				goto exit_err;
			break;
		case PSU_V_OUT:
			error = kstrtoint(buf, 10, &data->v_out);
			if (error)
				goto exit_err;
			break;
		case PSU_I_IN:
			error = kstrtoint(buf, 10, &data->i_in);
			if (error)
				goto exit_err;
			break;
		case PSU_I_OUT:
			error = kstrtoint(buf, 10, &data->i_out);
			if (error)
				goto exit_err;
			break;
		case PSU_P_IN:
			error = kstrtol(buf, 10, &data->p_in);
			if (error)
				goto exit_err;
			break;
		case PSU_P_OUT:
			error = kstrtol(buf, 10, &data->p_out);
			if (error)
				goto exit_err;
			break;
		case PSU_TEMP1_INPUT:
			error = kstrtoint(buf, 10, &data->temp_input);
			if (error)
				goto exit_err;
			break;
		case PSU_PWM:
			error = kstrtoint(buf, 10, &data->pwm);
			if (error)
				goto exit_err;
			break;
		case PSU_FAULT:
			error = kstrtoint(buf, 10, &data->fault);
			if (error)
				goto exit_err;
			break;
		case PSU_MFR_ID:
			memzero_explicit(&data->mfr_id, sizeof(data->mfr_id));
			strncpy(data->mfr_id, buf, sizeof(data->mfr_id) - 1);
			break;
		case PSU_MFR_MODEL:
			memzero_explicit(&data->mfr_model, sizeof(data->mfr_model));
			strncpy(data->mfr_model, buf, sizeof(data->mfr_model) - 1);
			break;
		case PSU_MFR_SERIAL:
			memzero_explicit(&data->mfr_serial, sizeof(data->mfr_serial));
			strncpy(data->mfr_serial, buf, sizeof(data->mfr_serial) - 1);
			break;
		case PSU_MFR_REV:
			memzero_explicit(&data->mfr_rev, sizeof(data->mfr_rev));
			strncpy(data->mfr_rev, buf, sizeof(data->mfr_rev) - 1);
			break;
		default:
			break;
	}

	mutex_unlock(&data->lock);
	return count;

exit_err:
	mutex_unlock(&data->lock);
	return error;
}

static const struct attribute_group wistron_psu_group = {
	.attrs = wistron_psu_attributes,
};

static int wistron_psu_probe(struct i2c_client *client)
{
	const struct i2c_device_id *dev_id = i2c_client_get_device_id(client);
	struct wistron_psu_data *data;
	int status;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK))
		return -EIO;

	data = kzalloc(sizeof(struct wistron_psu_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	i2c_set_clientdata(client, data);
	data->index = dev_id->driver_data;
	mutex_init(&data->lock);

	dev_info(&client->dev, "device found\n");

	/* Register sysfs hooks */
	status = sysfs_create_group(&client->dev.kobj, &wistron_psu_group);
	if (status)
		goto exit_free;

	data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, "wistron_psu", NULL, NULL);
	if (IS_ERR(data->hwmon_dev)) {
		status = PTR_ERR(data->hwmon_dev);
		goto exit_remove;
	}

	dev_info(&client->dev, "%s: psu '%s'\n", dev_name(data->hwmon_dev), client->name);

	return 0;

exit_remove:
	sysfs_remove_group(&client->dev.kobj, &wistron_psu_group);
exit_free:
	kfree(data);

	return status;
}

static void wistron_psu_remove(struct i2c_client *client)
{
	struct wistron_psu_data *data = i2c_get_clientdata(client);

	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&client->dev.kobj, &wistron_psu_group);
	kfree(data);
}

static const struct i2c_device_id wistron_psu_id[] = {
	{ "wistron_psu1", wistron_psu1 },
	{ "wistron_psu2", wistron_psu2 },
	{}
};
MODULE_DEVICE_TABLE(i2c, wistron_psu_id);

static struct i2c_driver wistron_psu_driver = {
	.class        = I2C_CLASS_HWMON,
	.driver = {
		.name     = "wistron_psu",
	},
	.probe        = wistron_psu_probe,
	.remove       = wistron_psu_remove,
	.id_table     = wistron_psu_id,
	.address_list = normal_i2c,
};

module_i2c_driver(wistron_psu_driver);

MODULE_AUTHOR("Wistron");
MODULE_DESCRIPTION("wistron_psu driver");
MODULE_LICENSE("GPL");

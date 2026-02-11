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

#define QSFP28_TYPE	0x11
#define QSFP_DD_TYPE	0x18

#define LOWER_PAGE_OFFSET	0x0
#define PAGE0_OFFSET	0x80
#define PAGE1_OFFSET	0x0
#define PAGE2_OFFSET	0x80
#define PAGE3_OFFSET	0x0
#define PAGE10_OFFSET	0x80
#define PAGE11_OFFSET	0x100

#define QSFP_DD_CHAN_MON_OFFSET	0x1a
#define QSFP_DD_TEMP_OFFSET	0xe
#define QSFP_DD_VOLT_OFFSET	0x10
#define QSFP_DD_RX_LOS_OFFSET	0x13
#define QSFP_DD_TX_FAULT_OFFSET	0x7
#define QSFP_DD_DISABLE_OFFSET	0x2

#define QSFP28_DOM_BULK_DATA_OFFSET	0x16
#define QSFP28_RX_LOS_OFFSET	0x3
#define QSFP28_TX_FAULT_OFFSET	0x4
#define QSFP28_DISABLE_OFFSET	0x56


/* QSFP-DD: page0 (low page + high page (128+128 byte)), page 1/2/3/10/11 (high page (128 byte))*/
#define EEPROM_DATA_SIZE        256
#define EEPROM3_DATA_SIZE        384

/* Addresses scanned */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };
#define MAX_PORT_NAME_LEN       20
#define TEMP_DATA_SIZE	2
#define VOLT_DATA_SIZE	2
#define QSFP_DD_CHAN_MON_DATA_SIZE	0x30
#define QSFP_DOM_BULK_DATA_SIZE	0x24

enum sysfs_oom_attributes {
	OOM_LP_MODE,
	OOM_TEMP,
	OOM_EEPROM1,
	OOM_EEPROM2,
	OOM_EEPROM3,
	OOM_PORT_NAME,
	OOM_TEMP_E,
	OOM_VOLT_E,
	OOM_QSFPDD_CHAN_MON,
	OOM_QSFP_DOM_BULK,
	OOM_CHAN_RX_LOS,
	OOM_CHAN_TX_FAULT,
	OOM_CHAN_DISABLE,
	OOM_ATTR_MAX
};

/* Each client has this additional data */
struct wistron_oom_data {
	struct device   *hwmon_dev;
	struct mutex    lock;
	u8              index;
	int             lp_mode;
	int             temp;
	unsigned char   eeprom1[EEPROM_DATA_SIZE];
	unsigned char   eeprom2[EEPROM_DATA_SIZE];
	unsigned char   eeprom3[EEPROM3_DATA_SIZE];
	char            port_name[MAX_PORT_NAME_LEN];

	unsigned char	qsfp_dd_chan_mon[QSFP_DD_CHAN_MON_DATA_SIZE];
	unsigned char	qsfp_dom_bulk[QSFP_DOM_BULK_DATA_SIZE];
	unsigned char	tempe[TEMP_DATA_SIZE];
	unsigned char	volte[VOLT_DATA_SIZE];
	int				rx_los;
	int				tx_fault;
	int				disable;
};

/* sysfs attributes for hwmon */
static ssize_t get_oom_value(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_oom_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t get_oom_info1(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_oom_info1(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t get_oom_info2(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_oom_info2(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t get_oom_info3(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_oom_info3(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t get_port_name(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_port_name(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t get_t_v_e(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_t_v_e(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t get_qsfp_dd_chan_mon(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_qsfp_dd_chan_mon(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t get_qsfp_dom_bulk(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_qsfp_dom_bulk(struct device *dev, struct device_attribute *da, const char *buf, size_t count);


static SENSOR_DEVICE_ATTR(lp_mode, S_IWUSR | S_IRUGO, get_oom_value, set_oom_value, OOM_LP_MODE);
static SENSOR_DEVICE_ATTR(temp, S_IWUSR | S_IRUGO, get_oom_value, set_oom_value, OOM_TEMP);
static SENSOR_DEVICE_ATTR(eeprom1, S_IWUSR | S_IRUGO, get_oom_info1, set_oom_info1, OOM_EEPROM1);
static SENSOR_DEVICE_ATTR(eeprom2, S_IWUSR | S_IRUGO, get_oom_info2, set_oom_info2, OOM_EEPROM2);
static SENSOR_DEVICE_ATTR(eeprom3, S_IWUSR | S_IRUGO, get_oom_info3, set_oom_info3, OOM_EEPROM3);
static SENSOR_DEVICE_ATTR(port_name, S_IRUGO | S_IWUSR, get_port_name, set_port_name, OOM_PORT_NAME);
static SENSOR_DEVICE_ATTR(qsfp_dd_chan_mon, S_IWUSR | S_IRUGO, get_qsfp_dd_chan_mon, set_qsfp_dd_chan_mon, OOM_QSFPDD_CHAN_MON);
static SENSOR_DEVICE_ATTR(qsfp_dom_bulk, S_IWUSR | S_IRUGO, get_qsfp_dom_bulk, set_qsfp_dom_bulk, OOM_QSFP_DOM_BULK);
static SENSOR_DEVICE_ATTR(tempe, S_IWUSR | S_IRUGO, get_t_v_e, set_t_v_e, OOM_TEMP_E);
static SENSOR_DEVICE_ATTR(volte, S_IWUSR | S_IRUGO, get_t_v_e, set_t_v_e, OOM_VOLT_E);
static SENSOR_DEVICE_ATTR(rx_los, S_IWUSR | S_IRUGO, get_oom_value, set_oom_value, OOM_CHAN_RX_LOS);
static SENSOR_DEVICE_ATTR(tx_fault, S_IWUSR | S_IRUGO, get_oom_value, set_oom_value, OOM_CHAN_TX_FAULT);
static SENSOR_DEVICE_ATTR(disable, S_IWUSR | S_IRUGO, get_oom_value, set_oom_value, OOM_CHAN_DISABLE);

static struct attribute *wistron_oom_attributes[] = {
	&sensor_dev_attr_lp_mode.dev_attr.attr,
	&sensor_dev_attr_temp.dev_attr.attr,
	&sensor_dev_attr_eeprom1.dev_attr.attr,
	&sensor_dev_attr_eeprom2.dev_attr.attr,
	&sensor_dev_attr_eeprom3.dev_attr.attr,
	&sensor_dev_attr_port_name.dev_attr.attr,
	&sensor_dev_attr_qsfp_dd_chan_mon.dev_attr.attr,
	&sensor_dev_attr_qsfp_dom_bulk.dev_attr.attr,
	&sensor_dev_attr_tempe.dev_attr.attr,
	&sensor_dev_attr_volte.dev_attr.attr,
	&sensor_dev_attr_rx_los.dev_attr.attr,
	&sensor_dev_attr_tx_fault.dev_attr.attr,
	&sensor_dev_attr_disable.dev_attr.attr,
	NULL
};

static ssize_t get_oom_value(struct device *dev, struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
	struct i2c_client               *client = to_i2c_client(dev);
	struct wistron_oom_data      	*data = i2c_get_clientdata(client);
	int                             value = 0;

	mutex_lock(&data->lock);
	switch (attr->index) {
		case OOM_LP_MODE:
			value = data->lp_mode;
			break;
		case OOM_TEMP:
			value = data->temp;
			break;
		case OOM_CHAN_RX_LOS:
			value = data->rx_los;
			break;
		case OOM_CHAN_TX_FAULT:
			value = data->tx_fault;
			break;
		case OOM_CHAN_DISABLE:
			value = data->disable;
			break;
		default:
			value = data->temp;
			break;

	}

	mutex_unlock(&data->lock);
	return sprintf(buf, "%d", value);
}

static ssize_t set_oom_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
	struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
	struct i2c_client               *client = to_i2c_client(dev);
	struct wistron_oom_data      	*data = i2c_get_clientdata(client);
	int                             error, value;

	error = kstrtoint(buf, 10, &value);
	if (error) {
		printk("oom_set_value error=%d\n", error);
		return error;
	}

	mutex_lock(&data->lock);
	switch (attr->index) {
		case OOM_LP_MODE:
			data->lp_mode = value;
			break;
		case OOM_TEMP:
			data->temp = value;
			break;
		case OOM_CHAN_RX_LOS:
			data->rx_los = value;
			if (data->eeprom1[0] == QSFP_DD_TYPE)
				data->eeprom3[PAGE11_OFFSET + QSFP_DD_RX_LOS_OFFSET] = value;
			if (data->eeprom1[0] == QSFP28_TYPE)
				data->eeprom1[LOWER_PAGE_OFFSET + QSFP28_RX_LOS_OFFSET] = value;
			break;
		case OOM_CHAN_TX_FAULT:
			data->tx_fault = value;
			if (data->eeprom1[0] == QSFP_DD_TYPE)
				data->eeprom3[PAGE11_OFFSET + QSFP_DD_TX_FAULT_OFFSET] = value;
			if (data->eeprom1[0] == QSFP28_TYPE)
				data->eeprom1[LOWER_PAGE_OFFSET + QSFP28_TX_FAULT_OFFSET] = value;
			break;
		case OOM_CHAN_DISABLE:
			data->disable = value;
			if (data->eeprom1[0] == QSFP_DD_TYPE)
				data->eeprom3[PAGE10_OFFSET + QSFP_DD_DISABLE_OFFSET] = value;
			if (data->eeprom1[0] == QSFP28_TYPE)
				data->eeprom1[LOWER_PAGE_OFFSET + QSFP28_DISABLE_OFFSET] = value;
			break;
		default:
			data->temp = value;
			break;
	}

	mutex_unlock(&data->lock);
	return count;
}

static ssize_t get_oom_info1(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client           *client = to_i2c_client(dev);
	struct wistron_oom_data  	*data = i2c_get_clientdata(client);

	mutex_lock(&data->lock);
	memcpy(buf, data->eeprom1, EEPROM_DATA_SIZE);
	mutex_unlock(&data->lock);

	return EEPROM_DATA_SIZE;
}

static ssize_t set_oom_info1(struct device *dev, struct device_attribute *da, const char *buf, size_t size)
{
	struct i2c_client           *client = to_i2c_client(dev);
	struct wistron_oom_data  	*data = i2c_get_clientdata(client);
	int                         i = 0, j = 0, k = 0;
	unsigned char               str[3];
	unsigned int                val;

	mutex_lock(&data->lock);
	memzero_explicit(data->eeprom1, EEPROM_DATA_SIZE);
	memzero_explicit(str, sizeof(str));

	if (strlen(buf) >= EEPROM_DATA_SIZE) {
		for (i=0; i < strlen(buf) ; i++) {
			for (j = 0; j < 2; j++) {
				str[j] = buf[i + j];
			}

			sscanf(str, "%x", &val);

			i = j + i - 1;

			if (k >= EEPROM_DATA_SIZE)
				break;

			data->eeprom1[k] = (unsigned char)val;
			k++;
		}
	}

	mutex_unlock(&data->lock);

	return size;
}

static ssize_t get_oom_info2(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client           *client = to_i2c_client(dev);
	struct wistron_oom_data  	*data = i2c_get_clientdata(client);

	mutex_lock(&data->lock);
	memcpy(buf, data->eeprom2, EEPROM_DATA_SIZE);
	mutex_unlock(&data->lock);

	return EEPROM_DATA_SIZE;
}

static ssize_t set_oom_info2(struct device *dev, struct device_attribute *da, const char *buf, size_t size)
{
	struct i2c_client           *client = to_i2c_client(dev);
	struct wistron_oom_data  	*data = i2c_get_clientdata(client);
	int                         i = 0, j = 0, k = 0;
	unsigned char               str[3];
	unsigned int                val;

	mutex_lock(&data->lock);
	memzero_explicit(data->eeprom2, EEPROM_DATA_SIZE);
	memzero_explicit(str, sizeof(str));

	if (strlen(buf) >= EEPROM_DATA_SIZE) {
		for (i = 0; i < strlen(buf) ; i++) {
			for (j = 0; j < 2; j++) {
				str[j] = buf[i + j];
			}

			sscanf(str, "%x", &val);

			i = j + i - 1;
			if (k >= EEPROM_DATA_SIZE)
				break;

			data->eeprom2[k]=(unsigned char)val;
			k++;
		}
	}

	mutex_unlock(&data->lock);

	return size;
}

static ssize_t get_oom_info3(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client           *client = to_i2c_client(dev);
	struct wistron_oom_data  	*data = i2c_get_clientdata(client);

	mutex_lock(&data->lock);
	memcpy(buf, data->eeprom3, EEPROM3_DATA_SIZE);
	mutex_unlock(&data->lock);

	return EEPROM3_DATA_SIZE;
}

static ssize_t set_oom_info3(struct device *dev, struct device_attribute *da, const char *buf, size_t size)
{
	struct i2c_client           *client = to_i2c_client(dev);
	struct wistron_oom_data  	*data = i2c_get_clientdata(client);
	int                         i = 0, j = 0, k = 0;
	unsigned char               str[3];
	unsigned int                val;

	mutex_lock(&data->lock);
	memzero_explicit(data->eeprom3, EEPROM3_DATA_SIZE);
	memzero_explicit(str, sizeof(str));

	if (strlen(buf) >= EEPROM3_DATA_SIZE) {
		for (i = 0; i < strlen(buf) ; i++) {
			for (j = 0; j < 2; j++) {
				str[j] = buf[i + j];
			}

			sscanf(str, "%x", &val);

			i = j + i - 1;
			if (k >= EEPROM3_DATA_SIZE)
				break;

			data->eeprom3[k]=(unsigned char)val;
			k++;
		}
	}

	mutex_unlock(&data->lock);

	return size;
}

static ssize_t get_port_name(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client           *client = to_i2c_client(dev);
	struct wistron_oom_data  	*data = i2c_get_clientdata(client);
	ssize_t                     count;

	mutex_lock(&data->lock);
	count = sprintf(buf, "%s", data->port_name);
	mutex_unlock(&data->lock);

	return count;
}

static ssize_t set_port_name(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
	struct i2c_client           *client = to_i2c_client(dev);
	struct wistron_oom_data 	*data = i2c_get_clientdata(client);
	char                        port_name[MAX_PORT_NAME_LEN];

	if (sscanf(buf, "%19s", port_name) != 1)
		return -EINVAL;

	mutex_lock(&data->lock);
	strcpy(data->port_name, port_name);
	mutex_unlock(&data->lock);

	return count;
}

static ssize_t get_t_v_e(struct device *dev, struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
	struct i2c_client           *client = to_i2c_client(dev);
	struct wistron_oom_data  	*data = i2c_get_clientdata(client);

	mutex_lock(&data->lock);
	if (attr->index == OOM_TEMP_E)
		memcpy(buf, data->tempe, TEMP_DATA_SIZE);
	else
		memcpy(buf, data->volte, VOLT_DATA_SIZE);
	mutex_unlock(&data->lock);

	return (attr->index == OOM_TEMP_E) ? TEMP_DATA_SIZE : VOLT_DATA_SIZE;
}

static ssize_t set_t_v_e(struct device *dev, struct device_attribute *da, const char *buf, size_t size)
{
	struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
	struct i2c_client           *client = to_i2c_client(dev);
	struct wistron_oom_data  	*data = i2c_get_clientdata(client);
	int                         i = 0, j = 0, k = 0;
	unsigned char               str[3];
	unsigned int                val;

	mutex_lock(&data->lock);
	if (attr->index == OOM_TEMP_E) {
		memzero_explicit(data->tempe, TEMP_DATA_SIZE);
		memzero_explicit(str, sizeof(str));

		if (strlen(buf) >= TEMP_DATA_SIZE) {
			for (i = 0; i < strlen(buf) ; i++) {
				for (j = 0; j < 2; j++) {
					str[j] = buf[i + j];
				}

				sscanf(str, "%x", &val);

				i = j + i - 1;
				if (k >= TEMP_DATA_SIZE)
					break;

				data->tempe[k]=(unsigned char)val;

				if (k == 0)
					data->temp = data->tempe[k];
				k++;
			}
		}

		if (data->eeprom1[0] == QSFP_DD_TYPE)
			memcpy(&data->eeprom1[LOWER_PAGE_OFFSET + QSFP_DD_TEMP_OFFSET], data->tempe, TEMP_DATA_SIZE);
	}
	else {
		memzero_explicit(data->volte, VOLT_DATA_SIZE);
		memzero_explicit(str, sizeof(str));

		if (strlen(buf) >= VOLT_DATA_SIZE) {
			for (i = 0; i < strlen(buf) ; i++) {
				for (j = 0; j < 2; j++) {
					str[j] = buf[i + j];
				}

				sscanf(str, "%x", &val);

				i = j + i - 1;
				if (k >= VOLT_DATA_SIZE)
					break;

				data->volte[k]=(unsigned char)val;
				k++;
			}
		}

		if (data->eeprom1[0] == QSFP_DD_TYPE)
			memcpy(&data->eeprom1[LOWER_PAGE_OFFSET + QSFP_DD_VOLT_OFFSET], data->volte, VOLT_DATA_SIZE);
	}

	mutex_unlock(&data->lock);

	return size;
}

static ssize_t get_qsfp_dd_chan_mon(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client           *client = to_i2c_client(dev);
	struct wistron_oom_data  	*data = i2c_get_clientdata(client);

	mutex_lock(&data->lock);
	memcpy(buf, data->qsfp_dd_chan_mon, QSFP_DD_CHAN_MON_DATA_SIZE);
	mutex_unlock(&data->lock);

	return QSFP_DD_CHAN_MON_DATA_SIZE;
}

static ssize_t set_qsfp_dd_chan_mon(struct device *dev, struct device_attribute *da, const char *buf, size_t size)
{
	struct i2c_client           *client = to_i2c_client(dev);
	struct wistron_oom_data  	*data = i2c_get_clientdata(client);
	int                         i = 0, j = 0, k = 0;
	unsigned char               str[3];
	unsigned int                val;

	mutex_lock(&data->lock);
	memzero_explicit(data->qsfp_dd_chan_mon, QSFP_DD_CHAN_MON_DATA_SIZE);
	memzero_explicit(str, sizeof(str));

	if (strlen(buf) >= QSFP_DD_CHAN_MON_DATA_SIZE) {
		for (i=0; i < strlen(buf) ; i++) {
			for (j = 0; j < 2; j++) {
				str[j] = buf[i + j];
			}

			sscanf(str, "%x", &val);

			i = j + i - 1;

			if (k >= QSFP_DD_CHAN_MON_DATA_SIZE)
				break;

			data->qsfp_dd_chan_mon[k] = (unsigned char)val;
			k++;
		}
	}

	if (data->eeprom1[0] == QSFP_DD_TYPE)
		memcpy(&data->eeprom3[PAGE11_OFFSET + QSFP_DD_CHAN_MON_OFFSET], data->qsfp_dd_chan_mon, QSFP_DD_CHAN_MON_DATA_SIZE);

	mutex_unlock(&data->lock);

	return size;
}

static ssize_t get_qsfp_dom_bulk(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client           *client = to_i2c_client(dev);
	struct wistron_oom_data  	*data = i2c_get_clientdata(client);

	mutex_lock(&data->lock);
	memcpy(buf, data->qsfp_dom_bulk, QSFP_DOM_BULK_DATA_SIZE);
	mutex_unlock(&data->lock);

	return QSFP_DOM_BULK_DATA_SIZE;
}

static ssize_t set_qsfp_dom_bulk(struct device *dev, struct device_attribute *da, const char *buf, size_t size)
{
	struct i2c_client           *client = to_i2c_client(dev);
	struct wistron_oom_data  	*data = i2c_get_clientdata(client);
	int                         i = 0, j = 0, k = 0;
	unsigned char               str[3];
	unsigned int                val;

	mutex_lock(&data->lock);
	memzero_explicit(data->qsfp_dom_bulk, QSFP_DOM_BULK_DATA_SIZE);
	memzero_explicit(str, sizeof(str));

	if (strlen(buf) >= QSFP_DOM_BULK_DATA_SIZE) {
		for (i=0; i < strlen(buf) ; i++) {
			for (j = 0; j < 2; j++) {
				str[j] = buf[i + j];
			}

			sscanf(str, "%x", &val);

			i = j + i - 1;

			if (k >= QSFP_DOM_BULK_DATA_SIZE)
				break;

			data->qsfp_dom_bulk[k] = (unsigned char)val;
			k++;
		}
	}

	if (data->eeprom1[0] == QSFP28_TYPE) {
		memcpy(&data->eeprom1[LOWER_PAGE_OFFSET + QSFP28_DOM_BULK_DATA_OFFSET], data->qsfp_dom_bulk, QSFP_DOM_BULK_DATA_SIZE);
		data->temp = data->qsfp_dom_bulk[0];
	}

	mutex_unlock(&data->lock);

	return size;
}

static const struct attribute_group wistron_oom_group = {
	.attrs = wistron_oom_attributes,
};

static int wistron_oom_probe(struct i2c_client *client)
{
	const struct i2c_device_id *dev_id = i2c_client_get_device_id(client);
	struct wistron_oom_data *data;
	int status;

	data = kzalloc(sizeof(struct wistron_oom_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	i2c_set_clientdata(client, data);
	data->index = dev_id->driver_data;
	mutex_init(&data->lock);

	dev_info(&client->dev, "device found\n");

	/* Register sysfs hooks */
	status = sysfs_create_group(&client->dev.kobj, &wistron_oom_group);
	if (status)
		goto exit_free;

	data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, "wistron_oom", NULL, NULL);
	if (IS_ERR(data->hwmon_dev)) {
		status = PTR_ERR(data->hwmon_dev);
		goto exit_remove;
	}

	dev_info(&client->dev, "%s: oom '%s'\n", dev_name(data->hwmon_dev), client->name);

	return 0;

exit_remove:
	sysfs_remove_group(&client->dev.kobj, &wistron_oom_group);
exit_free:
	kfree(data);

	return status;
}

static void wistron_oom_remove(struct i2c_client *client)
{
	struct wistron_oom_data *data = i2c_get_clientdata(client);

	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&client->dev.kobj, &wistron_oom_group);
	kfree(data);
}

static const struct i2c_device_id wistron_oom_id[] = {
	{ "wistron_oom", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, wistron_oom_id);

static struct i2c_driver wistron_oom_driver = {
	.class        = I2C_CLASS_HWMON,
	.driver = {
		.name     = "wistron_oom",
	},
	.probe        = wistron_oom_probe,
	.remove       = wistron_oom_remove,
	.id_table     = wistron_oom_id,
	.address_list = normal_i2c,
};

static int __init wistron_oom_init(void)
{
	return i2c_add_driver(&wistron_oom_driver);
}

static void __exit wistron_oom_exit(void)
{
	i2c_del_driver(&wistron_oom_driver);
}

module_init(wistron_oom_init);
module_exit(wistron_oom_exit);

MODULE_AUTHOR("Wistron");
MODULE_DESCRIPTION("wistron_oom driver");
MODULE_LICENSE("GPL");

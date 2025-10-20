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

#define EEPROM_DATA_SIZE        256

/* Addresses scanned */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

/* Each client has this additional data */
struct wistron_syseeprom_data {
	struct device   *hwmon_dev;
	struct mutex    lock;
	unsigned char   eeprom[EEPROM_DATA_SIZE];
};

static ssize_t get_syseeprom(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct wistron_syseeprom_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->lock);
	memcpy(buf, data->eeprom, EEPROM_DATA_SIZE);
	mutex_unlock(&data->lock);

	return EEPROM_DATA_SIZE;
}

static ssize_t set_syseeprom(struct device *dev, struct device_attribute *da, const char *buf, size_t size)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct wistron_syseeprom_data *data = i2c_get_clientdata(client);
	int i = 0, j = 0, k = 0;
	unsigned char str[3];
	unsigned int val;

	mutex_lock(&data->lock);
	memzero_explicit(data->eeprom, EEPROM_DATA_SIZE);
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

			data->eeprom[k] = (unsigned char)val;

			k++;
		}
	}

	mutex_unlock(&data->lock);
	return size;
}

static SENSOR_DEVICE_ATTR(eeprom, S_IWUSR | S_IRUGO, get_syseeprom, set_syseeprom, 0);

static struct attribute *wistron_syseeprom_attributes[] = {
	&sensor_dev_attr_eeprom.dev_attr.attr,
	NULL
};

static const struct attribute_group wistron_syseeprom_group = {
	.attrs = wistron_syseeprom_attributes,
};

static int wistron_syseeprom_probe(struct i2c_client *client)
{
	const struct i2c_device_id *dev_id = i2c_client_get_device_id(client);
	struct wistron_syseeprom_data *data;
	int status;

	data = kzalloc(sizeof(struct wistron_syseeprom_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	i2c_set_clientdata(client, data);
	mutex_init(&data->lock);

	dev_info(&client->dev, "device found\n");

	/* Register sysfs hooks */
	status = sysfs_create_group(&client->dev.kobj, &wistron_syseeprom_group);
	if (status)
		goto exit_free;

	data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, "wistron_syseeprom", NULL, NULL);
	if (IS_ERR(data->hwmon_dev)) {
		status = PTR_ERR(data->hwmon_dev);
		goto exit_remove;
	}

	dev_info(&client->dev, "%s: syseeprom '%s'\n", dev_name(data->hwmon_dev), client->name);

	return 0;

exit_remove:
	sysfs_remove_group(&client->dev.kobj, &wistron_syseeprom_group);
exit_free:
	kfree(data);

	return status;
}

static void wistron_syseeprom_remove(struct i2c_client *client)
{
	struct wistron_syseeprom_data *data = i2c_get_clientdata(client);

	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&client->dev.kobj, &wistron_syseeprom_group);
	kfree(data);
}

static const struct i2c_device_id wistron_syseeprom_id[] = {
	{ "wistron_syseeprom", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, wistron_syseeprom_id);

static struct i2c_driver wistron_syseeprom_driver = {
	.class        = I2C_CLASS_HWMON,
	.driver = {
		.name     = "wistron_syseeprom",
	},
	.probe        = wistron_syseeprom_probe,
	.remove       = wistron_syseeprom_remove,
	.id_table     = wistron_syseeprom_id,
	.address_list = normal_i2c,
};

static int __init wistron_syseeprom_init(void)
{
	return i2c_add_driver(&wistron_syseeprom_driver);
}

static void __exit wistron_syseeprom_exit(void)
{
	i2c_del_driver(&wistron_syseeprom_driver);
}

module_init(wistron_syseeprom_init);
module_exit(wistron_syseeprom_exit);

MODULE_AUTHOR("Wistron");
MODULE_DESCRIPTION("wistron syseeprom driver");
MODULE_LICENSE("GPL");

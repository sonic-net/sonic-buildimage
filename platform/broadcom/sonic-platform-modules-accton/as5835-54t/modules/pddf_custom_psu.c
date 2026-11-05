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
#include "pddf_client_defs.h"
#include "pddf_psu_defs.h"
#include "pddf_psu_driver.h"
#include "pddf_psu_api.h"

ssize_t pddf_show_custom_psu_v_out(struct device *dev, struct device_attribute *da, char *buf);
extern PSU_SYSFS_ATTR_DATA access_psu_v_out;
int pddf_post_get_custom_psu_model_name(void *i2c_client, PSU_DATA_ATTR *adata, void *data);
extern PSU_SYSFS_ATTR_DATA access_psu_model_name;
int pddf_post_get_custom_psu_fan_dir(void *i2c_client, PSU_DATA_ATTR *adata, void *data);
extern PSU_SYSFS_ATTR_DATA access_psu_fan_dir;
int pddf_custom_psu_post_probe(struct i2c_client *client, const struct i2c_device_id *dev_id);
int pddf_custom_psu_post_remove(struct i2c_client *client);
extern struct pddf_ops_t pddf_psu_ops;

const char FAN_DIR_F2B[] = "F2B\0";
const char FAN_DIR_B2F[] = "B2F\0";

static LIST_HEAD(psu_eeprom_client_list);
static struct mutex     list_lock;

struct psu_eeprom_client_node {
	struct i2c_client *client;
	struct list_head   list;
};

static int two_complement_to_int(u16 data, u8 valid_bit, int mask)
{
	u16  valid_data  = data & mask;
	bool is_negative = valid_data >> (valid_bit - 1);

	return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static u8 psu_get_vout_mode(struct i2c_client *client)
{
	u8 status = 0, retry = 10;
	uint8_t offset = 0x20; // VOUT_MODE

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
	uint8_t offset = 0x8b; // READ_VOUT

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

	exponent = two_complement_to_int(vout_mode, 5, 0x1f);
	mantissa = value;
	if (exponent >= 0)
		return sprintf(buf, "%d\n", (mantissa << exponent) * multiplier);
	else
		return sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}


int pddf_post_get_custom_psu_model_name(void *i2c_client, PSU_DATA_ATTR *adata, void *data)
{
    struct psu_attr_info *sysfs_attr_info = (struct psu_attr_info *)data;

    if (strlen(sysfs_attr_info->val.strval) > 8) {
        sysfs_attr_info->val.strval[8] = '-';
    }

    return 0;
}

/*
 * Get the PSU EEPROM I2C client with the same bus number.
 */
static struct i2c_client *find_psu_eeprom_client(struct i2c_client *pmbus_client)
{
    struct list_head *list_node = NULL;
    struct psu_eeprom_client_node *psu_eeprom_node = NULL;
    struct i2c_client *eeprom_client = NULL;

    mutex_lock(&list_lock);
    list_for_each(list_node, &psu_eeprom_client_list) {
        psu_eeprom_node = list_entry(list_node, struct psu_eeprom_client_node, list);
        /* Check if the bus adapter is the same or not. */
        if (psu_eeprom_node->client->adapter == pmbus_client->adapter) {
            eeprom_client = psu_eeprom_node->client;
            break;
        }
    }
    mutex_unlock(&list_lock);

    return eeprom_client;
}

int pddf_post_get_custom_psu_fan_dir(void *i2c_client, PSU_DATA_ATTR *adata, void *data)
{
    int i;
    struct i2c_client *client = (struct i2c_client *)i2c_client;
    struct psu_attr_info *psu_fan_dir_attr_info = (struct psu_attr_info *)data;
    struct psu_data *psu_eeprom_client_data = NULL;
    struct psu_attr_info *psu_eeprom_model_name = NULL;
    struct i2c_client *psu_eeprom_client = NULL;

    psu_eeprom_client = find_psu_eeprom_client(client);
    if (!psu_eeprom_client) {
        return 0;
    }

    /*
     * Get the model name from the PSU EEPROM I2C client.
     */
    psu_eeprom_client_data = i2c_get_clientdata(psu_eeprom_client);
    if (!psu_eeprom_client_data) {
        return 0;
    }
    for (i = 0; i < psu_eeprom_client_data->num_attr; i++) {
        if (strcmp(psu_eeprom_client_data->attr_info[i].name, "psu_model_name") == 0) {
            psu_eeprom_model_name = &psu_eeprom_client_data->attr_info[i];
            break;
        }
    }
    if (!psu_eeprom_model_name) {
        return 0;
    }

    /*
     * Compare the model name, then replace the content of psu_fan_dir.
     */
    if (strcmp(psu_eeprom_model_name->val.strval, "YM-2401H-CR") == 0) {
        strscpy(psu_fan_dir_attr_info->val.strval,
                FAN_DIR_F2B,
                sizeof(psu_fan_dir_attr_info->val.strval));
    } else if (strcmp(psu_eeprom_model_name->val.strval, "YM-2401H-DR") == 0) {
        strscpy(psu_fan_dir_attr_info->val.strval,
                FAN_DIR_B2F,
                sizeof(psu_fan_dir_attr_info->val.strval));
    }

    return 0;
}

int pddf_custom_psu_post_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
    struct psu_eeprom_client_node *psu_eeprom_node;

    if (strcmp(dev_id->name, "psu_eeprom") != 0) {
        return 0;
    }

    psu_eeprom_node = kzalloc(sizeof(struct psu_eeprom_client_node), GFP_KERNEL);
    if (!psu_eeprom_node) {
        dev_dbg(&client->dev, "Can't allocate psu_eeprom_client_node (0x%x)\n", client->addr);
        return -ENOMEM;
    }

    psu_eeprom_node->client = client;

    mutex_lock(&list_lock);
    list_add(&psu_eeprom_node->list, &psu_eeprom_client_list);
    mutex_unlock(&list_lock);

    return 0;
}

int pddf_custom_psu_post_remove(struct i2c_client *client)
{
    struct list_head    *list_node = NULL;
    struct psu_eeprom_client_node *psu_eeprom_node = NULL;
    int found = 0;

    mutex_lock(&list_lock);

    list_for_each(list_node, &psu_eeprom_client_list) {
        psu_eeprom_node = list_entry(list_node, struct psu_eeprom_client_node, list);

        if (psu_eeprom_node->client == client) {
            list_del_init(&psu_eeprom_node->list);
            found = 1;
            break;
        }
    }

    if (found) {
        kfree(psu_eeprom_node);
    }

    mutex_unlock(&list_lock);

    return 0;
}


static int __init pddf_custom_psu_init(void)
{
    mutex_init(&list_lock);
    access_psu_v_out.show = pddf_show_custom_psu_v_out;
    access_psu_v_out.do_get = NULL;
    access_psu_model_name.post_get = pddf_post_get_custom_psu_model_name;
    access_psu_fan_dir.post_get = pddf_post_get_custom_psu_fan_dir;
    pddf_psu_ops.post_probe = pddf_custom_psu_post_probe;
    pddf_psu_ops.post_remove = pddf_custom_psu_post_remove;
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

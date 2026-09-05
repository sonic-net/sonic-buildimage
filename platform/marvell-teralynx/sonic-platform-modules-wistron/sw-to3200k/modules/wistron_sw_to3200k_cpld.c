
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/version.h>
#include <linux/stat.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/delay.h>

#define PORT_NUM               32
#define SYSLED_NUM             5
#define SW_TO3200K_FPGA_ADDR   0x60

static LIST_HEAD(cpld_client_list);
static struct mutex     list_lock;

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

enum cpld_type {
    sw_to3200k_fpga,
    sw_to3200k_cpld1,
    sw_to3200k_cpld2
};

struct sw_to3200k_cpld_data {
    enum cpld_type   type;
    struct device   *hwmon_dev;
    struct mutex     lock;
    int              version;
    int              sysled_status[SYSLED_NUM];
    int              present[PORT_NUM];
    int              reset[PORT_NUM];
};

static const struct i2c_device_id sw_to3200k_cpld_id[] = {
    { "sw_to3200k_fpga", sw_to3200k_fpga },
    { "sw_to3200k_cpld1", sw_to3200k_cpld1 },
    { "sw_to3200k_cpld2", sw_to3200k_cpld2 },
    { }
};
MODULE_DEVICE_TABLE(i2c, sw_to3200k_cpld_id);

#define TRANSCEIVER_PRESENT_ATTR_ID(index)      MODULE_PRESENT_##index
#define TRANSCEIVER_RESET_ATTR_ID(index)        MODULE_RESET_##index

enum sw_to3200k_cpld_sysfs_attributes {
    /* chip version */
    CPLD_VERSION,
    /* system led */
    SYSTEM_LED_LOC,
    SYSTEM_LED_DIAG,
    SYSTEM_LED_FAN,
    SYSTEM_LED_PSU1,
    SYSTEM_LED_PSU2,
    /* transceiver attributes */
    TRANSCEIVER_PRESENT_ATTR_ID(1),
    TRANSCEIVER_PRESENT_ATTR_ID(2),
    TRANSCEIVER_PRESENT_ATTR_ID(3),
    TRANSCEIVER_PRESENT_ATTR_ID(4),
    TRANSCEIVER_PRESENT_ATTR_ID(5),
    TRANSCEIVER_PRESENT_ATTR_ID(6),
    TRANSCEIVER_PRESENT_ATTR_ID(7),
    TRANSCEIVER_PRESENT_ATTR_ID(8),
    TRANSCEIVER_PRESENT_ATTR_ID(9),
    TRANSCEIVER_PRESENT_ATTR_ID(10),
    TRANSCEIVER_PRESENT_ATTR_ID(11),
    TRANSCEIVER_PRESENT_ATTR_ID(12),
    TRANSCEIVER_PRESENT_ATTR_ID(13),
    TRANSCEIVER_PRESENT_ATTR_ID(14),
    TRANSCEIVER_PRESENT_ATTR_ID(15),
    TRANSCEIVER_PRESENT_ATTR_ID(16),
    TRANSCEIVER_PRESENT_ATTR_ID(17),
    TRANSCEIVER_PRESENT_ATTR_ID(18),
    TRANSCEIVER_PRESENT_ATTR_ID(19),
    TRANSCEIVER_PRESENT_ATTR_ID(20),
    TRANSCEIVER_PRESENT_ATTR_ID(21),
    TRANSCEIVER_PRESENT_ATTR_ID(22),
    TRANSCEIVER_PRESENT_ATTR_ID(23),
    TRANSCEIVER_PRESENT_ATTR_ID(24),
    TRANSCEIVER_PRESENT_ATTR_ID(25),
    TRANSCEIVER_PRESENT_ATTR_ID(26),
    TRANSCEIVER_PRESENT_ATTR_ID(27),
    TRANSCEIVER_PRESENT_ATTR_ID(28),
    TRANSCEIVER_PRESENT_ATTR_ID(29),
    TRANSCEIVER_PRESENT_ATTR_ID(30),
    TRANSCEIVER_PRESENT_ATTR_ID(31),
    TRANSCEIVER_PRESENT_ATTR_ID(32),
    TRANSCEIVER_RESET_ATTR_ID(1),
    TRANSCEIVER_RESET_ATTR_ID(2),
    TRANSCEIVER_RESET_ATTR_ID(3),
    TRANSCEIVER_RESET_ATTR_ID(4),
    TRANSCEIVER_RESET_ATTR_ID(5),
    TRANSCEIVER_RESET_ATTR_ID(6),
    TRANSCEIVER_RESET_ATTR_ID(7),
    TRANSCEIVER_RESET_ATTR_ID(8),
    TRANSCEIVER_RESET_ATTR_ID(9),
    TRANSCEIVER_RESET_ATTR_ID(10),
    TRANSCEIVER_RESET_ATTR_ID(11),
    TRANSCEIVER_RESET_ATTR_ID(12),
    TRANSCEIVER_RESET_ATTR_ID(13),
    TRANSCEIVER_RESET_ATTR_ID(14),
    TRANSCEIVER_RESET_ATTR_ID(15),
    TRANSCEIVER_RESET_ATTR_ID(16),
    TRANSCEIVER_RESET_ATTR_ID(17),
    TRANSCEIVER_RESET_ATTR_ID(18),
    TRANSCEIVER_RESET_ATTR_ID(19),
    TRANSCEIVER_RESET_ATTR_ID(20),
    TRANSCEIVER_RESET_ATTR_ID(21),
    TRANSCEIVER_RESET_ATTR_ID(22),
    TRANSCEIVER_RESET_ATTR_ID(23),
    TRANSCEIVER_RESET_ATTR_ID(24),
    TRANSCEIVER_RESET_ATTR_ID(25),
    TRANSCEIVER_RESET_ATTR_ID(26),
    TRANSCEIVER_RESET_ATTR_ID(27),
    TRANSCEIVER_RESET_ATTR_ID(28),
    TRANSCEIVER_RESET_ATTR_ID(29),
    TRANSCEIVER_RESET_ATTR_ID(30),
    TRANSCEIVER_RESET_ATTR_ID(31),
    TRANSCEIVER_RESET_ATTR_ID(32),
};

/* sysfs attributes for hwmon */
static ssize_t get_status(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_status(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t get_version(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_version(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t get_mode_reset(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_mode_reset(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t get_led_status(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_led_status(struct device *dev, struct device_attribute *da, const char *buf, size_t count);

/* version */
static SENSOR_DEVICE_ATTR(version, S_IWUSR | S_IRUGO, get_version, set_version, CPLD_VERSION);

/* system led */
static SENSOR_DEVICE_ATTR(sys_led_loc,  S_IWUSR | S_IRUGO, get_led_status, set_led_status, SYSTEM_LED_LOC);
static SENSOR_DEVICE_ATTR(sys_led_diag, S_IWUSR | S_IRUGO, get_led_status, set_led_status, SYSTEM_LED_DIAG);
static SENSOR_DEVICE_ATTR(sys_led_fan,  S_IWUSR | S_IRUGO, get_led_status, set_led_status, SYSTEM_LED_FAN);
static SENSOR_DEVICE_ATTR(sys_led_psu1, S_IWUSR | S_IRUGO, get_led_status, set_led_status, SYSTEM_LED_PSU1);
static SENSOR_DEVICE_ATTR(sys_led_psu2, S_IWUSR | S_IRUGO, get_led_status, set_led_status, SYSTEM_LED_PSU2);

/* transceiver attributes */
#define DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(index) \
    static SENSOR_DEVICE_ATTR(module_present_##index, S_IWUSR | S_IRUGO, get_status, set_status, MODULE_PRESENT_##index)
#define DECLARE_TRANSCEIVER_PRESENT_ATTR(index)  &sensor_dev_attr_module_present_##index.dev_attr.attr

/*reset*/
#define DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(index) \
    static SENSOR_DEVICE_ATTR(module_reset_##index, S_IWUSR | S_IRUGO, get_mode_reset, set_mode_reset, MODULE_RESET_##index)
#define DECLARE_TRANSCEIVER_RESET_ATTR(index)  &sensor_dev_attr_module_reset_##index.dev_attr.attr

/* transceiver attributes */
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(1);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(2);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(3);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(4);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(5);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(6);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(7);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(8);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(9);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(10);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(11);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(12);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(13);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(14);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(15);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(16);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(17);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(18);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(19);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(20);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(21);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(22);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(23);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(24);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(25);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(26);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(27);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(28);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(29);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(30);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(31);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(32);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(1);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(2);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(3);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(4);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(5);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(6);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(7);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(8);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(9);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(10);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(11);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(12);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(13);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(14);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(15);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(16);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(17);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(18);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(19);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(20);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(21);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(22);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(23);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(24);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(25);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(26);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(27);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(28);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(29);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(30);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(31);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(32);

static struct attribute *sw_to3200k_fpga_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_sys_led_loc.dev_attr.attr,
    &sensor_dev_attr_sys_led_diag.dev_attr.attr,
    &sensor_dev_attr_sys_led_fan.dev_attr.attr,
    &sensor_dev_attr_sys_led_psu1.dev_attr.attr,
    &sensor_dev_attr_sys_led_psu2.dev_attr.attr,
    NULL
};

static const struct attribute_group sw_to3200k_fpga_group = {
    .attrs = sw_to3200k_fpga_attributes,
};

static struct attribute *sw_to3200k_cpld1_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    DECLARE_TRANSCEIVER_PRESENT_ATTR(1),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(2),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(3),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(4),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(5),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(6),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(7),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(8),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(9),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(10),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(11),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(12),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(13),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(14),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(15),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(16),
    DECLARE_TRANSCEIVER_RESET_ATTR(1),
    DECLARE_TRANSCEIVER_RESET_ATTR(2),
    DECLARE_TRANSCEIVER_RESET_ATTR(3),
    DECLARE_TRANSCEIVER_RESET_ATTR(4),
    DECLARE_TRANSCEIVER_RESET_ATTR(5),
    DECLARE_TRANSCEIVER_RESET_ATTR(6),
    DECLARE_TRANSCEIVER_RESET_ATTR(7),
    DECLARE_TRANSCEIVER_RESET_ATTR(8),
    DECLARE_TRANSCEIVER_RESET_ATTR(9),
    DECLARE_TRANSCEIVER_RESET_ATTR(10),
    DECLARE_TRANSCEIVER_RESET_ATTR(11),
    DECLARE_TRANSCEIVER_RESET_ATTR(12),
    DECLARE_TRANSCEIVER_RESET_ATTR(13),
    DECLARE_TRANSCEIVER_RESET_ATTR(14),
    DECLARE_TRANSCEIVER_RESET_ATTR(15),
    DECLARE_TRANSCEIVER_RESET_ATTR(16),
    NULL
};

static const struct attribute_group sw_to3200k_cpld1_group = {
    .attrs = sw_to3200k_cpld1_attributes,
};

static struct attribute *sw_to3200k_cpld2_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    DECLARE_TRANSCEIVER_PRESENT_ATTR(17),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(18),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(19),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(20),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(21),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(22),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(23),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(24),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(25),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(26),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(27),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(28),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(29),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(30),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(31),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(32),
    DECLARE_TRANSCEIVER_RESET_ATTR(17),
    DECLARE_TRANSCEIVER_RESET_ATTR(18),
    DECLARE_TRANSCEIVER_RESET_ATTR(19),
    DECLARE_TRANSCEIVER_RESET_ATTR(20),
    DECLARE_TRANSCEIVER_RESET_ATTR(21),
    DECLARE_TRANSCEIVER_RESET_ATTR(22),
    DECLARE_TRANSCEIVER_RESET_ATTR(23),
    DECLARE_TRANSCEIVER_RESET_ATTR(24),
    DECLARE_TRANSCEIVER_RESET_ATTR(25),
    DECLARE_TRANSCEIVER_RESET_ATTR(26),
    DECLARE_TRANSCEIVER_RESET_ATTR(27),
    DECLARE_TRANSCEIVER_RESET_ATTR(28),
    DECLARE_TRANSCEIVER_RESET_ATTR(29),
    DECLARE_TRANSCEIVER_RESET_ATTR(30),
    DECLARE_TRANSCEIVER_RESET_ATTR(31),
    DECLARE_TRANSCEIVER_RESET_ATTR(32),
    NULL
};

static const struct attribute_group sw_to3200k_cpld2_group = {
    .attrs = sw_to3200k_cpld2_attributes,
};

static ssize_t get_status(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_cpld_data     *data = i2c_get_clientdata(client);
    int                             update_idx, status = 0;

    update_idx = attr->index - MODULE_PRESENT_1;
    mutex_lock(&data->lock);
    status = data->present[update_idx];
    mutex_unlock(&data->lock);
    return sprintf(buf, "%d", status);
}

static ssize_t set_status
(
    struct device *dev,
    struct device_attribute *da,
    const char *buf,
    size_t count
)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_cpld_data     *data = i2c_get_clientdata(client);
    long                            status=0;
    int                             update_idx, error;

    error = kstrtol(buf, 10, &status);
    if (error)
    {
        return error;
    }

    update_idx = attr->index - MODULE_PRESENT_1;
    mutex_lock(&data->lock);
    data->present[update_idx] = status;
    mutex_unlock(&data->lock);
    return count;
}

static ssize_t get_version(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_cpld_data     *data = i2c_get_clientdata(client);
    int                             version;

    mutex_lock(&data->lock);
    version = data->version;
    mutex_unlock(&data->lock);
    return sprintf(buf, "%d", version);
}

static ssize_t set_version
(
    struct device *dev,
    struct device_attribute *da,
    const char *buf,
    size_t count
)
{
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_cpld_data     *data = i2c_get_clientdata(client);
    int                             error, version;

    error = kstrtoint(buf, 10, &version);
    if (error)
    {
        return error;
    }

    mutex_lock(&data->lock);
    data->version = version;
    mutex_unlock(&data->lock);
    return count;
}

static ssize_t get_mode_reset(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_cpld_data     *data = i2c_get_clientdata(client);
    int                             update_idx, reset = 0;

    update_idx = attr->index - MODULE_RESET_1;
    mutex_lock(&data->lock);
    reset = data->reset[update_idx];
    mutex_unlock(&data->lock);
    return sprintf(buf, "%d", reset);
}

static ssize_t set_mode_reset(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_cpld_data     *data = i2c_get_clientdata(client);
    int                             error, reset;
    int                             update_idx;

    error = kstrtoint(buf, 10, &reset);
    if (error)
    {
        return error;
    }

    update_idx = attr->index - MODULE_RESET_1;
    mutex_lock(&data->lock);
    data->reset[update_idx] = reset;
    mutex_unlock(&data->lock);
    return count;
}

static ssize_t get_led_status(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_cpld_data     *data = i2c_get_clientdata(client);
    int                             update_idx, status = 0;

    update_idx = attr->index - SYSTEM_LED_LOC;
    mutex_lock(&data->lock);
    status = data->sysled_status[update_idx];
    mutex_unlock(&data->lock);
    return sprintf(buf, "%d", status);
}

static ssize_t set_led_status
(
    struct device *dev,
    struct device_attribute *da,
    const char *buf,
    size_t count
)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_cpld_data     *data = i2c_get_clientdata(client);
    int                             error, status;
    int                             update_idx;

    error = kstrtoint(buf, 10, &status);
    if (error)
    {
        return error;
    }

    update_idx = attr->index - SYSTEM_LED_LOC;
    mutex_lock(&data->lock);
    data->sysled_status[update_idx] = status;
    mutex_unlock(&data->lock);
    return count;
}

static int get_led_status_internal(struct i2c_client *client, int led_type)
{
    struct sw_to3200k_cpld_data     *data = i2c_get_clientdata(client);
    int                             status = 0;

    mutex_lock(&data->lock);
    status = data->sysled_status[led_type];
    mutex_unlock(&data->lock);
    return status;
}

static int set_led_status_internal(struct i2c_client *client, int led_type, int light_mode)
{
    struct sw_to3200k_cpld_data     *data = i2c_get_clientdata(client);

    mutex_lock(&data->lock);
    data->sysled_status[led_type] = light_mode;
    mutex_unlock(&data->lock);
    return 0;
}

static void sw_to3200k_cpld_add_client(struct i2c_client *client)
{
    struct cpld_client_node *node = kzalloc(sizeof(struct cpld_client_node), GFP_KERNEL);

    if (!node)
    {
        dev_dbg(&client->dev, "Can't allocate cpld_client_node (0x%x)\n", client->addr);
        return;
    }

    node->client = client;

    mutex_lock(&list_lock);
    list_add(&node->list, &cpld_client_list);
    mutex_unlock(&list_lock);
}

static void sw_to3200k_cpld_remove_client(struct i2c_client *client)
{
    struct list_head        *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int                     found = 0;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client == client)
        {
            found = 1;
            break;
        }
    }

    if (found)
    {
        list_del(list_node);
        kfree(cpld_node);
    }

    mutex_unlock(&list_lock);
}

/*
 * I2C init/probing/exit functions
 */
static int sw_to3200k_cpld_probe(struct i2c_client *client)
{
    const struct i2c_device_id *id = i2c_client_get_device_id(client);
    struct sw_to3200k_cpld_data     *data;
    int                             ret = -ENODEV;
    const struct attribute_group    *group = NULL;

    data = kzalloc(sizeof(struct sw_to3200k_cpld_data), GFP_KERNEL);
    if (!data)
    {
        ret = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    mutex_init(&data->lock);
    data->type = id->driver_data;

    /* Register sysfs hooks */
    switch (data->type)
    {
        case sw_to3200k_fpga:
            group = &sw_to3200k_fpga_group;
            break;
        case sw_to3200k_cpld1:
            group = &sw_to3200k_cpld1_group;
            break;
        case sw_to3200k_cpld2:
            group = &sw_to3200k_cpld2_group;
            break;
        default:
            break;
    }

    if (group)
    {
        ret = sysfs_create_group(&client->dev.kobj, group);
        if (ret)
        {
            goto exit_free;
        }
    }

    sw_to3200k_cpld_add_client(client);
    return 0;

exit_free:
    kfree(data);
exit:
    return ret;
}

static void sw_to3200k_cpld_remove(struct i2c_client *client)
{
    struct sw_to3200k_cpld_data *data = i2c_get_clientdata(client);
    const struct attribute_group *group = NULL;

    sw_to3200k_cpld_remove_client(client);

    /* Remove sysfs hooks */
    switch (data->type)
    {
        case sw_to3200k_fpga:
            group = &sw_to3200k_fpga_group;
            break;
        case sw_to3200k_cpld1:
            group = &sw_to3200k_cpld1_group;
            break;
        case sw_to3200k_cpld2:
            group = &sw_to3200k_cpld2_group;
            break;
        default:
            break;
    }

    if (group)
    {
        sysfs_remove_group(&client->dev.kobj, group);
    }

    kfree(data);
}

int sw_to3200k_fpga_sysled_get(int led_type)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EPERM;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == SW_TO3200K_FPGA_ADDR) {
            ret = get_led_status_internal(cpld_node->client, led_type);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(sw_to3200k_fpga_sysled_get);

int sw_to3200k_fpga_sysled_set(int led_type, int light_mode)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == SW_TO3200K_FPGA_ADDR) {
            ret = set_led_status_internal(cpld_node->client, led_type, light_mode);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(sw_to3200k_fpga_sysled_set);

static struct i2c_driver sw_to3200k_cpld_driver = {
    .driver     = {
        .name   = "sw_to3200k_cpld",
        .owner  = THIS_MODULE,
    },
    .probe      = sw_to3200k_cpld_probe,
    .remove     = sw_to3200k_cpld_remove,
    .id_table   = sw_to3200k_cpld_id,
};

static int __init sw_to3200k_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&sw_to3200k_cpld_driver);
}

static void __exit sw_to3200k_cpld_exit(void)
{
    i2c_del_driver(&sw_to3200k_cpld_driver);
}

MODULE_AUTHOR("Haowei Chung <haowei_chung@wistron.com>");
MODULE_DESCRIPTION("Wistron I2C CPLD driver");
MODULE_LICENSE("GPL");

module_init(sw_to3200k_cpld_init);
module_exit(sw_to3200k_cpld_exit);


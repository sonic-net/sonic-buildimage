#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hwmon-sysfs.h>
#include <linux/sysfs.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/slab.h>

#include "wb_pmbus.h"
#include <wb_bsp_kernel_debug.h>

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

#define MP8870_PAGE_NUM         (1)

enum chips {
    MP8870,
};

struct mp8870_data {
    struct pmbus_driver_info info;
    struct i2c_client *client;
    s32 rtop;
    s32 rbottom;
};

#define to_mp8870_data(_info) container_of(_info, struct mp8870_data, info)

static ssize_t mp8870_avs_vout_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    int vout_cmd, vout_scale_loop;
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    struct mp8870_data *mp8870_data = to_mp8870_data(wb_pmbus_get_driver_info(client));
    s32 vout_cmd_data;
    s32 vout_scale_loop_data;
    s64 vout;

    mutex_lock(&data->update_lock);
    vout_cmd = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_COMMAND);
    vout_scale_loop = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_SCALE_LOOP);
    mutex_unlock(&data->update_lock);
    if (vout_cmd < 0 || vout_scale_loop < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, vout command reg: 0x%x or scale loop reg: 0x%x failed, vout_cmd: %d, vout_scale_loop: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, PMBUS_VOUT_SCALE_LOOP, vout_cmd, vout_scale_loop);
        return -EINVAL;
    }

    vout_cmd_data = vout_cmd & 0x7fff;
    if (vout_cmd & 0x8000)
        vout_cmd_data = -vout_cmd_data;

    vout_scale_loop_data = vout_scale_loop & 0x03ff;
    if (vout_scale_loop & 0x0400)
        vout_scale_loop_data = -vout_scale_loop_data;

    DEBUG_INFO("vout_cmd_data: %d, vout_scale_loop_data: %d\n", vout_cmd_data, vout_scale_loop_data);
    DEBUG_INFO("rtop: %d, rbottom: %d\n", mp8870_data->rtop, mp8870_data->rbottom);
    /*
     * VOUT_COMMAND: bit15 sign, bit[14:0] value * 2^-10
     * VOUT_SCALE_LOOP: bit10 sign, bit[9:0] value * 2^-9
     * VFB = VOUT_COMMAND * VOUT_SCALE_LOOP = (cmd_data * scale_data) * 2^-19
     */
    vout = div_s64((s64)vout_cmd_data * vout_scale_loop_data * 1000L * 1000L, (1L << 19));
    DEBUG_INFO("vout: %lld\n", vout);
    vout = div_s64(vout * (mp8870_data->rtop + mp8870_data->rbottom), mp8870_data->rbottom);

    return snprintf(buf, PAGE_SIZE, "%lld\n", vout);
}

static ssize_t mp8870_avs_vout_store(struct device *dev, struct device_attribute *devattr,
                   const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    struct mp8870_data *mp8870_data = to_mp8870_data(wb_pmbus_get_driver_info(client));
    int ret, vout_scale_loop;
    s64 vout;
    s32 vout_cmd_data;
    s32 vout_scale_loop_data;
    u16 vout_cmd_reg;

    vout = 0;
    ret = kstrtos64(buf, 0, &vout);
    if (ret) {
        DEBUG_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }

    if (vout <= 0) {
        DEBUG_ERROR("%d-%04x: invalid value: %lld \n", client->adapter->nr, client->addr, vout);
        return -EINVAL;
    }

    /* Convert Vout to VFB by divider ratio: VFB = Vout * rbottom / (rtop + rbottom). */
    vout = div_s64(vout * mp8870_data->rbottom, (mp8870_data->rtop + mp8870_data->rbottom));
    DEBUG_INFO("vout: %lld\n", vout);
    mutex_lock(&data->update_lock);
    vout_scale_loop = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_SCALE_LOOP);
    mutex_unlock(&data->update_lock);
    if (vout_scale_loop < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, scale loop reg: 0x%x failed, vout_scale_loop: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_SCALE_LOOP, vout_scale_loop);
        return -EINVAL;
    }
    DEBUG_INFO("vout_scale_loop: 0x%x\n", vout_scale_loop);

    vout_scale_loop_data = vout_scale_loop & 0x03ff;
    if (vout_scale_loop & 0x0400) {
        vout_scale_loop_data = -vout_scale_loop_data;
    }

    if (vout_scale_loop_data == 0) {
        DEBUG_ERROR("%d-%04x: invalid scale loop value, page%d, reg: 0x%x, raw: 0x%x\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_SCALE_LOOP, vout_scale_loop);
        return -EINVAL;
    }
    DEBUG_INFO("vout_scale_loop_data: %d\n", vout_scale_loop_data);

    /*
     * Reverse of show path:
     * VFB(mV) = cmd_data * scale_data * 1000 / 2^19
     * cmd_data = VFB(mV) * 2^19 / (scale_data * 1000)
     */
    vout_cmd_data = div_s64(vout * (1L << 19), (s64)vout_scale_loop_data * 1000L * 1000L);
    DEBUG_INFO("vout_cmd_data: %d\n", vout_cmd_data);

    if (vout_cmd_data > 0x7fff || vout_cmd_data < -0x7fff) {
        DEBUG_ERROR("%d-%04x: invalid value, page%d, vout: %lld, vout_cmd_data: %d\n",
            client->adapter->nr, client->addr, attr->index, vout, vout_cmd_data);
        return -EINVAL;
    }

    if (vout_cmd_data < 0) {
        vout_cmd_reg = 0x8000 | (u16)(-vout_cmd_data);
    } else {
        vout_cmd_reg = (u16)vout_cmd_data;
    }
    DEBUG_INFO("vout_cmd_reg: 0x%x\n", vout_cmd_reg);

    mutex_lock(&data->update_lock);
    ret = wb_pmbus_write_word_data(client, attr->index, PMBUS_VOUT_COMMAND, vout_cmd_reg);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set page%d vout cmd reg: 0x%x, value: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, vout_cmd_reg, ret);
        return ret;
    }

    return count;
}

static SENSOR_DEVICE_ATTR_RW(avs0_vout, mp8870_avs_vout, 0);

static struct attribute *avs_ctrl_attrs[] = {
    &sensor_dev_attr_avs0_vout.dev_attr.attr,
    NULL,
};

static const struct attribute_group avs_ctrl_group = {
    .attrs = avs_ctrl_attrs,
};

static const struct attribute_group *mp8870_attribute_groups[] = {
    &avs_ctrl_group,
    NULL,
};

static struct pmbus_driver_info mp8870_info = {
    .pages = MP8870_PAGE_NUM,
    .func[0] = PMBUS_HAVE_STATUS_INPUT | PMBUS_HAVE_STATUS_TEMP
    | PMBUS_HAVE_STATUS_VOUT | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_POUT,
    .groups = mp8870_attribute_groups,
};

static int mp8870_parse_rtop_rbottom(struct i2c_client *client, struct mp8870_data *mp8870_data)
{
    struct device *dev = &client->dev;
    struct device_node *np = dev->of_node;
    s32 rtop = 0;
    s32 rbottom = 1;
    int rtop_ret;
    int rbottom_ret;

    mp8870_data->rtop = 0;
    mp8870_data->rbottom = 1;

    if (!np) {
        return 0;
    }

    rtop_ret = of_property_read_s32(np, "rtop", &rtop);
    rbottom_ret = of_property_read_s32(np, "rbottom", &rbottom);
    if (rtop_ret && rbottom_ret) {
        dev_dbg(dev, "rtop and rbottom not config, use default values\n");
        return 0;
    }

    if (rtop_ret || rbottom_ret) {
        dev_err(dev, "rtop and rbottom must be configured together\n");
        return -EINVAL;
    }

    if (rbottom == 0) {
        dev_err(dev, "rbottom cannot be 0\n");
        return -EINVAL;
    }

    mp8870_data->rtop = rtop;
    mp8870_data->rbottom = rbottom;
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0)
static int mp8870_probe(struct i2c_client *client,
             const struct i2c_device_id *id)
#else
static int mp8870_probe(struct i2c_client *client)
#endif
{
	int ret;
    struct mp8870_data *mp8870_data;
    struct pmbus_driver_info *info;

    mp8870_data = devm_kzalloc(&client->dev, sizeof(*mp8870_data), GFP_KERNEL);
    if (!mp8870_data) {
        dev_err(&client->dev, "devm_kzalloc failed.\n");
        return -ENOMEM;
    }

    mp8870_data->client = client;
    info = &mp8870_data->info;
    memcpy(info, &mp8870_info, sizeof(*info));

    ret = mp8870_parse_rtop_rbottom(client, mp8870_data);
    if (ret != 0) {
        return ret;
    }
    dev_info(&client->dev, "rtop: %d, rbottom: %d\n", mp8870_data->rtop, mp8870_data->rbottom);

    ret = wb_pmbus_do_probe(client, info);
    if (ret != 0) {
        dev_err(&client->dev, "wb_pmbus_do_probe failed, ret: %d.\n", ret);
        return ret;
    }

    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0)
static int mp8870_remove(struct i2c_client *client)
#else
static void mp8870_remove(struct i2c_client *client)
#endif
{
    (void)wb_pmbus_do_remove(client);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0)
    return 0;
#endif
}

static const struct i2c_device_id mp8870_id[] = {
    {"wb_mp8870", MP8870},
    {}
};

MODULE_DEVICE_TABLE(i2c, mp8870_id);

static const struct of_device_id __maybe_unused mp8870_of_match[] = {
    {.compatible = "wb_mp8870", .data = (void *)MP8870},
    {}
};
MODULE_DEVICE_TABLE(of, mp8870_of_match);

static struct i2c_driver mp8870_driver = {
    .driver = {
        .name = "wb_mp8870",
        .of_match_table = of_match_ptr(mp8870_of_match),
    },
    .probe = mp8870_probe,
    .remove = mp8870_remove,
    .id_table = mp8870_id,
};

module_i2c_driver(mp8870_driver);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("PMBus driver for MPS family");
MODULE_LICENSE("GPL");

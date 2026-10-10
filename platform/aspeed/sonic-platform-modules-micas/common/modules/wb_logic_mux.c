#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/of.h>

#include <wb_bsp_i2c_debug.h>
#include "wb_logic_mux.h"

#define LOGIC_MUX_MAX_NCHANS        (8)

static int switch_chan_delay = 400;   /* 400us */
module_param(switch_chan_delay, int, S_IRUGO | S_IWUSR);

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

enum logic_mux_type {
    logic_mux_chan1,
    logic_mux_chan2,
    logic_mux_chan4,
    logic_mux_chan8,
};

struct logic_mux {
    enum logic_mux_type type;
    const char *dev_name;
    uint32_t reg_offset;
    uint32_t logic_dev_func_mode;
    uint32_t base_nr;
    bool probe_hw_init;
    bool no_deselect;
    unsigned long write_intf_addr;
    unsigned long read_intf_addr;
};

struct chip_desc {
    u8 nchans;
    u8 enable;    /* used for muxes only */
    enum muxtype {
        ismux = 0,
        isswi
    } muxtype;
};

/* Provide specs for the logic mux types we know about */
static const struct chip_desc chips[] = {
    [logic_mux_chan1] = {
        .nchans = 1,
        .muxtype = isswi,
    },
    [logic_mux_chan2] = {
        .nchans = 2,
        .muxtype = isswi,
    },
    [logic_mux_chan4] = {
        .nchans = 4,
        .muxtype = isswi,
    },
    [logic_mux_chan8] = {
        .nchans = 8,
        .muxtype = isswi,
    }
};

static const struct i2c_device_id logic_mux_id[] = {
    { "wb_logic_mux_chan1", logic_mux_chan1 },
    { "wb_logic_mux_chan2", logic_mux_chan2 },
    { "wb_logic_mux_chan4", logic_mux_chan4 },
    { "wb_logic_mux_chan8", logic_mux_chan8 },
    { }
};
MODULE_DEVICE_TABLE(i2c, logic_mux_id);

static int logic_mux_reg_write(struct logic_mux *data, u8 val)
{
    device_func_write pfunc;

    DEBUG_VERBOSE("dev_name: %s, reg_offset: 0x%x, write val: 0x%02x\n",
        data->dev_name, data->reg_offset, val);
    pfunc = (device_func_write)data->write_intf_addr;
    return pfunc(data->dev_name, data->reg_offset, &val, sizeof(u8));
}

static int logic_mux_select_chan(struct i2c_mux_core *muxc, u32 chan)
{
    int ret;
    u8 regval;
    struct logic_mux *data = i2c_mux_priv(muxc);

    regval = 1 << chan;
    DEBUG_VERBOSE("select channel: %u, regval: 0x%02x\n", chan, regval);

    if (data == NULL) {
        DEBUG_INFO("logic_mux data is NULL\n");
        return -EINVAL;
    }

    ret = logic_mux_reg_write(data, regval);
    if (ret < 0) {
        DEBUG_INFO("logic_mux_reg_write failed, ret: %d\n", ret);
        return ret;
    }
    DEBUG_VERBOSE("select channel success\n");
    if (switch_chan_delay > 0) {
        usleep_range(switch_chan_delay, switch_chan_delay + 1);
    }

    return 0;
}

static int logic_mux_deselect_mux(struct i2c_mux_core *muxc, u32 chan)
{
    int ret;
    struct logic_mux *data = i2c_mux_priv(muxc);

    DEBUG_VERBOSE("deselect channel: %u\n", chan);
    if (data == NULL) {
        DEBUG_INFO("logic_mux data is NULL\n");
        return -EINVAL;
    }

    if (data->no_deselect) {
        DEBUG_VERBOSE("no need to deselect channel\n");
        return 0;
    }

    /* Deselect active channel */
    ret = logic_mux_reg_write(data, 0);
    if (ret < 0) {
        DEBUG_INFO("deselect channel: %u failed, ret: %d\n", chan, ret);
        return ret;
    }
    DEBUG_VERBOSE("deselect channel success\n");
    if (switch_chan_delay > 0) {
        usleep_range(switch_chan_delay, switch_chan_delay + 1);
    }
    return 0;
}

static int of_logic_mux_data_init(struct i2c_client *client, struct logic_mux *data)
{
    int ret;

    ret = 0;
    ret += of_property_read_string(client->dev.of_node, "dev_name", &data->dev_name);
    ret += of_property_read_u32(client->dev.of_node, "reg_offset", &data->reg_offset);
    ret += of_property_read_u32(client->dev.of_node, "logic_dev_func_mode", &data->logic_dev_func_mode);
    ret += of_property_read_u32(client->dev.of_node, "base_nr", &data->base_nr);
    if (ret != 0) {
        dev_err(&client->dev, "Failed to get dts config, ret: %d.\n", ret);
        return -ENXIO;
    }
    data->probe_hw_init = of_property_read_bool(client->dev.of_node, "probe_hw_init");
    data->no_deselect = of_property_read_bool(client->dev.of_node, "no_deselect");

    DEBUG_VERBOSE("dev_name: %s, reg_offset: 0x%x, logic_dev_func_mode: %u, base_nr: %u, probe_hw_init: %d, no_deselect: %d\n",
        data->dev_name, data->reg_offset, data->logic_dev_func_mode, data->base_nr, data->probe_hw_init, data->no_deselect);

    ret = find_intf_addr(&data->write_intf_addr, &data->read_intf_addr, data->logic_dev_func_mode);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to find_intf_addr, func mode %u, ret: %d\n",
            data->logic_dev_func_mode, ret);
        return ret;
    }

    if (!data->write_intf_addr || !data->read_intf_addr) {
        dev_err(&client->dev, "Fail: func mode %u rw symbol undefined.\n", data->logic_dev_func_mode);
        return -ENOSYS;
    }
    return 0;
}

static int logic_mux_data_init(struct i2c_client *client, struct logic_mux *data)
{
    int ret;
    logic_mux_device_t *logic_mux_device;

    if (client->dev.platform_data == NULL) {
        dev_err(&client->dev, "Failed to get platform data config.\n");
        return -ENXIO;
    }

    logic_mux_device = client->dev.platform_data;
    data->dev_name = logic_mux_device->dev_name;
    data->reg_offset = logic_mux_device->reg_offset;
    data->logic_dev_func_mode = logic_mux_device->logic_dev_func_mode;
    data->base_nr = logic_mux_device->base_nr;
    data->probe_hw_init = logic_mux_device->probe_hw_init;
    data->no_deselect = logic_mux_device->no_deselect;

    DEBUG_VERBOSE("dev_name: %s, reg_offset: 0x%x, logic_dev_func_mode: %u, base_nr: %u, probe_hw_init: %d, no_deselect: %d\n",
        data->dev_name, data->reg_offset, data->logic_dev_func_mode, data->base_nr, data->probe_hw_init, data->no_deselect);

    ret = find_intf_addr(&data->write_intf_addr, &data->read_intf_addr, data->logic_dev_func_mode);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to find_intf_addr, func mode %u, ret: %d\n",
            data->logic_dev_func_mode, ret);
        return ret;
    }

    if (!data->write_intf_addr || !data->read_intf_addr) {
        dev_err(&client->dev, "Fail: func mode %u rw symbol undefined.\n", data->logic_dev_func_mode);
        return -ENOSYS;
    }
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
static int logic_mux_probe(struct i2c_client *client, const struct i2c_device_id *id)
#else
static int logic_mux_probe(struct i2c_client *client)
#endif
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
    const struct i2c_device_id *id = i2c_client_get_device_id(client);
#endif
    struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
    int num, force, class;
    struct logic_mux *data;
    struct i2c_mux_core *muxc;
    int ret;

    muxc = i2c_mux_alloc(adap, &client->dev,
             LOGIC_MUX_MAX_NCHANS, sizeof(*data), 0,
             logic_mux_select_chan, logic_mux_deselect_mux);

    if (!muxc) {
        dev_info(&client->dev, "Failed to alloc logic mux\n");
        return -ENOMEM;
    }

    data = i2c_mux_priv(muxc);
    i2c_set_clientdata(client, muxc);
    data->type = id->driver_data;

    if (client->dev.of_node) {
        ret= of_logic_mux_data_init(client, data);
    } else {
        ret= logic_mux_data_init(client, data);
    }
    if (ret < 0) {
        return ret;
    }

    if(data->probe_hw_init) {
        ret = logic_mux_reg_write(data, 0); /* close channel */
        if (ret < 0) {
            dev_info(&client->dev, "probe_hw_init close channel failed, ret: %d\n", ret);
        }
    }

    /* Now create an adapter for each channel */
    for (num = 0; num < chips[data->type].nchans; num++) {
        class = 0;              /* no class by default */
        force = data->base_nr + num;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,10,0)
        ret = i2c_mux_add_adapter(muxc, force, num);
#else
        ret = i2c_mux_add_adapter(muxc, force, num, class);
#endif
        if (ret) {
            dev_err(&client->dev,"Failed to register multiplexed adapter %d as bus %d\n",
                num, force);
            goto fail_del_adapters;
        }
    }

    dev_info(&client->dev,
             "registered %d multiplexed busses for I2C %s %s\n",
             num, chips[data->type].muxtype == ismux
             ? "mux" : "switch", client->name);
    return 0;

fail_del_adapters:
    i2c_mux_del_adapters(muxc);
    return ret;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
static int logic_mux_remove(struct i2c_client *client)
#else
static void logic_mux_remove(struct i2c_client *client)
#endif
{
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);

    i2c_mux_del_adapters(muxc);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
    return 0;
#endif
}

static struct i2c_driver logic_mux_driver = {
    .driver        = {
        .name          = "wb_logic_mux",
        .owner         = THIS_MODULE,
    },
    .probe             = logic_mux_probe,
    .remove            = logic_mux_remove,
    .id_table          = logic_mux_id,
};

static int __init logic_mux_init(void)
{
    return i2c_add_driver(&logic_mux_driver);
}

static void __exit logic_mux_exit(void)
{
    i2c_del_driver(&logic_mux_driver);
}

module_init(logic_mux_init);
module_exit(logic_mux_exit);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("Logic device mux/switch driver");
MODULE_LICENSE("GPL");

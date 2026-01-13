#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/platform_data/i2c-mux-gpio.h>
#include <linux/platform_device.h>
#include <linux/i2c/sff-8436.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/gpio/machine.h>
#include <linux/gpio_keys.h>
#include <linux/nvram.h>

#define S6000_MUX_BASE_NR   10
#define QSFP_MODULE_BASE_NR 20

/* 74CBTLV3253 Dual 1-of-4 multiplexer/demultiplexer */
#define MUX_CHANNEL_NUM     2

#define CPLD_DEVICE_NUM     3
#define QSFP_MODULE_NUM     16
#define QSFP_DEVICE_NUM     2

#define GPIO_I2C_MUX_PIN    10

#define RTC_NVRAM_REBOOT_REASON_OFFSET 0x49

static void device_release(struct device *dev)
{
    return;
}

/*
 * S6000 74CBTLV3253 MUX
 */
static const unsigned s6000_mux_gpios[] = {
    1, 2
};

static const unsigned s6000_mux_values[] = {
    0, 1, 2, 3
};

static struct i2c_mux_gpio_platform_data s6000_mux_platform_data = {
    .parent             = 2,
    .base_nr            = S6000_MUX_BASE_NR,
    .values             = s6000_mux_values,
    .n_values           = ARRAY_SIZE(s6000_mux_values),
    .idle               = 0,
};

static struct gpiod_lookup_table dell_gpio_desc = {
	.dev_id = "i2c-mux-gpio",
	.table = {
                GPIO_LOOKUP_IDX("sch_gpio.3168", 1, "mux", 0, GPIO_ACTIVE_HIGH ),
                GPIO_LOOKUP_IDX("sch_gpio.3168", 2, "mux", 1, GPIO_ACTIVE_HIGH ),
		{ },
	},
};

static struct platform_device s6000_mux = {
    .name               = "i2c-mux-gpio",
    .id                 = 0,
    .dev                = {
                .init_name = "i2c-mux-gpio",
                .platform_data   = &s6000_mux_platform_data,
                .release          = device_release
    },
};

/*
 * S6000 CPLD
 */

enum cpld_type {
    system_cpld,
    master_cpld,
    slave_cpld,
};

struct cpld_platform_data {
    int reg_addr;
    struct i2c_client *client;
};

static struct cpld_platform_data s6000_cpld_platform_data[] = {
    [system_cpld] = {
        .reg_addr = 0x31,
    },

    [master_cpld] = {
        .reg_addr = 0x32,
    },

    [slave_cpld] = {
        .reg_addr = 0x33,
    },
};

static struct platform_device s6000_cpld = {
    .name               = "dell-s6000-cpld",
    .id                 = 0,
    .dev                = {
                .platform_data   = s6000_cpld_platform_data,
                .release         = device_release
    },
};

/*
 * S6000 QSFP MUX
 */

struct qsfp_mux_platform_data {
    int parent;
    int base_nr;
    int reg_addr;
    struct i2c_client *cpld;
};

struct qsfp_mux {
    struct qsfp_mux_platform_data data;
};

static struct qsfp_mux_platform_data s6000_qsfp_mux_platform_data[] = {
    {
        .parent         = S6000_MUX_BASE_NR + 2,
        .base_nr        = QSFP_MODULE_BASE_NR,
        .cpld           = NULL,
        .reg_addr       = 0x0,
    },
    {
        .parent         = S6000_MUX_BASE_NR + 3,
        .base_nr        = QSFP_MODULE_BASE_NR + QSFP_MODULE_NUM,
        .cpld           = NULL,
        .reg_addr       = 0xa,
    },
};

static struct platform_device s6000_qsfp_mux[] = {
    {
        .name           = "dell-s6000-qsfp-mux",
        .id             = 0,
        .dev            = {
                .platform_data   = &s6000_qsfp_mux_platform_data[0],
                .release         = device_release,
        },
    },
    {
        .name           = "dell-s6000-qsfp-mux",
        .id             = 1,
        .dev            = {
                .platform_data   = &s6000_qsfp_mux_platform_data[1],
                .release         = device_release,
        },
    },
};

static int cpld_reg_write_byte(struct i2c_client *client, u8 regaddr, u8 val)
{
    union i2c_smbus_data data;

    data.byte = val;
    return client->adapter->algo->smbus_xfer(client->adapter, client->addr,
                                             client->flags,
                                             I2C_SMBUS_WRITE,
                                             regaddr, I2C_SMBUS_BYTE_DATA, &data);
}

static int qsfp_mux_select(struct i2c_mux_core *muxc, u32 chan)
{
    struct qsfp_mux *mux = i2c_mux_priv(muxc);
    unsigned short mask = ~(unsigned short)(1 << chan);

    cpld_reg_write_byte(mux->data.cpld, mux->data.reg_addr, (u8)(mask & 0xff));
    return cpld_reg_write_byte(mux->data.cpld, mux->data.reg_addr + 1, (u8)(mask >> 8));
}

static int __init qsfp_mux_probe(struct platform_device *pdev)
{
    struct i2c_mux_core *muxc;
    struct qsfp_mux *mux;
    struct qsfp_mux_platform_data *pdata;
    struct i2c_adapter *parent;
    int i, ret;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
        return -ENODEV;
    }

    mux = devm_kzalloc(&pdev->dev, sizeof(*mux), GFP_KERNEL);
    if (!mux) {
        return -ENOMEM;
    }

    mux->data = *pdata;

    parent = i2c_get_adapter(pdata->parent);
    if (!parent) {
        dev_err(&pdev->dev, "Parent adapter (%d) not found\n",
            pdata->parent);
        return -EPROBE_DEFER;
    }

    muxc = i2c_mux_alloc(parent, &pdev->dev, QSFP_MODULE_NUM, 0, 0,
                         qsfp_mux_select, NULL);
    if (!muxc) {
        ret = -ENOMEM;
        goto alloc_failed;
    }
    muxc->priv = mux;

    platform_set_drvdata(pdev, muxc);

    for (i = 0; i < QSFP_MODULE_NUM; i++) {
        int nr = pdata->base_nr + i;

        ret = i2c_mux_add_adapter(muxc, nr, i);
        if (ret) {
            dev_err(&pdev->dev, "Failed to add adapter %d\n", i);
            goto add_adapter_failed;
        }
    }

    dev_info(&pdev->dev, "%d port mux on %s adapter\n", QSFP_MODULE_NUM, parent->name);

    return 0;

add_adapter_failed:
    i2c_mux_del_adapters(muxc);
alloc_failed:
    i2c_put_adapter(parent);

    return ret;
}

static void qsfp_mux_remove(struct platform_device *pdev)
{
    struct i2c_mux_core *muxc = platform_get_drvdata(pdev);

    i2c_mux_del_adapters(muxc);

    i2c_put_adapter(muxc->parent);
}

static struct platform_driver qsfp_mux_driver = {
    .probe  = qsfp_mux_probe,
    .remove = qsfp_mux_remove,
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "dell-s6000-qsfp-mux",
    },
};

/* TODO */
/* module_platform_driver */

static int dell_i2c_smbus_read_byte_data(const struct i2c_client *client,
                                         u8 command)
{
    int ret = 0;

    ret = i2c_smbus_read_byte_data(client, command);
    if(ret < 0) {
        printk(KERN_WARNING "I2C smbus read failed. Resetting mux with gpio10");
        gpio_set_value(GPIO_I2C_MUX_PIN, 1);
        gpio_set_value(GPIO_I2C_MUX_PIN, 0);
        ret = i2c_smbus_read_byte_data(client, command);
    }
    return ret;
}

static int dell_i2c_smbus_write_byte_data(const struct i2c_client *client,
                                          u8 command, u8 value)
{
    int ret = 0;

    ret = i2c_smbus_write_byte_data(client, command, value);
    if(ret < 0)
    {
        printk(KERN_WARNING "I2C smbus write failed. Resetting mux with gpio10");
        gpio_set_value(GPIO_I2C_MUX_PIN, 1);
        gpio_set_value(GPIO_I2C_MUX_PIN, 0);
        ret = i2c_smbus_write_byte_data(client, command, value);
    }
    return ret;
}

static ssize_t get_modsel(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[slave_cpld].client, 0x0);
    if (ret < 0)
        return sprintf(buf, "na");
    data = (u32)ret & 0xff;

    ret = dell_i2c_smbus_read_byte_data(pdata[slave_cpld].client, 0x1);
    if (ret < 0)
        return sprintf(buf, "na");
    data |= (u32)(ret & 0xff) << 8;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0xa);
    if (ret < 0)
        return sprintf(buf, "na");
    data |= (u32)(ret & 0xff) << 16;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0xb);
    if (ret < 0)
        return sprintf(buf, "na");
    data |= (u32)(ret & 0xff) << 24;

    return sprintf(buf, "0x%08x\n", data);
}

static ssize_t set_modsel(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    int err;
    unsigned long data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    err = kstrtoul(buf, 16, &data);
    if (err)
	return err;

    dell_i2c_smbus_write_byte_data(pdata[slave_cpld].client, 0x0, (u8)(data & 0xff));
    dell_i2c_smbus_write_byte_data(pdata[slave_cpld].client, 0x1, (u8)((data >> 8) & 0xff));
    dell_i2c_smbus_write_byte_data(pdata[master_cpld].client, 0xa, (u8)((data >> 16) & 0xff));
    dell_i2c_smbus_write_byte_data(pdata[master_cpld].client, 0xb, (u8)((data >> 24) & 0xff));

    msleep(2); // As per HW spec
    return count;
}

static ssize_t get_lpmode(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[slave_cpld].client, 0x2);
    if (ret < 0)
        return sprintf(buf, "na");
    data = (u32)ret & 0xff;

    ret = dell_i2c_smbus_read_byte_data(pdata[slave_cpld].client, 0x3);
    if (ret < 0)
        return sprintf(buf, "na");
    data |= (u32)(ret & 0xff) << 8;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0xc);
    if (ret < 0)
        return sprintf(buf, "na");
    data |= (u32)(ret & 0xff) << 16;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0xd);
    if (ret < 0)
        return sprintf(buf, "na");
    data |= (u32)(ret & 0xff) << 24;

    return sprintf(buf, "0x%08x\n", data);
}

static ssize_t set_lpmode(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long data;
    int err;
    struct cpld_platform_data *pdata = dev->platform_data;

    err = kstrtoul(buf, 16, &data);
    if (err)
        return err;

    dell_i2c_smbus_write_byte_data(pdata[slave_cpld].client, 0x2, (u8)(data & 0xff));
    dell_i2c_smbus_write_byte_data(pdata[slave_cpld].client, 0x3, (u8)((data >> 8) & 0xff));
    dell_i2c_smbus_write_byte_data(pdata[master_cpld].client, 0xc, (u8)((data >> 16) & 0xff));
    dell_i2c_smbus_write_byte_data(pdata[master_cpld].client, 0xd, (u8)((data >> 24) & 0xff));

    return count;
}

static ssize_t get_reset(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[slave_cpld].client, 0x6);
    if (ret < 0)
        return sprintf(buf, "na");
    data = (u32)ret & 0xff;

    ret = dell_i2c_smbus_read_byte_data(pdata[slave_cpld].client, 0x7);
    if (ret < 0)
        return sprintf(buf, "na");
    data |= (u32)(ret & 0xff) << 8;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x10);
    if (ret < 0)
        return sprintf(buf, "na");
    data |= (u32)(ret & 0xff) << 16;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x11);
    if (ret < 0)
        return sprintf(buf, "na");
    data |= (u32)(ret & 0xff) << 24;

    return sprintf(buf, "0x%08x\n", data);
}

static ssize_t set_reset(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long data;
    int err;
    struct cpld_platform_data *pdata = dev->platform_data;

    err = kstrtoul(buf, 16, &data);
    if (err)
        return err;

    dell_i2c_smbus_write_byte_data(pdata[slave_cpld].client, 0x6, (u8)(data & 0xff));
    dell_i2c_smbus_write_byte_data(pdata[slave_cpld].client, 0x7, (u8)((data >> 8)& 0xff));
    dell_i2c_smbus_write_byte_data(pdata[master_cpld].client, 0x10, (u8)((data >> 16) & 0xff));
    dell_i2c_smbus_write_byte_data(pdata[master_cpld].client, 0x11, (u8)((data >> 24) & 0xff));

    return count;
}

static ssize_t get_modprs(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[slave_cpld].client, 0x4);
    if (ret < 0)
        return sprintf(buf, "read error");
    data = (u32)ret & 0xff;

    ret = dell_i2c_smbus_read_byte_data(pdata[slave_cpld].client, 0x5);
    if (ret < 0)
        return sprintf(buf, "read error");
    data |= (u32)(ret & 0xff) << 8;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0xe);
    if (ret < 0)
        return sprintf(buf, "na");
    data |= (u32)(ret & 0xff) << 16;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0xf);
    if (ret < 0)
        return sprintf(buf, "na");
    data |= (u32)(ret & 0xff) << 24;

    return sprintf(buf, "0x%08x\n", data);
}

static ssize_t set_power_reset(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long data;
    int err;
    struct cpld_platform_data *pdata = dev->platform_data;

    err = kstrtoul(buf, 10, &data);
    if (err)
        return err;

    if (data)
    {
        dell_i2c_smbus_write_byte_data(pdata[system_cpld].client, 0x1, (u8)(0xfd));
    }

    return count;
}

static ssize_t get_power_reset(struct device *dev, struct device_attribute *devattr, char *buf)
{
    uint8_t ret = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[system_cpld].client, 0x1);
    if (ret < 0)
        return sprintf(buf, "read error");

    return sprintf(buf, "0x%x\n", ret);
}

static ssize_t get_fan_prs(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x8);
    if (ret < 0)
        return sprintf(buf, "read error");
    data = (u32)((ret & 0xc0) >> 6);

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x9);
    if (ret < 0)
        return sprintf(buf, "read error");
    data |= (u32)((ret & 0x01) << 2);
    data = ~data & 0x7;

    return sprintf(buf, "0x%x\n", data);
}

static ssize_t get_psu0_prs(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x3);
    if (ret < 0)
        return sprintf(buf, "read error");

    if (!(ret & 0x80))
        data = 1;

    return sprintf(buf, "%d\n", data);
}

static ssize_t get_psu1_prs(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x3);
    if (ret < 0)
        return sprintf(buf, "read error");

    if (!(ret & 0x08))
        data = 1;

    return sprintf(buf, "%d\n", data);
}

static ssize_t get_psu0_status(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x3);
    if (ret < 0)
        return sprintf(buf, "read error");

    if (!(ret & 0x40))
        data = 1;

    return sprintf(buf, "%d\n", data);
}

static ssize_t get_psu1_status(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x3);
    if (ret < 0)
        return sprintf(buf, "read error");

    if (!(ret & 0x04))
        data = 1;

    return sprintf(buf, "%d\n", data);
}

static ssize_t get_powersupply_status(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int data;
    struct cpld_platform_data *pdata = dev->platform_data;

    data = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x3);
    if (data < 0)
        return sprintf(buf, "read error");

    return sprintf(buf, "%x\n", data);
}

static ssize_t get_system_led(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x7);
    if (ret < 0)
        return sprintf(buf, "read error");

    data = (u32)(ret & 0x60) >> 5;

    switch (data)
    {
        case 0:
            ret = sprintf(buf, "blink_green\n");
            break;
        case 1:
            ret = sprintf(buf, "green\n");
            break;
        case 2:
            ret = sprintf(buf, "yellow\n");
            break;
        default:
            ret = sprintf(buf, "blink_yellow\n");
    }

    return ret;
}

static ssize_t set_system_led(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    if (!strncmp(buf, "blink_green", 11))
    {
        data = 0;
    }
    else if (!strncmp(buf, "green", 5))
    {
        data = 1;
    }
    else if (!strncmp(buf, "yellow", 6))
    {
        data = 2;
    }
    else if (!strncmp(buf, "blink_yellow", 12))
    {
        data = 3;
    }
    else
    {
        return -1;
    }

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x7);
    if (ret < 0)
        return ret;

    dell_i2c_smbus_write_byte_data(pdata[master_cpld].client, 0x7, (u8)((ret & 0x9F) | (data << 5)));

    return count;
}

static ssize_t get_locator_led(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x7);
    if (ret < 0)
        return sprintf(buf, "read error");

    data = (u32)(ret & 0x18) >> 3;

    switch (data)
    {
        case 0:
            ret = sprintf(buf, "off\n");
            break;
        case 1:
            ret = sprintf(buf, "blink_blue\n");
            break;
        case 2:
            ret = sprintf(buf, "blue\n");
            break;
        default:
            ret = sprintf(buf, "invalid\n");
    }

    return ret;
}

static ssize_t set_locator_led(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    if (!strncmp(buf, "off", 3))
    {
        data = 0;
    }
    else if (!strncmp(buf, "blink_blue", 10))
    {
        data = 1;
    }
    else if (!strncmp(buf, "blue", 4))
    {
        data = 2;
    }
    else
    {
        return -1;
    }

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x7);
    if (ret < 0)
        return ret;

    dell_i2c_smbus_write_byte_data(pdata[master_cpld].client, 0x7, (u8)((ret & 0xE7) | (data << 3)));

    return count;
}

static ssize_t get_power_led(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x7);
    if (ret < 0)
        return sprintf(buf, "read error");

    data = (u32)(ret & 0x06) >> 1;

    switch (data)
    {
        case 0:
            ret = sprintf(buf, "off\n");
            break;
        case 1:
            ret = sprintf(buf, "yellow\n");
            break;
        case 2:
            ret = sprintf(buf, "green\n");
            break;
        default:
            ret = sprintf(buf, "blink_yellow\n");
    }

    return ret;
}

static ssize_t set_power_led(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    if (!strncmp(buf, "off", 3))
    {
        data = 0;
    }
    else if (!strncmp(buf, "yellow", 6))
    {
        data = 1;
    }
    else if (!strncmp(buf, "green", 5))
    {
        data = 2;
    }
    else if (!strncmp(buf, "blink_yellow", 12))
    {
        data = 3;
    }
    else
    {
        return -1;
    }

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x7);
    if (ret < 0)
        return ret;

    dell_i2c_smbus_write_byte_data(pdata[master_cpld].client, 0x7, (u8)((ret & 0xF9) | (data << 1)));

    return count;
}

static ssize_t get_master_led(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x7);
    if (ret < 0)
        return sprintf(buf, "read error");

    data = (u32)(ret & 0x1);

    switch (data)
    {
        case 0:
            ret = sprintf(buf, "green\n");
            break;
        default:
            ret = sprintf(buf, "off\n");
            break;
    }

    return ret;
}

static ssize_t set_master_led(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    if (!strncmp(buf, "green", 5))
    {
        data = 0;
    }
    else if (!strncmp(buf, "off", 3))
    {
        data = 1;
    }
    else
    {
        return -1;
    }

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x7);
    if (ret < 0)
        return ret;

    dell_i2c_smbus_write_byte_data(pdata[master_cpld].client, 0x7, (u8)((ret & 0xFE) | data));

    return count;
}

static ssize_t get_fan_led(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x9);
    if (ret < 0)
        return sprintf(buf, "read error");

    data = (u32)(ret & 0x18) >> 3;

    switch (data)
    {
        case 0:
            ret = sprintf(buf, "off\n");
            break;
        case 1:
            ret = sprintf(buf, "yellow\n");
            break;
        case 2:
            ret = sprintf(buf, "green\n");
            break;
        default:
            ret = sprintf(buf, "blink_yellow\n");
    }

    return ret;
}

static ssize_t set_fan_led(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    if (!strncmp(buf, "off", 3))
    {
        data = 0;
    }
    else if (!strncmp(buf, "yellow", 6))
    {
        data = 1;
    }
    else if (!strncmp(buf, "green", 5))
    {
        data = 2;
    }
    else if (!strncmp(buf, "blink_yellow", 12))
    {
        data = 3;
    }
    else
    {
        return -1;
    }

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x9);
    if (ret < 0)
        return ret;

    dell_i2c_smbus_write_byte_data(pdata[master_cpld].client, 0x9, (u8)((ret & 0xE7) | (data << 3)));

    return count;
}

static ssize_t get_fan0_led(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x8);
    if (ret < 0)
        return sprintf(buf, "read error");

    data = (u32)(ret & 0x3);

    switch (data)
    {
        case 0:
            ret = sprintf(buf, "off\n");
            break;
        case 1:
            ret = sprintf(buf, "green\n");
            break;
        case 2:
            ret = sprintf(buf, "yellow\n");
            break;
        default:
            ret = sprintf(buf, "unknown\n");
    }

    return ret;
}

static ssize_t set_fan0_led(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    if (!strncmp(buf, "off", 3))
    {
        data = 0;
    }
    else if (!strncmp(buf, "yellow", 6))
    {
        data = 2;
    }
    else if (!strncmp(buf, "green", 5))
    {
        data = 1;
    }
    else
    {
        return -1;
    }

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x8);
    if (ret < 0)
        return ret;

    dell_i2c_smbus_write_byte_data(pdata[master_cpld].client, 0x8, (u8)((ret & 0xFC) | data));

    return count;
}


static ssize_t get_fan1_led(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x8);
    if (ret < 0)
        return sprintf(buf, "read error");

    data = (u32)(ret & 0xc) >> 2;

    switch (data)
    {
        case 0:
            ret = sprintf(buf, "off\n");
            break;
        case 1:
            ret = sprintf(buf, "green\n");
            break;
        case 2:
            ret = sprintf(buf, "yellow\n");
            break;
        default:
            ret = sprintf(buf, "unknown\n");
    }

    return ret;
}

static ssize_t set_fan1_led(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    if (!strncmp(buf, "off", 3))
    {
        data = 0;
    }
    else if (!strncmp(buf, "yellow", 6))
    {
        data = 2;
    }
    else if (!strncmp(buf, "green", 5))
    {
        data = 1;
    }
    else
    {
        return -1;
    }

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x8);
    if (ret < 0)
        return ret;

    dell_i2c_smbus_write_byte_data(pdata[master_cpld].client, 0x8, (u8)((ret & 0xF3) | (data << 2)));

    return count;
}

static ssize_t get_fan2_led(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x8);
    if (ret < 0)
        return sprintf(buf, "read error");

    data = (u32)(ret & 0x30) >> 4;

    switch (data)
    {
        case 0:
            ret = sprintf(buf, "off\n");
            break;
        case 1:
            ret = sprintf(buf, "green\n");
            break;
        case 2:
            ret = sprintf(buf, "yellow\n");
            break;
        default:
            ret = sprintf(buf, "unknown\n");
    }

    return ret;
}

static ssize_t set_fan2_led(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    if (!strncmp(buf, "off", 3))
    {
        data = 0;
    }
    else if (!strncmp(buf, "yellow", 6))
    {
        data = 2;
    }
    else if (!strncmp(buf, "green", 5))
    {
        data = 1;
    }
    else
    {
        return -1;
    }

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x8);
    if (ret < 0)
        return ret;

    dell_i2c_smbus_write_byte_data(pdata[master_cpld].client, 0x8, (u8)((ret & 0xCF) | (data << 4)));

    return count;
}

static ssize_t get_system_cpld_ver(struct device *dev,
                                   struct device_attribute *devattr, char *buf)
{
    uint8_t ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[system_cpld].client, 0x0);
    if (ret < 0)
        return sprintf(buf, "read error");

    data = ret & (0x0f);

    return sprintf(buf, "0x%x\n", data);
}

static ssize_t get_master_cpld_ver(struct device *dev,
                                   struct device_attribute *devattr, char *buf)
{
    uint8_t ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[master_cpld].client, 0x1);
    if (ret < 0)
        return sprintf(buf, "read error");

    data = ret & (0x0f);

    return sprintf(buf, "0x%x\n", data);
}

static ssize_t get_slave_cpld_ver(struct device *dev,
                                   struct device_attribute *devattr, char *buf)
{
    uint8_t ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = dell_i2c_smbus_read_byte_data(pdata[slave_cpld].client, 0xa);
    if (ret < 0)
        return sprintf(buf, "read error");

    data = ret & (0x0f);

    return sprintf(buf, "0x%x\n", data);
}

static ssize_t get_reboot_reason(struct device *dev,
                                 struct device_attribute *devattr, char *buf)
{
    uint8_t data = 0;

    /* Last Reboot reason in saved in RTC NVRAM offset 0x49
     * We write the reboot reason into nvram offset,
     * as part of platform_reboot implementation from userspace.

     * COLD_RESET = 0xE # Cold Reset (value)
     * WARM_RESET = 0x6 # Warm Reset (value)
     */

    /* Read it from this offset, and export it as last_reboot_reason */
    data = nvram_read_byte(RTC_NVRAM_REBOOT_REASON_OFFSET);

    return sprintf(buf, "0x%x\n", data);
}

static DEVICE_ATTR(qsfp_modsel, S_IRUGO | S_IWUSR, get_modsel, set_modsel);
static DEVICE_ATTR(qsfp_modprs, S_IRUGO, get_modprs, NULL);
static DEVICE_ATTR(qsfp_lpmode, S_IRUGO | S_IWUSR, get_lpmode, set_lpmode);
static DEVICE_ATTR(qsfp_reset,  S_IRUGO | S_IWUSR, get_reset, set_reset);
static DEVICE_ATTR(power_reset, S_IRUGO | S_IWUSR, get_power_reset, set_power_reset);
static DEVICE_ATTR(fan_prs, S_IRUGO, get_fan_prs, NULL);
static DEVICE_ATTR(psu0_prs, S_IRUGO, get_psu0_prs, NULL);
static DEVICE_ATTR(psu1_prs, S_IRUGO, get_psu1_prs, NULL);
static DEVICE_ATTR(psu0_status, S_IRUGO, get_psu0_status, NULL);
static DEVICE_ATTR(psu1_status, S_IRUGO, get_psu1_status, NULL);
static DEVICE_ATTR(powersupply_status, S_IRUGO, get_powersupply_status, NULL);
static DEVICE_ATTR(system_led, S_IRUGO | S_IWUSR, get_system_led, set_system_led);
static DEVICE_ATTR(locator_led, S_IRUGO | S_IWUSR, get_locator_led, set_locator_led);
static DEVICE_ATTR(power_led, S_IRUGO | S_IWUSR, get_power_led, set_power_led);
static DEVICE_ATTR(master_led, S_IRUGO | S_IWUSR, get_master_led, set_master_led);
static DEVICE_ATTR(fan_led, S_IRUGO | S_IWUSR, get_fan_led, set_fan_led);
static DEVICE_ATTR(fan0_led, S_IRUGO | S_IWUSR, get_fan0_led, set_fan0_led);
static DEVICE_ATTR(fan1_led, S_IRUGO | S_IWUSR, get_fan1_led, set_fan1_led);
static DEVICE_ATTR(fan2_led, S_IRUGO | S_IWUSR, get_fan2_led, set_fan2_led);
static DEVICE_ATTR(system_cpld_ver, S_IRUGO, get_system_cpld_ver, NULL);
static DEVICE_ATTR(master_cpld_ver, S_IRUGO, get_master_cpld_ver, NULL);
static DEVICE_ATTR(slave_cpld_ver,  S_IRUGO, get_slave_cpld_ver,  NULL);
static DEVICE_ATTR(last_reboot_reason,  S_IRUGO, get_reboot_reason,  NULL);

static struct attribute *s6000_cpld_attrs[] = {
    &dev_attr_qsfp_modsel.attr,
    &dev_attr_qsfp_lpmode.attr,
    &dev_attr_qsfp_reset.attr,
    &dev_attr_qsfp_modprs.attr,
    &dev_attr_power_reset.attr,
    &dev_attr_fan_prs.attr,
    &dev_attr_psu0_prs.attr,
    &dev_attr_psu1_prs.attr,
    &dev_attr_psu0_status.attr,
    &dev_attr_psu1_status.attr,
    &dev_attr_powersupply_status.attr,
    &dev_attr_system_led.attr,
    &dev_attr_locator_led.attr,
    &dev_attr_power_led.attr,
    &dev_attr_master_led.attr,
    &dev_attr_fan_led.attr,
    &dev_attr_fan0_led.attr,
    &dev_attr_fan1_led.attr,
    &dev_attr_fan2_led.attr,
    &dev_attr_system_cpld_ver.attr,
    &dev_attr_master_cpld_ver.attr,
    &dev_attr_slave_cpld_ver.attr,
    &dev_attr_last_reboot_reason.attr,
    NULL,
};

static struct attribute_group s6000_cpld_attr_grp = {
    .attrs = s6000_cpld_attrs,
};

static int __init cpld_probe(struct platform_device *pdev)
{
    struct cpld_platform_data *pdata;
    struct i2c_adapter *parent;
    int i;
    int ret;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
        return -ENODEV;
    }

    parent = i2c_get_adapter(S6000_MUX_BASE_NR);
    if (!parent) {
        printk(KERN_WARNING "Parent adapter (%d) not found\n",
            S6000_MUX_BASE_NR);
        return -ENODEV;
    }

    for (i = 0; i < CPLD_DEVICE_NUM; i++) {
        pdata[i].client = i2c_new_dummy_device(parent, pdata[i].reg_addr);
        if (!pdata[i].client) {
            printk(KERN_WARNING "Fail to create dummy i2c client for addr %d\n", pdata[i].reg_addr);
            goto error;
        }
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &s6000_cpld_attr_grp);
    if (ret)
        goto error;

    return 0;

error:
    i--;
    for (; i >= 0; i--) {
        if (pdata[i].client) {
            i2c_unregister_device(pdata[i].client);
        }
    }

    i2c_put_adapter(parent);

    return -ENODEV;
}

static void __exit cpld_remove(struct platform_device *pdev)
{
    int i;
    struct i2c_adapter *parent = NULL;
    struct cpld_platform_data *pdata = pdev->dev.platform_data;

    sysfs_remove_group(&pdev->dev.kobj, &s6000_cpld_attr_grp);

    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
    } else {
        for (i = 0; i < CPLD_DEVICE_NUM; i++) {
            if (pdata[i].client) {
                if (!parent) {
                    parent = (pdata[i].client)->adapter;
                }
                i2c_unregister_device(pdata[i].client);
            }
        }
    }

    i2c_put_adapter(parent);
}

static struct platform_driver cpld_driver = {
    .probe  = cpld_probe,
    .remove = __exit_p(cpld_remove),
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "dell-s6000-cpld",
    },
};

static int __init dell_s6000_platform_init(void)
{
    int ret = 0;
    struct cpld_platform_data *cpld_pdata;
    struct qsfp_mux_platform_data *qsfp_pdata;
    int i;
    bool gpio_allocated = false;

    printk("dell_s6000_platform module initialization\n");
    ret = gpio_request(GPIO_I2C_MUX_PIN, "gpio10");
    if(ret < 0) {
        printk(KERN_WARNING "Failed to request gpio 10");
        goto error_gpio_init;
    }
    gpio_allocated = true;

    ret = gpiod_export(gpio_to_desc(GPIO_I2C_MUX_PIN), false);
    if(ret < 0) {
        printk(KERN_WARNING "Failed to export gpio 10");
        goto error_gpio_init;
    }

    ret = gpio_direction_output(GPIO_I2C_MUX_PIN, 0);
    if(ret < 0) {
        printk(KERN_WARNING "Failed to set direction out on gpio 10");
        goto error_gpio_init;
    }

    gpiod_add_lookup_table(&dell_gpio_desc);
    ret = platform_driver_register(&cpld_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register cpld driver\n");
        goto error_cpld_driver;
    }

    ret = platform_driver_register(&qsfp_mux_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register qsfp mux driver\n");
        goto error_qsfp_mux_driver;
    }

    ret = platform_device_register(&s6000_mux);
    if (ret) {
        printk(KERN_WARNING "Fail to create gpio mux\n");
        goto error_mux;
    }

    ret = platform_device_register(&s6000_cpld);
    if (ret) {
        printk(KERN_WARNING "Fail to create cpld device\n");
        goto error_cpld;
    }

    cpld_pdata = s6000_cpld.dev.platform_data;
    qsfp_pdata = s6000_qsfp_mux[0].dev.platform_data;
    qsfp_pdata->cpld = cpld_pdata[slave_cpld].client;
    qsfp_pdata = s6000_qsfp_mux[1].dev.platform_data;
    qsfp_pdata->cpld = cpld_pdata[master_cpld].client;

    for (i = 0; i < QSFP_DEVICE_NUM; i++) {
        ret = platform_device_register(&s6000_qsfp_mux[i]);
        if (ret) {
            printk(KERN_WARNING "fail to create qsfp mux %d\n", i);
            goto error_qsfp_mux;
        }
    }

    if (ret)
        goto error_qsfp_mux;

    return 0;

error_qsfp_mux:
    i--;
    for (; i >= 0; i--) {
        platform_device_unregister(&s6000_qsfp_mux[i]);
    }
    platform_device_unregister(&s6000_cpld);
error_cpld:
    platform_device_unregister(&s6000_mux);
error_mux:
    platform_driver_unregister(&qsfp_mux_driver);
error_qsfp_mux_driver:
    platform_driver_unregister(&cpld_driver);
error_cpld_driver:
    return ret;
error_gpio_init:
    if(gpio_allocated) {
        gpio_free(GPIO_I2C_MUX_PIN);
    }
    return ret;
}

static void __exit dell_s6000_platform_exit(void)
{
    int i;
    gpiod_remove_lookup_table(&dell_gpio_desc);
    for (i = 0; i < MUX_CHANNEL_NUM; i++)
        platform_device_unregister(&s6000_qsfp_mux[i]);
    platform_device_unregister(&s6000_cpld);
    platform_device_unregister(&s6000_mux);

    platform_driver_unregister(&cpld_driver);
    platform_driver_unregister(&qsfp_mux_driver);
    gpio_free(GPIO_I2C_MUX_PIN);
}

module_init(dell_s6000_platform_init);
module_exit(dell_s6000_platform_exit);

MODULE_DESCRIPTION("DELL S6000 Platform Support");
MODULE_AUTHOR("Guohan Lu <gulv@microsoft.com>");
MODULE_LICENSE("GPL");

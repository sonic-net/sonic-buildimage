#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include <linux/version.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/i2c/sff-8436.h>
#include <linux/platform_data/pca954x.h>
#include <linux/platform_data/i2c-mux-gpio.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>



#define agc032_i2c_device_num(c){                                         \
        .name                   = "delta-agc032-i2c-device",                \
        .id                     = c,                                        \
        .dev                    = {                                           \
                    .platform_data = &agc032_i2c_device_platform_data[c], \
                    .release       = device_release,                          \
        },                                                                    \
}



/* Define struct to get client of i2c_new_deivce at 0x70 */
struct i2c_client * i2c_client_9548;

enum{
    BUS0 = 0,
    BUS1,
    BUS2,
    BUS3,
    BUS4,
    BUS5,
    BUS6,
    BUS7,
    BUS8,
};

struct i2c_device_platform_data {
    int parent;
    struct i2c_board_info           info;
    struct i2c_client              *client;
};

/* pca9548 - add 8 bus */
static struct pca954x_platform_mode pca954x_mode[] = {
  { .adap_id = 1,
    .deselect_on_exit = 1,
  },
  { .adap_id = 2,
    .deselect_on_exit = 1,
  },
  { .adap_id = 3,
    .deselect_on_exit = 1,
  },
  { .adap_id = 4,
    .deselect_on_exit = 1,
  },
  { .adap_id = 5,
    .deselect_on_exit = 1,
  },
  { .adap_id = 6,
    .deselect_on_exit = 1,
  },
  { .adap_id = 7,
    .deselect_on_exit = 1,
  },
  { .adap_id = 8,
    .deselect_on_exit = 1,
  },
};

static struct pca954x_platform_data pca954x_data = {
  .modes = pca954x_mode,
  .num_modes = ARRAY_SIZE(pca954x_mode),
};

static struct i2c_board_info __initdata i2c_info_pca9548[] =
{
        {
            I2C_BOARD_INFO("pca9548", 0x70),
            .platform_data = &pca954x_data,
        },
};

static struct i2c_device_platform_data agc032_i2c_device_platform_data[] = {
    {
        /* psu 1 (0x58) */
        .parent = 1,
        .info = { .type = "dni_agc032_psu", .addr = 0x58, .platform_data = (void *) 0 },
        .client = NULL,
    },
    {
        /* psu 2 (0x58) */
        .parent = 2,
        .info = { .type = "dni_agc032_psu", .addr = 0x58, .platform_data = (void *) 1 },
        .client = NULL,
    },
    {
        /* FAN 1 Controller (0x2e) */
        .parent = 4,
        .info = { I2C_BOARD_INFO("emc2302", 0x2e) },
        .client = NULL,
    },
    {
        /* FAN 2 Controller (0x4c) */
        .parent = 4,
        .info = { I2C_BOARD_INFO("emc2305", 0x4c) },
        .client = NULL,
    },
    {
        /* FAN 3 Controller (0x2d) */
        .parent = 4,
        .info = { I2C_BOARD_INFO("emc2305", 0x2d) },
        .client = NULL,
    },
    {
        /* tmp75 (0x4f) */
        .parent = 4,
        .info = { I2C_BOARD_INFO("tmp75", 0x4f) },
        .client = NULL,
    },
    {
        /* tmp75 (0x49) */
        .parent = 5,
        .info = { I2C_BOARD_INFO("tmp75", 0x49) },
        .client = NULL,
    },
    {
        /* tmp75 (0x4a) */
        .parent = 5,
        .info = { I2C_BOARD_INFO("tmp75", 0x4a) },
        .client = NULL,
    },
    {
        /* tmp75 (0x4b) */
        .parent = 5,
        .info = { I2C_BOARD_INFO("tmp75", 0x4b) },
        .client = NULL,
    },
    {
        /* tmp75 (0x4d) */
        .parent = 5,
        .info = { I2C_BOARD_INFO("tmp75", 0x4d) },
        .client = NULL,
    },
    {
        /* tmp75 (0x4e) */
        .parent = 5,
        .info = { I2C_BOARD_INFO("tmp75", 0x4e) },
        .client = NULL,
    },
};

static void device_release(struct device *dev)
{
    return;
}

static struct platform_device agc032_i2c_device[] = {
    agc032_i2c_device_num(0),
    agc032_i2c_device_num(1),
    agc032_i2c_device_num(2),
    agc032_i2c_device_num(3),
    agc032_i2c_device_num(4),
    agc032_i2c_device_num(5),
    agc032_i2c_device_num(6),
    agc032_i2c_device_num(7),
    agc032_i2c_device_num(8),
    agc032_i2c_device_num(9),
    agc032_i2c_device_num(10),
};

static int __init i2c_device_probe(struct platform_device *pdev)
{
    struct i2c_device_platform_data *pdata;
    struct i2c_adapter *parent;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
        return -ENODEV;
    }

    parent = i2c_get_adapter(pdata->parent);
    if (!parent) {
        dev_err(&pdev->dev, "Parent adapter (%d) not found\n",
            pdata->parent);
        return -ENODEV;
    }

    pdata->client = i2c_new_device(parent, &pdata->info);
    if (!pdata->client) {
        dev_err(&pdev->dev, "Failed to create i2c client %s at %d\n",
            pdata->info.type, pdata->parent);
        return -ENODEV;
    }

    return 0;
}

static int __exit i2c_deivce_remove(struct platform_device *pdev)
{
    struct i2c_adapter *parent;
    struct i2c_device_platform_data *pdata;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
        return -ENODEV;
    }

    if (pdata->client) {
        parent = (pdata->client)->adapter;
        i2c_unregister_device(pdata->client);
        i2c_put_adapter(parent);
    }

    return 0;
}


static struct platform_driver i2c_device_driver = {
    .probe = i2c_device_probe,
    .remove = __exit_p(i2c_deivce_remove),
    .driver = {
        .owner = THIS_MODULE,
        .name = "delta-agc032-i2c-device",
    }
};

static int __init delta_agc032_platform_init(void)
{
    struct i2c_adapter *adapter;
    struct cpld_platform_data *cpld_pdata;
    int ret = 0;
    int cnt = 0;

    printk("agc032_platform module initialization\n");

    adapter = i2c_get_adapter(BUS0);
//    i2c_client_9548 = i2c_new_device(adapter, &i2c_info_pca9548[0]);

    i2c_put_adapter(adapter);

    /* register the i2c devices */
    ret = platform_driver_register(&i2c_device_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register i2c device driver\n");
        goto error_i2c_device_driver;
    }

    for (cnt = 0; cnt < ARRAY_SIZE(agc032_i2c_device); cnt++)
    {
        ret = platform_device_register(&agc032_i2c_device[cnt]);
        if (ret) {
            printk(KERN_WARNING "Fail to create i2c device %d\n", cnt);
            goto error_agc032_i2c_device;
        }
    }

    return 0;

error_agc032_i2c_device:
    for (; cnt >= 0; cnt--) {
        platform_device_unregister(&agc032_i2c_device[cnt]);
    }
    platform_driver_unregister(&i2c_device_driver);
error_i2c_device_driver:
    return ret;
}

static void __exit delta_agc032_platform_exit(void)
{
    int i = 0;

    for ( i = 0; i < ARRAY_SIZE(agc032_i2c_device); i++ ) {
        platform_device_unregister(&agc032_i2c_device[i]);
    }

    platform_driver_unregister(&i2c_device_driver);
//    i2c_unregister_device(i2c_client_9548);
}


module_init(delta_agc032_platform_init);
module_exit(delta_agc032_platform_exit);

MODULE_DESCRIPTION("DNI agc032 Platform Support");
MODULE_AUTHOR("James Ke <james.ke@deltaww.com>");
MODULE_LICENSE("GPL");

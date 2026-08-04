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
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/platform_data/pca954x.h>
#include <linux/platform_data/i2c-mux-gpio.h>



#define agc032_i2c_device1_num(c){                                         \
        .name                   = "delta-agc032-i2c-pca9548-1",                \
        .id                     = c,                                        \
        .dev                    = {                                           \
                    .platform_data = &agc032_i2c_device_pca9548_1_data[c], \
                    .release       = device_release,                          \
        },                                                                    \
}

#define agc032_i2c_device2_num(c){                                         \
        .name                   = "delta-agc032-i2c-pca9548-2",                \
        .id                     = c,                                        \
        .dev                    = {                                           \
                    .platform_data = &agc032_i2c_device_pca9548_2_data[c], \
                    .release       = device_release,                          \
        },                                                                    \
}

#define agc032_i2c_device3_num(c){                                         \
        .name                   = "delta-agc032-i2c-pca9548-3",                \
        .id                     = c,                                        \
        .dev                    = {                                           \
                    .platform_data = &agc032_i2c_device_pca9548_3_data[c], \
                    .release       = device_release,                          \
        },                                                                    \
}

#define agc032_i2c_device4_num(c){                                         \
        .name                   = "delta-agc032-i2c-pca9548-4",                \
        .id                     = c,                                        \
        .dev                    = {                                           \
                    .platform_data = &agc032_i2c_device_pca9548_4_data[c], \
                    .release       = device_release,                          \
        },                                                                    \
}

#define agc032_i2c_device5_num(c){                                         \
        .name                   = "delta-agc032-i2c-pca9548-5",                \
        .id                     = c,                                        \
        .dev                    = {                                           \
                    .platform_data = &agc032_i2c_device_pca9548_5_data[c], \
                    .release       = device_release,                          \
        },                                                                    \
}

/* Define struct to get client of i2c_new_deivce at 0x70, 0x71, 0x72, 0x73 */
struct i2c_client * i2c_client_9548_1;
struct i2c_client * i2c_client_9548_2;
struct i2c_client * i2c_client_9548_3;
struct i2c_client * i2c_client_9548_4;
struct i2c_client * i2c_client_9548_5;

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

/* pca9548-1 - add 8 bus */
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

/* pca9548-2 - add 8 bus */
static struct pca954x_platform_mode pca954x_mode2[] = {
  { .adap_id = 9,
    .deselect_on_exit = 1,
  },
  { .adap_id = 10,
    .deselect_on_exit = 1,
  },
  { .adap_id = 11,
    .deselect_on_exit = 1,
  },
  { .adap_id = 12,
    .deselect_on_exit = 1,
  },
  { .adap_id = 13,
    .deselect_on_exit = 1,
  },
  { .adap_id = 14,
    .deselect_on_exit = 1,
  },
  { .adap_id = 15,
    .deselect_on_exit = 1,
  },
  { .adap_id = 16,
    .deselect_on_exit = 1,
  },
};

/* pca9548-3 - add 8 bus */
static struct pca954x_platform_mode pca954x_mode3[] = {
  { .adap_id = 17,
    .deselect_on_exit = 1,
  },
  { .adap_id = 18,
    .deselect_on_exit = 1,
  },
  { .adap_id = 19,
    .deselect_on_exit = 1,
  },
  { .adap_id = 20,
    .deselect_on_exit = 1,
  },
  { .adap_id = 21,
    .deselect_on_exit = 1,
  },
  { .adap_id = 22,
    .deselect_on_exit = 1,
  },
  { .adap_id = 23,
    .deselect_on_exit = 1,
  },
  { .adap_id = 24,
    .deselect_on_exit = 1,
  },
};

/* pca9548-4 - add 8 bus */
static struct pca954x_platform_mode pca954x_mode4[] = {
  { .adap_id = 25,
    .deselect_on_exit = 1,
  },
  { .adap_id = 26,
    .deselect_on_exit = 1,
  },
  { .adap_id = 27,
    .deselect_on_exit = 1,
  },
  { .adap_id = 28,
    .deselect_on_exit = 1,
  },
  { .adap_id = 29,
    .deselect_on_exit = 1,
  },
  { .adap_id = 30,
    .deselect_on_exit = 1,
  },
  { .adap_id = 31,
    .deselect_on_exit = 1,
  },
  { .adap_id = 32,
    .deselect_on_exit = 1,
  },
};

/* pca9548-5 - add 2 bus */
static struct pca954x_platform_mode pca954x_mode5[] = {
  { .adap_id = 33,
    .deselect_on_exit = 1,
  },
  { .adap_id = 34,
    .deselect_on_exit = 1,
  },
};

static struct pca954x_platform_data pca954x_data = {
  .modes = pca954x_mode,
  .num_modes = ARRAY_SIZE(pca954x_mode),
};

static struct pca954x_platform_data pca954x_data2 = {
  .modes = pca954x_mode2,
  .num_modes = ARRAY_SIZE(pca954x_mode2),
};

static struct pca954x_platform_data pca954x_data3 = {
  .modes = pca954x_mode3,
  .num_modes = ARRAY_SIZE(pca954x_mode3),
};

static struct pca954x_platform_data pca954x_data4 = {
  .modes = pca954x_mode4,
  .num_modes = ARRAY_SIZE(pca954x_mode4),
};

static struct pca954x_platform_data pca954x_data5 = {
  .modes = pca954x_mode5,
  .num_modes = ARRAY_SIZE(pca954x_mode5),
};

static struct i2c_board_info __initdata i2c_info_pca9548[] =
{
        {
            I2C_BOARD_INFO("pca9548", 0x70),
            .platform_data = &pca954x_data,
        },
        {
            I2C_BOARD_INFO("pca9548", 0x71),
            .platform_data = &pca954x_data2,
        },
        {
            I2C_BOARD_INFO("pca9548", 0x72),
            .platform_data = &pca954x_data3,
        },
        {
            I2C_BOARD_INFO("pca9548", 0x73),
            .platform_data = &pca954x_data4,
        },
        {
            I2C_BOARD_INFO("pca9548", 0x74),
            .platform_data = &pca954x_data5,
        },
};

static struct i2c_device_platform_data agc032_i2c_device_pca9548_1_data[] = {
    {
        // qsfp 1 (0x50)
        .parent = 1,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 2 (0x50)
        .parent = 2,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 3 (0x50)
        .parent = 3,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 4 (0x50)
        .parent = 4,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 5 (0x50)
        .parent = 5,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 6 (0x50)
        .parent = 6,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 7 (0x50)
        .parent = 7,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 8 (0x50)
        .parent = 8,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
};

static struct i2c_device_platform_data agc032_i2c_device_pca9548_2_data[] = {
    {
        // qsfp 9 (0x50)
        .parent = 9,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 10 (0x50)
        .parent = 10,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 11 (0x50)
        .parent = 11,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 12 (0x50)
        .parent = 12,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 13 (0x50)
        .parent = 13,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 14 (0x50)
        .parent = 14,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 15 (0x50)
        .parent = 15,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 16 (0x50)
        .parent = 16,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
};

static struct i2c_device_platform_data agc032_i2c_device_pca9548_3_data[] = {
    {
        // qsfp 17 (0x50)
        .parent = 17,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 18 (0x50)
        .parent = 18,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 19 (0x50)
        .parent = 19,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 20 (0x50)
        .parent = 20,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 21 (0x50)
        .parent = 21,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 22 (0x50)
        .parent = 22,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 23 (0x50)
        .parent = 23,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 24 (0x50)
        .parent = 24,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
};

static struct i2c_device_platform_data agc032_i2c_device_pca9548_4_data[] = {
    {
        // qsfp 25 (0x50)
        .parent = 25,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 26 (0x50)
        .parent = 26,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 27 (0x50)
        .parent = 27,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 28 (0x50)
        .parent = 28,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 29 (0x50)
        .parent = 29,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 30 (0x50)
        .parent = 30,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 31 (0x50)
        .parent = 31,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 32 (0x50)
        .parent = 32,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
};

static struct i2c_device_platform_data agc032_i2c_device_pca9548_5_data[] = {
    {
        // qsfp 33 (0x50)
        .parent = 33,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 34 (0x50)
        .parent = 34,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
};

static void device_release(struct device *dev)
{
    return;
}

static struct platform_device agc032_i2c_device1[] = {
    agc032_i2c_device1_num(0),
    agc032_i2c_device1_num(1),
    agc032_i2c_device1_num(2),
    agc032_i2c_device1_num(3),
    agc032_i2c_device1_num(4),
    agc032_i2c_device1_num(5),
    agc032_i2c_device1_num(6),
    agc032_i2c_device1_num(7),
};

static struct platform_device agc032_i2c_device2[] = {
    agc032_i2c_device2_num(0),
    agc032_i2c_device2_num(1),
    agc032_i2c_device2_num(2),
    agc032_i2c_device2_num(3),
    agc032_i2c_device2_num(4),
    agc032_i2c_device2_num(5),
    agc032_i2c_device2_num(6),
    agc032_i2c_device2_num(7),
};

static struct platform_device agc032_i2c_device3[] = {
    agc032_i2c_device3_num(0),
    agc032_i2c_device3_num(1),
    agc032_i2c_device3_num(2),
    agc032_i2c_device3_num(3),
    agc032_i2c_device3_num(4),
    agc032_i2c_device3_num(5),
    agc032_i2c_device3_num(6),
    agc032_i2c_device3_num(7),
};

static struct platform_device agc032_i2c_device4[] = {
    agc032_i2c_device4_num(0),
    agc032_i2c_device4_num(1),
    agc032_i2c_device4_num(2),
    agc032_i2c_device4_num(3),
    agc032_i2c_device4_num(4),
    agc032_i2c_device4_num(5),
    agc032_i2c_device4_num(6),
    agc032_i2c_device4_num(7),
};

static struct platform_device agc032_i2c_device5[] = {
    agc032_i2c_device5_num(0),
    agc032_i2c_device5_num(1),
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


static struct platform_driver i2c_device_pca9548_1_driver = {
    .probe = i2c_device_probe,
    .remove = __exit_p(i2c_deivce_remove),
    .driver = {
        .owner = THIS_MODULE,
        .name = "delta-agc032-i2c-pca9548-1",
    }
};

static struct platform_driver i2c_device_pca9548_2_driver = {
    .probe = i2c_device_probe,
    .remove = __exit_p(i2c_deivce_remove),
    .driver = {
        .owner = THIS_MODULE,
        .name = "delta-agc032-i2c-pca9548-2",
    }
};

static struct platform_driver i2c_device_pca9548_3_driver = {
    .probe = i2c_device_probe,
    .remove = __exit_p(i2c_deivce_remove),
    .driver = {
        .owner = THIS_MODULE,
        .name = "delta-agc032-i2c-pca9548-3",
    }
};

static struct platform_driver i2c_device_pca9548_4_driver = {
    .probe = i2c_device_probe,
    .remove = __exit_p(i2c_deivce_remove),
    .driver = {
        .owner = THIS_MODULE,
        .name = "delta-agc032-i2c-pca9548-4",
    }
};

static struct platform_driver i2c_device_pca9548_5_driver = {
    .probe = i2c_device_probe,
    .remove = __exit_p(i2c_deivce_remove),
    .driver = {
        .owner = THIS_MODULE,
        .name = "delta-agc032-i2c-pca9548-5",
    }
};

static int __init delta_agc032_platform_init(void)
{
    struct i2c_adapter *adapter;
    int ret = 0;
    int device1_cnt = 0;
    int device2_cnt = 0;
    int device3_cnt = 0;
    int device4_cnt = 0;
    int device5_cnt = 0;

    printk("agc032_qsfp module initialization\n");

    adapter = i2c_get_adapter(BUS0);
    i2c_client_9548_1 = i2c_new_device(adapter, &i2c_info_pca9548[0]);
    i2c_client_9548_2 = i2c_new_device(adapter, &i2c_info_pca9548[1]);
    i2c_client_9548_3 = i2c_new_device(adapter, &i2c_info_pca9548[2]);
    i2c_client_9548_4 = i2c_new_device(adapter, &i2c_info_pca9548[3]);
    i2c_client_9548_5 = i2c_new_device(adapter, &i2c_info_pca9548[4]);
    i2c_put_adapter(adapter);

    /* pca9548-0x70 */
    ret = platform_driver_register(&i2c_device_pca9548_1_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register i2c device driver\n");
        goto error_i2c_device_pca9548_1_driver;
    }

    for (device1_cnt = 0; device1_cnt < ARRAY_SIZE(agc032_i2c_device1); device1_cnt++)
    {
        ret = platform_device_register(&agc032_i2c_device1[device1_cnt]);
        if (ret) {
            printk(KERN_WARNING "Fail to create i2c device %d\n", device1_cnt);
            goto error_agc032_i2c_device;
        }
    }


    /* pca9548-0x71 */
    ret = platform_driver_register(&i2c_device_pca9548_2_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register i2c device driver\n");
        goto error_i2c_device_pca9548_2_driver;
    }

    for (device2_cnt = 0; device2_cnt < ARRAY_SIZE(agc032_i2c_device2); device2_cnt++)
    {
        ret = platform_device_register(&agc032_i2c_device2[device2_cnt]);
        if (ret) {
            printk(KERN_WARNING "Fail to create i2c device %d\n", device2_cnt);
            goto error_agc032_i2c_device2;
        }
    }

    /* pca9548-0x72 */
    ret = platform_driver_register(&i2c_device_pca9548_3_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register i2c device driver\n");
        goto error_i2c_device_pca9548_3_driver;
    }

    for (device3_cnt = 0; device3_cnt < ARRAY_SIZE(agc032_i2c_device3); device3_cnt++)
    {
        ret = platform_device_register(&agc032_i2c_device3[device3_cnt]);
        if (ret) {
            printk(KERN_WARNING "Fail to create i2c device %d\n", device3_cnt);
            goto error_agc032_i2c_device3;
        }
    }

    /* pca9548-0x73 */
    ret = platform_driver_register(&i2c_device_pca9548_4_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register i2c device driver\n");
        goto error_i2c_device_pca9548_4_driver;
    }

    for (device4_cnt = 0; device4_cnt < ARRAY_SIZE(agc032_i2c_device4); device4_cnt++)
    {
        ret = platform_device_register(&agc032_i2c_device4[device4_cnt]);
        if (ret) {
            printk(KERN_WARNING "Fail to create i2c device %d\n", device4_cnt);
            goto error_agc032_i2c_device4;
        }
    }

    /* pca9548-0x74 */
    ret = platform_driver_register(&i2c_device_pca9548_5_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register i2c device driver\n");
        goto error_i2c_device_pca9548_5_driver;
    }

    for (device5_cnt = 0; device5_cnt < ARRAY_SIZE(agc032_i2c_device5); device5_cnt++)
    {
        ret = platform_device_register(&agc032_i2c_device5[device5_cnt]);
        if (ret) {
            printk(KERN_WARNING "Fail to create i2c device %d\n", device5_cnt);
            goto error_agc032_i2c_device5;
        }
    }


    return 0;

/* Error handling */
error_agc032_i2c_device5:
    for (; device5_cnt >= 0; device5_cnt--) {
        platform_device_unregister(&agc032_i2c_device5[device5_cnt]);
    }
    platform_driver_unregister(&i2c_device_pca9548_5_driver);
error_i2c_device_pca9548_5_driver:
error_agc032_i2c_device4:
    for (; device4_cnt >= 0; device4_cnt--) {
        platform_device_unregister(&agc032_i2c_device4[device4_cnt]);
    }
    platform_driver_unregister(&i2c_device_pca9548_4_driver);
error_i2c_device_pca9548_4_driver:
error_agc032_i2c_device3:
    for (; device3_cnt >= 0; device3_cnt--) {
        platform_device_unregister(&agc032_i2c_device3[device3_cnt]);
    }
    platform_driver_unregister(&i2c_device_pca9548_3_driver);
error_i2c_device_pca9548_3_driver:
error_agc032_i2c_device2:
    for (; device2_cnt >= 0; device2_cnt--) {
        platform_device_unregister(&agc032_i2c_device2[device2_cnt]);
    }
    platform_driver_unregister(&i2c_device_pca9548_2_driver);
error_i2c_device_pca9548_2_driver:
error_agc032_i2c_device:
    for (; device1_cnt >= 0; device1_cnt--) {
        platform_device_unregister(&agc032_i2c_device1[device1_cnt]);
    }
    platform_driver_unregister(&i2c_device_pca9548_1_driver);
error_i2c_device_pca9548_1_driver:
    return ret;
}

static void __exit delta_agc032_platform_exit(void)
{
    int i = 0;

    // unregister pca9548-1 (0x70)
    for ( i = 0; i < ARRAY_SIZE(agc032_i2c_device1); i++ ) {
        platform_device_unregister(&agc032_i2c_device1[i]);
    }

    platform_driver_unregister(&i2c_device_pca9548_1_driver);
    i2c_unregister_device(i2c_client_9548_1);

    // unregister pca9548-2 (0x71)
    for ( i = 0; i < ARRAY_SIZE(agc032_i2c_device2); i++ ) {
        platform_device_unregister(&agc032_i2c_device2[i]);
    }

    platform_driver_unregister(&i2c_device_pca9548_2_driver);
    i2c_unregister_device(i2c_client_9548_2);

    // unregister pca9548-3 (0x72)
    for ( i = 0; i < ARRAY_SIZE(agc032_i2c_device3); i++ ) {
        platform_device_unregister(&agc032_i2c_device3[i]);
    }

    platform_driver_unregister(&i2c_device_pca9548_3_driver);
    i2c_unregister_device(i2c_client_9548_3);

    // unregister pca9548-4 (0x73)
    for ( i = 0; i < ARRAY_SIZE(agc032_i2c_device4); i++ ) {
        platform_device_unregister(&agc032_i2c_device4[i]);
    }

    platform_driver_unregister(&i2c_device_pca9548_4_driver);
    i2c_unregister_device(i2c_client_9548_4);

    // unregister pca9548-5 (0x74)
    for ( i = 0; i < ARRAY_SIZE(agc032_i2c_device5); i++ ) {
        platform_device_unregister(&agc032_i2c_device5[i]);
    }

    platform_driver_unregister(&i2c_device_pca9548_5_driver);
    i2c_unregister_device(i2c_client_9548_5);
}


module_init(delta_agc032_platform_init);
module_exit(delta_agc032_platform_exit);

MODULE_DESCRIPTION("Delta agc032 QSFP-DD eeprom Support");
MODULE_AUTHOR("Zoe Kuan <zoe.kuan@deltaww.com>");
MODULE_LICENSE("GPL");

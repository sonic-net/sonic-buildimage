/*
 * wb_vr_common.c
 * ko to read the PMBUS_IC_DEVICE_ID to determine the VR device type and load the driver.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/pmbus.h>
#include <linux/init.h>
#include <linux/delay.h>
#include "wb_pmbus.h"
#include "wb_vr_common.h"

#define PROXY_NAME "wb-vr-common"
#define PMBUS_RETRY_SLEEP_TIME   (10000)   /* 10ms */
#define PMBUS_RETRY_TIME         (3)

static int g_wb_vr_common_debug = 0;
static int g_wb_vr_common_error = 0;

module_param(g_wb_vr_common_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_vr_common_error, int, S_IRUGO | S_IWUSR);

#define WB_VR_COMMON_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_vr_common_debug) { \
        printk(KERN_INFO "[WB_VR_COMMON][VER][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_VR_COMMON_ERROR(fmt, args...) do {                                        \
    if (g_wb_vr_common_error) { \
        printk(KERN_ERR "[WB_VR_COMMON][ERR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static int of_vr_common_config_init(struct platform_device *pdev, wb_vr_common_t *wb_vr_common_dev)
{
    int ret = 0;

    ret += of_property_read_u32(pdev->dev.of_node, "i2c_bus", &wb_vr_common_dev->i2c_bus);
    ret += of_property_read_u32(pdev->dev.of_node, "i2c_addr", &wb_vr_common_dev->i2c_addr);
    if (ret != 0) {
        dev_err(&pdev->dev, "Failed to get dts config, ret:%d.\n", ret);
        return -ENXIO;
    }

    WB_VR_COMMON_VERBOSE("i2c_bus: %d, i2c_addr: 0x%x\n", wb_vr_common_dev->i2c_bus, wb_vr_common_dev->i2c_addr);

    return 0;
}

static int vr_common_config_init(struct platform_device *pdev, wb_vr_common_t *wb_vr_common_dev)
{
    wb_vr_common_t *wb_vr_common_device;

    if (pdev->dev.platform_data == NULL) {
        dev_err(&pdev->dev, "Failed to get platform data config.\n");
        return -ENXIO;
    }
    wb_vr_common_device = pdev->dev.platform_data;

    wb_vr_common_dev->i2c_bus = wb_vr_common_device->i2c_bus;
    wb_vr_common_dev->i2c_addr = wb_vr_common_device->i2c_addr;

    WB_VR_COMMON_VERBOSE("i2c_bus: %d, i2c_addr: 0x%x\n", wb_vr_common_dev->i2c_bus, wb_vr_common_dev->i2c_addr);

    return 0;
}

static int vr_common_probe(struct platform_device *pdev)
{
    wb_vr_common_t *wb_vr_common_dev;
    struct i2c_client *client = NULL;
    char device_id[DEVICE_ID_LEN];
    struct i2c_adapter *adapter;
    struct i2c_board_info board_info;
    u8 buf[I2C_SMBUS_BLOCK_MAX];
    int i, ret, len, offset;

    wb_vr_common_dev = devm_kzalloc(&pdev->dev, sizeof(wb_vr_common_t), GFP_KERNEL);
    if (!wb_vr_common_dev) {
        dev_err(&pdev->dev, "devm_kzalloc failed.\n");
        return -ENOMEM;
    }

    if (pdev->dev.of_node) {
        ret = of_vr_common_config_init(pdev, wb_vr_common_dev);
    } else {
        ret = vr_common_config_init(pdev, wb_vr_common_dev);
    }

    if (ret < 0) {
        return ret;
    }

    adapter = i2c_get_adapter(wb_vr_common_dev->i2c_bus);
    if (!adapter) {
        dev_err(&pdev->dev, "Failed to get I2C adapter for bus %d\n", wb_vr_common_dev->i2c_bus);
        wb_vr_common_dev->adap = NULL;
        return -ENODEV;
    }
    wb_vr_common_dev->adap = adapter;

	client = i2c_new_dummy_device(adapter, wb_vr_common_dev->i2c_addr);
	if (IS_ERR(client)) {
        dev_err(&pdev->dev, "%s: new dummy i2c device failed\n", __func__);
        ret = PTR_ERR(client);
        goto put_adapter;
	}

    /* Enable PEC if the controller supports it */
    for(i = 0; i < PMBUS_RETRY_TIME; i++) {
        ret = i2c_smbus_read_byte_data(client, PMBUS_CAPABILITY);
        if (ret >= 0) {
            break;
        }
        usleep_range(PMBUS_RETRY_SLEEP_TIME, PMBUS_RETRY_SLEEP_TIME + 1);
    }
    if (ret >= 0 && (ret & PB_CAPABILITY_ERROR_CHECK)){
        client->flags |= I2C_CLIENT_PEC;
    }

    for(i = 0; i < PMBUS_RETRY_TIME; i++) {
        mem_clear(buf, sizeof(buf));
        len = i2c_smbus_read_block_data(client, PMBUS_IC_DEVICE_ID, buf);
        if (len >= 0) {
            break;
        }
        usleep_range(PMBUS_RETRY_SLEEP_TIME, PMBUS_RETRY_SLEEP_TIME + 1);
    }
	if (len < 0) {
        dev_err(&pdev->dev, "read i2c block data failed, ret:%d.\n", len);
        i2c_unregister_device(client);
        ret = -ENODEV;
        goto put_adapter;
    }
    i2c_unregister_device(client);
    client = NULL;

    offset = 0;
    mem_clear(device_id, sizeof(device_id));
    for (i = 0; i < len; i++) {
        offset += scnprintf(device_id + offset, DEVICE_ID_LEN - offset, "0x%02x ", buf[i]);
    }
    dev_info(&pdev->dev, "get vr dev device id success, id: %s.\n", device_id);

    mem_clear(&board_info, sizeof(struct i2c_board_info));
    ret = -ENODEV;
    for (i = 0; i < ARRAY_SIZE(pmbus_dev_infos); i++) {
        if ((len == pmbus_dev_infos[i].dev_id_len) && (memcmp(buf, pmbus_dev_infos[i].device_id, len) == 0)) {
            dev_info(&pdev->dev, "find vr device success, device name: %s, i2c bus: %d, i2c addr: 0x%x.\n",
                pmbus_dev_infos[i].device_name, wb_vr_common_dev->i2c_bus, wb_vr_common_dev->i2c_addr);
            strlcpy(board_info.type, pmbus_dev_infos[i].device_name, I2C_NAME_SIZE);
            board_info.addr = wb_vr_common_dev->i2c_addr;
            client = i2c_new_client_device(adapter, &board_info);
            if (IS_ERR(client)) {
                dev_err(&pdev->dev, "register device i2c client fail, device name: %s, i2c bus: %d, i2c addr: 0x%x.\n",
                    pmbus_dev_infos[i].device_name, wb_vr_common_dev->i2c_bus, wb_vr_common_dev->i2c_addr);
                ret = PTR_ERR(client);
                goto put_adapter;
            }
            wb_vr_common_dev->client = client;
            platform_set_drvdata(pdev, wb_vr_common_dev);
            ret = 0;
            break;
        }
    }

    if (ret == 0) {
        dev_info(&pdev->dev, "register vr device success, device name: %s, i2c bus: %d, i2c addr: 0x%x.\n",
            pmbus_dev_infos[i].device_name, wb_vr_common_dev->i2c_bus, wb_vr_common_dev->i2c_addr);
    } else {
        dev_err(&pdev->dev, "find vr device fail, not support device id : %s.\n", device_id);
    }

put_adapter:
    i2c_put_adapter(adapter);
    return ret;
}

static int vr_common_remove(struct platform_device *pdev)
{
    wb_vr_common_t *wb_vr_common_dev;
    int i2c_bus, i2c_addr;
    struct i2c_adapter *adap;

    wb_vr_common_dev = platform_get_drvdata(pdev);
    if (wb_vr_common_dev == NULL) {
        dev_warn(&pdev->dev, "wb_vr_common_dev is NULL, exit\n");
        return 0;
    }

    i2c_bus = wb_vr_common_dev->i2c_bus;
    i2c_addr = wb_vr_common_dev->i2c_addr;
    WB_VR_COMMON_VERBOSE("Enter vr_common_remove, i2c_bus: %d, i2c addr: 0x%02x\n", i2c_bus, i2c_addr);

    adap = i2c_get_adapter(i2c_bus);
    if (adap == NULL) {
        dev_warn(&pdev->dev, "i2c adapter-%d is NULL, skip to unregister i2c client: %d-%04x\n", i2c_bus, i2c_bus, i2c_addr);
        goto exit;
    }
    if (adap != wb_vr_common_dev->adap) {
        dev_warn(&pdev->dev, "i2c adapter-%d address changed, origin addr: %p, new addr: %p, skip to unregister i2c client: %d-%04x\n",
            i2c_bus, wb_vr_common_dev->adap, adap, i2c_bus, i2c_addr);
        goto exit_i2c_put;
    }

    if (wb_vr_common_dev->client != NULL) {
        WB_VR_COMMON_VERBOSE("Starting unregister i2c client: %d-%04x\n", i2c_bus, i2c_addr);
        if (device_is_registered(&wb_vr_common_dev->client->dev)) {
            i2c_unregister_device(wb_vr_common_dev->client);
            dev_info(&pdev->dev, "Unregister i2c client: %d-%04x success\n", i2c_bus, i2c_addr);
        } else {
            dev_warn(&pdev->dev, "i2c client: %d-%04x already unregister\n", i2c_bus,i2c_addr);
        }
        wb_vr_common_dev->client = NULL;
    } else {
        dev_warn(&pdev->dev, "i2c client: %d-%04x is NULL\n", i2c_bus,i2c_addr);
    }
exit_i2c_put:
    i2c_put_adapter(adap);
exit:
    wb_vr_common_dev->adap = NULL;
    platform_set_drvdata(pdev, NULL);
    return 0;
}

static struct of_device_id vr_common_match[] = {
    {
        .compatible = "wb-vr-common",
    },
    {},
};
MODULE_DEVICE_TABLE(of, vr_common_match);

static struct platform_driver wb_vr_common_driver = {
    .probe      = vr_common_probe,
    .remove     = vr_common_remove,
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = PROXY_NAME,
        .of_match_table = vr_common_match,
    },
};

static int __init wb_vr_common_init(void)
{
    return platform_driver_register(&wb_vr_common_driver);
}

static void __exit wb_vr_common_exit(void)
{
    platform_driver_unregister(&wb_vr_common_driver);
}

module_init(wb_vr_common_init);
module_exit(wb_vr_common_exit);
MODULE_DESCRIPTION("vr common driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");

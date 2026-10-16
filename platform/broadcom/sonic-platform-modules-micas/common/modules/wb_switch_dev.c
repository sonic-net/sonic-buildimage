#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include "wb_mdio_dev.h"
#include "wb_switch_dev.h"

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

uint32_t bcm53134_phy_addr[BCM53134_MAX_DEV_NUMBER] = {0x0, 0x1, 0x2, 0x3, 0x04};

static const struct of_device_id wb_switch_match[] = {
    { .compatible = "bcm53134", .data = (void *)BCM53134},
    {},
};
MODULE_DEVICE_TABLE(of, wb_switch_match);

static const struct platform_device_id wb_switch_id_table[] = {
    { "bcm53134", BCM53134 },
    { }
};
MODULE_DEVICE_TABLE(platform, wb_switch_id_table);


static void switch_dev_device_release(struct device *dev)
{
    return;
}

static int get_dev_reg_info(int dev_type, uint32_t **phy_addr, int *num)
{
    switch(dev_type)
    {
        case BCM53134:
            *phy_addr = bcm53134_phy_addr;
            *num = BCM53134_MAX_DEV_NUMBER;
            break;
        default:
            return -EOPNOTSUPP;
    }
    return 0;
}

static int of_switch_dev_config_init(switch_dev_info_t *switch_dev)
{
    struct device *dev;
    int num_phys, i, ret = 0;
    uint32_t *phy_addr;
    int max_num_phy;
    const struct of_device_id *match;

    dev = switch_dev->dev;

    /* Match device type from device tree */
    match = of_match_node(wb_switch_match, dev->of_node);
    if (!match) {
        dev_err(dev, "dev type match fail.\n");
        return -EINVAL;
    }
    switch_dev->dev_type = (enum chips)(unsigned long)of_device_get_match_data(dev);

    ret = get_dev_reg_info(switch_dev->dev_type, &phy_addr, &max_num_phy);
    if (ret != 0) {
        DEBUG_ERROR("get_dev_reg_info fail, ret:%d\n", ret);
        return ret;
    }

    ret = of_property_read_string(dev->of_node, "mdio_bus_name", &switch_dev->mdio_bus_name);
    if (ret != 0) {
        DEBUG_ERROR("Missing required properties\n");
        return -EINVAL;
    }

    num_phys = of_property_count_strings(dev->of_node, "mdio_names");
    if (num_phys <= 0) {
        dev_err(dev, "phy num[%d] err.\n", num_phys);
        return -EINVAL;
    }

    if ((num_phys != max_num_phy) || (num_phys > MAX_MDIO_DEVICES) ) {
        dev_err(dev, "phy num[%d] config err, not equal dev_count:%d or exceed max:%d.\n", num_phys, max_num_phy, MAX_MDIO_DEVICES);
        return -EINVAL;
    }

    switch_dev->dev_num = 0;
    for (i = 0; i < num_phys; i++) {
        switch_dev->mdio_dev_device[i].mdio_bus_name = switch_dev->mdio_bus_name;
        ret = of_property_read_string_index(dev->of_node, "mdio_names", i,
                                    &switch_dev->mdio_dev_device[i].dev_name);
        if (ret != 0) {
            dev_err(dev, "mdio device[%d] config err.\n", i);
            return -EINVAL;
        }

        switch_dev->mdio_dev_device[i].addr = phy_addr[i];
        switch_dev->platform_device[i].name = MDIO_DEV_COMPATIBLE_NAME;
        switch_dev->platform_device[i].id = i;
        switch_dev->platform_device[i].dev.platform_data = &switch_dev->mdio_dev_device[i];
        switch_dev->platform_device[i].dev.release = switch_dev_device_release;
        switch_dev->dev_num++;
    }

    return 0;
}

static void mdio_device_register(switch_dev_info_t *switch_dev)
{
    int i, ret = 0;
    for (i = 0; i < switch_dev->dev_num; i++) {
        ret = platform_device_register(&switch_dev->platform_device[i]);
        if (ret < 0) {
            switch_dev->mdio_dev_device[i].device_flag = -1; /* device register failed, set flag -1 */
            dev_info(switch_dev->dev, "wb-switch-dev.%d register failed!\n", i + 1);
        } else {
            switch_dev->mdio_dev_device[i].device_flag = 0;  /* device register suucess, set flag 0 */
        }
    }
}

static void mdio_device_unregister(switch_dev_info_t *switch_dev)
{
    int i;
    for (i = switch_dev->dev_num - 1; i >= 0; i--) {
        if (switch_dev->mdio_dev_device[i].device_flag == 0) {
            platform_device_unregister(&switch_dev->platform_device[i]);
        }
    }
}


static int switch_dev_config_init(switch_dev_info_t *switch_dev)
{
    switch_dev_info_t *platform_switch_info;
    int i, ret;
    uint32_t *phy_addr;
    int max_num_phy, num_phys;
    struct platform_device *pdev = to_platform_device(switch_dev->dev);
    const struct platform_device_id *pid;

    num_phys = 0;

    platform_switch_info = switch_dev->dev->platform_data;
    if (platform_switch_info == NULL) {
        dev_err(switch_dev->dev, "Failed to get platform data config.\n");
        return -ENXIO;
    }

    /* Configure switch_dev based on platform_switch_info */
    switch_dev->mdio_bus_name = platform_switch_info->mdio_bus_name;

    /* Get device type from platform_device_id table */
    pid = platform_get_device_id(pdev);
    if (!pid) {
        dev_err(switch_dev->dev, "Failed to get platform device ID\n");
        return -ENODEV;
    }

    /* Set device type based on platform_device_id */
    switch_dev->dev_type = pid->driver_data;

    /* Get device configuration based on device type */
    ret = get_dev_reg_info(switch_dev->dev_type, &phy_addr, &max_num_phy);
    if (ret != 0) {
        dev_err(switch_dev->dev, "get_dev_reg_info fail, ret:%d\n", ret);
        return ret;
    }

    /* Calculate number of devices by checking valid dev_name in platform_switch_info */
    num_phys = 0;
    for (i = 0; i < MAX_MDIO_DEVICES; i++) {
        if (platform_switch_info->mdio_dev_device[i].dev_name) {
            num_phys++;
        } else {
            break;
        }
    }

    if (num_phys <= 0 || num_phys > MAX_MDIO_DEVICES) {
        dev_err(switch_dev->dev, "phy num[%d] err.\n", num_phys);
        return -EINVAL;
    }

    if (num_phys != max_num_phy) {
        dev_err(switch_dev->dev, "phy num[%d] config err, not equal dev_count:%d.\n",
                num_phys, max_num_phy);
        return -EINVAL;
    }

    switch_dev->dev_num = num_phys;
    for (i = 0; i < num_phys; i++) {
        switch_dev->mdio_dev_device[i].dev_name = platform_switch_info->mdio_dev_device[i].dev_name;
        switch_dev->mdio_dev_device[i].alias = platform_switch_info->mdio_dev_device[i].dev_name;
        switch_dev->mdio_dev_device[i].mdio_bus_name = switch_dev->mdio_bus_name; /* Use the switch device's bus name */
        switch_dev->mdio_dev_device[i].addr = phy_addr[i];
        switch_dev->mdio_dev_device[i].device_flag = 0;

        /* Setup platform device */
        switch_dev->platform_device[i].name = MDIO_DEV_COMPATIBLE_NAME;
        switch_dev->platform_device[i].id = i;
        switch_dev->platform_device[i].dev.platform_data = &switch_dev->mdio_dev_device[i];
        switch_dev->platform_device[i].dev.release = switch_dev_device_release;
    }

    return 0;
}

static int wb_switch_dev_probe(struct platform_device *pdev)
{
    switch_dev_info_t *switch_dev;
    int ret;

    switch_dev = devm_kzalloc(&pdev->dev, sizeof(switch_dev_info_t), GFP_KERNEL);
    if (!switch_dev) {
        dev_err(&pdev->dev, "switch_dev memory allocation failed\n");
        return -ENOMEM;
    }

    switch_dev->dev = &pdev->dev;

    /* Try to get device type from device tree if available */
    if (pdev->dev.of_node) {
        ret = of_switch_dev_config_init(switch_dev);
    } else {
        ret = switch_dev_config_init(switch_dev);
    }

    if (ret) {
        dev_err(&pdev->dev, "wb_switch_dev config_init fail, ret: %d\n", ret);
        return -EINVAL;
    }

    platform_set_drvdata(pdev, switch_dev);
    mdio_device_register(switch_dev);
    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,11,0)
static void wb_switch_dev_remove(struct platform_device *pdev)
#else
static int wb_switch_dev_remove(struct platform_device *pdev)
#endif
{
    switch_dev_info_t *switch_dev;

    switch_dev = platform_get_drvdata(pdev);
    mdio_device_unregister(switch_dev);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,11,0)
    return 0;
#endif
}

static struct platform_driver wb_switch_driver = {
    .driver = {
        .name   = KBUILD_MODNAME,
        .of_match_table = wb_switch_match,
    },
    .probe = wb_switch_dev_probe,
    .remove = wb_switch_dev_remove,
    .id_table = wb_switch_id_table,
};

module_platform_driver(wb_switch_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
MODULE_DESCRIPTION("Driver for WB Switch Device with Multiple MDIO Devices");
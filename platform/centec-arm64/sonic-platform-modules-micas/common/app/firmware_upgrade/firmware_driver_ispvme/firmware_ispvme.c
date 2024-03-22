/*
 * firmware.c
 * Original Author : support <support>2013-10-25
 *
 * firmware upgrade driver
 *
 * History
 *    v1.0    support <support> 2013-10-25  Initial version.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <firmware_ispvme.h>
#include <config_ispvme.h>

int drv_debug = 0;
module_param(drv_debug, int, S_IRUGO | S_IWUSR);

static LIST_HEAD(drv_list);
static LIST_HEAD(dev_list);

void firmware_set_debug(int value)
{
    drv_debug = value;
    return;
}

int firmware_debug(void)
{
    return drv_debug;
}

int firmware_driver_register(firmware_driver_t *fw_drv)
{
    int ret;

    if (fw_drv == NULL) {
        return FIRMWARE_FAILED;
    }

    ret = platform_driver_register(fw_drv->drv);
    if (ret < 0) {
        return FIRMWARE_FAILED;
    }

    list_add(&fw_drv->list, &drv_list);

    return FIRMWARE_SUCCESS;
}

void firmware_driver_unregister(firmware_driver_t *fw_drv)
{
    list_del_init(&fw_drv->list);
    platform_driver_unregister(fw_drv->drv);
}

int firmware_get_device_num(int type)
{
    int num;
    firmware_device_t *tmp;

    num = 0;
    list_for_each_entry(tmp, &dev_list, list) {
        if (tmp->type == type) {
            num++;
        }
    }

    return num;
}

firmware_device_t *firmware_get_device_by_minor(int type, int minor)
{
    firmware_device_t *tmp;

    list_for_each_entry(tmp, &dev_list, list) {
        if (tmp->type == type && tmp->dev.minor == minor) {
            return tmp;
        }
    }

    return NULL;
}

int firmware_device_register(firmware_device_t *fw_dev)
{
    int ret;
    firmware_device_t *tmp;

    if (fw_dev == NULL) {
        return FIRMWARE_FAILED;
    }

    list_for_each_entry(tmp, &dev_list, list) {
        if (strcmp(tmp->name, fw_dev->name) == 0) {
            return FIRMWARE_FAILED;
        }
    }

    ret = misc_register(&fw_dev->dev);
    if (ret < 0) {
        return FIRMWARE_FAILED;
    }

    list_add(&fw_dev->list, &dev_list);
    return FIRMWARE_SUCCESS;
}

void firmware_device_unregister(firmware_device_t *fw_dev)
{
    list_del(&fw_dev->list);
    misc_deregister(&fw_dev->dev);
}

static int __init firmware_driver_init(void)
{
    INIT_LIST_HEAD(&drv_list);
    INIT_LIST_HEAD(&dev_list);
    dev_debug(firmware_debug(), "firmware_driver_init cpld init.\n");
    firmware_cpld_init();

    return FIRMWARE_SUCCESS;
}

static void __exit firmware_driver_exit(void)
{
    dev_debug(firmware_debug(), "firmware_driver_exit cpld exit.\n");
    firmware_cpld_exit();
    INIT_LIST_HEAD(&drv_list);
    INIT_LIST_HEAD(&dev_list);
    return;
}

module_init(firmware_driver_init);
module_exit(firmware_driver_exit);

MODULE_AUTHOR("support <support>");
MODULE_DESCRIPTION("Platform Support");
MODULE_LICENSE("GPL");
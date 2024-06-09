#include <linux/kernel.h>
#include <linux/module.h>
#include <firmware_sysfs.h>

int g_firmware_driver_debug = 0;
module_param(g_firmware_driver_debug, int, S_IRUGO | S_IWUSR);

static LIST_HEAD(drv_list);
static LIST_HEAD(dev_list);

/**
 * firmware_driver_register
 * function:Registered Device Driver
 * @fw_drv:param[in] Driver information
 * return value : success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
int firmware_driver_register(firmware_driver_t *fw_drv)
{
    int ret;

    if (fw_drv == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Parameter error.\n");
        return FIRMWARE_FAILED;
    }

    ret = platform_driver_register(fw_drv->drv);
    if (ret < 0) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: failed to register firmware upgrade driver \n");
        return FIRMWARE_FAILED;
    }

    /* Adds driver information to the driver list */
    list_add(&fw_drv->list, &drv_list);
    FIRMWARE_DRIVER_DEBUG_VERBOSE("firmware upgrade driver register sucess \n");

    return FIRMWARE_SUCCESS;
}

/**
 * firmware_driver_unregister
 * function:unregister Device Driver
 * @fw_drv:param[in] Driver information
 */
void firmware_driver_unregister(firmware_driver_t *fw_drv)
{
    list_del_init(&fw_drv->list);
    platform_driver_unregister(fw_drv->drv);
}

/*
 * firmware_get_device_by_minor
 * function: Get device information based on minor
 */
firmware_device_t *firmware_get_device_by_minor(int minor)
{
    firmware_device_t *tmp;

    list_for_each_entry(tmp, &dev_list, list) {
        if (tmp->dev.minor == minor) {
            return tmp;
        }
    }

    return NULL;
}

/**
 * firmware_device_register
 * function:Registered Driver Device
 * @fw_dev: param[in] Driver information
 * return value:success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
int firmware_device_register(firmware_device_t *fw_dev)
{
    int ret;
    firmware_device_t *tmp;

    if (fw_dev == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Parameter error.\n");
        return FIRMWARE_FAILED;
    }
    /* Check whether the device file name already exists in the device linked list */
    list_for_each_entry(tmp, &dev_list, list) {
        if (strcmp(tmp->name, fw_dev->name) == 0) {
            FIRMWARE_DRIVER_DEBUG_ERROR("devie %s already exists.\n", fw_dev->name);
            return FIRMWARE_FAILED;
        }
    }

    ret = misc_register(&fw_dev->dev);
    if (ret < 0) {
        FIRMWARE_DRIVER_DEBUG_ERROR("register misc error, ret=%d.\n", ret);
        return FIRMWARE_FAILED;
    }

    /* Adds driver information to the driver list */
    list_add(&fw_dev->list, &dev_list);

    return FIRMWARE_SUCCESS;
}

/**
 * firmware_device_unregister
 * function: unregister Driver Device
 */
void firmware_device_unregister(firmware_device_t *fw_dev)
{
    list_del(&fw_dev->list);
    misc_deregister(&fw_dev->dev);
}

static int __init firmware_driver_init(void)
{
    int ret;

    INIT_LIST_HEAD(&drv_list);
    INIT_LIST_HEAD(&dev_list);
    FIRMWARE_DRIVER_DEBUG_VERBOSE("firmware driver sysfs init.\n");
    ret = firmware_sysfs_init();
    if (ret < 0) {
        FIRMWARE_DRIVER_DEBUG_ERROR("firmware driver sysfs init failed.\n");
        return FIRMWARE_FAILED;
    }

    return FIRMWARE_SUCCESS;
}

static void __exit firmware_driver_exit(void)
{
    FIRMWARE_DRIVER_DEBUG_VERBOSE("firmware driver sysfs exit.\n");
    firmware_sysfs_exit();
    INIT_LIST_HEAD(&drv_list);
    INIT_LIST_HEAD(&dev_list);
    return;
}

module_init(firmware_driver_init);
module_exit(firmware_driver_exit);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("Firmware upgrade driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <firmware.h>
#include <firmware_cpld.h>
#include <firmware_upgrade.h>

static int firmware_cpld_open(struct inode *inode, struct file *file)
{
    firmware_device_t *frm_dev;

    FIRMWARE_DRIVER_DEBUG_VERBOSE("Open cpld device.\n");
    frm_dev = firmware_get_device_by_minor(MINOR(inode->i_rdev));
    if (frm_dev == NULL) {
        return -ENXIO;
    }
    file->private_data = frm_dev;

    return FIRMWARE_SUCCESS;
}

static ssize_t firmware_cpld_read (struct file *file, char __user *buf, size_t count,
                                   loff_t *offset)
{
    return 0;
}

static ssize_t firmware_cpld_write (struct file *file, const char __user *buf, size_t count,
                                    loff_t *offset)
{
    return 0;
}

static loff_t firmware_cpld_llseek(struct file *file, loff_t offset, int origin)
{
    return 0;
}

/*
 * firmware_cpld_ioctl
 * function: ioctl command parsing function
 * @file: param[in] device file name
 * @cmd:  param[in] command
 * @arg:  param[in] the parameters in the command
 * return value: success-FIRMWARE_SUCCESS; fail:other value
 */
static long firmware_cpld_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret;
    char *buf;
    void __user *argp;
    char version[FIRMWARE_NAME_LEN];
    char chip_name[FIRMWARE_NAME_LEN];
    cmd_info_t cmd_info;
    firmware_device_t *frm_dev;
    firmware_cpld_t *cpld_info;

    /* Get device private data */
    mem_clear(&cmd_info, sizeof(cmd_info_t));
    frm_dev = (firmware_device_t *)file->private_data;
    cpld_info = NULL;
    if (frm_dev != NULL) {
        if (frm_dev->priv != NULL) {
            cpld_info = (firmware_cpld_t *)frm_dev->priv;
        }
    }
    if (cpld_info == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: Failed to frm_dev->priv sysfs info.\n");
        return FIRMWARE_FAILED;
    }
    argp = (void __user *)arg;

    switch (cmd) {
    case FIRMWARE_GET_CHIPNAME:
        /* get chip name */
        if (copy_from_user(&cmd_info, argp, sizeof(cmd_info_t))) {
            return -EFAULT;
        }
        mem_clear(chip_name, FIRMWARE_NAME_LEN);
        ret = fmw_cpld_upg_get_chip_name(frm_dev->chain, cpld_info, chip_name, FIRMWARE_NAME_LEN);
        if (ret != FIRMWARE_SUCCESS) {
            FIRMWARE_DRIVER_DEBUG_ERROR("Failed to get chip name.\n");
            return -ENXIO;
        }
        if (copy_to_user(cmd_info.data, chip_name, cmd_info.size)) {
            return -EFAULT;
        }
        break;
    case FIRMWARE_PROGRAM:
    case FIRMWARE_PROGRAM_JBI:
        /* firmware upgrade */
        if (copy_from_user(&cmd_info, argp, sizeof(cmd_info_t))) {
            return -EFAULT;
        }
        buf = (char *) kzalloc(cmd_info.size + 1, GFP_KERNEL);
        if (buf == NULL) {
            return -ENOMEM;
        }
        if (copy_from_user(buf, cmd_info.data, cmd_info.size)) {
            kfree(buf);
            return -EFAULT;
        }
        buf[cmd_info.size] = 0;
        if (cmd == FIRMWARE_PROGRAM_JBI) {
            /* JBI firmware upgrade */
            ret = fmw_cpld_upg_program_jbi(frm_dev->chain, cpld_info, buf, cmd_info.size);
        } else {
            /* ISC firmware upgrade */
            ret = fmw_cpld_upg_program(frm_dev->chain, cpld_info, buf, cmd_info.size);
        }
        if (ret != FIRMWARE_SUCCESS) {
            FIRMWARE_DRIVER_DEBUG_ERROR("Failed to program cpld.\n");
            kfree(buf);
            return -ESRCH;
        }
        kfree(buf);
        break;
    case FIRMWARE_GET_VERSION:
        /* get version */
        if (copy_from_user(&cmd_info, argp, sizeof(cmd_info_t))) {
            return -EFAULT;
        }
        mem_clear(version, FIRMWARE_NAME_LEN);
        ret = fmw_cpld_upg_get_version(frm_dev->chain, cpld_info, version, FIRMWARE_NAME_LEN);
        if (ret != FIRMWARE_SUCCESS) {
            FIRMWARE_DRIVER_DEBUG_ERROR("Failed to get version.\n");
            return -ENXIO;
        }
        if (copy_to_user(cmd_info.data, version, cmd_info.size)) {
            return -EFAULT;
        }
        break;
    default:
        FIRMWARE_DRIVER_DEBUG_ERROR("not find cmd: %d\r\n", cmd);
        return -ENOTTY;
    } /* End of switch */

    return FIRMWARE_SUCCESS;
}

static int firmware_cpld_release(struct inode *inode, struct file *file)
{
    return 0;
}

static const struct file_operations cpld_dev_fops = {
    .owner      = THIS_MODULE,
    .llseek     = firmware_cpld_llseek,
    .read       = firmware_cpld_read,
    .write      = firmware_cpld_write,
    .unlocked_ioctl = firmware_cpld_ioctl,
    .open       = firmware_cpld_open,
    .release    = firmware_cpld_release,
};

static int of_firmware_upgrade_config_init(struct device *dev, firmware_cpld_t *cpld_info)
{
    int ret;
    char *name;
    int i;
    char buf[64];

    FIRMWARE_DRIVER_DEBUG_VERBOSE("Enter firmware_upgrade_config_init\r\n");
    if (cpld_info == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("info is null\r\n");
        return -1;
    }

    mem_clear(cpld_info, sizeof(firmware_cpld_t));
    ret = 0;
    ret += of_property_read_string(dev->of_node, "type", (const char **)&name);
    ret += of_property_read_u32(dev->of_node, "tdi", &cpld_info->tdi);
    ret += of_property_read_u32(dev->of_node, "tck", &cpld_info->tck);
    ret += of_property_read_u32(dev->of_node, "tms", &cpld_info->tms);
    ret += of_property_read_u32(dev->of_node, "tdo", &cpld_info->tdo);

    ret += of_property_read_u32(dev->of_node, "chain", &cpld_info->chain);
    ret += of_property_read_u32(dev->of_node, "chip_index", &cpld_info->chip_index);

    if (ret != 0) {
       FIRMWARE_DRIVER_DEBUG_ERROR("dts config error, ret:%d.\n", ret);
       return -ENXIO;
    }

    strncpy(cpld_info->type, name, sizeof(cpld_info->type) - 1);

    ret = of_property_read_u32(dev->of_node, "tck_delay", &cpld_info->tck_delay);
    if(ret != 0) {
        cpld_info->tck_delay = 60;
    }

    cpld_info->gpio_en_info_num = 0;
    /* Enable through GPIO */
    for (i = 0; i < FIRMWARE_EN_INFO_MAX; i++) {
        mem_clear(buf, sizeof(buf));
        snprintf(buf, sizeof(buf) - 1, "en_gpio_%d", i);
        ret = of_property_read_u32(dev->of_node, buf, &cpld_info->gpio_en_info[i].en_gpio);
        if(ret != 0) {
            break;
        }

        mem_clear(buf, sizeof(buf));
        snprintf(buf, sizeof(buf) - 1, "en_level_%d", i);
        ret = of_property_read_u32(dev->of_node, buf, &cpld_info->gpio_en_info[i].en_level);
        if(ret != 0) {
            break;
        }
        cpld_info->gpio_en_info_num++;
    }

    FIRMWARE_DRIVER_DEBUG_VERBOSE("type:%s, chain:%u, chip_index:%u, en_info_num:%u\n",
        cpld_info->type, cpld_info->chain, cpld_info->chip_index, cpld_info->gpio_en_info_num);
    FIRMWARE_DRIVER_DEBUG_VERBOSE("tdi:%u, tck:%u, tms:%u, tdo:%u tck_delay:%u.\n",
        cpld_info->tdi, cpld_info->tck, cpld_info->tms, cpld_info->tdo, cpld_info->tck_delay);

    return 0;
}

static int firmware_upgrade_config_init(struct device *dev, firmware_cpld_t *cpld_info)
{
    int i;

    firmware_upgrade_device_t *firmware_upgrade_device;
    firmware_jtag_device_t jtag_upg_device;

    FIRMWARE_DRIVER_DEBUG_VERBOSE("Enter firmware_upgrade_config_init\r\n");
    if (cpld_info == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("info is null\r\n");
        return -1;
    }

    if (dev->platform_data == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("platform data config error.\n");
        return -1;
    }
    firmware_upgrade_device = dev->platform_data;
    jtag_upg_device = firmware_upgrade_device->upg_type.jtag;

    mem_clear(cpld_info, sizeof(firmware_cpld_t));

    strncpy(cpld_info->type, firmware_upgrade_device->type, sizeof(cpld_info->type) - 1);
    cpld_info->tdi = jtag_upg_device.tdi;
    cpld_info->tck = jtag_upg_device.tck;
    cpld_info->tms = jtag_upg_device.tms;
    cpld_info->tdo = jtag_upg_device.tdo;
    cpld_info->chain = firmware_upgrade_device->chain;
    cpld_info->chip_index = firmware_upgrade_device->chip_index;

    if (jtag_upg_device.tck_delay == 0) {
        cpld_info->tck_delay = 60;
        FIRMWARE_DRIVER_DEBUG_VERBOSE("no config tck_delay, use default value:%u\n", cpld_info->tck_delay);
    } else {
        cpld_info->tck_delay = jtag_upg_device.tck_delay;
    }

    if (firmware_upgrade_device->en_gpio_num > FIRMWARE_EN_INFO_MAX) {
        FIRMWARE_DRIVER_DEBUG_ERROR("The number of en_gpio_num:%u configurations exceeds the maximum limit:%u.\n",
            firmware_upgrade_device->en_gpio_num, FIRMWARE_EN_INFO_MAX);
        return -ENXIO;
    }
    cpld_info->gpio_en_info_num = firmware_upgrade_device->en_gpio_num;
    /* Enable through GPIO */
    for (i = 0; i < cpld_info->gpio_en_info_num; i++) {
        cpld_info->gpio_en_info[i].en_gpio = firmware_upgrade_device->en_gpio[i];
        cpld_info->gpio_en_info[i].en_level = firmware_upgrade_device->en_level[i];
    }

    FIRMWARE_DRIVER_DEBUG_VERBOSE("type:%s, chain:%u, chip_index:%u, en_info_num:%u\n",
        cpld_info->type, cpld_info->chain, cpld_info->chip_index, cpld_info->gpio_en_info_num);
    FIRMWARE_DRIVER_DEBUG_VERBOSE("tdi:%u, tck:%u, tms:%u, tdo:%u tck_delay:%u.\n",
        cpld_info->tdi, cpld_info->tck, cpld_info->tms, cpld_info->tdo, cpld_info->tck_delay);

    return 0;
}

static int  firmware_cpld_probe(struct platform_device *pdev)
{
    int ret;
    firmware_cpld_t *cpld_info;
    firmware_device_t *frm_dev;

    FIRMWARE_DRIVER_DEBUG_VERBOSE("Enter firmware_cpld_probe\r\n");
    /* Gets the information in the device tree */
    cpld_info = devm_kzalloc(&pdev->dev, sizeof(firmware_cpld_t), GFP_KERNEL);
    if (cpld_info == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Failed to kzalloc cpld device tree.\n");
        return -EPERM;
    }

    if (pdev->dev.of_node) {
        ret = of_firmware_upgrade_config_init(&pdev->dev, cpld_info);
    } else {
        ret = firmware_upgrade_config_init(&pdev->dev, cpld_info);
    }
    if (ret < 0) {
        FIRMWARE_DRIVER_DEBUG_ERROR("get config init from dts error.\n");
        return -EPERM;
    }

    frm_dev = devm_kzalloc(&pdev->dev, sizeof(firmware_device_t), GFP_KERNEL);
    if (frm_dev == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Failed to kzalloc firmware device.\n");
        return -EPERM;
    }

    /* Based on the link number, determine the name of the device file */
    frm_dev->chain = cpld_info->chain;
    snprintf(frm_dev->name, FIRMWARE_NAME_LEN - 1, "firmware_cpld%d", frm_dev->chain);
    strncpy(cpld_info->devname, frm_dev->name, strlen(frm_dev->name) + 1);

    INIT_LIST_HEAD(&frm_dev->list);
    frm_dev->dev.minor = MISC_DYNAMIC_MINOR;
    frm_dev->dev.name = frm_dev->name;
    frm_dev->dev.fops = &cpld_dev_fops;
    frm_dev->priv = cpld_info;

    FIRMWARE_DRIVER_DEBUG_VERBOSE("Register cpld firmware chain:%d, name:%s.\n", frm_dev->chain, frm_dev->name);

    ret = firmware_device_register(frm_dev);
    if (ret < 0) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Failed to register firmware device.\n");
        return -EPERM;
    }

    platform_set_drvdata(pdev, frm_dev);
    return 0;
}

static int __exit firmware_cpld_remove(struct platform_device *pdev)
{
    firmware_device_t *frm_dev;

    frm_dev = (firmware_device_t *)platform_get_drvdata(pdev);
    firmware_device_unregister(frm_dev);
    platform_set_drvdata(pdev, NULL);

    return 0;
}

static struct of_device_id cpld_match[] = {
    {
        .compatible = "firmware_cpld",
    },
    {},
};

static struct platform_driver cpld_driver = {
    .driver = {
        .name = "firmware_cpld",
        .owner = THIS_MODULE,
        .of_match_table = cpld_match,
    },
    .probe = firmware_cpld_probe,
    .remove = firmware_cpld_remove,
};

static firmware_driver_t fmw_drv_cpld = {
    .name = "firmware_cpld",
    .drv = &cpld_driver,
};

int firmware_cpld_init(void)
{
    int ret;

    INIT_LIST_HEAD(&fmw_drv_cpld.list);
    FIRMWARE_DRIVER_DEBUG_VERBOSE("cpld upgrade driver register \n");
    ret = firmware_driver_register(&fmw_drv_cpld);
    if (ret < 0) {
        FIRMWARE_DRIVER_DEBUG_ERROR("cpld upgrade driver register failed.\n");
        return ret;
    }
    return 0;
}

void firmware_cpld_exit(void)
{
    firmware_driver_unregister(&fmw_drv_cpld);
    INIT_LIST_HEAD(&fmw_drv_cpld.list);
}

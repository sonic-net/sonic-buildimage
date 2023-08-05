#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <firmware_sysfs.h>
#include <firmware_sysfs_upgrade.h>
#include <firmware_upgrade.h>

static int firmware_sysfs_open(struct inode *inode, struct file *file)
{
    firmware_device_t *frm_dev;

    FIRMWARE_DRIVER_DEBUG_VERBOSE("Open device.\n");
    frm_dev = firmware_get_device_by_minor(MINOR(inode->i_rdev));
    if (frm_dev == NULL) {
        return -ENXIO;
    }
    file->private_data = frm_dev;

    return FIRMWARE_SUCCESS;
}

static ssize_t firmware_sysfs_read (struct file *file, char __user *buf, size_t count,
                                   loff_t *offset)
{
    return 0;
}

static ssize_t firmware_sysfs_write (struct file *file, const char __user *buf, size_t count,
                                    loff_t *offset)
{
    return 0;
}

static loff_t firmware_sysfs_llseek(struct file *file, loff_t offset, int origin)
{
    return 0;
}

/* firmware_sysfs_ioctl
* function:ioctl command parsing function
* @file: param[in] device file name
* @cmd:  param[in] command
* @arg:  param[in] the parameters in the command
* return value: success-FIRMWARE_SUCCESS; fail:other value
*/
static long firmware_sysfs_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    void __user *argp;
    firmware_device_t *frm_dev;
    firmware_sysfs_t *sysfs_info;
    int ret;

    /* Get device private data */
    frm_dev = (firmware_device_t *)file->private_data;
    sysfs_info = NULL;
    if (frm_dev != NULL) {
        if (frm_dev->priv != NULL) {
            sysfs_info = (firmware_sysfs_t *)frm_dev->priv;
        }
    }
    if (sysfs_info == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: Failed to frm_dev->priv sysfs info.\n");
        return FIRMWARE_FAILED;
    }
    argp = (void __user *)arg;

    switch (cmd) {
    case FIRMWARE_SYSFS_INIT:
        /* enable upgrade access */
        ret = firmware_init_dev_loc(sysfs_info);
        if (ret != FIRMWARE_SUCCESS) {
           FIRMWARE_DRIVER_DEBUG_ERROR("Error: Failed to init upgrade.(chain = %d)\n",
                frm_dev != NULL ? frm_dev->chain : -1);
           return FIRMWARE_FAILED;
        }
        break;
    case FIRMWARE_SYSFS_FINISH:
        /* disable upgrade access */
        ret = firmware_finish_dev_loc(sysfs_info);
        if (ret != FIRMWARE_SUCCESS) {
            FIRMWARE_DRIVER_DEBUG_ERROR("Error: Failed to release upgrade.(chain = %d)\n",
                frm_dev != NULL ? frm_dev->chain : -1);
            return FIRMWARE_FAILED;
        }
        break;
    case FIRMWARE_SYSFS_SPI_INFO:
        /* Get SPI logic device information */
        if (copy_to_user(argp, &sysfs_info->info.spi_logic_info, sizeof(firmware_spi_logic_info_t))) {
            return -EFAULT;
        }
        break;
    case FIRMWARE_SYSFS_DEV_FILE_INFO:
        /*Get logic device information */
        if (copy_to_user(argp, &sysfs_info->info.dev_file_info, sizeof(firmware_dev_file_info_t))) {
            return -EFAULT;
        }
        break;
    case FIRMWARE_SYSFS_MTD_INFO:
        /*Get logic device information */
        if (copy_to_user(argp, &sysfs_info->info.mtd_info, sizeof(firmware_mtd_info_t))) {
            return -EFAULT;
        }
        break;
    default:
        FIRMWARE_DRIVER_DEBUG_ERROR("not find cmd: %d\r\n", cmd);
        return -ENOTTY;
    } /* End of switch */

    return FIRMWARE_SUCCESS;
}

static int firmware_sysfs_release(struct inode *inode, struct file *file)
{
    return 0;
}

static const struct file_operations sysfs_dev_fops = {
    .owner      = THIS_MODULE,
    .llseek     = firmware_sysfs_llseek,
    .read       = firmware_sysfs_read,
    .write      = firmware_sysfs_write,
    .unlocked_ioctl = firmware_sysfs_ioctl,
    .open       = firmware_sysfs_open,
    .release    = firmware_sysfs_release,
};

/* Gets the information in the device tree */
static int of_firmware_upgrade_config_init(struct device *dev, firmware_sysfs_t *sysfs_info)
{
    int ret;
    char *name;
    int8_t buf[64];
    int i;
    firmware_logic_dev_en_t *firmware_logic_dev_en_point;
    uint32_t test_base, test_size;

    FIRMWARE_DRIVER_DEBUG_VERBOSE("Enter firmware_dev_loc_config_init\r\n");
    if (sysfs_info == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("info is null\r\n");
        return -1;
    }

    mem_clear(sysfs_info, sizeof(firmware_sysfs_t));
    ret = 0;
    ret += of_property_read_string(dev->of_node, "type", (const char **)&name);

    ret += of_property_read_u32(dev->of_node, "chain", &sysfs_info->chain);
    ret += of_property_read_u32(dev->of_node, "chip_index", &sysfs_info->chip_index);
    if (ret != 0) {
       FIRMWARE_DRIVER_DEBUG_ERROR("dts config error, ret:%d.\n", ret);
       return -ENXIO;
    }
    strncpy(sysfs_info->type, name, sizeof(sysfs_info->type) - 1);

    ret = of_property_read_u32(dev->of_node, "test_base", &test_base);
    if (ret != 0) {
        FIRMWARE_DRIVER_DEBUG_ERROR("dts config test_base, ret:%d.\n", ret);
        test_base = 0;
    }

    ret = of_property_read_u32(dev->of_node, "test_size", &test_size);
    if (ret != 0) {
        FIRMWARE_DRIVER_DEBUG_ERROR("dts config test_size, ret:%d.\n", ret);
        test_size = 0;
    }

    if (strcmp(sysfs_info->type, FIRMWARE_SYSFS_TYPE_SPI_LOGIC) == 0) {
        ret = of_property_read_string(dev->of_node, "dev_name", (const char **)&name);
        if (ret != 0) {
            FIRMWARE_DRIVER_DEBUG_ERROR("dts config dev_name error, ret:%d.\n", ret);
            return -ENXIO;
        }
        strncpy(sysfs_info->info.spi_logic_info.dev_name, name, FIRMWARE_DEV_NAME_LEN - 1);

        ret = of_property_read_u32(dev->of_node, "flash_base", &sysfs_info->info.spi_logic_info.flash_base);
        if (ret != 0) {
            FIRMWARE_DRIVER_DEBUG_ERROR("dts config flash_base error, ret:%d.\n", ret);
            return -ENXIO;
        }

        ret = of_property_read_u32(dev->of_node, "ctrl_base", &sysfs_info->info.spi_logic_info.ctrl_base);
        if (ret != 0) {
            FIRMWARE_DRIVER_DEBUG_ERROR("dts config ctrl_base error, ret:%d.\n", ret);
            return -ENXIO;
        }
        sysfs_info->info.spi_logic_info.test_base = test_base;
        sysfs_info->info.spi_logic_info.test_size = test_size;
    } else if (strcmp(sysfs_info->type, FIRMWARE_SYSFS_TYPE_SYSFS) == 0) {
        ret = of_property_read_string(dev->of_node, "sysfs_name", (const char **)&name);
        if (ret != 0) {
            FIRMWARE_DRIVER_DEBUG_ERROR("dts config sysfs_name error, ret:%d.\n", ret);
            return -ENXIO;
        }
        strncpy(sysfs_info->info.dev_file_info.sysfs_name, name, FIRMWARE_DEV_NAME_LEN - 1);

        ret = of_property_read_u32(dev->of_node, "dev_base", &sysfs_info->info.dev_file_info.dev_base);
        if (ret != 0) {
            FIRMWARE_DRIVER_DEBUG_VERBOSE("dts don't config dev_base, dev_base is 0.\n");
            sysfs_info->info.dev_file_info.dev_base = 0;
        }

        ret = of_property_read_u32(dev->of_node, "per_len", &sysfs_info->info.dev_file_info.per_len);
        if (ret != 0) {
            FIRMWARE_DRIVER_DEBUG_VERBOSE("dts don't config per_len, per_len is 0.\n");
            sysfs_info->info.dev_file_info.per_len = 0;
        }
        sysfs_info->info.dev_file_info.test_base = test_base;
        sysfs_info->info.dev_file_info.test_size = test_size;
    } else if (strcmp(sysfs_info->type, FIRMWARE_SYSFS_TYPE_MTD) == 0) {
        ret = of_property_read_string(dev->of_node, "mtd_name", (const char **)&name);
        if (ret != 0) {
            FIRMWARE_DRIVER_DEBUG_ERROR("dts config mtd_name error, ret:%d.\n", ret);
            return -ENXIO;
        }
        strncpy(sysfs_info->info.mtd_info.mtd_name, name, FIRMWARE_DEV_NAME_LEN - 1);

        ret = of_property_read_u32(dev->of_node, "flash_base", &sysfs_info->info.mtd_info.flash_base);
        if (ret != 0) {
            FIRMWARE_DRIVER_DEBUG_ERROR("dts config flash_base error, ret:%d.\n", ret);
            return -ENXIO;
        }
        sysfs_info->info.mtd_info.test_base = test_base;
        sysfs_info->info.mtd_info.test_size = test_size;
    } else {
        FIRMWARE_DRIVER_DEBUG_ERROR("dts config sysfs type[%s] is not support, ret:%d.\n", sysfs_info->type, ret);
        return -ENXIO;
    }

    sysfs_info->gpio_en_info_num = 0;
    /* Enable through GPIO */
    for (i = 0; i < FIRMWARE_EN_INFO_MAX; i++) {
        mem_clear(buf, sizeof(buf));
        snprintf(buf, sizeof(buf) - 1, "en_gpio_%d", i);
        ret = of_property_read_u32(dev->of_node, buf, &sysfs_info->gpio_en_info[i].en_gpio);
        if(ret != 0) {
            break;
        }

        mem_clear(buf, sizeof(buf));
        snprintf(buf, sizeof(buf) - 1, "en_level_%d", i);
        ret = of_property_read_u32(dev->of_node, buf, &sysfs_info->gpio_en_info[i].en_level);
        if(ret != 0) {
            break;
        }
        sysfs_info->gpio_en_info_num++;
    }

    sysfs_info->logic_dev_en_num = 0;
    /* Enable through register */
    for (i = 0; i < FIRMWARE_EN_INFO_MAX; i++) {
        firmware_logic_dev_en_point = &sysfs_info->logic_dev_en_info[i];
        mem_clear(buf, sizeof(buf));
        snprintf(buf, sizeof(buf) - 1, "en_logic_dev_%d", i);
        ret = 0;
        ret += of_property_read_string(dev->of_node, buf, (const char **)&name);
        if(ret != 0) {
            /* Failure to resolve to EN_LOGIC_DEV means no logical device is enabled. No failure is returned */
            ret = 0;
            break;
        }
        strncpy(firmware_logic_dev_en_point->dev_name, name, FIRMWARE_DEV_NAME_LEN - 1);

        mem_clear(buf, sizeof(buf));
        snprintf(buf, sizeof(buf) - 1, "en_logic_addr_%d", i);
        ret = of_property_read_u32(dev->of_node, buf, &firmware_logic_dev_en_point->addr);
        if (ret != 0) {
            FIRMWARE_DRIVER_DEBUG_ERROR("Failed to config en en_logic_addr_%d ret =%d.\n", i, ret);
            break;
        }

        mem_clear(buf, sizeof(buf));
        snprintf(buf, sizeof(buf) - 1, "en_logic_mask_%d", i);
        ret = of_property_read_u32(dev->of_node, buf, &firmware_logic_dev_en_point->mask);
        if (ret != 0) {
            FIRMWARE_DRIVER_DEBUG_ERROR("Failed to config en en_logic_mask_%d ret =%d.\n", i, ret);
            break;
        }

        mem_clear(buf, sizeof(buf));
        snprintf(buf, sizeof(buf) - 1, "en_logic_en_val_%d", i);
        ret = of_property_read_u32(dev->of_node, buf, &firmware_logic_dev_en_point->en_val);
        if (ret != 0) {
            FIRMWARE_DRIVER_DEBUG_ERROR("Failed to config en en_logic_en_val_%d ret =%d.\n", i, ret);
            break;
        }

        mem_clear(buf, sizeof(buf));
        snprintf(buf, sizeof(buf) - 1, "en_logic_dis_val_%d", i);
        ret = of_property_read_u32(dev->of_node, buf, &firmware_logic_dev_en_point->dis_val);
        if (ret != 0) {
            FIRMWARE_DRIVER_DEBUG_ERROR("Failed to config en en_logic_dis_val_%d ret =%d.\n", i, ret);
            break;
        }

        mem_clear(buf, sizeof(buf));
        snprintf(buf, sizeof(buf) - 1, "en_logic_width_%d", i);
        ret = of_property_read_u32(dev->of_node, buf, &firmware_logic_dev_en_point->width);
        if (ret != 0) {
            FIRMWARE_DRIVER_DEBUG_ERROR("Failed to config en en_logic_width_%d ret =%d.\n", i, ret);
            break;
        }

        sysfs_info->logic_dev_en_num++;
    }

    return ret;
}

static int firmware_upgrade_config_init(struct device *dev, firmware_sysfs_t *sysfs_info)
{
    int i;
    firmware_logic_dev_en_t *firmware_logic_dev_en_point;
    firmware_upgrade_device_t *firmware_upgrade_device;
    firmware_sysfs_device_t sysfs_upg_device;

    FIRMWARE_DRIVER_DEBUG_VERBOSE("Enter firmware_dev_loc_config_init\r\n");
    if (sysfs_info == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("info is null\r\n");
        return -1;
    }

    if (dev->platform_data == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("platform data config error.\n");
        return -1;
    }
    firmware_upgrade_device = dev->platform_data;
    sysfs_upg_device = firmware_upgrade_device->upg_type.sysfs;

    mem_clear(sysfs_info, sizeof(firmware_sysfs_t));
    strncpy(sysfs_info->type, firmware_upgrade_device->type, sizeof(sysfs_info->type) - 1);
    sysfs_info->chain = firmware_upgrade_device->chain;
    sysfs_info->chip_index = firmware_upgrade_device->chip_index;

    if (strcmp(sysfs_info->type, FIRMWARE_SYSFS_TYPE_SPI_LOGIC) == 0) {
        strncpy(sysfs_info->info.spi_logic_info.dev_name, sysfs_upg_device.dev_name, FIRMWARE_DEV_NAME_LEN - 1);
        sysfs_info->info.spi_logic_info.flash_base = sysfs_upg_device.flash_base;
        sysfs_info->info.spi_logic_info.ctrl_base = sysfs_upg_device.ctrl_base;
        sysfs_info->info.spi_logic_info.test_base = sysfs_upg_device.test_base;
        sysfs_info->info.spi_logic_info.test_size = sysfs_upg_device.test_size;
    } else if (strcmp(sysfs_info->type, FIRMWARE_SYSFS_TYPE_SYSFS) == 0) {
        strncpy(sysfs_info->info.dev_file_info.sysfs_name, sysfs_upg_device.sysfs_name, FIRMWARE_DEV_NAME_LEN - 1);
        sysfs_info->info.dev_file_info.dev_base = sysfs_upg_device.dev_base;
        sysfs_info->info.dev_file_info.per_len = sysfs_upg_device.per_len;
        sysfs_info->info.dev_file_info.test_base = sysfs_upg_device.test_base;
        sysfs_info->info.dev_file_info.test_size = sysfs_upg_device.test_size;
    } else if (strcmp(sysfs_info->type, FIRMWARE_SYSFS_TYPE_MTD) == 0) {
        strncpy(sysfs_info->info.mtd_info.mtd_name, sysfs_upg_device.mtd_name, FIRMWARE_DEV_NAME_LEN - 1);
        sysfs_info->info.mtd_info.flash_base = sysfs_upg_device.flash_base;
        sysfs_info->info.mtd_info.test_base = sysfs_upg_device.test_base;
        sysfs_info->info.mtd_info.test_size = sysfs_upg_device.test_size;
    } else {
        FIRMWARE_DRIVER_DEBUG_ERROR("config sysfs type[%s] is not support.\n", sysfs_info->type);
        return -ENXIO;
    }

    if (firmware_upgrade_device->en_gpio_num > FIRMWARE_EN_INFO_MAX) {
        FIRMWARE_DRIVER_DEBUG_ERROR("The number of en_gpio_num:%u configurations exceeds the maximum limit:%u.\n",
            firmware_upgrade_device->en_gpio_num, FIRMWARE_EN_INFO_MAX);
        return -ENXIO;
    }
    sysfs_info->gpio_en_info_num = firmware_upgrade_device->en_gpio_num;
    /* Enable through GPIO */
    for (i = 0; i < sysfs_info->gpio_en_info_num; i++) {
        sysfs_info->gpio_en_info[i].en_gpio = firmware_upgrade_device->en_gpio[i];
        sysfs_info->gpio_en_info[i].en_level = firmware_upgrade_device->en_level[i];
    }

    if (firmware_upgrade_device->en_logic_num > FIRMWARE_EN_INFO_MAX) {
        FIRMWARE_DRIVER_DEBUG_ERROR("The number of en_logic_num:%u configurations exceeds the maximum limit:%u.\n",
            firmware_upgrade_device->en_logic_num, FIRMWARE_EN_INFO_MAX);
        return -ENXIO;
    }
    sysfs_info->logic_dev_en_num = firmware_upgrade_device->en_logic_num;
    /* Enable through register */
    for (i = 0; i < sysfs_info->logic_dev_en_num; i++) {
        firmware_logic_dev_en_point = &sysfs_info->logic_dev_en_info[i];
        strncpy(firmware_logic_dev_en_point->dev_name, firmware_upgrade_device->en_logic_dev[i], FIRMWARE_DEV_NAME_LEN - 1);
        firmware_logic_dev_en_point->addr = firmware_upgrade_device->en_logic_addr[i];
        firmware_logic_dev_en_point->mask = firmware_upgrade_device->en_logic_mask[i];
        firmware_logic_dev_en_point->en_val = firmware_upgrade_device->en_logic_en_val[i];
        firmware_logic_dev_en_point->dis_val = firmware_upgrade_device->en_logic_dis_val[i];
        firmware_logic_dev_en_point->width = firmware_upgrade_device->en_logic_width[i];
    }

    return 0;
}

static int  firmware_sysfs_probe(struct platform_device *pdev)
{
    int ret;
    firmware_sysfs_t *sysfs_info;
    firmware_device_t *frm_dev;

    FIRMWARE_DRIVER_DEBUG_VERBOSE("Enter firmware_sysfs_probe\r\n");
    sysfs_info = devm_kzalloc(&pdev->dev, sizeof(firmware_sysfs_t), GFP_KERNEL);
    if (sysfs_info == NULL) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Failed to kzalloc device tree.\n");
        return -EPERM;
    }

    if (pdev->dev.of_node) {
        ret = of_firmware_upgrade_config_init(&pdev->dev, sysfs_info);
    } else {
        ret = firmware_upgrade_config_init(&pdev->dev, sysfs_info);
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
    frm_dev->chain = sysfs_info->chain;
    snprintf(frm_dev->name, FIRMWARE_NAME_LEN - 1, "firmware_sysfs%d", frm_dev->chain);
    strncpy(sysfs_info->devname, frm_dev->name, strlen(frm_dev->name) + 1);

    INIT_LIST_HEAD(&frm_dev->list);
    frm_dev->dev.minor = MISC_DYNAMIC_MINOR;
    frm_dev->dev.name = frm_dev->name;
    frm_dev->dev.fops = &sysfs_dev_fops;
    frm_dev->priv = sysfs_info;

    FIRMWARE_DRIVER_DEBUG_VERBOSE("Register sysfs firmware chain:%d, name:%s.\n", frm_dev->chain, frm_dev->name);

    ret = firmware_device_register(frm_dev);
    if (ret < 0) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Failed to register firmware device.\n");
        return -EPERM;
    }

    platform_set_drvdata(pdev, frm_dev);
    return 0;
}

static int __exit firmware_sysfs_remove(struct platform_device *pdev)
{
    firmware_device_t *frm_dev;

    frm_dev = (firmware_device_t *)platform_get_drvdata(pdev);
    firmware_device_unregister(frm_dev);
    platform_set_drvdata(pdev, NULL);

    return 0;
}

static struct of_device_id sysfs_match[] = {
    {
        .compatible = "firmware_sysfs",
    },
    {},
};

static struct platform_driver sysfs_driver = {
    .driver = {
        .name = "firmware_sysfs",
        .owner = THIS_MODULE,
        .of_match_table = sysfs_match,
    },
    .probe = firmware_sysfs_probe,
    .remove = firmware_sysfs_remove,
};

static firmware_driver_t fmw_drv_sysfs = {
    .name = "firmware_sysfs",
    .drv = &sysfs_driver,
};

int firmware_sysfs_init(void)
{
    int ret;

    INIT_LIST_HEAD(&fmw_drv_sysfs.list);
    FIRMWARE_DRIVER_DEBUG_VERBOSE("sysfs upgrade driver register \n");
    ret = firmware_driver_register(&fmw_drv_sysfs);
    if (ret < 0) {
        FIRMWARE_DRIVER_DEBUG_ERROR("sysfs upgrade driver register failed\n");
        return ret;
    }
    return 0;
}

void firmware_sysfs_exit(void)
{
    firmware_driver_unregister(&fmw_drv_sysfs);
    INIT_LIST_HEAD(&fmw_drv_sysfs.list);
}

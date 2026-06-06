/*
 * file function:
 * 1. reg platform drv to get firmware i2c upgrade's conf info(@dts + @reg platform_device with platdata)
 * 2. reg misc dev (/dev/firmware_ispvme_i2cx) to provide firmware i2c upgrade's conf info to user by ioctl cmd
 *
 * History
 *  [Version]        [Author]               [Date]            [Description]
 *    v1.0             yxq                 2025-04-08           init version
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <firmware_ispvme_i2c.h>

#include <wb_bsp_kernel_debug.h>

#define DEV_NAME_MAX_LEN  48

typedef struct firmware_ispvme_i2c_info_s {
    struct miscdevice dev;
    struct list_head list;
    char name[DEV_NAME_MAX_LEN];
    fw_ispvme_i2c_info_t user_info;
} firmware_ispvme_i2c_info_t;

static LIST_HEAD(info_list_head);
static DEFINE_SPINLOCK(info_list_lock);

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

firmware_ispvme_i2c_info_t *firmware_ispvme_i2c_info_get_by_minor(int minor)
{
    firmware_ispvme_i2c_info_t *p_info;

    spin_lock(&info_list_lock);
    list_for_each_entry(p_info, &info_list_head, list) {
        if (p_info->dev.minor == minor) {
            spin_unlock(&info_list_lock);
            DEBUG_INFO("finded\n");
            return p_info;
        }
    }
    spin_unlock(&info_list_lock);

    DEBUG_INFO("no finded\n");
    return NULL;
}

static int firmware_ispvme_i2c_open(struct inode *inode, struct file *file)
{
    firmware_ispvme_i2c_info_t *p_info;

    DEBUG_INFO("open\n");
    p_info = firmware_ispvme_i2c_info_get_by_minor(MINOR(inode->i_rdev));
    if (p_info == NULL) {
        DEBUG_ERROR("get info by minor fail\n");
        return -ENXIO;
    }
    file->private_data = p_info;

    return 0;
}

static ssize_t firmware_ispvme_i2c_read (struct file *file, char __user *buf, size_t count,
                                   loff_t *offset)
{
    return 0;
}

static ssize_t firmware_ispvme_i2c_write (struct file *file, const char __user *buf, size_t count,
                                    loff_t *offset)
{
    return 0;
}

static loff_t firmware_ispvme_i2c_llseek(struct file *file, loff_t offset, int origin)
{
    return 0;
}

static long firmware_ispvme_i2c_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    firmware_ispvme_i2c_info_t *p_info;

    DEBUG_INFO("cmd: %d\n", cmd);

    p_info = (firmware_ispvme_i2c_info_t *)file->private_data;
    if (p_info == NULL) {
        DEBUG_ERROR("file private_data null\n");
        return -EPERM;
    }

    switch (cmd) {
    case FIRMWARE_ISPVME_I2C_INFO:
        if (copy_to_user((void __user *)arg, &p_info->user_info, sizeof(fw_ispvme_i2c_info_t))) {
            DEBUG_ERROR("copy to user fail\n");
            return -EFAULT;
        }
        break;

    default:
        DEBUG_ERROR("unknown cmd: %d\n", cmd);
        return -ENOTTY;
    } /* End of switch */

    return 0;
}

static int firmware_ispvme_i2c_release(struct inode *inode, struct file *file)
{
    return 0;
}

static const struct file_operations firmware_ispvme_i2c_dev_fops = {
    .owner          = THIS_MODULE,
    .llseek         = firmware_ispvme_i2c_llseek,
    .read           = firmware_ispvme_i2c_read,
    .write          = firmware_ispvme_i2c_write,
    .unlocked_ioctl = firmware_ispvme_i2c_ioctl,
    .open           = firmware_ispvme_i2c_open,
    .release        = firmware_ispvme_i2c_release,
};

/* Gets the information in the device tree */
static int firmware_ispvme_i2c_get_cfg_by_dts(struct device *dev, firmware_ispvme_i2c_info_t *p_info)
{
    int ret;

    ret  = of_property_read_u32(dev->of_node, "chain",      &p_info->user_info.chain);
    ret |= of_property_read_u32(dev->of_node, "bus_num",    &p_info->user_info.bus_num);
    ret |= of_property_read_u32(dev->of_node, "slave_addr", &p_info->user_info.slave_addr);
    ret |= of_property_read_u32(dev->of_node, "cpld_type",  &p_info->user_info.cpld_type);
    if (ret != 0) {
       DEBUG_ERROR("get cfg by dts fail, ret:%d.\n", ret);
       return -ENXIO;
    }

    return ret;
}

static int firmware_ispvme_i2c_get_cfg_by_platdata(struct device *dev, firmware_ispvme_i2c_info_t *p_info)
{
    if (dev->platform_data == NULL) {
        DEBUG_ERROR("platform data null\n");
        return -ENXIO;
    }

    memcpy(&p_info->user_info, (fw_ispvme_i2c_info_t *)dev->platform_data, sizeof(fw_ispvme_i2c_info_t));
    return 0;
}

static int firmware_ispvme_i2c_probe(struct platform_device *pdev)
{
    int ret;
    firmware_ispvme_i2c_info_t *p_info;

    DEBUG_VERBOSE("probe begin\n");
    p_info = devm_kzalloc(&pdev->dev, sizeof(firmware_ispvme_i2c_info_t), GFP_KERNEL);
    if (p_info == NULL) {
        dev_err(&pdev->dev, "Failed to kzalloc firmware_ispvme_i2c_info.\n");
        return -ENOMEM;
    }
    platform_set_drvdata(pdev, p_info);

    if (pdev->dev.of_node) {
        ret = firmware_ispvme_i2c_get_cfg_by_dts(&pdev->dev, p_info);
    } else {
        ret = firmware_ispvme_i2c_get_cfg_by_platdata(&pdev->dev, p_info);
    }
    DEBUG_INFO("ret:%d, chain:%u, bus_num:%u, slave_addr:%u, cpld_type:%u\n", ret, p_info->user_info.chain,
               p_info->user_info.bus_num, p_info->user_info.slave_addr, p_info->user_info.cpld_type);
    if (ret < 0) {
        dev_err(&pdev->dev, "get cfg fail\n");
        return -EPERM;
    }

    (void)snprintf(p_info->name, DEV_NAME_MAX_LEN - 1,
                   ISPVME_I2C_MISCDEV_NAME_PREFIX"%u", p_info->user_info.chain);
    p_info->dev.name  = p_info->name;
    p_info->dev.minor = MISC_DYNAMIC_MINOR;
    p_info->dev.fops  = &firmware_ispvme_i2c_dev_fops;
    ret = misc_register(&p_info->dev);
    if (ret < 0) {
        dev_err(&pdev->dev, "reg misc dev: %s fail, ret=%d\n", p_info->dev.name, ret);
        return -EPERM;
    }
    DEBUG_INFO("reg misc dev: %s ok\n", p_info->name);

    spin_lock(&info_list_lock);
    list_add_tail(&p_info->list, &info_list_head);
    spin_unlock(&info_list_lock);

    dev_info(&pdev->dev, "firmware_ispvme_i2c%u probe ok\n", p_info->user_info.chain);
    return 0;
}

static int firmware_ispvme_i2c_remove(struct platform_device *pdev)
{
    firmware_ispvme_i2c_info_t *p_info;

    DEBUG_VERBOSE("remove begin\n");
    p_info = platform_get_drvdata(pdev);

    spin_lock(&info_list_lock);
    list_del(&p_info->list);
    spin_unlock(&info_list_lock);

    misc_deregister(&p_info->dev);

    DEBUG_VERBOSE("remove end\n");
    return 0;
}

static struct of_device_id firmware_ispvme_i2c_match[] = {
    {
        .compatible = "firmware_ispvme_i2c",
    },
    { /* Sentinel */ }
};

static struct platform_driver firmware_ispvme_i2c_drv = {
    .driver = {
        .name = "firmware_ispvme_i2c",
        .owner = THIS_MODULE,
        .of_match_table = firmware_ispvme_i2c_match,
    },
    .probe  = firmware_ispvme_i2c_probe,
    .remove = firmware_ispvme_i2c_remove,
};

module_platform_driver(firmware_ispvme_i2c_drv);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("Firmware upgrade ispvme i2c driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

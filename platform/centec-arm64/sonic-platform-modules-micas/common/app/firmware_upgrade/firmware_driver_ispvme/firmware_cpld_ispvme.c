/*
 * firmware_cpld.c
 * Original Author : support <support> 2013-10-25
 *
 * CPLD driver
 *
 * History
 *    v1.0    support <support> 2013-10-25  Initial version.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <asm/uaccess.h>
#include <firmware_ispvme.h>
#include <firmware_cpld_ispvme.h>

static int firmware_cpld_open(struct inode *inode, struct file *file)
{
    firmware_cpld_t *cpld_info;
    firmware_device_t *frm_dev;

    dev_debug(firmware_debug(), "Open cpld device.\n");

    frm_dev = firmware_get_device_by_minor(FIRMWARE_CPLD, MINOR(inode->i_rdev));
    if (frm_dev == NULL) {
        return -ENXIO;
    }

    file->private_data = frm_dev;
    cpld_info = (firmware_cpld_t *)frm_dev->priv;

    if (cpld_info != NULL && cpld_info->init_cpld) {
        cpld_info->init_cpld();
    }

    if (cpld_info != NULL && cpld_info->init_chip) {
        cpld_info->init_chip(0);
    }

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

static long firmware_cpld_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    char value;
    void __user *argp;
	cmd_info_t cmd_info;
	char *buf;
	int ret;

    argp = (void __user *)arg;

    switch (cmd) {
    case FIRMWARE_JTAG_TDI:
        if (copy_from_user(&value, argp, sizeof(value))) {
            return -EFAULT;
        }
        fwm_cpld_tdi_op(value);
        break;
    case FIRMWARE_JTAG_TCK:
        if (copy_from_user(&value, argp, sizeof(value))) {
            return -EFAULT;
        }
        fwm_cpld_tck_op(value);
        break;

    case FIRMWARE_JTAG_TMS:
        if (copy_from_user(&value, argp, sizeof(value))) {
            return -EFAULT;
        }
        fwm_cpld_tms_op(value);
        break;

    case FIRMWARE_JTAG_TDO:
        value = fwm_cpld_tdo_op();

        if (copy_to_user(argp, &value, sizeof(value))) {
            return -EFAULT;
        }

        break;

	case FIRMWARE_SET_GPIO_INFO:
        /* set GPIO information */
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

        ret = fmw_cpld_set_gpio_info((firmware_upg_gpio_info_t*)buf);
        if (ret != FIRMWARE_SUCCESS) {
            dev_debug(firmware_debug(), "Failed to set gpio info.\n");
            kfree(buf);
            return -ESRCH;
        }

        kfree(buf);
        break;

    case FIRMWARE_SET_DEBUG_ON:
        /* DEBUG ON */
        firmware_set_debug(1);
        break;
    case FIRMWARE_SET_DEBUG_OFF:
        /* DEBUG_OFF */
        firmware_set_debug(0);
        break;

    default:
        return -ENOTTY;
    }

    return FIRMWARE_SUCCESS;
}

static int firmware_cpld_release(struct inode *inode, struct file *file)
{
    firmware_cpld_t *cpld_info;
    firmware_device_t *frm_dev;

    frm_dev = (firmware_device_t *)(file->private_data);
    cpld_info = (firmware_cpld_t *)(frm_dev->priv);

    if (cpld_info != NULL && cpld_info->finish_chip) {
        cpld_info->finish_chip(0);
    }

    if (cpld_info != NULL && cpld_info->finish_cpld) {
        cpld_info->finish_cpld();
    }
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

static int  firmware_cpld_probe(struct platform_device *pdev)
{
    const __be32 *slot;
    int len;
    int ret;
    firmware_cpld_t *cpld_info;
    firmware_device_t *frm_dev;

    frm_dev = (firmware_device_t *) kzalloc(sizeof(firmware_device_t), GFP_KERNEL);
    if (frm_dev == NULL) {
        dev_debug(firmware_debug(), "Failed to kzalloc firmware device.\n");
        return -EPERM;
    }

    slot = of_get_property(pdev->dev.of_node, "slot", &len);
    if (slot && len == sizeof(*slot)) {
        frm_dev->slot = be32_to_cpup(slot);
    } else {
        frm_dev->slot = firmware_get_device_num(FIRMWARE_CPLD) + 1;
    }

    snprintf(frm_dev->name, FIRMWARE_NAME_LEN - 1, "firmware_cpld_ispvme%d", frm_dev->slot - 1);
    cpld_info = fmw_cpld_upg_get_cpld(frm_dev->name);

    INIT_LIST_HEAD(&frm_dev->list);
    frm_dev->type = FIRMWARE_CPLD;
    frm_dev->dev.minor = MISC_DYNAMIC_MINOR;
    frm_dev->dev.name = frm_dev->name;
    frm_dev->dev.fops = &cpld_dev_fops;
    frm_dev->priv = cpld_info;

    dev_debug(firmware_debug(), "Register cpld firmware slot:%d, name:%s.\n", frm_dev->slot, frm_dev->name);

    ret = firmware_device_register(frm_dev);
    if (ret < 0) {
        dev_debug(firmware_debug(), "Failed to register firmware device.\n");
        kfree(frm_dev);
        return -EPERM;
    }

    platform_set_drvdata(pdev, frm_dev);

    return 0;
}

static int __exit firmware_cpld_remove(struct platform_device *dev)
{
    firmware_device_t *frm_dev;

    frm_dev = (firmware_device_t *)platform_get_drvdata(dev);
    firmware_device_unregister(frm_dev);

    kfree(frm_dev);

    return 0;
}

static struct of_device_id cpld_match[] = {
    {
        .compatible = "firmware_cpld_ispvme",
    },
    {},
};

static struct platform_driver cpld_driver = {
    .driver = {
        .name = "firmware_cpld_ispvme",
        .owner = THIS_MODULE,
        .of_match_table = cpld_match,
    },
    .probe = firmware_cpld_probe,
    .remove = firmware_cpld_remove,
};

static firmware_driver_t fmw_drv = {
    .name = "firmware_cpld_ispvme",
    .type = FIRMWARE_CPLD,
    .drv = &cpld_driver,
};

void firmware_cpld_init(void)
{
    int ret;
#if 0
    struct device_node* node;
    node = of_find_node_by_name(NULL, "cpld_upgrade");
    if (node == NULL) {
        pr_notice("No cpld_upgrade\r\n");
        return;
    }
    pr_notice("Found cpld_upgrade\r\n");
#else
    printk(KERN_INFO "Do init cpld_upgrade\r\n");
#endif
    INIT_LIST_HEAD(&fmw_drv.list);
    ret = fmw_cpld_upg_init();
    if (ret < 0) {
        return;
    }
    firmware_driver_register(&fmw_drv);
}

void firmware_cpld_exit(void)
{
    fmw_cpld_upg_exit();
    firmware_driver_unregister(&fmw_drv);
    INIT_LIST_HEAD(&fmw_drv.list);
}
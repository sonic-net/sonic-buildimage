#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/uio_driver.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/version.h>
#include <linux/spinlock.h>

typedef struct dfd_irq_s {
    int gpio;
    int irq_type;
    struct uio_info dfd_irq_info;
    spinlock_t lock;
    struct attribute_group attr_group;
} dfd_irq_t;

#define DRV_NAME                      "uio-irq"
#define DRV_VERSION                   "1.0"
#define ENABLE_VAL                    (1)
#define DISABLE_VAL                   (0)

#define DEBUG_ERR_LEVEL       (0x1)
#define DEBUG_WARN_LEVEL      (0x2)
#define DEBUG_INFO_LEVEL      (0x4)
#define DEBUG_VER_LEVEL       (0x8)

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);
#define DEBUG_ERROR(fmt, args...)                                                           \
    do {                                                                                    \
        if (debug & DEBUG_ERR_LEVEL) {                                                                    \
            printk(KERN_ERR "[ERR][func:%s line:%d]  "fmt, __func__, __LINE__, ## args);         \
        } else {                                                                            \
            pr_debug(fmt, ## args);                                                         \
        }                                                                                   \
    } while(0)

#define DEBUG_WARN(fmt, args...)                                                            \
        do {                                                                                \
            if (debug & DEBUG_WARN_LEVEL) {                                                                \
                printk(KERN_WARNING "[WARN][func:%s line:%d]  "fmt, __func__, __LINE__, ## args); \
            } else {                                                                        \
                pr_debug(fmt, ## args);                                                     \
            }                                                                               \
        } while(0)

#define DEBUG_INFO(fmt, args...)                                                            \
    do {                                                                                    \
        if (debug & DEBUG_INFO_LEVEL) {                                                                    \
            printk(KERN_INFO "[INFO][func:%s line:%d]  "fmt, __func__, __LINE__, ## args);        \
        } else {                                                                            \
            pr_debug(fmt, ## args);                                                         \
        }                                                                                   \
    } while(0)

#define DEBUG_VERBOSE(fmt, args...)                                                         \
    do {                                                                                    \
        if (debug & DEBUG_VER_LEVEL) {                                                                    \
            printk(KERN_DEBUG "[VER][func:%s line:%d]  "fmt, __func__, __LINE__, ## args);       \
        } else {                                                                            \
            pr_debug(fmt, ## args);                                                         \
        }                                                                                   \
    } while(0)

static irqreturn_t dfd_genirq_handler(int irq, struct uio_info *dev_info)
{
    disable_irq_nosync(irq);
    DEBUG_VERBOSE("handler disable irq");
    return IRQ_HANDLED;
}

static int dfd_genirq_irqcontrol(struct uio_info *dev_info, s32 irq_on)
{
    struct irq_data *irqdata;

    irqdata = irq_get_irq_data(dev_info->irq);

    if (irqd_irq_disabled(irqdata) == !irq_on) {
        DEBUG_VERBOSE("irq already disable");
        return 0;
    }
    if (irq_on) {
        DEBUG_VERBOSE("irqcontrol enable irq");
        enable_irq(dev_info->irq);
    } else {
        DEBUG_VERBOSE("irqcontrol disable irq");
        disable_irq(dev_info->irq);
    }

    return 0;
}

static ssize_t set_irq_enable(struct device *dev,
            struct device_attribute *attr,
            const char *buf, size_t count)
{
    dfd_irq_t *dfd_irq;
    struct uio_info *dev_info;
    unsigned long flags;
    int ret, val;

    dfd_irq = dev_get_drvdata(dev);
    dev_info = &dfd_irq->dfd_irq_info;

    spin_lock_irqsave(&dfd_irq->lock, flags);

    sscanf(buf, "%d", &val);
    DEBUG_VERBOSE("set val:%d.\n", val);

    if ((val != ENABLE_VAL) && (val != DISABLE_VAL)) {
        DEBUG_ERROR("unsupport val:%d ", val);
        ret = -EINVAL;
        goto fail;
    }

    if (val) {
        DEBUG_VERBOSE("sysfs enable irq");
        enable_irq(dev_info->irq);
    } else {
        DEBUG_VERBOSE("sysfs disable irq");
        disable_irq(dev_info->irq);
    }

    spin_unlock_irqrestore(&dfd_irq->lock, flags);
    return count;

fail:
    spin_unlock_irqrestore(&dfd_irq->lock, flags);
    return ret;
}

static DEVICE_ATTR(irq_enable,  S_IWUSR, NULL, set_irq_enable);

static struct attribute *irq_attrs[] = {
    &dev_attr_irq_enable.attr,
    NULL,
};

static struct attribute_group irq_attr_group = {
    .attrs = irq_attrs,
};

static int dfd_irq_probe(struct platform_device *pdev)
{
    u32 gpio, irq_type, pirq_line;
    int ret, ret1, ret2;
    struct uio_info *dfd_irq_info;
    dfd_irq_t *dfd_irq;

    dfd_irq = kzalloc(sizeof(dfd_irq_t), GFP_KERNEL);
    if (!dfd_irq) {
        dev_err(&pdev->dev, "dfd_irq_t kzalloc failed.\n");
        return -ENOMEM;
    }

    dfd_irq_info = &dfd_irq->dfd_irq_info;
    dfd_irq_info->version = "1.0";
    dfd_irq_info->name = "uio-irq";

    /* get pirq line for x86 */
    ret1 = of_property_read_u32(pdev->dev.of_node, "pirq-line", &pirq_line);
    if (!ret1) {
        DEBUG_VERBOSE("use pirq-line method, pirq-line:%u", pirq_line);
        dfd_irq_info->irq = pirq_line;
    }

    ret2 = of_property_read_u32(pdev->dev.of_node, "gpio", &gpio);
    if (!ret2 && ret1) {
        dfd_irq->gpio = gpio;
        gpio_request(dfd_irq->gpio, "GPIOA");
        dfd_irq_info->irq = gpio_to_irq(dfd_irq->gpio);
        DEBUG_VERBOSE("use gpio:%u, irq num:%ld", gpio, dfd_irq_info->irq);
    } else if (ret2 && ret1){
        ret = -ENXIO;
        dev_err(&pdev->dev, "no define irq num. ret2:%d, ret1:%d.\n", ret2, ret1);
        goto free_mem;
    }

    ret = of_property_read_u32(pdev->dev.of_node, "irq_type", &irq_type);
    if (!ret && ret1) {
        DEBUG_VERBOSE("use irq_type:%u", irq_type);
        dfd_irq->irq_type = irq_type;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
        irq_set_irq_type(dfd_irq_info->irq, dfd_irq->irq_type);
#else
        set_irq_type(dfd_irq_info->irq, dfd_irq->irq_type);
#endif
    } else if (ret && ret1){
        ret = -ENXIO;
        dev_err(&pdev->dev, "no define irq type. ret:%d, ret1:%d.\n", ret, ret1);
        goto free_mem;
    }

    dfd_irq_info->irq_flags = IRQF_SHARED;
    dfd_irq_info->handler = dfd_genirq_handler;
    dfd_irq_info->irqcontrol = dfd_genirq_irqcontrol;

    if(uio_register_device(&pdev->dev, dfd_irq_info)){
        ret = -ENODEV;
        dev_err(&pdev->dev, "uio register failed.\n");
        goto free_mem;
    }

    spin_lock_init(&dfd_irq->lock);

    dfd_irq->attr_group = irq_attr_group;
    ret = sysfs_create_group(&pdev->dev.kobj, &dfd_irq->attr_group);
    if (ret != 0) {
        dev_err(&pdev->dev, "sysfs_create_group failed. ret:%d.\n", ret);
        goto free_mem;
    }
    DEBUG_VERBOSE("sysfs create group success\n");

    platform_set_drvdata(pdev, dfd_irq);

    return 0;

free_mem:
    kfree(dfd_irq);

    return ret;
}

static int dfd_irq_remove(struct platform_device *pdev)
{
    dfd_irq_t *dfd_irq;
    struct uio_info *dfd_irq_info;

    dfd_irq = platform_get_drvdata(pdev);
    dfd_irq_info = &dfd_irq->dfd_irq_info;

    uio_unregister_device(dfd_irq_info);
    kfree(dfd_irq);

    sysfs_remove_group(&pdev->dev.kobj, &dfd_irq->attr_group);

    return 0;
}

static struct of_device_id dfd_irq_match[] = {
    {
        .compatible = "uio-irq",
    },
    {},
};
MODULE_DEVICE_TABLE(of, dfd_irq_match);

static struct platform_driver dfd_irq_driver = {
    .probe      = dfd_irq_probe,
    .remove     = dfd_irq_remove,
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = DRV_NAME,
        .of_match_table = dfd_irq_match,
    },
};

static int __init dfd_irq_init(void)
{
    int ret;

    ret =  platform_driver_register(&dfd_irq_driver);
    if (ret != 0 ) {
        return ret;
    }

    return 0;
}

static void __exit dfd_irq_exit(void)
{
    platform_driver_unregister(&dfd_irq_driver);
}

module_init(dfd_irq_init);
module_exit(dfd_irq_exit);
MODULE_LICENSE("GPL");


#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/uio_driver.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/of.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

typedef struct dfd_irq_s {
    int gpio;
    int irq_type;
    atomic64_t irq_count;
    char dev_name[32];
    struct uio_info dfd_irq_info;
    struct list_head list;
} dfd_irq_t;

#define DRV_NAME                      "uio-irq"
#define DFD_IRQ_COMPATIBLE            "uio-irq"
#define DFD_IRQ_COUNTS_FILE           "dfd_irq_counts"

static DEFINE_MUTEX(dfd_irq_list_lock);
static LIST_HEAD(dfd_irq_list);
static struct dentry *dfd_irq_debugfs_dir;
/* dts:
 *   irq0 {
 *      compatible = "uio-irq";
 *      gpio = <27>;
 *      irq_type = <0x00000008>;
 *   };
 */
 
static int dfd_genirq_irqcontrol(struct uio_info *dev_info, s32 irq_on)
{
    struct irq_data *irqdata;
    irqdata = irq_get_irq_data(dev_info->irq);
    if (irqdata == NULL) {
        return -EPERM;
    } 
    if (irqd_irq_disabled(irqdata) == !irq_on) {
        return 0;
    }
    if (irq_on) {
        enable_irq(dev_info->irq);
    } else {
        disable_irq(dev_info->irq);
    }
    return 0;
}

static irqreturn_t dfd_genirq_handler(int irq, struct uio_info *dev_info)
{
    dfd_irq_t *dfd_irq;

    if (dev_info == NULL) {
        return IRQ_NONE;
    }

    disable_irq_nosync(irq);

    dfd_irq = container_of(dev_info, dfd_irq_t, dfd_irq_info);
    atomic64_inc(&dfd_irq->irq_count);
    return IRQ_HANDLED;
}

static int dfd_irq_debugfs_show(struct seq_file *seq, void *unused)
{
    dfd_irq_t *dfd_irq;

    mutex_lock(&dfd_irq_list_lock);
    list_for_each_entry(dfd_irq, &dfd_irq_list, list) {
        seq_printf(seq, "%s: %lld\n", dfd_irq->dev_name,
            (long long)atomic64_read(&dfd_irq->irq_count));
    }
    mutex_unlock(&dfd_irq_list_lock);

    return 0;
}

static int dfd_irq_debugfs_open(struct inode *inode, struct file *file)
{
    return single_open(file, dfd_irq_debugfs_show, inode->i_private);
}

static const struct file_operations dfd_irq_debugfs_fops = {
    .owner = THIS_MODULE,
    .open = dfd_irq_debugfs_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static ssize_t gpio_value_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct platform_device *pdev = to_platform_device(dev);
    dfd_irq_t *dfd_irq = platform_get_drvdata(pdev);
    int value;

    value = gpio_get_value(dfd_irq->gpio);
    return sprintf(buf, "%d\n", value);
}

static DEVICE_ATTR(gpio_value, 0444, gpio_value_show, NULL);

static const char *dfd_irq_get_gpio_line_name(unsigned int gpio)
{
    struct gpio_desc *desc;
    struct gpio_chip *gpio_chip;
    struct device_node *gpio_np;
    const char *line_name;
    int offset;
    int ret;

    desc = gpio_to_desc(gpio);
    if (!desc) {
        return NULL;
    }

    gpio_chip = gpiod_to_chip(desc);
    if (!gpio_chip) {
        return NULL;
    }

    gpio_np = to_of_node(gpio_chip->fwnode);
    if (!gpio_np && gpio_chip->parent) {
        gpio_np = gpio_chip->parent->of_node;
    }
    if (!gpio_np) {
        return NULL;
    }

    if (gpio_chip->base < 0 || gpio < gpio_chip->base) {
        return NULL;
    }

    offset = desc_to_gpio(desc) - gpio_chip->base;
    if (offset < 0 || offset >= gpio_chip->ngpio) {
        return NULL;
    }

    ret = of_property_read_string_index(gpio_np, "gpio-line-names", offset,
        &line_name);
    if (ret || !line_name || !line_name[0]) {
        return NULL;
    }

    return line_name;
}

static int dfd_irq_probe(struct platform_device *pdev)
{
    u32 gpio, irq_type;
    const char *irq_name;
    int ret;
    struct uio_info *dfd_irq_info;
    dfd_irq_t *dfd_irq;

    dfd_irq = kzalloc(sizeof(dfd_irq_t), GFP_KERNEL);
    if (!dfd_irq) {
        dev_err(&pdev->dev, "wb_platform_irq, failed to allocate memory\n");
        return -ENOMEM;
    }
    dfd_irq_info = &dfd_irq->dfd_irq_info;
    memset(dfd_irq_info, 0, sizeof(*dfd_irq_info));
    INIT_LIST_HEAD(&dfd_irq->list);
    atomic64_set(&dfd_irq->irq_count, 0);
    dfd_irq_info->version = "1.0";
    ret = of_property_read_u32(pdev->dev.of_node, "gpio", &gpio);
    if (!ret) {
        dev_dbg(&pdev->dev, "wb_platform_irq, gpio: %d\n", gpio);
        dfd_irq->gpio = gpio;
        irq_name = dfd_irq_get_gpio_line_name(dfd_irq->gpio);
        if (!irq_name) {
            irq_name = dev_name(&pdev->dev);
        }
        snprintf(dfd_irq->dev_name, sizeof(dfd_irq->dev_name), "%s", irq_name);
        dfd_irq_info->name = dfd_irq->dev_name;
        ret = gpio_request(dfd_irq->gpio, dfd_irq->dev_name);
        if (ret) {
            dev_err(&pdev->dev, "Failed to request GPIO %d\n", dfd_irq->gpio);
            goto free_mem;
        }
        ret = gpio_direction_input(dfd_irq->gpio);
        if (ret) {
            dev_err(&pdev->dev, "Failed to set GPIO %d input, ret: %d\n",
                dfd_irq->gpio, ret);
            goto free_gpio;
        }
        ret = gpio_to_irq(dfd_irq->gpio);
        if (ret < 0) {
            dev_err(&pdev->dev, "Failed to map GPIO %d to IRQ, ret: %d\n",
                dfd_irq->gpio, ret);
            goto free_gpio;
        }
        dfd_irq_info->irq = ret;
    } else{
        dev_err(&pdev->dev, "wb_platform_irq, failed to get property: gpio\n");
        ret = -ENXIO;
        goto free_mem;
    }
    ret = of_property_read_u32(pdev->dev.of_node, "irq_type", &irq_type);
    if (!ret) {
        dfd_irq->irq_type = irq_type;
        irq_set_irq_type(dfd_irq_info->irq, dfd_irq->irq_type);
    } else {
        dev_err(&pdev->dev, "wb_platform_irq, failed to get property: irq_type\n");
        ret = -ENXIO;
        goto free_gpio;
    }
    dfd_irq_info->irq_flags = IRQF_SHARED;
    dfd_irq_info->handler = dfd_genirq_handler;
    dfd_irq_info->irqcontrol = dfd_genirq_irqcontrol;
    if(uio_register_device(&pdev->dev, dfd_irq_info)){
        dev_err(&pdev->dev, "wb_platform_irq, failed to register_device, gpio: %d\n", gpio);
        ret = -ENODEV;
        goto free_gpio;
    }

    /*
     * Disable the IRQ immediately after registration to close the window
     * between request_irq() (called inside uio_register_device) and the
     * userspace daemon's first open()+write(enable).  Without this, any
     * hardware interrupt that fires before userspace opens /dev/uioN will
     * increment irq_count in the kernel but be silently swallowed by the
     * UIO core (listener->event_count = current_event at open time), causing
     * the kernel debugfs counter and the userspace soft-irq log to diverge.
     * The IRQ will be re-enabled by dfd_genirq_irqcontrol() when userspace
     * calls write(fd, 1) for the first time.
     */
    disable_irq(dfd_irq_info->irq);

    mutex_lock(&dfd_irq_list_lock);
    list_add_tail(&dfd_irq->list, &dfd_irq_list);
    mutex_unlock(&dfd_irq_list_lock);

    platform_set_drvdata(pdev, dfd_irq);

    ret = device_create_file(&pdev->dev, &dev_attr_gpio_value);
    if (ret) {
        dev_err(&pdev->dev, "Failed to create sysfs file\n");
        goto unregister_uio;
    }
    return 0;

unregister_uio:
    uio_unregister_device(dfd_irq_info);
free_gpio:
    gpio_free(dfd_irq->gpio);
free_mem:
    kfree(dfd_irq);
    return ret;
}
static int dfd_irq_remove(struct platform_device *pdev)
{
    dfd_irq_t *dfd_irq;
    struct uio_info *dfd_irq_info;
    dfd_irq = platform_get_drvdata(pdev);
    if (dfd_irq == NULL) {
        return 0;
    }

    device_remove_file(&pdev->dev, &dev_attr_gpio_value);
    mutex_lock(&dfd_irq_list_lock);
    list_del_init(&dfd_irq->list);
    mutex_unlock(&dfd_irq_list_lock);
    dfd_irq_info = &dfd_irq->dfd_irq_info;
    uio_unregister_device(dfd_irq_info);
    gpio_free(dfd_irq->gpio);
    kfree(dfd_irq);
    return 0;
}
static struct of_device_id dfd_irq_match[] = {
    {
        .compatible = DFD_IRQ_COMPATIBLE,
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

    dfd_irq_debugfs_dir = debugfs_create_dir(DRV_NAME, NULL);
    if (!IS_ERR_OR_NULL(dfd_irq_debugfs_dir)) {
        debugfs_create_file(DFD_IRQ_COUNTS_FILE, 0444, dfd_irq_debugfs_dir, NULL,
            &dfd_irq_debugfs_fops);
    } else {
        dfd_irq_debugfs_dir = NULL;
    }

    ret =  platform_driver_register(&dfd_irq_driver);
    if (ret != 0 ) {
        if (dfd_irq_debugfs_dir != NULL) {
            debugfs_remove_recursive(dfd_irq_debugfs_dir);
            dfd_irq_debugfs_dir = NULL;
        }
        return ret;
    }
    return 0;
}
static void __exit dfd_irq_exit(void)
{
    if (dfd_irq_debugfs_dir != NULL) {
        debugfs_remove_recursive(dfd_irq_debugfs_dir);
        dfd_irq_debugfs_dir = NULL;
    }
    platform_driver_unregister(&dfd_irq_driver);
}
module_init(dfd_irq_init);
module_exit(dfd_irq_exit);
MODULE_LICENSE("GPL");

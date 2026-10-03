/*
 *
 * wb_gpio_init.c
 *
 * The main function of this file is to initialize GPIO.
 *
 * History
 *  [Version]                    [Date]            [Description]
 *    v1.0                      2026-02-13         Initial version
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <linux/err.h>
#include <linux/sysfs.h>
#include <linux/gpio.h>
#include "wb_gpio_init.h"

#include <wb_bsp_kernel_debug.h>

#define DRV_NAME                      "gpio_init"

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static gpio_init_t *g_gpio_init_info = NULL;
static int g_match_device_flag = 0;

/* Obtaining gpio parameters via sysfs */
static ssize_t get_gpio_attr(struct device *dev, struct device_attribute *attr, char *buf)
{
    gpio_port_t *gpio_port_info;
    int offset;
    unsigned long flags;

    offset = 0;
    mem_clear(buf, PAGE_SIZE);
    spin_lock_irqsave(&g_gpio_init_info->gpio_init_lock, flags);
    list_for_each_entry(gpio_port_info, &g_gpio_init_info->list, list) {

        if (gpio_port_info->init_direction == GPIO_OUTPUT_MODE) {
            offset += scnprintf(buf + offset, PAGE_SIZE - offset, "gpio [%d], direction: output, init_level: %d, flag: 0x%lx\n",
                gpio_port_info->gpio, gpio_port_info->init_level, gpio_port_info->flag);
        } else {
            offset += scnprintf(buf + offset, PAGE_SIZE - offset, "gpio [%d], direction: input, flag: 0x%lx\n",
                gpio_port_info->gpio, gpio_port_info->flag);
        }
    }
    spin_unlock_irqrestore(&g_gpio_init_info->gpio_init_lock, flags);

    return strlen(buf);
}

static DEVICE_ATTR(gpio_attr, S_IRUGO, get_gpio_attr, NULL);

static struct attribute *gpio_attrs[] = {
    &dev_attr_gpio_attr.attr,
    NULL,
};

static const struct attribute_group gpio_sysfs_group = {
    .attrs = gpio_attrs,
};

/**
 * Restore GPIO to its initialized status (direction and level)
 *
 * @param gpio_port_info  Pointer to GPIO port configuration structure
 * @return 0 on success, negative error code on failure
 */
static int restore_gpio_init_status(gpio_port_t *gpio_port_info)
{
    int ret;

    if (gpio_port_info->init_direction == GPIO_OUTPUT_MODE) {
        ret = gpio_direction_output(gpio_port_info->gpio, gpio_port_info->init_level);
        if (ret) {
            DEBUG_ERROR("set GPIO[%d] direction_output fail, ret = [%d].\n", gpio_port_info->gpio, ret);
            return ret;
        }

        gpio_port_info->flag |= GGPIOD_IS_OUT;
        if (gpio_port_info->init_level == GPIO_HIGH_LEVEL) {
            gpio_port_info->flag |= GGPIOD_IS_OUT_ACTIVE;
        } else {
            gpio_port_info->flag &= (~GGPIOD_IS_OUT_ACTIVE);
        }
    } else {
        ret = gpio_direction_input(gpio_port_info->gpio);
        if (ret) {
            DEBUG_ERROR("set GPIO[%d] direction_input fail, ret = [%d].\n", gpio_port_info->gpio, ret);
            return ret;
        }

        gpio_port_info->flag &= (~GGPIOD_IS_OUT);
        gpio_port_info->flag &= (~GGPIOD_IS_OUT_ACTIVE);
    }

    return 0;
}

/**
 * @deprecated    Please use driver model instead
 * Request a GPIO. This should be called before any of the other functions
 * are used on this GPIO.
 *
 * Note: With driver model, the label is allocated so there is no need for
 * the caller to preserve it.
 *
 * @param gpio    GPIO number
 * @param label    User label for this GPIO
 * @return 0 if ok
 */
int bsp_gpio_request(unsigned int gpio, const char *label)
{
    int manage_flag;
    int ret;
    unsigned long flags;
    gpio_port_t *gpio_port_info;

    if (label == NULL) {
        DEBUG_ERROR("Parameter error, label is NULL\n");
        return -EINVAL;
    }

    manage_flag = 0;
    spin_lock_irqsave(&g_gpio_init_info->gpio_init_lock, flags);
    /* Traverse the initialization management linked list */
    list_for_each_entry(gpio_port_info, &g_gpio_init_info->list, list) {
        /* The GPIO to be requested is in management */
        if (gpio_port_info->gpio == gpio) {
            manage_flag = 1;
            /* The managed GPIO resource has not been requested by other modules */
            if (!(gpio_port_info->flag & GGPIOD_REQUESTED_BY_OTHER)) {
                DEBUG_VERBOSE("User [%s] requesting GPIO[%d] in management.\n", label, gpio);
                /* Set flag to indicate the GPIO is requested by other modules */
                gpio_port_info->flag |= GGPIOD_REQUESTED_BY_OTHER;
                break;
            } else {
                spin_unlock_irqrestore(&g_gpio_init_info->gpio_init_lock, flags);
                DEBUG_ERROR("GPIO [%d] has been requested.", gpio_port_info->gpio);
                return -EBUSY;
            }
        }
    }
    spin_unlock_irqrestore(&g_gpio_init_info->gpio_init_lock, flags);

    /* GPIO not in the initialization management linked list, request via kernel interface directly */
    if (manage_flag == 0) {
        ret = gpio_request(gpio, label);
        if (ret) {
            DEBUG_ERROR("User [%s] request GPIO[%d] fail, ret = [%d].\n", label, gpio, ret);
            return ret;
        }
    }

    DEBUG_VERBOSE("User [%s] request GPIO[%d] success.\n", label, gpio);
    return 0;
}
EXPORT_SYMBOL_GPL(bsp_gpio_request);

/**
 * @deprecated    Please use driver model instead
 * Stop using the GPIO.  This function should not alter pin configuration.
 *
 * @param gpio    GPIO number
 * @return void
 */
void bsp_gpio_free(unsigned int gpio)
{
    int manage_flag;
    unsigned long flags;
    gpio_port_t *gpio_port_info;

    manage_flag = 0;
    spin_lock_irqsave(&g_gpio_init_info->gpio_init_lock, flags);
    /* Traverse the initialization management linked list */
    list_for_each_entry(gpio_port_info, &g_gpio_init_info->list, list) {
        if (gpio_port_info->gpio == gpio) {
            manage_flag = 1;
            /* The managed GPIO resource has been requested by other modules */
            if (gpio_port_info->flag & GGPIOD_REQUESTED_BY_OTHER) {
                /* Set flag to indicate the GPIO is idle */
                gpio_port_info->flag &= (~GGPIOD_REQUESTED_BY_OTHER);
                break;
            } else {
                /* Failed to release: the managed GPIO resource has not been requested by other modules */
                spin_unlock_irqrestore(&g_gpio_init_info->gpio_init_lock, flags);
                DEBUG_ERROR("GPIO[%d] hasn't been requested.", gpio_port_info->gpio);
                return;
            }
        }
    }
    spin_unlock_irqrestore(&g_gpio_init_info->gpio_init_lock, flags);

    if (manage_flag == 1) {
        /* Restore GPIO to its initialized direction and level when releasing the resource */
        (void)restore_gpio_init_status(gpio_port_info);
    } else {
        gpio_free(gpio);
    }

    DEBUG_VERBOSE("free GPIO[%d] success.\n", gpio);
    return;
}
EXPORT_SYMBOL_GPL(bsp_gpio_free);

/**
 * @deprecated    Please use driver model instead
 * Configure a GPIO as input
 *
 * @param gpio    GPIO number
 * @return 0 on success, negative error code on failure
 */
int bsp_gpio_direction_input(unsigned int gpio)
{
    int ret;
    unsigned long flags;
    gpio_port_t *gpio_port_info;

    ret = gpio_direction_input(gpio);
    if (ret) {
        DEBUG_ERROR("set GPIO[%d] direction_input fail, ret = [%d].\n.", gpio, ret);
        return ret;
    }

    spin_lock_irqsave(&g_gpio_init_info->gpio_init_lock, flags);
    /* Traverse the initialization management linked list and update flags if the GPIO is in management */
    list_for_each_entry(gpio_port_info, &g_gpio_init_info->list, list) {
        if (gpio_port_info->gpio == gpio) {
            gpio_port_info->flag &= (~GGPIOD_IS_OUT);
            break;
        }
    }
    spin_unlock_irqrestore(&g_gpio_init_info->gpio_init_lock, flags);

    return 0;
}
EXPORT_SYMBOL_GPL(bsp_gpio_direction_input);

/**
 * @deprecated    Please use driver model instead
 * Configure a GPIO as output and set its initial level
 *
 * @param gpio    GPIO number
 * @param value    GPIO value (0 for low level, 1 for high level)
 * @return 0 on success, negative error code on failure
 */
int bsp_gpio_direction_output(unsigned int gpio, int value)
{
    int ret;
    unsigned long flags;
    gpio_port_t *gpio_port_info;

    ret = gpio_direction_output(gpio, value);
    if (ret) {
        DEBUG_ERROR("set GPIO[%d] direction_output fail, value = [%d], ret = [%d].\n.", gpio, value, ret);
        return ret;
    }

    spin_lock_irqsave(&g_gpio_init_info->gpio_init_lock, flags);
    /* Traverse the initialization management linked list and update flags if the GPIO is in management */
    list_for_each_entry(gpio_port_info, &g_gpio_init_info->list, list) {
        if (gpio_port_info->gpio == gpio) {
            gpio_port_info->flag |= GGPIOD_IS_OUT;
            if (value == GPIO_HIGH_LEVEL) {
                gpio_port_info->flag |= GGPIOD_IS_OUT_ACTIVE;
            } else {
                gpio_port_info->flag &= (~GGPIOD_IS_OUT_ACTIVE);
            }
            break;
        }
    }
    spin_unlock_irqrestore(&g_gpio_init_info->gpio_init_lock, flags);

    return 0;
}
EXPORT_SYMBOL_GPL(bsp_gpio_direction_output);

/**
 * @deprecated    Please use driver model instead
 * Read the current value of a GPIO. Works for both input and output GPIOs.
 *
 * @param gpio    GPIO number
 * @return 0 for low level, 1 for high level
 */
int bsp_gpio_get_value(unsigned int gpio)
{
    return gpio_get_value(gpio);
}
EXPORT_SYMBOL_GPL(bsp_gpio_get_value);

/**
 * @deprecated    Please use driver model instead
 * Set the value of an output GPIO. The GPIO must be configured as output first,
 * otherwise this function may have no effect.
 *
 * @param gpio    GPIO number
 * @param value    GPIO value (0 for low level, 1 for high level)
 * @return void
 */
void bsp_gpio_set_value(unsigned int gpio, int value)
{
    unsigned long flags;
    gpio_port_t *gpio_port_info;

    spin_lock_irqsave(&g_gpio_init_info->gpio_init_lock, flags);
    /* Traverse the initialization management linked list and update flags if the GPIO is in management */
    list_for_each_entry(gpio_port_info, &g_gpio_init_info->list, list) {
        if (gpio_port_info->gpio == gpio) {
            if (value == GPIO_HIGH_LEVEL) {
                gpio_port_info->flag |= GGPIOD_IS_OUT_ACTIVE;
            } else {
                gpio_port_info->flag &= (~GGPIOD_IS_OUT_ACTIVE);
            }
            break;
        }
    }
    spin_unlock_irqrestore(&g_gpio_init_info->gpio_init_lock, flags);

    gpio_set_value(gpio, value);
    return ;
}
EXPORT_SYMBOL_GPL(bsp_gpio_set_value);

static int gpio_init_func(struct device *dev)
{
    int ret;
    gpio_port_t *gpio_port_info, *gpio_port_info_next;

    DEBUG_VERBOSE("enter gpio_init_func\n");

    list_for_each_entry_safe(gpio_port_info, gpio_port_info_next, &g_gpio_init_info->list, list) {
        /* Request the configuration of the gpio node in the device tree */
        ret = gpio_request(gpio_port_info->gpio, gpio_port_info->gpio_name);
        if (ret) {
            dev_err(dev, "request GPIO[%d] fail, ret: %d .\n", gpio_port_info->gpio, ret);
            return ret;
        }

        /* Set a flag to indicate that the GPIO has been requested. */
        gpio_port_info->flag |= GGPIOD_REQUESTED;

        /* Initialize GPIO according to the device tree configuration. */
        ret = restore_gpio_init_status(gpio_port_info);
        if (ret) {
            dev_err(dev, "init GPIO[%d] status fail, ret: %d.\n", gpio_port_info->gpio, ret);
            return ret;
        }

        /* If a GPIO is set to be released, then that GPIO should be released after initialization. */
        if (gpio_port_info->free_flag) {
            DEBUG_VERBOSE("Due to free_flag, free gpio_init GPIO[%d].\n", gpio_port_info->gpio);
            gpio_free(gpio_port_info->gpio);
            gpio_port_info->flag &= (~GGPIOD_REQUESTED);
            list_del(&gpio_port_info->list);
            g_gpio_init_info->gpio_num--;
        }
    }

    return 0;
}

static void gpio_list_free(void)
{
    gpio_port_t *gpio_port_info, *gpio_port_info_next;

    list_for_each_entry_safe(gpio_port_info, gpio_port_info_next,
        &g_gpio_init_info->list, list) {
        list_del(&gpio_port_info->list);
        g_gpio_init_info->gpio_num--;
    }

    return;
}

static int of_gpio_init_config_init(struct device *dev)
{
    int ret, rv;
    int i;
    struct device_node *child;
    gpio_port_t *gpio_port_info;

    i = 0;
    for_each_child_of_node(dev->of_node, child) {
        gpio_port_info = devm_kzalloc(dev, sizeof(gpio_port_t), GFP_KERNEL);
        if (gpio_port_info == NULL) {
            dev_err(dev, "Failed to devm_kzalloc gpio_port_info\n");
            of_node_put(child);
            return -ENOMEM;
        }

        ret = of_property_read_u32(child, "gpio", &gpio_port_info->gpio);
        if (ret != 0) {
            dev_err(dev, "dts config error, gpio not found, ret: %d.\n", ret);
            of_node_put(child);
            return -ENXIO;
        }

        ret = of_property_read_u32(child, "init_direction", &gpio_port_info->init_direction);
        if (ret != 0) {
            dev_err(dev, "GPIO [%d] init_direction config not found, ret: %d.\n",
                gpio_port_info->gpio, ret);
            of_node_put(child);
            return -ENXIO;
        }

        if ((gpio_port_info->init_direction != GPIO_OUTPUT_MODE) &&
            (gpio_port_info->init_direction != GPIO_INPUT_MODE)) {
            dev_err(dev, "GPIO [%d] init_direction config error, invalid init_direction: %d.\n",
                gpio_port_info->gpio, gpio_port_info->init_direction);
            of_node_put(child);
            return -EINVAL;
        }

        if (gpio_port_info->init_direction == GPIO_OUTPUT_MODE) {
            ret = of_property_read_u32(child, "init_level", &gpio_port_info->init_level);
            if (ret != 0) {
                dev_err(dev, "GPIO [%d] is output but init_level config not found, ret: %d.\n",
                    gpio_port_info->gpio, ret);
                of_node_put(child);
                return -ENXIO;
            }
        }

        /* Optional configurations in the device tree */
        rv = of_property_read_u32(child, "free_flag", &gpio_port_info->free_flag);
        if (rv != 0) {
            DEBUG_VERBOSE("GPIO [%d] doesn't need to be freed after initialization.\n",
                gpio_port_info->gpio);
            gpio_port_info->free_flag = 0;
        }

        rv = of_property_read_string(child, "gpio_name", &gpio_port_info->gpio_name);
        if (rv != 0) {
            DEBUG_VERBOSE("GPIO [%d] gpio_name not found, use driver name.\n",
                gpio_port_info->gpio);
            gpio_port_info->gpio_name = DRV_NAME;
        }

        /* Add the gpio node to the management list */
        list_add_tail(&gpio_port_info->list, &g_gpio_init_info->list);
        i++;
    }

    if (i == 0) {
        dev_err(dev, "GPIO child node not found\n");
        return -EINVAL;
    }

    g_gpio_init_info->gpio_num = i;
    /* debug info */
    dev_info(dev, "gpio_init_parse_info : gpio_num: %d\n", g_gpio_init_info->gpio_num);
    list_for_each_entry(gpio_port_info, &g_gpio_init_info->list, list) {
        if (gpio_port_info->init_direction == GPIO_OUTPUT_MODE) {
            dev_info(dev, "GPIO [%d], label: %s, init_direction: output, init_level: %d, free_flag: %d\n",
                gpio_port_info->gpio, gpio_port_info->gpio_name,
                gpio_port_info->init_level, gpio_port_info->free_flag);
        } else {
            dev_info(dev, "GPIO [%d], label: %s, init_direction: input, free_flag: %d\n",
                gpio_port_info->gpio, gpio_port_info->gpio_name, gpio_port_info->free_flag);
        }
    }

    return 0;
}

static int gpio_init_config_init(struct device *dev)
{

    int ret;

    DEBUG_VERBOSE("enter gpio_init_config_init\n");
    /* This definition must be enabled in the kernel for the device tree to be used. */
    if (dev->of_node) {
        ret = of_gpio_init_config_init(dev);
    } else {
        dev_err(dev, "dev of_node must be defined\n");
        ret = -ENXIO;
    }

    return ret;
}

static void gpio_init_free(void)
{
    gpio_port_t *gpio_port_info;

    list_for_each_entry(gpio_port_info, &g_gpio_init_info->list, list) {
        if (gpio_port_info->flag & GGPIOD_REQUESTED) {
            gpio_free(gpio_port_info->gpio);
            gpio_port_info->flag &= (~GGPIOD_REQUESTED);
        }
    }

    return;
}

static int gpio_init_probe(struct platform_device *pdev)
{
    int ret;
    struct device *dev;

    DEBUG_VERBOSE("enter gpio_init_probe.\n");

    dev = &pdev->dev;
    if (g_match_device_flag == 1) {
        dev_err(dev, "gpio init driver can only match one device.\n");
        return -EBUSY;
    }

    g_match_device_flag = 1;

    ret = gpio_init_config_init(dev);
    if (ret < 0) {
        (void)gpio_list_free();
        DEBUG_ERROR("gpio_init_config_init fail, ret: %d.\n", ret);
        g_match_device_flag = 0;
        return ret;
    }

    ret = gpio_init_func(dev);
    if (ret < 0) {
        (void)gpio_init_free();
        (void)gpio_list_free();
        DEBUG_ERROR("gpio_init_func fail, ret: %d.\n", ret);
        g_match_device_flag = 0;
        return ret;
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &gpio_sysfs_group);
    if (ret != 0) {
        (void)gpio_init_free();
        (void)gpio_list_free();
        dev_err(dev, "Failed to create gpio_sysfs_group, ret: %d\n", ret);
        g_match_device_flag = 0;
        return ret;
    }

    return 0;
}

static void __exit gpio_init_remove(struct platform_device *pdev)
{

    sysfs_remove_group(&pdev->dev.kobj, &gpio_sysfs_group);
    (void)gpio_init_free();
    (void)gpio_list_free();
    g_match_device_flag = 0;
}

static struct of_device_id gpio_init_match[] = {
    {
        .compatible = "gpio_init",
    },
    {},
};

MODULE_DEVICE_TABLE(of, gpio_init_match);

static struct platform_driver wb_gpio_init_driver = {
    .probe = gpio_init_probe,
    .remove = gpio_init_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = DRV_NAME,
        .of_match_table = gpio_init_match,
    },
};

static int __init wb_gpio_init_init(void)
{
    g_gpio_init_info = kzalloc(sizeof(gpio_init_t), GFP_KERNEL);
    if (g_gpio_init_info == NULL) {
        PRINT_ERROR("Failed to kzalloc g_gpio_init_info.\n");
        return -ENOMEM;
    }

    INIT_LIST_HEAD(&g_gpio_init_info->list);
    spin_lock_init(&g_gpio_init_info->gpio_init_lock);

    return platform_driver_register(&wb_gpio_init_driver);
}

static void __exit wb_gpio_init_exit(void)
{
    platform_driver_unregister(&wb_gpio_init_driver);

    if (g_gpio_init_info != NULL) {
        kfree(g_gpio_init_info);
        g_gpio_init_info = NULL;
    }
    DEBUG_VERBOSE("wb_gpio_init_exit\n");
}

module_init(wb_gpio_init_init);
module_exit(wb_gpio_init_exit);
MODULE_DESCRIPTION("gpio init driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/gpio/driver.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include "wb_common_logic_mux.h"
#include <wb_bsp_kernel_debug.h>

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);
static int chip_select_sleep = 400;   /* us */
module_param(chip_select_sleep, int, S_IRUGO | S_IWUSR);

static LIST_HEAD(cs_lock_pool_list);
static DEFINE_MUTEX(cs_pool_lock);

static wb_chip_select_cfg_info_t* g_cs_info_arry[MAX_CS_SIZE];

static bool wb_cs_lock_mode_valid(uint32_t lock_mode)
{
    return (lock_mode == WB_SPIN_LOCK_MODE) || (lock_mode == WB_MUTEX_LOCK_MODE);
}

static void wb_cs_dev_lock_init(struct cs_lock_pool *pool)
{
    if (pool->lock_mode == WB_MUTEX_LOCK_MODE) {
        mutex_init(&pool->lock.mutex_lock);
    } else {
        spin_lock_init(&pool->lock.spin_lock);
    }
}

static void *wb_cs_dev_lock_addr(struct cs_lock_pool *pool)
{
    if (pool->lock_mode == WB_MUTEX_LOCK_MODE) {
        return &pool->lock.mutex_lock;
    }
    return &pool->lock.spin_lock;
}

static void wb_cs_dev_lock(wb_chip_select_cfg_info_t *cfg, unsigned long *flags)
{
    if (cfg->lock_mode == WB_MUTEX_LOCK_MODE) {
        mutex_lock((struct mutex *)cfg->cs_lock);
    } else {
        spin_lock_irqsave((spinlock_t *)cfg->cs_lock, *flags);
    }
}

static void wb_cs_dev_unlock(wb_chip_select_cfg_info_t *cfg, unsigned long *flags)
{
    if (cfg->lock_mode == WB_MUTEX_LOCK_MODE) {
        mutex_unlock((struct mutex *)cfg->cs_lock);
    } else {
        spin_unlock_irqrestore((spinlock_t *)cfg->cs_lock, *flags);
    }
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,12,0)
static int match_chip_by_name(struct gpio_chip *gc, void *data)
{
    const char *name = data;

    return gc->label && !strcmp(gc->label, name);
}

static struct gpio_chip *find_gpio_chip_by_name(const char *name)
{
    return gpiochip_find((void *)name, match_chip_by_name);
}
#endif

static struct cs_lock_pool *cs_find_lock_pool(wb_chip_select_cfg_info_t *cfg, struct list_head *list)
{
    struct cs_lock_pool *pool;

    list_for_each_entry(pool, list, list) {
        if (pool->op_type != cfg->op_type)
            continue;

        if (cfg->op_type == CS_OP_TYPE_GPIO) {
            if (pool->key.gpio.gpio_offset == cfg->attr.gpio_config.gpio_offset) {
                return pool;
            }
        } else if (cfg->op_type == CS_OP_TYPE_LOGIC_DEV) {
            if (strcmp(pool->key.logic.dev_name, cfg->attr.logic_dev_config.dev_name) == 0 &&
                pool->key.logic.reg_addr == cfg->attr.logic_dev_config.reg_addr) {
                return pool;
            }
        }
    }
    return NULL;
}

static int cs_gpio_request(cs_gpio_config_t *cs_gpio_info)
{
    int ret;
    char gpio_label[MAX_NAME_SIZE];

    mem_clear(gpio_label, sizeof(gpio_label));
    snprintf(gpio_label, sizeof(gpio_label), "cs%d_gpio", cs_gpio_info->gpio_offset);
    ret = gpio_request(cs_gpio_info->gpio_offset, gpio_label);
    if (ret) {
        DEBUG_ERROR("Failed to request GPIO %d: %d\n", 
                cs_gpio_info->gpio_offset, ret);
        return -ENODEV;
    }
    ret = gpio_direction_output(cs_gpio_info->gpio_offset, 
                            cs_gpio_info->dis_level);
    if (ret) {
        DEBUG_ERROR("Failed to set GPIO %d direction: %d\n", 
                cs_gpio_info->gpio_offset, ret);
        gpio_free(cs_gpio_info->gpio_offset);
        return -EIO;
    }
    return 0;
}

static void *cs_find_or_create_lock_nolock(wb_chip_select_cfg_info_t *cfg)
{
    int ret;
    struct cs_lock_pool *pool, *new_pool;

    pool = cs_find_lock_pool(cfg, &cs_lock_pool_list);
    if (pool) {
        pool->user_count++;
    } else {
        new_pool = kzalloc(sizeof(struct cs_lock_pool), GFP_KERNEL);
        if (!new_pool) {
            return NULL;
        }
        new_pool->lock_mode = cfg->lock_mode;
        new_pool->op_type = cfg->op_type;
        new_pool->user_count = 1;

        if (cfg->op_type == CS_OP_TYPE_GPIO) {
            ret = cs_gpio_request(&cfg->attr.gpio_config);
            if (ret < 0) {
                kfree(new_pool);
                return NULL;
            }
            new_pool->key.gpio.gpio_offset = cfg->attr.gpio_config.gpio_offset;
        } else if (cfg->op_type == CS_OP_TYPE_LOGIC_DEV) {
            strscpy(new_pool->key.logic.dev_name, cfg->attr.logic_dev_config.dev_name, 
                        sizeof(new_pool->key.logic.dev_name));
            new_pool->key.logic.reg_addr = cfg->attr.logic_dev_config.reg_addr;
        }
        wb_cs_dev_lock_init(new_pool);
        list_add(&new_pool->list, &cs_lock_pool_list);
        pool = new_pool;
    }
    DEBUG_VERBOSE("lock user_count %d", pool->user_count);
    DEBUG_VERBOSE("cs_index: %d, cs_lock:addr 0x%p, lock_mode: %u",
        cfg->cs_index, wb_cs_dev_lock_addr(pool), pool->lock_mode);
    return wb_cs_dev_lock_addr(pool);
}

static void *cs_find_or_create_lock(wb_chip_select_cfg_info_t *cfg)
{
    void *lock_addr;
    mutex_lock(&cs_pool_lock);
    lock_addr = cs_find_or_create_lock_nolock(cfg);
    mutex_unlock(&cs_pool_lock);
    return lock_addr;
}

static void cs_release_lock(wb_chip_select_cfg_info_t *cfg)
{
    struct cs_lock_pool *pool;

    mutex_lock(&cs_pool_lock);
    pool = cs_find_lock_pool(cfg, &cs_lock_pool_list);
    if (pool) {
        if (pool->user_count > 0) {
            pool->user_count--;
            DEBUG_VERBOSE("lock user_count decreased to %d", pool->user_count);
        }

        if (pool->user_count == 0) {
            if (pool->op_type == CS_OP_TYPE_GPIO) {
                gpio_free(pool->key.gpio.gpio_offset);
            }
            list_del(&pool->list);
            kfree(pool);
            DEBUG_VERBOSE("lock freed and removed from pool");
        }
    } else {
        DEBUG_ERROR("Failed to find lock pool for cs_index: %d", cfg->cs_index);
    }
    mutex_unlock(&cs_pool_lock);
}

static int wb_logic_reg_write(cs_logic_dev_config_t *logic_dev_config, uint32_t pos, uint8_t *val, size_t size)
{
    device_func_write pfunc;

    pfunc = (device_func_write)logic_dev_config->write_intf_addr;
    return pfunc(logic_dev_config->dev_name, pos, val, size);
}

static int wb_logic_reg_read(cs_logic_dev_config_t *logic_dev_config, uint32_t pos, uint8_t *val, size_t size)
{
    device_func_read pfunc;

    pfunc = (device_func_read)logic_dev_config->read_intf_addr;
    return pfunc(logic_dev_config->dev_name, pos, val, size);
}

static int chip_select_write_logic_dev(cs_logic_dev_config_t *logic_dev_config, uint32_t value)
{
    int ret;
    uint8_t read_value[4], write_value[4];
    uint8_t tmp_read8;
    uint32_t tmp_read32, tmp_write32;

    mem_clear(read_value, sizeof(read_value));
    mem_clear(write_value, sizeof(write_value));

    ret = wb_logic_reg_read(logic_dev_config, logic_dev_config->reg_addr, read_value, logic_dev_config->width);
    if (ret < 0) {
        return ret;
    }

    if (logic_dev_config->width == WIDTH_1Byte) {
        tmp_read8 = read_value[0];
        write_value[0] = ((tmp_read8 & (~logic_dev_config->mask)) | value) & 0xFF;
        DEBUG_VERBOSE("1 byte write val[0]: 0x%x\n", write_value[0]);
    } else {
        memcpy((uint8_t *)&tmp_read32, read_value, logic_dev_config->width);
        tmp_write32 = (tmp_read32 & (~logic_dev_config->mask)) | value;
        memcpy(write_value, (uint8_t *)&tmp_write32, logic_dev_config->width);
        DEBUG_VERBOSE("4 byte write val[0]:0x%x, val[1]:0x%x, val[2]:0x%x, val[3]:0x%x",
            write_value[0], write_value[1], write_value[2], write_value[3]);
    }

    ret = wb_logic_reg_write(logic_dev_config, logic_dev_config->reg_addr, write_value, logic_dev_config->width);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

int chip_select_enable(uint32_t cs_index)
{
    int ret = 0;
    wb_chip_select_cfg_info_t *chip_select_cfg_info;

    if (cs_index >= MAX_CS_SIZE) {
        return -EINVAL;
    }
    
    chip_select_cfg_info = g_cs_info_arry[cs_index];
    if (!chip_select_cfg_info) {
        return -EINVAL;
    }

    DEBUG_VERBOSE("cs_lock:addr 0x%p", chip_select_cfg_info->cs_lock);
    wb_cs_dev_lock(chip_select_cfg_info, &chip_select_cfg_info->lock_flags);
    /* Perform enable operation based on type */
    DEBUG_INFO("op_type %d", chip_select_cfg_info->op_type);
    switch (chip_select_cfg_info->op_type) {
    case CS_OP_TYPE_GPIO:
        gpio_set_value(chip_select_cfg_info->attr.gpio_config.gpio_offset, 
                       chip_select_cfg_info->attr.gpio_config.en_level);
        DEBUG_VERBOSE("GPIO chip select enabled: gpio=%d, level=%d\n",
                chip_select_cfg_info->attr.gpio_config.gpio_offset, chip_select_cfg_info->attr.gpio_config.en_level);
        break;
    case CS_OP_TYPE_LOGIC_DEV:
        ret = chip_select_write_logic_dev(&chip_select_cfg_info->attr.logic_dev_config, chip_select_cfg_info->attr.logic_dev_config.en_value);
        if (ret) {
            DEBUG_ERROR("Failed to write enable value to logic device %s: %d\n", 
                    chip_select_cfg_info->attr.logic_dev_config.dev_name, ret);
            goto exit;
        }
        break;
    default:
        DEBUG_ERROR("Unsupported chip select operation type: %d\n", chip_select_cfg_info->op_type);
        ret = -EINVAL;
        goto exit;
    }
    sleep_by_lock_mode(chip_select_sleep, chip_select_cfg_info->lock_mode);

    return 0;
exit:
    wb_cs_dev_unlock(chip_select_cfg_info, &chip_select_cfg_info->lock_flags);
    return ret;
}
EXPORT_SYMBOL(chip_select_enable);


int chip_select_disable(uint32_t cs_index)
{
    int ret = 0;
    wb_chip_select_cfg_info_t *chip_select_cfg_info;

    if (cs_index >= MAX_CS_SIZE) {
        return -EINVAL;
    }

    chip_select_cfg_info = g_cs_info_arry[cs_index];
    if (!chip_select_cfg_info) {
        return -EINVAL;
    }

    switch (chip_select_cfg_info->op_type) {
    case CS_OP_TYPE_GPIO:
        gpio_set_value(chip_select_cfg_info->attr.gpio_config.gpio_offset, chip_select_cfg_info->attr.gpio_config.dis_level);
        break;
    case CS_OP_TYPE_LOGIC_DEV:
        ret = chip_select_write_logic_dev(&chip_select_cfg_info->attr.logic_dev_config, chip_select_cfg_info->attr.logic_dev_config.dis_value);
        if (ret) {
            DEBUG_ERROR("Failed to write disable value to logic device %s: %d\n", chip_select_cfg_info->attr.logic_dev_config.dev_name, ret);
            goto exit;
        }
        break;
    default:
        DEBUG_ERROR("Unsupported chip select operation type: %d\n", chip_select_cfg_info->op_type);
        ret = -EINVAL;
        goto exit;
    }
    sleep_by_lock_mode(chip_select_sleep, chip_select_cfg_info->lock_mode);

    wb_cs_dev_unlock(chip_select_cfg_info, &chip_select_cfg_info->lock_flags);
    return 0;
exit:
    wb_cs_dev_unlock(chip_select_cfg_info, &chip_select_cfg_info->lock_flags);
    return ret;
}
EXPORT_SYMBOL(chip_select_disable);

static int get_chip_select_gpio_offset(cs_gpio_config_t *gpio_config)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,12,0)
    struct gpio_device *gdev;
    struct gpio_chip *chip;

    gdev = gpio_device_find_by_label(gpio_config->controller_name);
    if (!gdev) {
        DEBUG_ERROR("Failed to find GPIO controller: %s\n", gpio_config->controller_name);
        return -ENODEV;
    }

    chip = gpio_device_get_chip(gdev);
    if (!chip) {
        gpio_device_put(gdev);
        DEBUG_ERROR("Failed to get GPIO chip for controller: %s\n", gpio_config->controller_name);
        return -ENODEV;
    }

    if (gpio_config->gpio_index >= chip->ngpio) {
        DEBUG_ERROR("GPIO index %u is out of range for controller %s\n",
                gpio_config->gpio_index, gpio_config->controller_name);
        gpio_device_put(gdev);
        return -EINVAL;
    }

    gpio_config->gpio_offset = gpio_device_get_base(gdev) + gpio_config->gpio_index;
    gpio_device_put(gdev);
    return 0;
#else
    struct gpio_chip *chip;

    chip = find_gpio_chip_by_name(gpio_config->controller_name);
    if (!chip) {
        DEBUG_ERROR("Failed to find GPIO controller: %s\n", gpio_config->controller_name);
        return -ENODEV;
    }
    if (gpio_config->gpio_index >= chip->ngpio) {
        DEBUG_ERROR("GPIO index %u is out of range for controller %s\n",
                gpio_config->gpio_index, gpio_config->controller_name);
        return -EINVAL;
    }
    gpio_config->gpio_offset = chip->base + gpio_config->gpio_index;
    return 0;
#endif
}

static int wb_chip_select_data_init(struct platform_device *pdev, wb_chip_select_cfg_info_t *data)
{
    int ret = 0;
    chip_select_device_t *chip_select_device;
    struct device *dev = &pdev->dev;

    if (pdev->dev.platform_data == NULL) {
        dev_err(dev, "Failed to get platform data config.\n");
        return -ENXIO;
    }

    chip_select_device = pdev->dev.platform_data;
    data->cs_index = chip_select_device->cs_index;
    data->op_type = chip_select_device->op_type;
    data->lock_mode = chip_select_device->lock_mode;
    if (data->lock_mode == 0) {
        data->lock_mode = WB_SPIN_LOCK_MODE;
    }
    if (!wb_cs_lock_mode_valid(data->lock_mode)) {
        dev_err(dev, "Invalid lock_mode: %u\n", data->lock_mode);
        return -EINVAL;
    }
    if (data->cs_index >= MAX_CS_SIZE) {
        dev_err(dev, "cs_index %d is out of range.\n", data->cs_index);
        return -EINVAL;
    }
    if (data->op_type == CS_OP_TYPE_GPIO) {
        data->attr.gpio_config = chip_select_device->attr.gpio_config;
        ret = get_chip_select_gpio_offset(&data->attr.gpio_config);
        if (ret) {
            dev_err(dev, "Failed to get GPIO offset: %d\n", ret);
            return ret;
        }
    } else if (data->op_type == CS_OP_TYPE_LOGIC_DEV) {
        data->attr.logic_dev_config = chip_select_device->attr.logic_dev_config;

        ret = find_intf_addr(&data->attr.logic_dev_config.write_intf_addr, &data->attr.logic_dev_config.read_intf_addr, data->attr.logic_dev_config.dev_mode);
        if (ret) {
            dev_err(dev, "find_intf_addr func mode %d fail, ret: %d.\n", data->attr.logic_dev_config.dev_mode, ret);
            return ret;
        }
        DEBUG_VERBOSE("find_intf_addr func mode %d success.\n", data->attr.logic_dev_config.dev_mode);
    } else {
        dev_err(dev, "Invalid chip select config type: %d\n", data->op_type);
        return -EINVAL;
    }

    return 0;
}

static int of_wb_chip_select_data_init(struct platform_device *pdev, wb_chip_select_cfg_info_t *data)
{
    int ret = 0;
    struct device *dev = &pdev->dev;

    ret += of_property_read_u32(pdev->dev.of_node, "cs_index", &data->cs_index);
    ret += of_property_read_u32(pdev->dev.of_node, "op_type", &data->op_type);
    if (ret) {
        dev_err(dev, "Failed to get cs_index or op_type property.\n");
        return ret;
    }
    if (data->cs_index >= MAX_CS_SIZE) {
        dev_err(dev, "cs_index %d is out of range.\n", data->cs_index);
        return -EINVAL;
    }
    if (of_property_read_u32(pdev->dev.of_node, "lock_mode", &data->lock_mode)) {
        data->lock_mode = WB_SPIN_LOCK_MODE;
    }
    if (!wb_cs_lock_mode_valid(data->lock_mode)) {
        dev_err(dev, "Invalid lock_mode: %u\n", data->lock_mode);
        return -EINVAL;
    }
    if (data->op_type == CS_OP_TYPE_GPIO) {
        ret += of_property_read_u32(pdev->dev.of_node, "gpio_index", &data->attr.gpio_config.gpio_index);
        ret += of_property_read_u32(pdev->dev.of_node, "en_level", &data->attr.gpio_config.en_level);
        ret += of_property_read_u32(pdev->dev.of_node, "dis_level", &data->attr.gpio_config.dis_level);
        ret += of_property_read_string(pdev->dev.of_node, "controller_name", &data->attr.gpio_config.controller_name);
        if (ret) {
            dev_err(dev, "Failed to get gpio_index, en_level, controller_name, or dis_level property.\n");
            return ret;
        }
        ret = get_chip_select_gpio_offset(&data->attr.gpio_config);
        if (ret) {
            dev_err(dev, "Failed to get GPIO offset: %d\n", ret);
            return ret;
        }
    } else if (data->op_type == CS_OP_TYPE_LOGIC_DEV) {
        ret += of_property_read_u32(pdev->dev.of_node, "dev_mode", &data->attr.logic_dev_config.dev_mode);
        ret += of_property_read_string(pdev->dev.of_node, "dev_name", &data->attr.logic_dev_config.dev_name);
        ret += of_property_read_u32(pdev->dev.of_node, "reg_addr", &data->attr.logic_dev_config.reg_addr);
        ret += of_property_read_u32(pdev->dev.of_node, "en_value", &data->attr.logic_dev_config.en_value);
        ret += of_property_read_u32(pdev->dev.of_node, "dis_value", &data->attr.logic_dev_config.dis_value);
        ret += of_property_read_u32(pdev->dev.of_node, "width", &data->attr.logic_dev_config.width);
        ret += of_property_read_u32(pdev->dev.of_node, "mask", &data->attr.logic_dev_config.mask);
        if (ret) {
            dev_err(dev, "Failed to get dis_value property.\n");
            return ret;
        }

        ret = find_intf_addr(&data->attr.logic_dev_config.write_intf_addr, &data->attr.logic_dev_config.read_intf_addr, data->attr.logic_dev_config.dev_mode);
        if (ret) {
            dev_err(dev, "find_intf_addr func mode %d fail, ret: %d.\n", data->attr.logic_dev_config.dev_mode, ret);
            return ret;
        }
        DEBUG_VERBOSE("find_intf_addr func mode %d success.\n", data->attr.logic_dev_config.dev_mode);
    } else {
        dev_err(dev, "Invalid chip select config type: %d\n", data->op_type);
        return -EINVAL;
    }

    return 0;
}

/**
 * wb_chip_select_probe - Platform driver probe function
 * @pdev: Platform device pointer
 *
 * Returns 0 on success, negative error code on failure.
 */
static int wb_chip_select_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    wb_chip_select_cfg_info_t *chip_select_cfg_info;
    int ret = 0;

    dev_info(dev, "Probing chip select platform device\n");
    chip_select_cfg_info = devm_kzalloc(dev, sizeof(wb_chip_select_cfg_info_t), GFP_KERNEL);
    if (!chip_select_cfg_info) {
        dev_err(dev, "Failed to allocate memory for chip select config.\n");
        return -ENOMEM;
    }

    if (pdev->dev.of_node) {
        ret = of_wb_chip_select_data_init(pdev, chip_select_cfg_info);
    } else {
        ret = wb_chip_select_data_init(pdev, chip_select_cfg_info);
    }
    if (ret) {
        dev_err(dev, "Failed to initialize chip select data.\n");
        return ret;
    }

    if(chip_select_cfg_info->op_type == CS_OP_TYPE_LOGIC_DEV) {
        ret = wb_logic_lock_mode_check(chip_select_cfg_info->lock_mode, chip_select_cfg_info->attr.logic_dev_config.dev_mode);
        if (ret != 0) {
            dev_err(dev, "Invalid lock mode %u for func mode %u\n", chip_select_cfg_info->lock_mode, chip_select_cfg_info->attr.logic_dev_config.dev_mode);
            return ret;
        }
    }

    platform_set_drvdata(pdev, chip_select_cfg_info);

    chip_select_cfg_info->cs_lock = cs_find_or_create_lock(chip_select_cfg_info);
    if (!chip_select_cfg_info->cs_lock) {
        dev_err(dev, "Failed to create lock for chip select index %d.\n", chip_select_cfg_info->cs_index);
        return -ENOMEM;
    }
    DEBUG_VERBOSE("cs_index: %d, cs_lock:addr 0x%p, lock_mode: %u",
        chip_select_cfg_info->cs_index, chip_select_cfg_info->cs_lock, chip_select_cfg_info->lock_mode);
    g_cs_info_arry[chip_select_cfg_info->cs_index] = chip_select_cfg_info;

    return 0;
}

/**
 * wb_chip_select_remove - Platform driver remove function
 * @pdev: Platform device pointer
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,11,0)
static void wb_chip_select_remove(struct platform_device *pdev)
#else
static int wb_chip_select_remove(struct platform_device *pdev)
#endif
{
    struct device *dev = &pdev->dev;
    wb_chip_select_cfg_info_t *chip_select_cfg_info;

    chip_select_cfg_info = platform_get_drvdata(pdev);

    dev_info(dev, "Removing chip select platform device\n");
    cs_release_lock(chip_select_cfg_info);
    g_cs_info_arry[chip_select_cfg_info->cs_index] = NULL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,11,0)
    return 0;
#endif
}

/* Device tree match table */
static const struct of_device_id wb_chip_select_of_match[] = {
    { .compatible = "wb_common_logic_mux", },
    {},
};
MODULE_DEVICE_TABLE(of, wb_chip_select_of_match);

/* Platform driver structure */
static struct platform_driver wb_chip_select_driver = {
    .driver = {
        .name = "wb_common_logic_mux",
        .of_match_table = wb_chip_select_of_match,
    },
    .probe = wb_chip_select_probe,
    .remove = wb_chip_select_remove,
};

/* Module initialization */
static int __init wb_chip_select_init(void)
{
    return platform_driver_register(&wb_chip_select_driver);
}

/* Module exit */
static void __exit wb_chip_select_exit(void)
{
    platform_driver_unregister(&wb_chip_select_driver);
}

module_init(wb_chip_select_init);
module_exit(wb_chip_select_exit);

MODULE_DESCRIPTION("Whitebox Chip Select Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Support");

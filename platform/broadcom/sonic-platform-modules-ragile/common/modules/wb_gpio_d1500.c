/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2011, 2012 Cavium Inc.
 */
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/spinlock.h>

#define GPIO_NAME           "wb_gpio_d1500"

#define GPIO_BASE           (0x500)
#define GP_IO_SEL           (GPIO_BASE + 0x4)
#define GP_LVL              (GPIO_BASE + 0xC)
#define GPI_NMI_EN          (GPIO_BASE + 0x28)
#define GPI_NMI_STS         (GPIO_BASE + 0x2a)
#define GPI_INV             (GPIO_BASE + 0x2c)
#define GPIO_USE_SEL2       (GPIO_BASE + 0x30)
#define GP_IO_SEL2          (GPIO_BASE + 0x34)
#define GP_LVL2             (GPIO_BASE + 0x38)
#define GPI_NMI_EN_2        (GPIO_BASE + 0x3c)
#define GPI_NMI_STS_2       (GPIO_BASE + 0x3e)
#define GPIO_USE_SEL3       (GPIO_BASE + 0x40)
#define GP_IO_SEL3          (GPIO_BASE + 0x44)
#define GP_LVL3             (GPIO_BASE + 0x48)
#define GPI_NMI_EN_3        (GPIO_BASE + 0x50)
#define GPI_NMI_STS_3       (GPIO_BASE + 0x54)

#define GPIO_BASE_ID        (0)
#define BANKSIZE            (32)
#define D1500_GPIO_PIN_NUM  (96)
#define CELL_NUM            (2)

int g_gpio_d1500_debug = 0;
int g_gpio_d1500_error = 0;
module_param(g_gpio_d1500_debug, int, S_IRUGO | S_IWUSR);
module_param(g_gpio_d1500_error, int, S_IRUGO | S_IWUSR);

#define GPIO_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_gpio_d1500_debug) { \
        printk(KERN_ERR "[GPIO-D1500][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define GPIO_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_gpio_d1500_error) { \
        printk(KERN_ERR "[GPIO-D1500][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static DEFINE_SPINLOCK(sio_lock);

struct gpio_d1500_t {
    struct gpio_chip chip;
    u64 register_base;
};

static int wb_gpio_get(struct gpio_chip *gc, unsigned gpio_num)
{
    u32 data = 0;
    unsigned int bank, offset;
    unsigned long flags;

    bank = gpio_num / BANKSIZE;
    offset = gpio_num % BANKSIZE;

    spin_lock_irqsave(&sio_lock, flags);
    if (bank == 0) {
        data = inl(GP_LVL) & (1 << offset);
        if (data) {
            data = 1;
        }
    } else if (bank == 1) {
        data = inl(GP_LVL2) & (1 << offset);
        if (data) {
            data = 1;
        }
    } else if (bank == 2) {
        data = inl(GP_LVL3) & (1 << offset);
        if (data) {
            data = 1;
        }
    }
    spin_unlock_irqrestore(&sio_lock, flags);

    return data;
}

static int wb_gpio_direction_in(struct gpio_chip *gc, unsigned gpio_num)
{
    u32 data;
    unsigned int bank, offset;
    unsigned long flags;

    bank = gpio_num / BANKSIZE;
    offset = gpio_num % BANKSIZE;

    spin_lock_irqsave(&sio_lock, flags);
    if (bank == 0) {
        data = inl(GP_IO_SEL);
        data = data | (1 << offset);
        outl(data, GP_IO_SEL);
    } else if (bank == 1) {
        data = inl(GP_IO_SEL2);
        data = data | (1 << offset);
        outl(data, GP_IO_SEL2);
    } else if (bank == 2) {
        data = inl(GP_IO_SEL3);
        data = data | (1 << offset);
        outl(data, GP_IO_SEL3);
    }
    spin_unlock_irqrestore(&sio_lock, flags);

    return 0;
}

static void wb_gpio_set(struct gpio_chip *gc,
                unsigned gpio_num, int val)
{
    u32 data;
    unsigned int bank, offset;
    unsigned long flags;

    bank = gpio_num / BANKSIZE;
    offset = gpio_num % BANKSIZE;

    spin_lock_irqsave(&sio_lock, flags);
    if (bank == 0) {
        data = inl(GP_LVL);
        if (val) {
            data = data | (1 << offset);
        } else {
            data = data & ~(1 << offset);
        }
        outl(data, GP_LVL);
    } else if (bank == 1) {
        data = inl(GP_LVL2);
        if (val) {
            data = data | (1 << offset);
        } else {
            data = data & ~(1 << offset);
        }
        outl(data, GP_LVL2);
    } else if (bank == 2) {
        data = inl(GP_LVL3);
        if (val) {
            data = data | (1 << offset);
        } else {
            data = data & ~(1 << offset);
        }
        outl(data, GP_LVL3);
    }
    spin_unlock_irqrestore(&sio_lock, flags);

    return;
}

static int wb_gpio_direction_out(struct gpio_chip *gc,
                    unsigned gpio_num, int val)
{
    u32 data;
    unsigned int bank, offset;
    unsigned long flags;

    bank = gpio_num / BANKSIZE;
    offset = gpio_num % BANKSIZE;

    spin_lock_irqsave(&sio_lock, flags);
    if (bank == 0) {
        data = inl(GP_IO_SEL);
        data = data & ~(1 << offset);
        outl(data, GP_IO_SEL);

        data = inl(GP_LVL);
        if (val) {
            data = data | (1 << offset);
        } else {
            data = data & ~(1 << offset);
        }
        outl(data, GP_LVL);
    } else if (bank == 1) {
        data = inl(GP_IO_SEL2);
        data = data & ~(1 << offset);
        outl(data, GP_IO_SEL2);

        data = inl(GP_LVL2);
        if (val) {
            data = data | (1 << offset);
        } else {
            data = data & ~(1 << offset);
        }
        outl(data, GP_LVL2);
    } else if (bank == 2) {
        data = inl(GP_IO_SEL3);
        data = data & ~(1 << offset);
        outl(data, GP_IO_SEL3);

        data = inl(GP_LVL3);
        if (val) {
            data = data | (1 << offset);
        } else {
            data = data & ~(1 << offset);
        }
        outl(data, GP_LVL3);
    }
    spin_unlock_irqrestore(&sio_lock, flags);

    return 0;
}

#ifdef CONFIG_OF
static int wb_gpio_of_xlate(struct gpio_chip *chip,
                              const struct of_phandle_args *gpio_desc,
                              u32 *flags)
{
    if (chip->of_gpio_n_cells < 2) {
        return -EINVAL;
    }

    if (flags) {
        *flags = gpio_desc->args[1];
    }

    return gpio_desc->args[0];
}
#endif

static int wb_gpio_request(struct gpio_chip *chip, unsigned int offset)
{
    u32 data;
    unsigned int bank, tmp_offset;
    unsigned long flags;

    bank = offset / BANKSIZE;
    tmp_offset = offset % BANKSIZE;

    spin_lock_irqsave(&sio_lock, flags);
    if (bank == 0) {
        data = inl(GPIO_BASE);
        data = data | (1 << tmp_offset);
        outl(data, GPIO_BASE);
    } else if (bank == 1) {
        data = inl(GPIO_USE_SEL2);
        data = data | (1 << tmp_offset);
        outl(data, GPIO_USE_SEL2);
    } else if (bank == 2) {
        data = inl(GPIO_USE_SEL3);
        data = data | (1 << tmp_offset);
        outl(data, GPIO_USE_SEL3);
    }
    spin_unlock_irqrestore(&sio_lock, flags);

    return 0;
}

#if 0
static void wb_gpio_free(struct gpio_chip *chip, unsigned int offset)
{
    u32 data;
    unsigned int bank, tmp_offset;
    unsigned long flags;

    bank = offset / BANKSIZE;
    tmp_offset = offset % BANKSIZE;

    spin_lock_irqsave(&sio_lock, flags);
    if (bank == 0) {
        data = inl(GPIO_BASE);
        data = data & ~(1 << tmp_offset);
        outl(data, GPIO_BASE);
    } else if (bank == 1) {
        data = inl(GPIO_USE_SEL2);
        data = data & ~(1 << tmp_offset);
        outl(data, GPIO_USE_SEL2);
    } else if (bank == 2) {
        data = inl(GPIO_USE_SEL3);
        data = data & ~(1 << tmp_offset);
        outl(data, GPIO_USE_SEL3);
    }

    spin_unlock_irqrestore(&sio_lock, flags);

    return;
}
#endif

static struct gpio_chip wb_gpio_chip = {
    .label              = GPIO_NAME,
    .owner              = THIS_MODULE,
    .base               = GPIO_BASE_ID,
    .get                = wb_gpio_get,
    .direction_input    = wb_gpio_direction_in,
    .set                = wb_gpio_set,
    .direction_output   = wb_gpio_direction_out,
#ifdef CONFIG_OF
    .of_xlate           = wb_gpio_of_xlate,
#endif
    .request            = wb_gpio_request,
    .ngpio              = D1500_GPIO_PIN_NUM,
#ifdef CONFIG_OF
    .of_gpio_n_cells    = CELL_NUM,
#endif
    .can_sleep          = false,
};

static int wb_gpio_probe(struct platform_device *pdev)
{
    struct gpio_d1500_t *gpio;
    int err;

    gpio = devm_kzalloc(&pdev->dev, sizeof(*gpio), GFP_KERNEL);
    if (!gpio) {
        dev_err(&pdev->dev, "gpio kzalloc failed\n");
        return -ENOMEM;
    }

    wb_gpio_chip.parent = &pdev->dev;
    gpio->register_base = GPIO_BASE;
    gpio->chip = wb_gpio_chip;
    pdev->dev.platform_data = &wb_gpio_chip;
    err = devm_gpiochip_add_data(&pdev->dev, &wb_gpio_chip, gpio);
    if (err) {
        dev_err(&pdev->dev, "gpiochip add failed\n");
        return err;
    }

    dev_info(&pdev->dev, "register %llu gpio success.\n", gpio->register_base);

    return 0;
}

static int wb_gpio_remove(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "unregister d1500 gpio success\n");
    return 0;
}

static const struct of_device_id gpio_d1500_match[] = {
    {
        .compatible = "wb_gpio_d1500",
    },
    {},
};
MODULE_DEVICE_TABLE(of, gpio_d1500_match);

static struct platform_driver wb_gpio_driver = {
    .driver = {
        .name        = GPIO_NAME,
        .of_match_table = gpio_d1500_match,
    },
    .probe      = wb_gpio_probe,
    .remove     = wb_gpio_remove,
};

module_platform_driver(wb_gpio_driver);

MODULE_DESCRIPTION("d1500 gpio driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");

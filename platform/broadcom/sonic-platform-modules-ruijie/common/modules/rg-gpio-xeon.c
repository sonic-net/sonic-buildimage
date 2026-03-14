/*
 *  GPIO interface for XEON Super I/O chip
 *
 *  Author: sonic_rd <sonic_rd@ruijie.com.cn>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License 2 as published
 *  by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/platform_data/i2c-gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <asm/delay.h>
#include <linux/miscdevice.h>
#include <linux/gpio/machine.h>

#define GPIO_NAME           "xeon-gpio"
#define GPIO_IOSIZE         7
#define GPIO_BASE           0x500

#define GPIO_USE_SEL        GPIO_BASE
#define GP_IO_SEL           (GPIO_BASE+0x4)
#define GP_LVL              (GPIO_BASE+0xC)

#define GPIO_USE_SEL2       (GPIO_BASE+0x30)
#define GP_IO_SEL2          (GPIO_BASE+0x34)
#define GP_LVL2             (GPIO_BASE+0x38)

#define GPIO_USE_SEL3       (GPIO_BASE+0x40)
#define GP_IO_SEL3          (GPIO_BASE+0x44)
#define GP_LVL3             (GPIO_BASE+0x48)


#define GPIO_BASE_ID        0
#define BANKSIZE            32

#define GPIO_SDA            17
#define GPIO_SCL            1

#define GPIO_XEON_SPIN_LOCK(lock, flags) spin_lock_irqsave(&(lock), (flags))
#define GPIO_XEON_SPIN_UNLOCK(lock, flags) spin_unlock_irqrestore(&(lock), (flags))
static DEFINE_SPINLOCK(sio_lock);

/****************** i2c adapter with gpio ***********************/

static struct i2c_gpio_platform_data i2c_pdata = {
    .timeout = 200,
    .udelay = 10,
    .scl_is_output_only = 0,
    .sda_is_open_drain  = 0,
    .scl_is_open_drain = 0,
};

static struct gpiod_lookup_table rg_gpio_lookup_table = {
	.dev_id = "i2c-gpio",
	.table = {
		GPIO_LOOKUP(GPIO_NAME, GPIO_SDA, "sda",
				GPIO_ACTIVE_HIGH),
		GPIO_LOOKUP(GPIO_NAME, GPIO_SCL, "scl",
				GPIO_ACTIVE_HIGH),
	},
};

static void i2c_gpio_release(struct device *dev)
{
    return;
}

static struct platform_device i2c_gpio = {
    .name           = "i2c-gpio",
    .num_resources  = 0,
    .id             = -1,

    .dev = {
        .platform_data = &i2c_pdata,
        .release = i2c_gpio_release,
    }
};

static int xeon_gpio_get(struct gpio_chip *gc, unsigned gpio_num)
{
    unsigned int data;
    unsigned int bank, offset;
    unsigned long flags;

	data = 0;
    bank = gpio_num / BANKSIZE;
    offset = gpio_num % BANKSIZE;

    GPIO_XEON_SPIN_LOCK(sio_lock, flags);
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
    GPIO_XEON_SPIN_UNLOCK(sio_lock, flags);

    return data;
}

static int xeon_gpio_direction_in(struct gpio_chip *gc, unsigned gpio_num)
{
    unsigned int data;
    unsigned int bank, offset;
    unsigned long flags;

    bank = gpio_num / BANKSIZE;
    offset = gpio_num % BANKSIZE;

    GPIO_XEON_SPIN_LOCK(sio_lock, flags);
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
    GPIO_XEON_SPIN_UNLOCK(sio_lock, flags);

    return 0;
}

static void xeon_gpio_set(struct gpio_chip *gc,
                unsigned gpio_num, int val)
{
    unsigned int data;
    unsigned int bank, offset;
    unsigned long flags;

    bank = gpio_num / BANKSIZE;
    offset = gpio_num % BANKSIZE;

    GPIO_XEON_SPIN_LOCK(sio_lock, flags);
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
    GPIO_XEON_SPIN_UNLOCK(sio_lock, flags);
}

static int xeon_gpio_direction_out(struct gpio_chip *gc,
                    unsigned gpio_num, int val)
{
    unsigned int data;
    unsigned int bank, offset;
    unsigned long flags;

    bank = gpio_num / BANKSIZE;
    offset = gpio_num % BANKSIZE;

    GPIO_XEON_SPIN_LOCK(sio_lock, flags);
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
    GPIO_XEON_SPIN_UNLOCK(sio_lock, flags);

    return 0;
}
                    
static int xeon_gpio_request(struct gpio_chip *chip, unsigned int offset)
{
    unsigned int data;
    unsigned int bank, tmp_offset;
    unsigned long flags;

    bank = offset / BANKSIZE;
    tmp_offset = offset % BANKSIZE;
    
    GPIO_XEON_SPIN_LOCK(sio_lock, flags);
    if (bank == 0) {
        data = inl(GPIO_USE_SEL);
        data = data | (1 << tmp_offset);
        outl(data, GPIO_USE_SEL);
    } else if (bank == 1) {
        data = inl(GPIO_USE_SEL2);
        data = data | (1 << tmp_offset);
        outl(data, GPIO_USE_SEL2);
    } else if (bank == 2) {
        data = inl(GPIO_USE_SEL3);
        data = data | (1 << tmp_offset);
        outl(data, GPIO_USE_SEL3);
    }
    GPIO_XEON_SPIN_UNLOCK(sio_lock, flags);
    return 0;
}

static void xeon_gpio_free(struct gpio_chip *chip, unsigned int offset)
{
    unsigned int data;
    unsigned int bank, tmp_offset;
    unsigned long flags;

    bank = offset / BANKSIZE;
    tmp_offset = offset % BANKSIZE;  

    GPIO_XEON_SPIN_LOCK(sio_lock, flags);
    if (bank == 0) {
        data = inl(GPIO_USE_SEL);
        data = data & ~(1 << tmp_offset);
        outl(data, GPIO_USE_SEL);
    } else if (bank == 1) {
        data = inl(GPIO_USE_SEL2);
        data = data & ~(1 << tmp_offset);
        outl(data, GPIO_USE_SEL2);
    } else if (bank == 2) {
        data = inl(GPIO_USE_SEL3);
        data = data & ~(1 << tmp_offset);
        outl(data, GPIO_USE_SEL3);
    }
    GPIO_XEON_SPIN_UNLOCK(sio_lock, flags);
}

static struct gpio_chip xeon_gpio_chip = {
    .label          = GPIO_NAME,
    .owner          = THIS_MODULE,
    .get            = xeon_gpio_get,
    .direction_input    = xeon_gpio_direction_in,
    .set            = xeon_gpio_set,
    .direction_output   = xeon_gpio_direction_out,
    .request   = xeon_gpio_request,
    .free      = xeon_gpio_free,
};

static int __init xeon_gpio_init(void)
{
    int err;
    if (!request_region(GPIO_BASE, GPIO_IOSIZE, GPIO_NAME))
        return -EBUSY;

    xeon_gpio_chip.base = GPIO_BASE_ID;
    xeon_gpio_chip.ngpio = 96;

    err = gpiochip_add_data(&xeon_gpio_chip, NULL);
    if (err < 0)
        goto gpiochip_add_err;
    gpiod_add_lookup_table(&rg_gpio_lookup_table);
    err = platform_device_register(&i2c_gpio);
    if (err < 0) {
        goto i2c_get_adapter_err;
    }
   	return 0;

i2c_get_adapter_err:
    gpiod_remove_lookup_table(&rg_gpio_lookup_table);
    platform_device_unregister(&i2c_gpio);
    gpiochip_remove(&xeon_gpio_chip);

gpiochip_add_err:
    release_region(GPIO_BASE, GPIO_IOSIZE);
    return -1;
}

static void __exit xeon_gpio_exit(void)
{
    gpiod_remove_lookup_table(&rg_gpio_lookup_table);
    platform_device_unregister(&i2c_gpio);
    mdelay(100);
    gpiochip_remove(&xeon_gpio_chip);
    release_region(GPIO_BASE, GPIO_IOSIZE);
}

module_init(xeon_gpio_init);
module_exit(xeon_gpio_exit);

MODULE_AUTHOR("sonic_rd <sonic_rd@ruijie.com.cn>");
MODULE_DESCRIPTION("GPIO interface for XEON Super I/O chip");
MODULE_LICENSE("GPL");

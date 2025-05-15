// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A LED driver for the es9618xx_led
 *
 * Copyright (C) 2016 Accton Technology Corporation.
 * Brandon Chuang <brandon_chuang@accton.com.tw>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/leds.h>
#include <linux/slab.h>

#define DRVNAME "es9618xx_led"

#define DEBUG_MODE 0

#if (DEBUG_MODE == 1)
    #define DEBUG_PRINT(fmt, args...)                                         \
        printk (KERN_INFO "%s:%s[%d]: " fmt "\r\n", __FILE__, __FUNCTION__, __LINE__, ##args)
#else
    #define DEBUG_PRINT(fmt, args...)
#endif

extern int es9618xx_cpld_read(unsigned short cpld_addr, u8 reg);
extern int es9618xx_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

struct es9618xx_led_data {
    struct platform_device *pdev;
    struct mutex    update_lock;
    char            valid;          /* != 0 if registers are valid */
    unsigned long    last_updated;  /* In jiffies */
    u8                reg_val[2];   /* Register value, 0 = FAN/PSU LED,
						                               1 = DIAG LED
                                                        */
};

static struct es9618xx_led_data  *ledctl = NULL;

#define LED_CNTRLER_I2C_ADDRESS          (0x61)

#define LED_TYPE_DIAG_REG_MASK           (0x07)
#define LED_MODE_DIAG_OFF_VALUE          (0x00)
#define LED_MODE_DIAG_GREEN_VALUE        (0x01)
#define LED_MODE_DIAG_AMBER_VALUE        (0x02)
#define LED_MODE_DIAG_GREEN_BLINK_VALUE  (0x05)
#define LED_MODE_DIAG_AMBER_BLINK_VALUE  (0x06)

#define LED_TYPE_LOC_REG_MASK            (0xC0)
#define LED_MODE_LOC_OFF_VALUE           (0x00)
#define LED_MODE_LOC_AMBER_VALUE         (0x40)
#define LED_MODE_LOC_AMBER_BLINK_VALUE   (0xC0)

#define LED_TYPE_PSU1_REG_MASK           (0x03)
#define LED_MODE_PSU1_OFF_VALUE          (0x00)
#define LED_MODE_PSU1_GREEN_VALUE        (0x01)
#define LED_MODE_PSU1_AMBER_VALUE        (0x02)

#define LED_TYPE_PSU2_REG_MASK           (0x0C)
#define LED_MODE_PSU2_OFF_VALUE          (0x00)
#define LED_MODE_PSU2_GREEN_VALUE        (0x04)
#define LED_MODE_PSU2_AMBER_VALUE        (0x08)

#define LED_TYPE_FAN_REG_MASK            (0x30)
#define LED_MODE_FAN_OFF_VALUE           (0x00)
#define LED_MODE_FAN_GREEN_VALUE         (0x10)
#define LED_MODE_FAN_AMBER_VALUE         (0x20)

static const u8 led_reg[] = {
    0x30,    /* LOC LED */
    0x31,    /* DIAG LED */
};

enum led_type {
    LED_TYPE_DIAG,
    LED_TYPE_LOC,
    LED_TYPE_FAN,
    LED_TYPE_PSU1,
    LED_TYPE_PSU2
};

/* FAN/PSU/DIAG/RELEASE led mode */
enum led_light_mode {
    LED_MODE_OFF = 0,
    LED_MODE_GREEN,
    LED_MODE_GREEN_BLINK,
    LED_MODE_AMBER,
    LED_MODE_AMBER_BLINK,
    LED_MODE_RED,
    LED_MODE_RED_BLINK,
    LED_MODE_BLUE,
    LED_MODE_BLUE_BLINK,
    LED_MODE_AUTO,
    LED_MODE_UNKNOWN
};

struct led_type_mode {
    enum led_type type;
    enum led_light_mode mode;
    int  type_mask;
    int  mode_value;
};

static struct led_type_mode led_type_mode_data[] = {
{LED_TYPE_LOC,  LED_MODE_OFF,         LED_TYPE_LOC_REG_MASK,  LED_MODE_LOC_OFF_VALUE},
{LED_TYPE_LOC,  LED_MODE_AMBER,       LED_TYPE_LOC_REG_MASK,  LED_MODE_LOC_AMBER_VALUE},
{LED_TYPE_LOC,  LED_MODE_AMBER_BLINK, LED_TYPE_LOC_REG_MASK,  LED_MODE_LOC_AMBER_BLINK_VALUE},
{LED_TYPE_DIAG, LED_MODE_GREEN_BLINK, LED_TYPE_DIAG_REG_MASK, LED_MODE_DIAG_GREEN_BLINK_VALUE},
{LED_TYPE_DIAG, LED_MODE_GREEN,       LED_TYPE_DIAG_REG_MASK, LED_MODE_DIAG_GREEN_VALUE},
{LED_TYPE_DIAG, LED_MODE_AMBER,       LED_TYPE_DIAG_REG_MASK, LED_MODE_DIAG_AMBER_VALUE},
{LED_TYPE_DIAG, LED_MODE_AMBER_BLINK, LED_TYPE_DIAG_REG_MASK, LED_MODE_DIAG_AMBER_BLINK_VALUE},
{LED_TYPE_DIAG, LED_MODE_OFF,         LED_TYPE_DIAG_REG_MASK, LED_MODE_DIAG_OFF_VALUE},
{LED_TYPE_FAN,  LED_MODE_GREEN,       LED_TYPE_FAN_REG_MASK,  LED_MODE_FAN_GREEN_VALUE},
{LED_TYPE_FAN,  LED_MODE_AMBER,       LED_TYPE_FAN_REG_MASK,  LED_MODE_FAN_AMBER_VALUE},
{LED_TYPE_FAN,  LED_MODE_OFF,         LED_TYPE_FAN_REG_MASK,  LED_MODE_FAN_OFF_VALUE},
{LED_TYPE_PSU1, LED_MODE_GREEN,       LED_TYPE_PSU1_REG_MASK, LED_MODE_PSU1_GREEN_VALUE},
{LED_TYPE_PSU1, LED_MODE_AMBER,       LED_TYPE_PSU1_REG_MASK, LED_MODE_PSU1_AMBER_VALUE},
{LED_TYPE_PSU1, LED_MODE_OFF,         LED_TYPE_PSU1_REG_MASK, LED_MODE_PSU1_OFF_VALUE},
{LED_TYPE_PSU2, LED_MODE_GREEN,       LED_TYPE_PSU2_REG_MASK, LED_MODE_PSU2_GREEN_VALUE},
{LED_TYPE_PSU2, LED_MODE_AMBER,       LED_TYPE_PSU2_REG_MASK, LED_MODE_PSU2_AMBER_VALUE},
{LED_TYPE_PSU2, LED_MODE_OFF,         LED_TYPE_PSU2_REG_MASK, LED_MODE_PSU2_OFF_VALUE},
};

static int led_reg_val_to_light_mode(enum led_type type, u8 reg_val) {
    int i;

    for (i = 0; i < ARRAY_SIZE(led_type_mode_data); i++) {
        if (type != led_type_mode_data[i].type) {
            continue;
        }

        if ((led_type_mode_data[i].type_mask & reg_val) ==
             led_type_mode_data[i].mode_value) {
            return led_type_mode_data[i].mode;
        }
    }

    return LED_MODE_UNKNOWN;
}

static u8 led_light_mode_to_reg_val(enum led_type type,
                                    enum led_light_mode mode, u8 reg_val) {
    int i;

    for (i = 0; i < ARRAY_SIZE(led_type_mode_data); i++) {
        int type_mask, mode_value;

        if (type != led_type_mode_data[i].type)
            continue;

        if (mode != led_type_mode_data[i].mode)
            continue;

        type_mask  = led_type_mode_data[i].type_mask;
        mode_value = led_type_mode_data[i].mode_value;
        reg_val = (reg_val & ~type_mask) | mode_value;
    }

    return reg_val;
}

static int es9618xx_led_read_value(u8 reg)
{
    return es9618xx_cpld_read(LED_CNTRLER_I2C_ADDRESS, reg);
}

static int es9618xx_led_write_value(u8 reg, u8 value)
{
    return es9618xx_cpld_write(LED_CNTRLER_I2C_ADDRESS, reg, value);
}

static void es9618xx_led_update(void)
{
    mutex_lock(&ledctl->update_lock);

    if (time_after(jiffies, ledctl->last_updated + HZ + HZ / 2)
        || !ledctl->valid) {
        int i;

        dev_dbg(&ledctl->pdev->dev, "Starting es9618xx_led update\n");
        ledctl->valid = 0;

        /* Update LED data
         */
        for (i = 0; i < ARRAY_SIZE(ledctl->reg_val); i++) {
            int status = es9618xx_led_read_value(led_reg[i]);

            if (status < 0) {
                dev_dbg(&ledctl->pdev->dev, "reg %d, err %d\n", led_reg[i], status);
                goto exit;
            }
            else
                ledctl->reg_val[i] = status;
        }

        ledctl->last_updated = jiffies;
        ledctl->valid = 1;
    }

exit:
    mutex_unlock(&ledctl->update_lock);
}

static void es9618xx_led_set(struct led_classdev *led_cdev,
                                      enum led_brightness led_light_mode,
                                      u8 reg, enum led_type type)
{
    int reg_val;

    mutex_lock(&ledctl->update_lock);
    reg_val = es9618xx_led_read_value(reg);

    if (reg_val < 0) {
        dev_dbg(&ledctl->pdev->dev, "reg %d, err %d\n", reg, reg_val);
        goto exit;
    }

    reg_val = led_light_mode_to_reg_val(type, led_light_mode, reg_val);
    es9618xx_led_write_value(reg, reg_val);
    ledctl->valid = 0;

exit:
    mutex_unlock(&ledctl->update_lock);
}

static void es9618xx_led_fan_set(struct led_classdev *led_cdev,
                                           enum led_brightness led_light_mode)
{
    es9618xx_led_set(led_cdev, led_light_mode, led_reg[0], LED_TYPE_FAN);
}

static enum led_brightness es9618xx_led_fan_get(struct led_classdev *cdev)
{
    es9618xx_led_update();
    return led_reg_val_to_light_mode(LED_TYPE_FAN, ledctl->reg_val[0]);
}

static void es9618xx_led_psu1_set(struct led_classdev *led_cdev,
                                           enum led_brightness led_light_mode)
{
    es9618xx_led_set(led_cdev, led_light_mode, led_reg[0], LED_TYPE_PSU1);
}

static enum led_brightness es9618xx_led_psu1_get(struct led_classdev *cdev)
{
    es9618xx_led_update();
    return led_reg_val_to_light_mode(LED_TYPE_PSU1, ledctl->reg_val[0]);
}

static void es9618xx_led_psu2_set(struct led_classdev *led_cdev,
                                           enum led_brightness led_light_mode)
{
    es9618xx_led_set(led_cdev, led_light_mode, led_reg[0], LED_TYPE_PSU2);
}

static enum led_brightness es9618xx_led_psu2_get(struct led_classdev *cdev)
{
    es9618xx_led_update();
    return led_reg_val_to_light_mode(LED_TYPE_PSU2, ledctl->reg_val[0]);
}

static void es9618xx_led_diag_set(struct led_classdev *led_cdev,
                                           enum led_brightness led_light_mode)
{
    es9618xx_led_set(led_cdev, led_light_mode, led_reg[1], LED_TYPE_DIAG);
}

static enum led_brightness es9618xx_led_diag_get(struct led_classdev *cdev)
{
    es9618xx_led_update();
    return led_reg_val_to_light_mode(LED_TYPE_DIAG, ledctl->reg_val[1]);
}

static enum led_brightness es9618xx_led_loc_get(struct led_classdev *cdev)
{
    es9618xx_led_update();
    return led_reg_val_to_light_mode(LED_TYPE_LOC, ledctl->reg_val[0]);
}

static void es9618xx_led_loc_set(struct led_classdev *led_cdev,
                                           enum led_brightness led_light_mode)
{
    es9618xx_led_set(led_cdev, led_light_mode, led_reg[0], LED_TYPE_LOC);
}

static struct led_classdev es9618xx_leds[] = {
    [LED_TYPE_LOC] = {
        .name             = "es9618xx_led::loc",
        .default_trigger = "unused",
        .brightness_set     = es9618xx_led_loc_set,
        .brightness_get  = es9618xx_led_loc_get,
        .max_brightness  = LED_MODE_AMBER_BLINK,
    },
    [LED_TYPE_DIAG] = {
        .name             = "es9618xx_led::diag",
        .default_trigger = "unused",
        .brightness_set     = es9618xx_led_diag_set,
        .brightness_get  = es9618xx_led_diag_get,
        .max_brightness  = LED_MODE_AMBER_BLINK,
    },
    [LED_TYPE_PSU1] = {
        .name             = "es9618xx_led::psu1",
        .default_trigger = "unused",
        .brightness_set     = es9618xx_led_psu1_set,
        .brightness_get  = es9618xx_led_psu1_get,
        .max_brightness  = LED_MODE_AMBER,
    },
    [LED_TYPE_PSU2] = {
        .name             = "es9618xx_led::psu2",
        .default_trigger = "unused",
        .brightness_set     = es9618xx_led_psu2_set,
        .brightness_get  = es9618xx_led_psu2_get,
        .max_brightness  = LED_MODE_AMBER,
    },
    [LED_TYPE_FAN] = {
        .name             = "es9618xx_led::fan",
        .default_trigger = "unused",
        .brightness_set     = es9618xx_led_fan_set,
        .brightness_get  = es9618xx_led_fan_get,
        .max_brightness  = LED_MODE_AMBER,
    },
};

static int es9618xx_led_probe(struct platform_device *pdev)
{
    int ret, i;

    for (i = 0; i < ARRAY_SIZE(es9618xx_leds); i++) {
        ret = led_classdev_register(&pdev->dev, &es9618xx_leds[i]);

        if (ret < 0) {
            break;
        }
    }

    /* Check if all LEDs were successfully registered */
    if (i != ARRAY_SIZE(es9618xx_leds)){
        int j;

        /* only unregister the LEDs that were successfully registered */
        for (j = 0; j < i; j++) {
            led_classdev_unregister(&es9618xx_leds[i]);
        }
    }

    return ret;
}

static int es9618xx_led_remove(struct platform_device *pdev)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(es9618xx_leds); i++) {
        led_classdev_unregister(&es9618xx_leds[i]);
    }

    return 0;
}

static struct platform_driver es9618xx_led_driver = {
    .probe    = es9618xx_led_probe,
    .remove    = es9618xx_led_remove,
    .driver = {
        .name   = DRVNAME,
        .owner  = THIS_MODULE,
    },
};

static int __init es9618xx_led_init(void)
{
    int ret;

    ret = platform_driver_register(&es9618xx_led_driver);
    if (ret < 0) {
        goto exit;
    }

    ledctl = kzalloc(sizeof(struct es9618xx_led_data), GFP_KERNEL);
    if (!ledctl) {
        ret = -ENOMEM;
        goto exit_driver;
    }

    mutex_init(&ledctl->update_lock);

    ledctl->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
    if (IS_ERR(ledctl->pdev)) {
        ret = PTR_ERR(ledctl->pdev);
        goto exit_free;
    }

    return 0;

exit_free:
    kfree(ledctl);
exit_driver:
    platform_driver_unregister(&es9618xx_led_driver);
exit:
    return ret;
}

static void __exit es9618xx_led_exit(void)
{
    platform_device_unregister(ledctl->pdev);
    platform_driver_unregister(&es9618xx_led_driver);
    kfree(ledctl);
}

late_initcall(es9618xx_led_init);
module_exit(es9618xx_led_exit);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("es9618xx_led driver");
MODULE_LICENSE("GPL");

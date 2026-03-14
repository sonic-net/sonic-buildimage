/*
 * A LED driver for the accton_as5812_54x_led
 *
 * Copyright (C) 2013 Accton Technology Corporation.
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
#if 0
#define DEBUG
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/leds.h>
#include <linux/slab.h>

extern int as5812_54x_cpld_read (unsigned short cpld_addr, u8 reg);
extern int as5812_54x_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

extern void led_classdev_unregister(struct led_classdev *led_cdev);
extern void led_classdev_resume(struct led_classdev *led_cdev);
extern void led_classdev_suspend(struct led_classdev *led_cdev);

#define DRVNAME "as5812_54x_led"

struct accton_as5812_54x_led_data {
    struct platform_device *pdev;
    struct mutex     update_lock;
    char             valid;           /* != 0 if registers are valid */
    unsigned long    last_updated;    /* In jiffies */
    u8               reg_val[4];     /* Register value, 0 = LOC/DIAG/FAN LED
                                                        1 = PSU1/PSU2 LED
                                                        2 = FAN1-4 LED
                                                        3 = FAN5-6 LED */
};

static struct accton_as5812_54x_led_data  *ledctl = NULL;

/* LED related data
 */
#define LED_TYPE_PSU1_REG_MASK   0x03
#define LED_MODE_PSU1_GREEN_MASK 0x02
#define LED_MODE_PSU1_AMBER_MASK 0x01
#define LED_MODE_PSU1_OFF_MASK   0x03
#define LED_MODE_PSU1_AUTO_MASK  0x00

#define LED_TYPE_PSU2_REG_MASK   0x0C
#define LED_MODE_PSU2_GREEN_MASK 0x08
#define LED_MODE_PSU2_AMBER_MASK 0x04
#define LED_MODE_PSU2_OFF_MASK   0x0C
#define LED_MODE_PSU2_AUTO_MASK  0x00

#define LED_TYPE_DIAG_REG_MASK    0x0C
#define LED_MODE_DIAG_GREEN_MASK  0x08
#define LED_MODE_DIAG_AMBER_MASK  0x04
#define LED_MODE_DIAG_OFF_MASK    0x0C

#define LED_TYPE_FAN_REG_MASK     0x03
#define LED_MODE_FAN_GREEN_MASK   0x02
#define LED_MODE_FAN_AMBER_MASK   0x01
#define LED_MODE_FAN_OFF_MASK     0x03
#define LED_MODE_FAN_AUTO_MASK    0x00

#define LED_TYPE_FAN1_REG_MASK    0x03
#define LED_TYPE_FAN2_REG_MASK    0x0C
#define LED_TYPE_FAN3_REG_MASK    0x30
#define LED_TYPE_FAN4_REG_MASK    0xC0
#define LED_TYPE_FAN5_REG_MASK    0x03
#define LED_TYPE_FAN6_REG_MASK    0x0C

#define LED_MODE_FANX_GREEN_MASK   0x01
#define LED_MODE_FANX_RED_MASK     0x02
#define LED_MODE_FANX_OFF_MASK     0x00

#define LED_TYPE_LOC_REG_MASK     0x30
#define LED_MODE_LOC_ON_MASK      0x00
#define LED_MODE_LOC_OFF_MASK     0x10
#define LED_MODE_LOC_BLINK_MASK   0x20

static const u8 led_reg[] = {
    0xA,        /* LOC/DIAG/FAN LED*/
    0xB,        /* PSU1/PSU2 LED */
    0x16,      /* FAN1-4 LED */
    0x17,      /* FAN4-6 LED */
};

enum led_type {
    LED_TYPE_PSU1,
    LED_TYPE_PSU2,
    LED_TYPE_DIAG,
    LED_TYPE_FAN,
    LED_TYPE_FAN1,
    LED_TYPE_FAN2,
    LED_TYPE_FAN3,
    LED_TYPE_FAN4,
    LED_TYPE_FAN5,
    LED_TYPE_LOC
};

enum led_light_mode {
    LED_MODE_OFF = 0,
    LED_MODE_GREEN,
    LED_MODE_AMBER,
    LED_MODE_RED,
    LED_MODE_GREEN_BLINK,
    LED_MODE_AMBER_BLINK,
    LED_MODE_RED_BLINK,
    LED_MODE_AUTO,
};

struct led_type_mode {
    enum led_type type;
    int  type_mask;
    enum led_light_mode mode;
    int  mode_mask;
};

static struct led_type_mode led_type_mode_data[] = {
{LED_TYPE_PSU1, LED_TYPE_PSU1_REG_MASK, LED_MODE_GREEN, LED_MODE_PSU1_GREEN_MASK},
{LED_TYPE_PSU1, LED_TYPE_PSU1_REG_MASK, LED_MODE_AMBER, LED_MODE_PSU1_AMBER_MASK},
{LED_TYPE_PSU1, LED_TYPE_PSU1_REG_MASK, LED_MODE_AUTO,  LED_MODE_PSU1_AUTO_MASK},
{LED_TYPE_PSU1, LED_TYPE_PSU1_REG_MASK, LED_MODE_OFF,   LED_MODE_PSU1_OFF_MASK},
{LED_TYPE_PSU2, LED_TYPE_PSU2_REG_MASK, LED_MODE_GREEN, LED_MODE_PSU2_GREEN_MASK},
{LED_TYPE_PSU2, LED_TYPE_PSU2_REG_MASK, LED_MODE_AMBER, LED_MODE_PSU2_AMBER_MASK},
{LED_TYPE_PSU2, LED_TYPE_PSU2_REG_MASK, LED_MODE_AUTO,  LED_MODE_PSU2_AUTO_MASK},
{LED_TYPE_PSU2, LED_TYPE_PSU2_REG_MASK, LED_MODE_OFF,   LED_MODE_PSU2_OFF_MASK},
{LED_TYPE_FAN,  LED_TYPE_FAN_REG_MASK,  LED_MODE_GREEN, LED_MODE_FAN_GREEN_MASK},
{LED_TYPE_FAN,  LED_TYPE_FAN_REG_MASK,  LED_MODE_AMBER, LED_MODE_FAN_AMBER_MASK},
{LED_TYPE_FAN,  LED_TYPE_FAN_REG_MASK,  LED_MODE_AUTO,  LED_MODE_FAN_AUTO_MASK},
{LED_TYPE_FAN,  LED_TYPE_FAN_REG_MASK,  LED_MODE_OFF,   LED_MODE_FAN_OFF_MASK},
{LED_TYPE_FAN1,  LED_TYPE_FAN1_REG_MASK,  LED_MODE_GREEN, LED_MODE_FANX_GREEN_MASK << 0},
{LED_TYPE_FAN1,  LED_TYPE_FAN1_REG_MASK,  LED_MODE_RED,   LED_MODE_FANX_RED_MASK << 0},
{LED_TYPE_FAN1,  LED_TYPE_FAN1_REG_MASK,  LED_MODE_OFF,   LED_MODE_FANX_OFF_MASK   << 0},
{LED_TYPE_FAN2,  LED_TYPE_FAN2_REG_MASK,  LED_MODE_GREEN, LED_MODE_FANX_GREEN_MASK << 2},
{LED_TYPE_FAN2,  LED_TYPE_FAN2_REG_MASK,  LED_MODE_RED,   LED_MODE_FANX_RED_MASK << 2},
{LED_TYPE_FAN2,  LED_TYPE_FAN2_REG_MASK,  LED_MODE_OFF,   LED_MODE_FANX_OFF_MASK   << 2},
{LED_TYPE_FAN3,  LED_TYPE_FAN3_REG_MASK,  LED_MODE_GREEN, LED_MODE_FANX_GREEN_MASK << 4},
{LED_TYPE_FAN3,  LED_TYPE_FAN3_REG_MASK,  LED_MODE_RED,   LED_MODE_FANX_RED_MASK << 4},
{LED_TYPE_FAN3,  LED_TYPE_FAN3_REG_MASK,  LED_MODE_OFF,   LED_MODE_FANX_OFF_MASK   << 4},
{LED_TYPE_FAN4,  LED_TYPE_FAN4_REG_MASK,  LED_MODE_GREEN, LED_MODE_FANX_GREEN_MASK << 6},
{LED_TYPE_FAN4,  LED_TYPE_FAN4_REG_MASK,  LED_MODE_RED,   LED_MODE_FANX_RED_MASK << 6},
{LED_TYPE_FAN4,  LED_TYPE_FAN4_REG_MASK,  LED_MODE_OFF,   LED_MODE_FANX_OFF_MASK   << 6},
{LED_TYPE_FAN5,  LED_TYPE_FAN5_REG_MASK,  LED_MODE_GREEN, LED_MODE_FANX_GREEN_MASK << 0},
{LED_TYPE_FAN5,  LED_TYPE_FAN5_REG_MASK,  LED_MODE_RED,   LED_MODE_FANX_RED_MASK << 0},
{LED_TYPE_FAN5,  LED_TYPE_FAN5_REG_MASK,  LED_MODE_OFF,   LED_MODE_FANX_OFF_MASK   << 0},
{LED_TYPE_DIAG, LED_TYPE_DIAG_REG_MASK, LED_MODE_GREEN, LED_MODE_DIAG_GREEN_MASK},
{LED_TYPE_DIAG, LED_TYPE_DIAG_REG_MASK, LED_MODE_AMBER, LED_MODE_DIAG_AMBER_MASK},
{LED_TYPE_DIAG, LED_TYPE_DIAG_REG_MASK, LED_MODE_OFF,   LED_MODE_DIAG_OFF_MASK},
{LED_TYPE_LOC, LED_TYPE_LOC_REG_MASK, LED_MODE_AMBER,       LED_MODE_LOC_ON_MASK},
{LED_TYPE_LOC, LED_TYPE_LOC_REG_MASK, LED_MODE_OFF,         LED_MODE_LOC_OFF_MASK},
{LED_TYPE_LOC, LED_TYPE_LOC_REG_MASK, LED_MODE_AMBER_BLINK, LED_MODE_LOC_BLINK_MASK}
};


struct fanx_info_s {
    u8 cname; /* device name */
    enum led_type type;
    u8   reg_id; /* map to led_reg & reg_val */
};

static struct fanx_info_s fanx_info[] = {
    {'1', LED_TYPE_FAN1, 2},
    {'2', LED_TYPE_FAN2, 2},
    {'3', LED_TYPE_FAN3, 2},
    {'4', LED_TYPE_FAN4, 2},
    {'5', LED_TYPE_FAN5, 3}
};

static int led_reg_val_to_light_mode(enum led_type type, u8 reg_val) {
    int i;

    for (i = 0; i < ARRAY_SIZE(led_type_mode_data); i++) {

        if (type != led_type_mode_data[i].type)
            continue;

        if ((led_type_mode_data[i].type_mask & reg_val) ==
             led_type_mode_data[i].mode_mask)
        {
            return led_type_mode_data[i].mode;
        }
    }

    return 0;
}

static u8 led_light_mode_to_reg_val(enum led_type type,
                                    enum led_light_mode mode, u8 reg_val) {
    int i;

    for (i = 0; i < ARRAY_SIZE(led_type_mode_data); i++) {
        if (type != led_type_mode_data[i].type)
            continue;

        if (mode != led_type_mode_data[i].mode)
            continue;

        reg_val = led_type_mode_data[i].mode_mask |
                 (reg_val & (~led_type_mode_data[i].type_mask));
    }

    return reg_val;
}

static int accton_as5812_54x_led_read_value(u8 reg)
{
    return as5812_54x_cpld_read(0x60, reg);
}

static int accton_as5812_54x_led_write_value(u8 reg, u8 value)
{
    return as5812_54x_cpld_write(0x60, reg, value);
}

static void accton_as5812_54x_led_update(void)
{
    mutex_lock(&ledctl->update_lock);

    if (time_after(jiffies, ledctl->last_updated + HZ + HZ / 2)
        || !ledctl->valid) {
        int i;

        dev_dbg(&ledctl->pdev->dev, "Starting accton_as5812_54x_led update\n");

        /* Update LED data
         */
        for (i = 0; i < ARRAY_SIZE(ledctl->reg_val); i++) {
            int status = accton_as5812_54x_led_read_value(led_reg[i]);

            if (status < 0) {
                ledctl->valid = 0;
                dev_dbg(&ledctl->pdev->dev, "reg %d, err %d\n", led_reg[i], status);
				goto exit;
            }
            else
            {
                ledctl->reg_val[i] = status;
            }
        }

        ledctl->last_updated = jiffies;
        ledctl->valid = 1;
    }

exit:
    mutex_unlock(&ledctl->update_lock);
}

static void accton_as5812_54x_led_set(struct led_classdev *led_cdev,
                                      enum led_brightness led_light_mode,
                                      u8 reg, enum led_type type)
{
    int reg_val;

    mutex_lock(&ledctl->update_lock);

    reg_val = accton_as5812_54x_led_read_value(reg);

    if (reg_val < 0) {
        dev_dbg(&ledctl->pdev->dev, "reg %d, err %d\n", reg, reg_val);
        goto exit;
    }

    reg_val = led_light_mode_to_reg_val(type, led_light_mode, reg_val);
    accton_as5812_54x_led_write_value(reg, reg_val);

    /* to prevent the slow-update issue */
    ledctl->valid = 0;

exit:
    mutex_unlock(&ledctl->update_lock);
}

static void accton_as5812_54x_led_psu_1_set(struct led_classdev *led_cdev,
                                            enum led_brightness led_light_mode)
{
    accton_as5812_54x_led_set(led_cdev, led_light_mode, led_reg[1], LED_TYPE_PSU1);
}

static enum led_brightness accton_as5812_54x_led_psu_1_get(struct led_classdev *cdev)
{
    accton_as5812_54x_led_update();
    return led_reg_val_to_light_mode(LED_TYPE_PSU1, ledctl->reg_val[1]);
}

static void accton_as5812_54x_led_psu_2_set(struct led_classdev *led_cdev,
                                            enum led_brightness led_light_mode)
{
    accton_as5812_54x_led_set(led_cdev, led_light_mode, led_reg[1], LED_TYPE_PSU2);
}

static enum led_brightness accton_as5812_54x_led_psu_2_get(struct led_classdev *cdev)
{
    accton_as5812_54x_led_update();
    return led_reg_val_to_light_mode(LED_TYPE_PSU2, ledctl->reg_val[1]);
}

static void accton_as5812_54x_led_fan_set(struct led_classdev *led_cdev,
                                          enum led_brightness led_light_mode)
{
    accton_as5812_54x_led_set(led_cdev, led_light_mode, led_reg[0], LED_TYPE_FAN);
}

static enum led_brightness accton_as5812_54x_led_fan_get(struct led_classdev *cdev)
{
    accton_as5812_54x_led_update();
    return led_reg_val_to_light_mode(LED_TYPE_FAN, ledctl->reg_val[0]);
}


static void accton_as5812_54x_led_fanx_set(struct led_classdev *led_cdev,
                                          enum led_brightness led_light_mode)
{
    enum led_type led_type1;
    int reg_id;
    int i, nsize;
    int ncount = sizeof(fanx_info)/sizeof(struct fanx_info_s);

    for(i=0;i<ncount;i++)
    {
        nsize=strlen(led_cdev->name);

        if (led_cdev->name[nsize-1] == fanx_info[i].cname)
        {
            led_type1 = fanx_info[i].type;
            reg_id = fanx_info[i].reg_id;
            accton_as5812_54x_led_set(led_cdev, led_light_mode, led_reg[reg_id], led_type1);
            return;
        }
    }
}


static enum led_brightness accton_as5812_54x_led_fanx_get(struct led_classdev *cdev)
{
    enum led_type led_type1;
    int reg_id;
    int i, nsize;
    int ncount = sizeof(fanx_info)/sizeof(struct fanx_info_s);

    for(i=0;i<ncount;i++)
    {
        nsize=strlen(cdev->name);

        if (cdev->name[nsize-1] == fanx_info[i].cname)
        {
            led_type1 = fanx_info[i].type;
            reg_id = fanx_info[i].reg_id;
            accton_as5812_54x_led_update();
            return led_reg_val_to_light_mode(led_type1, ledctl->reg_val[reg_id]);
        }
    }


    return led_reg_val_to_light_mode(LED_TYPE_FAN1, ledctl->reg_val[2]);
}


static void accton_as5812_54x_led_diag_set(struct led_classdev *led_cdev,
                                           enum led_brightness led_light_mode)
{
    accton_as5812_54x_led_set(led_cdev, led_light_mode, led_reg[0], LED_TYPE_DIAG);
}

static enum led_brightness accton_as5812_54x_led_diag_get(struct led_classdev *cdev)
{
    accton_as5812_54x_led_update();
    return led_reg_val_to_light_mode(LED_TYPE_DIAG, ledctl->reg_val[0]);
}

static void accton_as5812_54x_led_loc_set(struct led_classdev *led_cdev,
                                          enum led_brightness led_light_mode)
{
    accton_as5812_54x_led_set(led_cdev, led_light_mode, led_reg[0], LED_TYPE_LOC);
}

static enum led_brightness accton_as5812_54x_led_loc_get(struct led_classdev *cdev)
{
    accton_as5812_54x_led_update();
    return led_reg_val_to_light_mode(LED_TYPE_LOC, ledctl->reg_val[0]);
}

static struct led_classdev accton_as5812_54x_leds[] = {
    [LED_TYPE_PSU1] = {
        .name             = "accton_as5812_54x_led::psu1",
        .default_trigger = "unused",
        .brightness_set     = accton_as5812_54x_led_psu_1_set,
        .brightness_get  = accton_as5812_54x_led_psu_1_get,
        .flags             = LED_CORE_SUSPENDRESUME,
        .max_brightness  = LED_MODE_AUTO,
    },
    [LED_TYPE_PSU2] = {
        .name             = "accton_as5812_54x_led::psu2",
        .default_trigger = "unused",
        .brightness_set     = accton_as5812_54x_led_psu_2_set,
        .brightness_get  = accton_as5812_54x_led_psu_2_get,
        .flags             = LED_CORE_SUSPENDRESUME,
        .max_brightness  = LED_MODE_AUTO,
    },
    [LED_TYPE_FAN] = {
        .name             = "accton_as5812_54x_led::fan",
        .default_trigger = "unused",
        .brightness_set     = accton_as5812_54x_led_fan_set,
        .brightness_get  = accton_as5812_54x_led_fan_get,
        .flags             = LED_CORE_SUSPENDRESUME,
        .max_brightness  = LED_MODE_AUTO,
    },
    [LED_TYPE_FAN1] = {
        .name             = "accton_as5812_54x_led::fan1",
        .default_trigger = "unused",
        .brightness_set     = accton_as5812_54x_led_fanx_set,
        .brightness_get  = accton_as5812_54x_led_fanx_get,
        .flags             = LED_CORE_SUSPENDRESUME,
        .max_brightness  = LED_MODE_AUTO,
    },
    [LED_TYPE_FAN2] = {
        .name             = "accton_as5812_54x_led::fan2",
        .default_trigger = "unused",
        .brightness_set     = accton_as5812_54x_led_fanx_set,
        .brightness_get  = accton_as5812_54x_led_fanx_get,
        .flags             = LED_CORE_SUSPENDRESUME,
        .max_brightness  = LED_MODE_AUTO,
    },
    [LED_TYPE_FAN3] = {
        .name             = "accton_as5812_54x_led::fan3",
        .default_trigger = "unused",
        .brightness_set     = accton_as5812_54x_led_fanx_set,
        .brightness_get  = accton_as5812_54x_led_fanx_get,
        .flags             = LED_CORE_SUSPENDRESUME,
        .max_brightness  = LED_MODE_AUTO,
    },
    [LED_TYPE_FAN4] = {
        .name             = "accton_as5812_54x_led::fan4",
        .default_trigger = "unused",
        .brightness_set     = accton_as5812_54x_led_fanx_set,
        .brightness_get  = accton_as5812_54x_led_fanx_get,
        .flags             = LED_CORE_SUSPENDRESUME,
        .max_brightness  = LED_MODE_AUTO,
    },
    [LED_TYPE_FAN5] = {
        .name             = "accton_as5812_54x_led::fan5",
        .default_trigger = "unused",
        .brightness_set     = accton_as5812_54x_led_fanx_set,
        .brightness_get  = accton_as5812_54x_led_fanx_get,
        .flags             = LED_CORE_SUSPENDRESUME,
        .max_brightness  = LED_MODE_AUTO,
    },
    [LED_TYPE_DIAG] = {
        .name             = "accton_as5812_54x_led::diag",
        .default_trigger = "unused",
        .brightness_set     = accton_as5812_54x_led_diag_set,
        .brightness_get  = accton_as5812_54x_led_diag_get,
        .flags             = LED_CORE_SUSPENDRESUME,
        .max_brightness  = LED_MODE_AUTO,
    },
    [LED_TYPE_LOC] = {
        .name             = "accton_as5812_54x_led::loc",
        .default_trigger = "unused",
        .brightness_set     = accton_as5812_54x_led_loc_set,
        .brightness_get  = accton_as5812_54x_led_loc_get,
        .flags             = LED_CORE_SUSPENDRESUME,
        .max_brightness  = LED_MODE_AUTO,
    },
};

static int accton_as5812_54x_led_suspend(struct platform_device *dev,
        pm_message_t state)
{
    int i = 0;

    for (i = 0; i < ARRAY_SIZE(accton_as5812_54x_leds); i++) {
        led_classdev_suspend(&accton_as5812_54x_leds[i]);
    }

    return 0;
}

static int accton_as5812_54x_led_resume(struct platform_device *dev)
{
    int i = 0;

    for (i = 0; i < ARRAY_SIZE(accton_as5812_54x_leds); i++) {
        led_classdev_resume(&accton_as5812_54x_leds[i]);
    }

    return 0;
}

static int accton_as5812_54x_led_probe(struct platform_device *pdev)
{
    int ret, i;

    for (i = 0; i < ARRAY_SIZE(accton_as5812_54x_leds); i++) {
        ret = led_classdev_register(&pdev->dev, &accton_as5812_54x_leds[i]);

        if (ret < 0)
            break;
    }

    /* Check if all LEDs were successfully registered */
    if (i != ARRAY_SIZE(accton_as5812_54x_leds)){
        int j;

        /* only unregister the LEDs that were successfully registered */
        for (j = 0; j < i; j++) {
            led_classdev_unregister(&accton_as5812_54x_leds[i]);
        }
    }

    return ret;
}

static int accton_as5812_54x_led_remove(struct platform_device *pdev)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(accton_as5812_54x_leds); i++) {
        led_classdev_unregister(&accton_as5812_54x_leds[i]);
    }

    return 0;
}

static struct platform_driver accton_as5812_54x_led_driver = {
    .probe      = accton_as5812_54x_led_probe,
    .remove     = accton_as5812_54x_led_remove,
    .suspend    = accton_as5812_54x_led_suspend,
    .resume     = accton_as5812_54x_led_resume,
    .driver     = {
    .name   = DRVNAME,
    .owner  = THIS_MODULE,
    },
};

static int __init accton_as5812_54x_led_init(void)
{
    int ret;

    ret = platform_driver_register(&accton_as5812_54x_led_driver);
    if (ret < 0) {
        goto exit;
    }

    ledctl = kzalloc(sizeof(struct accton_as5812_54x_led_data), GFP_KERNEL);
    if (!ledctl) {
        ret = -ENOMEM;
        platform_driver_unregister(&accton_as5812_54x_led_driver);
        goto exit;
    }

    mutex_init(&ledctl->update_lock);

    ledctl->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
    if (IS_ERR(ledctl->pdev)) {
        ret = PTR_ERR(ledctl->pdev);
        platform_driver_unregister(&accton_as5812_54x_led_driver);
        kfree(ledctl);
        goto exit;
    }

exit:
    return ret;
}

static void __exit accton_as5812_54x_led_exit(void)
{
    platform_device_unregister(ledctl->pdev);
    platform_driver_unregister(&accton_as5812_54x_led_driver);
    kfree(ledctl);
}

module_init(accton_as5812_54x_led_init);
module_exit(accton_as5812_54x_led_exit);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("accton_as5812_54x_led driver");
MODULE_LICENSE("GPL");

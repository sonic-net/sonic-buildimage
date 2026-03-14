/*
 * A LED driver for the accton_as7315_27xb_led
 *
 * Copyright (C) 2019 Accton Technology Corporation.
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

/*#define DEBUG*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/leds.h>
#include <linux/slab.h>

#define DRVNAME "as7315_27xb_led"
#define CPLD_I2C_ADDR  0x64

enum led_type {
    TYPE_DIAG,
    TYPE_LOC,
    TYPE_MAX
};

struct led_list_s {
    enum led_type type;
    char name[64];
    u8   reg_addr;
    u8   slave_addr;
} led_list[TYPE_MAX] = {
    {TYPE_DIAG, "as7315_27xb_diag", 0x41, CPLD_I2C_ADDR},
    {TYPE_LOC,  "as7315_27xb_loc", 0x40, CPLD_I2C_ADDR}
};

struct accton_as7315_27xb_led_data {
    struct platform_device *pdev;
    struct mutex     update_lock;
    char             valid;           /* != 0 if registers are valid */
    unsigned long    last_updated;    /* In jiffies */
    u8               slave_addr[TYPE_MAX];     /* For LOC and DIAG.*/
    u8               reg_addr[TYPE_MAX];     /* For LOC and DIAG.*/
    u8               reg_val[TYPE_MAX];      /* Register value, 0 = LOC
                                                        1 = DIAG  */
};
static struct accton_as7315_27xb_led_data  *ledctl = NULL;

/* LED related data
 */
#define TYPE_DIAG_REG_MASK    0x30
#define MODE_DIAG_GREEN_MASK  0x10
#define MODE_DIAG_AMBER_MASK  0x20
#define MODE_DIAG_GBLINK_MASK  0x00
#define MODE_DIAG_OFF_MASK    0x30

#define TYPE_LOC_REG_MASK     0x40
#define MODE_LOC_OFF_MASK     0x40
#define MODE_LOC_BLINK_MASK   0x00


typedef enum onlp_led_mode_e {
    ONLP_LED_MODE_OFF,
    ONLP_LED_MODE_ON,
    ONLP_LED_MODE_BLINKING,
    ONLP_LED_MODE_RED = 10,
    ONLP_LED_MODE_RED_BLINKING = 11,
    ONLP_LED_MODE_ORANGE = 12,
    ONLP_LED_MODE_ORANGE_BLINKING = 13,
    ONLP_LED_MODE_YELLOW = 14,
    ONLP_LED_MODE_YELLOW_BLINKING = 15,
    ONLP_LED_MODE_GREEN = 16,
    ONLP_LED_MODE_GREEN_BLINKING = 17,
    ONLP_LED_MODE_BLUE = 18,
    ONLP_LED_MODE_BLUE_BLINKING = 19,
    ONLP_LED_MODE_PURPLE = 20,
    ONLP_LED_MODE_PURPLE_BLINKING = 21,
    ONLP_LED_MODE_AUTO = 22,
    ONLP_LED_MODE_AUTO_BLINKING = 23,
} onlp_led_mode_t;


struct led_type_mode {
    enum led_type type;
    int  type_mask;
    enum onlp_led_mode_e mode;
    int  mode_mask;
};

static struct led_type_mode led_type_mode_data[] = {
    {TYPE_DIAG, TYPE_DIAG_REG_MASK, ONLP_LED_MODE_GREEN_BLINKING, MODE_DIAG_GBLINK_MASK},
    {TYPE_DIAG, TYPE_DIAG_REG_MASK, ONLP_LED_MODE_GREEN, MODE_DIAG_GREEN_MASK},
    {TYPE_DIAG, TYPE_DIAG_REG_MASK, ONLP_LED_MODE_ORANGE, MODE_DIAG_AMBER_MASK},
    {TYPE_DIAG, TYPE_DIAG_REG_MASK, ONLP_LED_MODE_OFF,   MODE_DIAG_OFF_MASK},
    {TYPE_LOC, TYPE_LOC_REG_MASK, ONLP_LED_MODE_BLUE_BLINKING,  MODE_LOC_BLINK_MASK},
    {TYPE_LOC, TYPE_LOC_REG_MASK, ONLP_LED_MODE_OFF,       MODE_LOC_OFF_MASK},
};

extern int accton_i2c_cpld_read (u8 cpld_addr, u8 reg);
extern int accton_i2c_cpld_write(u8 cpld_addr, u8 reg, u8 value);


static int pdata_init(struct accton_as7315_27xb_led_data  *data) {
    int i;

    for (i=0; i<ARRAY_SIZE(led_list) ; i++) {
        data->reg_addr[i] = led_list[i].reg_addr;
        data->slave_addr[i] = led_list[i].slave_addr;
    }

    return 0;
}

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
                                    enum onlp_led_mode_e mode, u8 reg_val) {
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

static int accton_as7315_27xb_led_read_value(u8 slave, u8 reg)
{
    return accton_i2c_cpld_read(slave, reg);
}

static int accton_as7315_27xb_led_write_value(u8 slave, u8 reg, u8 value)
{
    return accton_i2c_cpld_write(slave, reg, value);
}

static void accton_as7315_27xb_led_update(void)
{
    mutex_lock(&ledctl->update_lock);

    if (time_after(jiffies, ledctl->last_updated + HZ + HZ / 2)
            || !ledctl->valid) {
        int i;

        dev_dbg(&ledctl->pdev->dev, "Starting accton_as7315_27xb_led update\n");

        /* Update LED data
         */
        for (i = 0; i < ARRAY_SIZE(ledctl->reg_val); i++) {
            int status;
            u8 addr, offset;
            addr = ledctl->slave_addr[i];
            offset = ledctl->reg_addr[i];
            status = accton_as7315_27xb_led_read_value(addr, offset);
            if (status < 0) {
                ledctl->valid = 0;
                dev_dbg(&ledctl->pdev->dev, "reg %d, err %d\n", ledctl->reg_addr[i], status);
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

static void accton_as7315_27xb_led_set(struct led_classdev *led_cdev,
                                       enum led_brightness led_light_mode,
                                       enum led_type type)
{
    int reg_val;
    u8 addr, offset;

    if (type >= ARRAY_SIZE(led_list)) {
        dev_dbg(&ledctl->pdev->dev, "Illegal type:%d\n", type);
        return;
    }
    mutex_lock(&ledctl->update_lock);
    addr = ledctl->slave_addr[type],
    offset = ledctl->reg_addr[type],
    reg_val = accton_as7315_27xb_led_read_value(addr, offset);
    if (reg_val < 0) {
        dev_dbg(&ledctl->pdev->dev, "reg %d, err %d\n", offset, reg_val);
        goto exit;
    }

    reg_val = led_light_mode_to_reg_val(type, led_light_mode, reg_val);
    accton_as7315_27xb_led_write_value(addr, offset, reg_val);

    /* to prevent the slow-update issue */
    ledctl->valid = 0;

exit:
    mutex_unlock(&ledctl->update_lock);
}

static enum led_type get_led_type(struct led_classdev *cdev)
{
    int i;

    for (i=0; i<ARRAY_SIZE(led_list) ; i++) {
        if (strstr(led_list[i].name, cdev->name))
            return i;
    }
    return -EINVAL;
}

static void led_mode_set(struct led_classdev *cdev,
                         enum led_brightness led_light_mode)
{
    enum led_type type;

    type = get_led_type(cdev);
    if (type < 0) {
        dev_dbg(&ledctl->pdev->dev, "Found no corresponding type:%d\n", type);
        return ;
    }
    accton_as7315_27xb_led_set(cdev, led_light_mode, type);
}

static enum led_brightness led_mode_get(struct led_classdev *cdev)
{
    enum led_type type;

    type = get_led_type(cdev);
    if (type < 0)
        return type;

    accton_as7315_27xb_led_update();
    return led_reg_val_to_light_mode(type, ledctl->reg_val[type]);
}

static struct led_classdev accton_as7315_27xb_leds[TYPE_MAX] = {
    [TYPE_DIAG] = {
        .name             = led_list[TYPE_DIAG].name,
        .default_trigger = "unused",
        .brightness_set     = led_mode_set,
        .brightness_get  = led_mode_get,
        .flags             = LED_CORE_SUSPENDRESUME,
        .max_brightness  = ONLP_LED_MODE_AUTO,
    },
    [TYPE_LOC] = {
        .name             = led_list[TYPE_LOC].name,
        .default_trigger = "unused",
        .brightness_set     = led_mode_set,
        .brightness_get  = led_mode_get,
        .flags             = LED_CORE_SUSPENDRESUME,
        .max_brightness  = ONLP_LED_MODE_AUTO,
    },
};

static int accton_as7315_27xb_led_suspend(struct platform_device *dev,
        pm_message_t state)
{
    int i = 0;

    for (i = 0; i < ARRAY_SIZE(accton_as7315_27xb_leds); i++) {
        led_classdev_suspend(&accton_as7315_27xb_leds[i]);
    }

    return 0;
}

static int accton_as7315_27xb_led_resume(struct platform_device *dev)
{
    int i = 0;

    for (i = 0; i < ARRAY_SIZE(accton_as7315_27xb_leds); i++) {
        led_classdev_resume(&accton_as7315_27xb_leds[i]);
    }

    return 0;
}

static int accton_as7315_27xb_led_probe(struct platform_device *pdev)
{
    int ret, i;

    for (i = 0; i < ARRAY_SIZE(accton_as7315_27xb_leds); i++) {
        ret = led_classdev_register(&pdev->dev, &accton_as7315_27xb_leds[i]);

        if (ret < 0)
            break;
    }

    /* Check if all LEDs were successfully registered */
    if (i != ARRAY_SIZE(accton_as7315_27xb_leds)) {
        int j;

        /* only unregister the LEDs that were successfully registered */
        for (j = 0; j < i; j++) {
            led_classdev_unregister(&accton_as7315_27xb_leds[i]);
        }
    }

    return ret;
}

static int accton_as7315_27xb_led_remove(struct platform_device *pdev)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(accton_as7315_27xb_leds); i++) {
        led_classdev_unregister(&accton_as7315_27xb_leds[i]);
    }

    return 0;
}

static struct platform_driver accton_as7315_27xb_led_driver = {
    .probe      = accton_as7315_27xb_led_probe,
    .remove     = accton_as7315_27xb_led_remove,
    .suspend    = accton_as7315_27xb_led_suspend,
    .resume     = accton_as7315_27xb_led_resume,
    .driver     = {
        .name   = DRVNAME,
        .owner  = THIS_MODULE,
    },
};

static int __init accton_as7315_27xb_led_init(void)
{
    int ret;

    ret = platform_driver_register(&accton_as7315_27xb_led_driver);
    if (ret < 0) {
        goto exit;
    }

    ledctl = kzalloc(sizeof(struct accton_as7315_27xb_led_data), GFP_KERNEL);
    if (!ledctl) {
        ret = -ENOMEM;
        platform_driver_unregister(&accton_as7315_27xb_led_driver);
        goto exit;
    }

    pdata_init(ledctl);
    mutex_init(&ledctl->update_lock);

    ledctl->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
    if (IS_ERR(ledctl->pdev)) {
        ret = PTR_ERR(ledctl->pdev);
        platform_driver_unregister(&accton_as7315_27xb_led_driver);
        kfree(ledctl);
        goto exit;
    }

exit:
    return ret;
}

static void __exit accton_as7315_27xb_led_exit(void)
{
    platform_device_unregister(ledctl->pdev);
    platform_driver_unregister(&accton_as7315_27xb_led_driver);
    kfree(ledctl);
}

module_init(accton_as7315_27xb_led_init);
module_exit(accton_as7315_27xb_led_exit);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("accton_as7315_27xb_led driver");
MODULE_LICENSE("GPL");

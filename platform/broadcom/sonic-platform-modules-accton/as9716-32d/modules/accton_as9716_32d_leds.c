/*
 * A LED driver for the accton_as9716_32d_led
 *
 * Copyright (C) 2014 Accton Technology Corporation.
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
#include <linux/dmi.h>

extern int as9716_32d_cpld_read (unsigned short cpld_addr, u8 reg);
extern int as9716_32d_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

#define DRVNAME "accton_as9716_32d_led"

struct accton_as9716_32d_led_data {
    struct platform_device *pdev;
    struct mutex	 update_lock;
    char			 valid;		   /* != 0 if registers are valid */
    unsigned long	last_updated;	/* In jiffies */
    u8			   reg_val[2];	  /* only 1 register*/
};

static struct accton_as9716_32d_led_data  *ledctl = NULL;

/* LED related data
 */

#define LED_CNTRLER_I2C_ADDRESS	(0x61)

#define LED_TYPE_DIAG_REG_MASK	   (0x1|0x2|0x4)
#define LED_MODE_DIAG_GREEN_VALUE       (0x01)
#define LED_MODE_DIAG_AMBER_VALUE       (0x02)  /*It's yellow actually. Green+Red=Yellow*/
#define LED_MODE_DIAG_GREEN_BLINK_VALUE	(0x04)
#define LED_MODE_DIAG_OFF_VALUE	        (0x0)


#define LED_TYPE_LOC_REG_MASK	        (0x80|0x40)
#define LED_MODE_LOC_GREEN_BLIKN_VALUE   0x80
#define LED_MODE_LOC_AMBER_VALUE         0x40
#define LED_MODE_LOC_OFF_VALUE	        (0x0)

#define LED_TYPE_FAN_REG_MASK           (0x20|0x10)
#define LED_MODE_FAN_AMBER_VALUE        0x20
#define LED_MODE_FAN_GREEN_VALUE        0x10
#define LED_MODE_FAN_OFF_VALUE	        (0x0)

#define LED_TYPE_PSU2_REG_MASK          (0x8|0x4)
#define LED_MODE_PSU2_AMBER_VALUE        0x8
#define LED_MODE_PSU2_GREEN_VALUE        0x4
#define LED_MODE_PSU2_OFF_VALUE	        (0x0)

#define LED_TYPE_PSU1_REG_MASK          (0x2|0x1)
#define LED_MODE_PSU1_AMBER_VALUE        0x2
#define LED_MODE_PSU1_GREEN_VALUE        0x1
#define LED_MODE_PSU1_OFF_VALUE	        (0x0)

enum led_type {
    LED_TYPE_DIAG,
    LED_TYPE_LOC,
    LED_TYPE_FAN,
    LED_TYPE_PSU1,
    LED_TYPE_PSU2
};

struct led_reg {
    u32  types;
    u8   reg_addr;
};

static const struct led_reg led_reg_map[] = {    
    {(1<<LED_TYPE_DIAG), 0x31},
    {(1<<LED_TYPE_PSU2) | (1<<LED_TYPE_PSU1) | (1<<LED_TYPE_FAN) | (1<<LED_TYPE_LOC), 0x30},
};


enum led_light_mode {
    LED_MODE_OFF = 0,
    LED_MODE_GREEN,
    LED_MODE_AMBER,
    LED_MODE_RED,
    LED_MODE_BLUE,
    LED_MODE_GREEN_BLINK,
    LED_MODE_AMBER_BLINK,
    LED_MODE_RED_BLINK,
    LED_MODE_BLUE_BLINK,
    LED_MODE_AUTO,
    LED_MODE_UNKNOWN
};

struct led_type_mode {
    enum led_type type;
    enum led_light_mode mode;
    int  reg_bit_mask;
    int  mode_value;
};

static struct led_type_mode led_type_mode_data[] = {
  {LED_TYPE_LOC, LED_MODE_OFF,         LED_TYPE_LOC_REG_MASK, LED_MODE_LOC_OFF_VALUE},
  {LED_TYPE_LOC, LED_MODE_GREEN_BLINK, LED_TYPE_LOC_REG_MASK, LED_MODE_LOC_GREEN_BLIKN_VALUE},
  {LED_TYPE_LOC, LED_MODE_AMBER,	   LED_TYPE_LOC_REG_MASK, LED_MODE_LOC_AMBER_VALUE},  
  {LED_TYPE_DIAG,LED_MODE_OFF,   LED_TYPE_DIAG_REG_MASK,  LED_MODE_DIAG_OFF_VALUE},
  {LED_TYPE_DIAG,LED_MODE_GREEN, LED_TYPE_DIAG_REG_MASK,  LED_MODE_DIAG_GREEN_VALUE},  
  {LED_TYPE_DIAG,LED_MODE_AMBER, LED_TYPE_DIAG_REG_MASK,  LED_MODE_DIAG_AMBER_VALUE},
  {LED_TYPE_DIAG,LED_MODE_GREEN_BLINK, LED_TYPE_DIAG_REG_MASK,  LED_MODE_DIAG_GREEN_BLINK_VALUE},  
  {LED_TYPE_FAN,LED_MODE_OFF,   LED_TYPE_FAN_REG_MASK,  LED_MODE_FAN_OFF_VALUE},
  {LED_TYPE_FAN,LED_MODE_GREEN, LED_TYPE_FAN_REG_MASK,  LED_MODE_FAN_GREEN_VALUE},  
  {LED_TYPE_FAN,LED_MODE_AMBER, LED_TYPE_FAN_REG_MASK,  LED_MODE_FAN_AMBER_VALUE},
  
  {LED_TYPE_PSU1,LED_MODE_OFF,   LED_TYPE_PSU1_REG_MASK,  LED_MODE_PSU1_OFF_VALUE},
  {LED_TYPE_PSU1,LED_MODE_GREEN, LED_TYPE_PSU1_REG_MASK,  LED_MODE_PSU1_GREEN_VALUE},  
  {LED_TYPE_PSU1,LED_MODE_AMBER, LED_TYPE_PSU1_REG_MASK,  LED_MODE_PSU1_AMBER_VALUE},
  
  {LED_TYPE_PSU2,LED_MODE_OFF,   LED_TYPE_PSU2_REG_MASK,  LED_MODE_PSU2_OFF_VALUE},
  {LED_TYPE_PSU2,LED_MODE_GREEN, LED_TYPE_PSU2_REG_MASK,  LED_MODE_PSU2_GREEN_VALUE},  
  {LED_TYPE_PSU2,LED_MODE_AMBER, LED_TYPE_PSU2_REG_MASK,  LED_MODE_PSU2_AMBER_VALUE},
  
};



static void accton_as9716_32d_led_set(struct led_classdev *led_cdev,
                                      enum led_brightness led_light_mode, enum led_type type);





static int accton_getLedReg(enum led_type type, u8 *reg)
{
    int i;
    
    for (i = 0; i < ARRAY_SIZE(led_reg_map); i++) {        
        if(led_reg_map[i].types & (1<<type)) {
            *reg = led_reg_map[i].reg_addr;
            return 0;
        }
    }
    
    return 1;
}


static int led_reg_val_to_light_mode(enum led_type type, u8 reg_val) {
    int i;

    for (i = 0; i < ARRAY_SIZE(led_type_mode_data); i++) {

        if (type != led_type_mode_data[i].type)
            continue;

        if ((led_type_mode_data[i].reg_bit_mask & reg_val) ==
                led_type_mode_data[i].mode_value)
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
        
        reg_val = led_type_mode_data[i].mode_value |
                  (reg_val & (~led_type_mode_data[i].reg_bit_mask));
        break;
    }

    return reg_val;
}

static int accton_as9716_32d_led_read_value(u8 reg)
{
    return as9716_32d_cpld_read(LED_CNTRLER_I2C_ADDRESS, reg);
}

static int accton_as9716_32d_led_write_value(u8 reg, u8 value)
{
    return as9716_32d_cpld_write(LED_CNTRLER_I2C_ADDRESS, reg, value);
}

static void accton_as9716_32d_led_update(void)
{
    mutex_lock(&ledctl->update_lock);

    if (time_after(jiffies, ledctl->last_updated + HZ + HZ / 2)
            || !ledctl->valid) {
        int i;

        dev_dbg(&ledctl->pdev->dev, "Starting accton_as9716_32d_led update\n");

        /* Update LED data
         */
        for (i = 0; i < ARRAY_SIZE(ledctl->reg_val); i++) {
            int status = accton_as9716_32d_led_read_value(led_reg_map[i].reg_addr);

            if (status < 0) {
                ledctl->valid = 0;
                dev_dbg(&ledctl->pdev->dev, "reg %d, err %d\n", led_reg_map[i].reg_addr, status);
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

static void accton_as9716_32d_led_set(struct led_classdev *led_cdev,
                                      enum led_brightness led_light_mode,
                                      enum led_type type)
{
    int reg_val;
    u8 reg	;
    mutex_lock(&ledctl->update_lock);

    if( !accton_getLedReg(type, &reg))
    {
        dev_dbg(&ledctl->pdev->dev, "Not match item for %d.\n", type);
    }
    
    reg_val = accton_as9716_32d_led_read_value(reg);
    
    if (reg_val < 0) {
        dev_dbg(&ledctl->pdev->dev, "reg %d, err %d\n", reg, reg_val);
        goto exit;
    }
    reg_val = led_light_mode_to_reg_val(type, led_light_mode, reg_val);
    
    accton_as9716_32d_led_write_value(reg, reg_val);

    /* to prevent the slow-update issue */
    ledctl->valid = 0;

exit:
    mutex_unlock(&ledctl->update_lock);
}


static void accton_as9716_32d_led_diag_set(struct led_classdev *led_cdev,
        enum led_brightness led_light_mode)
{
    accton_as9716_32d_led_set(led_cdev, led_light_mode,  LED_TYPE_DIAG);
}

static enum led_brightness accton_as9716_32d_led_diag_get(struct led_classdev *cdev)
{
    accton_as9716_32d_led_update();
    return led_reg_val_to_light_mode(LED_TYPE_DIAG, ledctl->reg_val[0]);
}

static void accton_as9716_32d_led_loc_set(struct led_classdev *led_cdev,
        enum led_brightness led_light_mode)
{
    accton_as9716_32d_led_set(led_cdev, led_light_mode, LED_TYPE_LOC);
}

static enum led_brightness accton_as9716_32d_led_loc_get(struct led_classdev *cdev)
{
    accton_as9716_32d_led_update();
    return led_reg_val_to_light_mode(LED_TYPE_LOC, ledctl->reg_val[1]);
}

static void accton_as9716_32d_led_fan_set(struct led_classdev *led_cdev,
        enum led_brightness led_light_mode)
{
    accton_as9716_32d_led_set(led_cdev, led_light_mode, LED_TYPE_FAN);
}

static enum led_brightness accton_as9716_32d_led_fan_get(struct led_classdev *cdev)
{
    accton_as9716_32d_led_update();
    return led_reg_val_to_light_mode(LED_TYPE_FAN, ledctl->reg_val[1]);
}

static void accton_as9716_32d_led_psu1_set(struct led_classdev *led_cdev,
        enum led_brightness led_light_mode)
{
    accton_as9716_32d_led_set(led_cdev, led_light_mode, LED_TYPE_PSU1);
}

static enum led_brightness accton_as9716_32d_led_psu1_get(struct led_classdev *cdev)
{
    accton_as9716_32d_led_update();
    return led_reg_val_to_light_mode(LED_TYPE_PSU1, ledctl->reg_val[1]);
}

static void accton_as9716_32d_led_psu2_set(struct led_classdev *led_cdev,
        enum led_brightness led_light_mode)
{
    accton_as9716_32d_led_set(led_cdev, led_light_mode, LED_TYPE_PSU2);
}

static enum led_brightness accton_as9716_32d_led_psu2_get(struct led_classdev *cdev)
{
    accton_as9716_32d_led_update();
    return led_reg_val_to_light_mode(LED_TYPE_PSU2, ledctl->reg_val[1]);
}

static struct led_classdev accton_as9716_32d_leds[] = {
    [LED_TYPE_DIAG] = {
        .name			 = "accton_as9716_32d_led::diag",
        .default_trigger = "unused",
        .brightness_set	 = accton_as9716_32d_led_diag_set,
        .brightness_get	 = accton_as9716_32d_led_diag_get,
        .flags			 = LED_CORE_SUSPENDRESUME,
        .max_brightness	 = LED_MODE_GREEN_BLINK,
    },
    [LED_TYPE_LOC] = {
        .name			 = "accton_as9716_32d_led::loc",
        .default_trigger = "unused",
        .brightness_set	 = accton_as9716_32d_led_loc_set,
        .brightness_get	 = accton_as9716_32d_led_loc_get,
        .flags			 = LED_CORE_SUSPENDRESUME,
        .max_brightness	 = LED_MODE_GREEN_BLINK,
    },
    [LED_TYPE_FAN] = {
        .name			 = "accton_as9716_32d_led::fan",
        .default_trigger = "unused",
        .brightness_set	 = accton_as9716_32d_led_fan_set,
        .brightness_get  = accton_as9716_32d_led_fan_get,
        .flags			 = LED_CORE_SUSPENDRESUME,
        .max_brightness  = LED_MODE_AMBER,
    },
    [LED_TYPE_PSU1] = {
        .name			 = "accton_as9716_32d_led::psu1",
        .default_trigger = "unused",
        .brightness_set	 = accton_as9716_32d_led_psu1_set,
        .brightness_get  = accton_as9716_32d_led_psu1_get,
        .flags			 = LED_CORE_SUSPENDRESUME,
        .max_brightness  = LED_MODE_AMBER,
    },
    [LED_TYPE_PSU2] = {
        .name			 = "accton_as9716_32d_led::psu2",
        .default_trigger = "unused",
        .brightness_set	 = accton_as9716_32d_led_psu2_set,
        .brightness_get  = accton_as9716_32d_led_psu2_get,
        .flags			 = LED_CORE_SUSPENDRESUME,
        .max_brightness  = LED_MODE_AMBER,
    },
};

static int accton_as9716_32d_led_suspend(struct platform_device *dev,
        pm_message_t state)
{
    int i = 0;

    for (i = 0; i < ARRAY_SIZE(accton_as9716_32d_leds); i++) {
        led_classdev_suspend(&accton_as9716_32d_leds[i]);
    }

    return 0;
}

static int accton_as9716_32d_led_resume(struct platform_device *dev)
{
    int i = 0;

    for (i = 0; i < ARRAY_SIZE(accton_as9716_32d_leds); i++) {
        led_classdev_resume(&accton_as9716_32d_leds[i]);
    }

    return 0;
}

static int accton_as9716_32d_led_probe(struct platform_device *pdev)
{
    int ret, i;

    for (i = 0; i < ARRAY_SIZE(accton_as9716_32d_leds); i++) {
        ret = led_classdev_register(&pdev->dev, &accton_as9716_32d_leds[i]);

        if (ret < 0)
            break;
    }

    /* Check if all LEDs were successfully registered */
    if (i != ARRAY_SIZE(accton_as9716_32d_leds)) {
        int j;

        /* only unregister the LEDs that were successfully registered */
        for (j = 0; j < i; j++) {
            led_classdev_unregister(&accton_as9716_32d_leds[i]);
        }
    }

    return ret;
}

static int accton_as9716_32d_led_remove(struct platform_device *pdev)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(accton_as9716_32d_leds); i++) {
        led_classdev_unregister(&accton_as9716_32d_leds[i]);
    }

    return 0;
}

static struct platform_driver accton_as9716_32d_led_driver = {
    .probe	  = accton_as9716_32d_led_probe,
    .remove	 = accton_as9716_32d_led_remove,
    .suspend	= accton_as9716_32d_led_suspend,
    .resume	 = accton_as9716_32d_led_resume,
    .driver	 = {
        .name   = DRVNAME,
        .owner  = THIS_MODULE,
    },
};

static int __init accton_as9716_32d_led_init(void)
{
    int ret;

    ret = platform_driver_register(&accton_as9716_32d_led_driver);
    if (ret < 0) {
        goto exit;
    }

    ledctl = kzalloc(sizeof(struct accton_as9716_32d_led_data), GFP_KERNEL);
    if (!ledctl) {
        ret = -ENOMEM;
        platform_driver_unregister(&accton_as9716_32d_led_driver);
        goto exit;
    }

    mutex_init(&ledctl->update_lock);

    ledctl->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
    if (IS_ERR(ledctl->pdev)) {
        ret = PTR_ERR(ledctl->pdev);
        platform_driver_unregister(&accton_as9716_32d_led_driver);
        kfree(ledctl);
        goto exit;
    }

exit:
    return ret;
}

static void __exit accton_as9716_32d_led_exit(void)
{
    platform_device_unregister(ledctl->pdev);
    platform_driver_unregister(&accton_as9716_32d_led_driver);
    kfree(ledctl);
}

module_init(accton_as9716_32d_led_init);
module_exit(accton_as9716_32d_led_exit);

MODULE_AUTHOR("Jostar Yang <jostar_yang@accton.com.tw>");
MODULE_DESCRIPTION("accton_as9716_32d_led driver");
MODULE_LICENSE("GPL");

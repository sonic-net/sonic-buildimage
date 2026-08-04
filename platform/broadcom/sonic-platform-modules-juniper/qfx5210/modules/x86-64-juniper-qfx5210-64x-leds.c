/*
 * LED driver for the qfx5210
 * 
 * Modified and tested to work on Juniper QFX5210
 * Ciju Rajan K <crajank@juniper.net>
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

extern int juniper_i2c_cpld_read (u8 cpld_addr, u8 reg);
extern int juniper_i2c_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

extern void led_classdev_unregister(struct led_classdev *led_cdev);
extern void led_classdev_resume(struct led_classdev *led_cdev);
extern void led_classdev_suspend(struct led_classdev *led_cdev);

#define DRVNAME "qfx5210_64x_led"

struct qfx5210_64x_led_data {
	struct platform_device  *pdev;
	struct mutex            update_lock;
	char                    valid;		/* != 0 if registers are valid */
	unsigned long	        last_updated;	/* In jiffies */
	u8			reg_val[1];	/* only 1 register*/
};

static struct qfx5210_64x_led_data  *ledctl = NULL;

/* 
 * LED related data
 */

#define LED_CNTRLER_I2C_ADDRESS               (0x60) /* CPLD1 i2c address */
 
#define LED_TYPE_ALARM_REG_MASK               (0x03)
#define LED_MODE_ALARM_RED_VALUE              (0x01)
#define LED_MODE_ALARM_AMBER_VALUE            (0x02)
#define LED_MODE_ALARM_OFF_VALUE              (0x00)

#define LED_TYPE_SYSTEM_REG_MASK              (0x0C)
#define LED_MODE_SYSTEM_GREEN_VALUE           (0x04)
#define LED_MODE_SYSTEM_GREEN_BLINKING_VALUE  (0x08)
#define LED_MODE_SYSTEM_OFF_VALUE             (0x00)

#define LED_TYPE_MASTER_REG_MASK              (0x30)
#define LED_MODE_MASTER_GREEN_VALUE           (0x10)
#define LED_MODE_MASTER_GREEN_BLINKING_VALUE  (0x20)
#define LED_MODE_MASTER_OFF_VALUE             (0x00)

#define LED_TYPE_BEACON_REG_MASK              (0xC0)
#define LED_MODE_BEACON_VALUE                 (0x40)
#define LED_MODE_BEACON_OFF_VALUE             (0x00)

enum led_type {
	LED_TYPE_ALARM,
	LED_TYPE_SYSTEM,
	LED_TYPE_MASTER,
	LED_TYPE_BEACON
};

struct led_reg {
	u32  types;
	u8   reg_addr;
};

static const struct led_reg led_reg_map[] = {
	{(1 << LED_TYPE_ALARM) | (1 << LED_TYPE_SYSTEM) | (1 << LED_TYPE_MASTER) | (1 << LED_TYPE_BEACON), 0x30},
};

enum led_light_mode {
    LED_MODE_OFF             = 0,
    LED_MODE_RED             = 1,
    LED_MODE_AMBER           = 2,
    LED_MODE_GREEN           = 1,
    LED_MODE_GREEN_BLINKING  = 2,
    LED_MODE_BLUE_BLINKING   = 1
};

struct led_type_mode {
	enum led_type type;
	enum led_light_mode mode;	
	int  reg_bit_mask;
	int  mode_value;
};

static struct led_type_mode led_type_mode_data[] = {
	{LED_TYPE_ALARM,  LED_MODE_RED,            LED_TYPE_ALARM_REG_MASK,   LED_MODE_ALARM_RED_VALUE},
	{LED_TYPE_ALARM,  LED_MODE_AMBER,          LED_TYPE_ALARM_REG_MASK,   LED_MODE_ALARM_AMBER_VALUE},
	{LED_TYPE_ALARM,  LED_MODE_OFF,            LED_TYPE_ALARM_REG_MASK,   LED_MODE_ALARM_OFF_VALUE},

	{LED_TYPE_SYSTEM, LED_MODE_GREEN,	   LED_TYPE_SYSTEM_REG_MASK,  LED_MODE_SYSTEM_GREEN_VALUE},
	{LED_TYPE_SYSTEM, LED_MODE_GREEN_BLINKING, LED_TYPE_SYSTEM_REG_MASK,  LED_MODE_SYSTEM_GREEN_BLINKING_VALUE},
	{LED_TYPE_SYSTEM, LED_MODE_OFF,  	   LED_TYPE_SYSTEM_REG_MASK,  LED_MODE_SYSTEM_OFF_VALUE},

	{LED_TYPE_MASTER, LED_MODE_GREEN,          LED_TYPE_MASTER_REG_MASK,  LED_MODE_MASTER_GREEN_VALUE},
	{LED_TYPE_MASTER, LED_MODE_GREEN_BLINKING, LED_TYPE_MASTER_REG_MASK,  LED_MODE_MASTER_GREEN_BLINKING_VALUE},
	{LED_TYPE_MASTER, LED_MODE_OFF,            LED_TYPE_MASTER_REG_MASK,  LED_MODE_MASTER_OFF_VALUE},

	{LED_TYPE_BEACON, LED_MODE_BLUE_BLINKING,  LED_TYPE_BEACON_REG_MASK,  LED_MODE_BEACON_VALUE},
	{LED_TYPE_BEACON, LED_MODE_OFF,            LED_TYPE_BEACON_REG_MASK,  LED_MODE_BEACON_OFF_VALUE}
};
  
static int get_led_reg(enum led_type type, u8 *reg)
{	 
	int i;

	for (i = 0; i < ARRAY_SIZE(led_reg_map); i++) {	
		if(led_reg_map[i].types & (1 << type)) {
			*reg = led_reg_map[i].reg_addr;
			return 0;
		}
	}

	return 1;
}

static int led_reg_val_to_light_mode(enum led_type type, u8 reg_val)
{
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

static int qfx5210_64x_led_read_value(u8 reg)
{
	return juniper_i2c_cpld_read(LED_CNTRLER_I2C_ADDRESS, reg);
}

static int qfx5210_64x_led_write_value(u8 reg, u8 value)
{
	return juniper_i2c_cpld_write(LED_CNTRLER_I2C_ADDRESS, reg, value);
}

static void qfx5210_64x_led_update(void)
{
	mutex_lock(&ledctl->update_lock);

	if (time_after(jiffies, ledctl->last_updated + HZ + HZ / 2)
		|| !ledctl->valid) {
		int i;

		dev_dbg(&ledctl->pdev->dev, "Starting qfx5210_64x_led update\n");

		/* Update LED data
		 */
		for (i = 0; i < ARRAY_SIZE(ledctl->reg_val); i++) {
			int status = qfx5210_64x_led_read_value(led_reg_map[i].reg_addr);
			
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

static void qfx5210_64x_led_set(struct led_classdev *led_cdev,
                                enum led_brightness led_light_mode,
                                enum led_type type)
{
	int reg_val;
	u8 reg	;
	mutex_lock(&ledctl->update_lock);

	if( !get_led_reg(type, &reg)) {
		dev_dbg(&ledctl->pdev->dev, "Not match register for %d.\n", type);
	}
	
	reg_val = qfx5210_64x_led_read_value(reg);
	if (reg_val < 0) {
		dev_dbg(&ledctl->pdev->dev, "reg %d, err %d\n", reg, reg_val);
		goto exit;
	}

	reg_val = led_light_mode_to_reg_val(type, led_light_mode, reg_val);  
	qfx5210_64x_led_write_value(reg, reg_val);

	/* to prevent the slow-update issue */
	ledctl->valid = 0;

exit:
	mutex_unlock(&ledctl->update_lock);
}


static void qfx5210_64x_led_system_set(struct led_classdev *led_cdev, 
                                       enum led_brightness led_light_mode)
{
	qfx5210_64x_led_set(led_cdev, led_light_mode,  LED_TYPE_SYSTEM);
}

static enum led_brightness qfx5210_64x_led_system_get(struct led_classdev *cdev)
{
	qfx5210_64x_led_update();
	return led_reg_val_to_light_mode(LED_TYPE_SYSTEM, ledctl->reg_val[0]);
}

static void qfx5210_64x_led_master_set(struct led_classdev *led_cdev,
                                       enum led_brightness led_light_mode)
{
	qfx5210_64x_led_set(led_cdev, led_light_mode, LED_TYPE_MASTER);
}

static enum led_brightness qfx5210_64x_led_master_get(struct led_classdev *cdev)
{
	qfx5210_64x_led_update();
	return led_reg_val_to_light_mode(LED_TYPE_MASTER, ledctl->reg_val[0]);
}

static void qfx5210_64x_led_alarm_set(struct led_classdev *led_cdev,
                                      enum led_brightness led_light_mode)
{
	qfx5210_64x_led_set(led_cdev, led_light_mode, LED_TYPE_ALARM);
}

static enum led_brightness qfx5210_64x_led_alarm_get(struct led_classdev *cdev)
{
	qfx5210_64x_led_update();
	return led_reg_val_to_light_mode(LED_TYPE_ALARM, ledctl->reg_val[0]);
}

static void qfx5210_64x_led_beacon_set(struct led_classdev *led_cdev,
                                       enum led_brightness led_light_mode)
{
	qfx5210_64x_led_set(led_cdev, led_light_mode, LED_TYPE_BEACON);
}

static enum led_brightness qfx5210_64x_led_beacon_get(struct led_classdev *cdev)
{
	qfx5210_64x_led_update();
	return led_reg_val_to_light_mode(LED_TYPE_BEACON, ledctl->reg_val[0]);
}

static struct led_classdev qfx5210_64x_leds[] = {
	[LED_TYPE_ALARM] = {
		.name		 = "alarm",
		.default_trigger = "unused",
		.brightness_set	 = qfx5210_64x_led_alarm_set,
		.brightness_get  = qfx5210_64x_led_alarm_get,
		.flags		 = LED_CORE_SUSPENDRESUME,
		.max_brightness  = LED_MODE_AMBER,
	},
	[LED_TYPE_SYSTEM] = {
		.name		 = "system",
		.default_trigger = "unused",
		.brightness_set	 = qfx5210_64x_led_system_set,
		.brightness_get	 = qfx5210_64x_led_system_get,
		.flags		 = LED_CORE_SUSPENDRESUME,
		.max_brightness	 = LED_MODE_GREEN_BLINKING,
	},
	[LED_TYPE_MASTER] = {
		.name		 = "master",
		.default_trigger = "unused",
		.brightness_set	 = qfx5210_64x_led_master_set,
		.brightness_get	 = qfx5210_64x_led_master_get,
		.flags		 = LED_CORE_SUSPENDRESUME,
		.max_brightness	 = LED_MODE_GREEN_BLINKING,
	},
	[LED_TYPE_BEACON] = {
		.name            = "beacon",
		.default_trigger = "unused",
		.brightness_set	 = qfx5210_64x_led_beacon_set,
		.brightness_get  = qfx5210_64x_led_beacon_get,
		.flags		 = LED_CORE_SUSPENDRESUME,
		.max_brightness  = LED_MODE_BLUE_BLINKING,
	},
};

static int qfx5210_64x_led_suspend(struct platform_device *dev,
		pm_message_t state)
{
	int i = 0;
	
	for (i = 0; i < ARRAY_SIZE(qfx5210_64x_leds); i++) {
		led_classdev_suspend(&qfx5210_64x_leds[i]);
	}

	return 0;
}

static int qfx5210_64x_led_resume(struct platform_device *dev)
{
	int i = 0;
	
	for (i = 0; i < ARRAY_SIZE(qfx5210_64x_leds); i++) {
		led_classdev_resume(&qfx5210_64x_leds[i]);
	}

	return 0;
}

static int qfx5210_64x_led_probe(struct platform_device *pdev)
{
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(qfx5210_64x_leds); i++) {
		ret = led_classdev_register(&pdev->dev, &qfx5210_64x_leds[i]);
		
		if (ret < 0)
			break;
	}
	
	/* Check if all LEDs were successfully registered */
	if (i != ARRAY_SIZE(qfx5210_64x_leds)){
		int j;
		
		/* only unregister the LEDs that were successfully registered */
		for (j = 0; j < i; j++) {
			led_classdev_unregister(&qfx5210_64x_leds[i]);
		}
	}

	return ret;
}

static int qfx5210_64x_led_remove(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(qfx5210_64x_leds); i++) {
		led_classdev_unregister(&qfx5210_64x_leds[i]);
	}

	return 0;
}

static struct platform_driver qfx5210_64x_led_driver = {
	.probe	 = qfx5210_64x_led_probe,
	.remove	 = qfx5210_64x_led_remove,
	.suspend = qfx5210_64x_led_suspend,
	.resume	 = qfx5210_64x_led_resume,
	.driver	 = {
	.name   = DRVNAME,
	.owner  = THIS_MODULE,
	},
};

static int __init qfx5210_64x_led_init(void)
{
	int ret;

	ret = platform_driver_register(&qfx5210_64x_led_driver);
	if (ret < 0) {
		goto exit;
	}

	ledctl = kzalloc(sizeof(struct qfx5210_64x_led_data), GFP_KERNEL);
	if (!ledctl) {
		ret = -ENOMEM;
		platform_driver_unregister(&qfx5210_64x_led_driver);
		goto exit;
	}

	mutex_init(&ledctl->update_lock);

	ledctl->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
	if (IS_ERR(ledctl->pdev)) {
		ret = PTR_ERR(ledctl->pdev);
		platform_driver_unregister(&qfx5210_64x_led_driver);
		kfree(ledctl);
		goto exit;
	}

exit:
	return ret;
}

static void __exit qfx5210_64x_led_exit(void)
{
	platform_device_unregister(ledctl->pdev);
	platform_driver_unregister(&qfx5210_64x_led_driver);
	kfree(ledctl);
}

module_init(qfx5210_64x_led_init);
module_exit(qfx5210_64x_led_exit);

MODULE_AUTHOR("Ciju Rajan K <crajank@juniper.net>");
MODULE_DESCRIPTION("qfx5210_64x_led driver");
MODULE_LICENSE("GPL");


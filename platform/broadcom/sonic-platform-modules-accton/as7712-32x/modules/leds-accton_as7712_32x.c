/*
 * A LED driver for the accton_as7712_32x_led
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

extern int accton_i2c_cpld_read (unsigned short cpld_addr, u8 reg);
extern int accton_i2c_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

extern void led_classdev_unregister(struct led_classdev *led_cdev);
extern void led_classdev_resume(struct led_classdev *led_cdev);
extern void led_classdev_suspend(struct led_classdev *led_cdev);

#define DRVNAME "accton_as7712_32x_led"
#define ENABLE_PORT_LED		1

struct accton_as7712_32x_led_data {
	struct platform_device *pdev;
	struct mutex	 update_lock;
	char			 valid;		   /* != 0 if registers are valid */
	unsigned long	last_updated;	/* In jiffies */
	u8			   reg_val[1];	  /* only 1 register*/
};

static struct accton_as7712_32x_led_data  *ledctl = NULL;

/* LED related data
 */

#define LED_CNTRLER_I2C_ADDRESS		(0x60)
 
#define LED_TYPE_DIAG_REG_MASK	 	(0x3)
#define LED_MODE_DIAG_GREEN_VALUE  	(0x02)
#define LED_MODE_DIAG_RED_VALUE		(0x01)
#define LED_MODE_DIAG_AMBER_VALUE  	(0x00)  /*It's yellow actually. Green+Red=Yellow*/
#define LED_MODE_DIAG_OFF_VALUE		(0x03)

#define LED_TYPE_LOC_REG_MASK	 	(0x80)
#define LED_MODE_LOC_ON_VALUE	 	(0)
#define LED_MODE_LOC_OFF_VALUE		(0x80)

#if (ENABLE_PORT_LED == 1)
#define LED_TYPE_PORT_LED(port)	\
	LED_TYPE_PORT##port##_LED0,	\
	LED_TYPE_PORT##port##_LED1,	\
	LED_TYPE_PORT##port##_LED2,	\
	LED_TYPE_PORT##port##_LED3
#endif

enum led_type {
	LED_TYPE_DIAG,
	LED_TYPE_LOC,
	LED_TYPE_FAN,
	LED_TYPE_PSU1,
	LED_TYPE_PSU2,
#if (ENABLE_PORT_LED == 1)
	LED_TYPE_PORT_LED(0),
	LED_TYPE_PORT_LED(1),
	LED_TYPE_PORT_LED(2),
	LED_TYPE_PORT_LED(3),
	LED_TYPE_PORT_LED(4),
	LED_TYPE_PORT_LED(5),
	LED_TYPE_PORT_LED(6),
	LED_TYPE_PORT_LED(7),
	LED_TYPE_PORT_LED(8),
	LED_TYPE_PORT_LED(9),
	LED_TYPE_PORT_LED(10),
	LED_TYPE_PORT_LED(11),
	LED_TYPE_PORT_LED(12),
	LED_TYPE_PORT_LED(13),
	LED_TYPE_PORT_LED(14),
	LED_TYPE_PORT_LED(15),
	LED_TYPE_PORT_LED(16),
	LED_TYPE_PORT_LED(17),
	LED_TYPE_PORT_LED(18),
	LED_TYPE_PORT_LED(19),
	LED_TYPE_PORT_LED(20),
	LED_TYPE_PORT_LED(21),
	LED_TYPE_PORT_LED(22),
	LED_TYPE_PORT_LED(23),
	LED_TYPE_PORT_LED(24),
	LED_TYPE_PORT_LED(25),
	LED_TYPE_PORT_LED(26),
	LED_TYPE_PORT_LED(27),
	LED_TYPE_PORT_LED(28),
	LED_TYPE_PORT_LED(29),
	LED_TYPE_PORT_LED(30),
	LED_TYPE_PORT_LED(31),
#endif
};

struct led_reg {
	u32  types;
	u8   reg_addr;
};

static const struct led_reg led_reg_map[] = {
	{(1<<LED_TYPE_LOC) | (1<<LED_TYPE_DIAG), 0x41},
};

#if 0
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
#else
enum led_light_mode {
    LED_MODE_OFF,
    LED_MODE_RED 				= 10,
    LED_MODE_RED_BLINKING 		= 11,
    LED_MODE_ORANGE 			= 12,
    LED_MODE_ORANGE_BLINKING 	= 13,
    LED_MODE_YELLOW 			= 14,
    LED_MODE_YELLOW_BLINKING 	= 15,
    LED_MODE_GREEN 				= 16,
    LED_MODE_GREEN_BLINKING 	= 17,
    LED_MODE_BLUE 				= 18,
    LED_MODE_BLUE_BLINKING 		= 19,
    LED_MODE_PURPLE 			= 20,
    LED_MODE_PURPLE_BLINKING 	= 21,
    LED_MODE_AUTO 				= 22,
    LED_MODE_AUTO_BLINKING 		= 23,
    LED_MODE_WHITE 				= 24,
    LED_MODE_WHITE_BLINKING 	= 25,
    LED_MODE_CYAN 				= 26,
    LED_MODE_CYAN_BLINKING 		= 27,
};
#endif

struct led_type_mode {
	enum led_type type;
	enum led_light_mode mode;	
	int  reg_bit_mask;
	int  mode_value;
};

static struct led_type_mode led_type_mode_data[] = {
{LED_TYPE_LOC,  LED_MODE_OFF,	LED_TYPE_LOC_REG_MASK,   LED_MODE_LOC_OFF_VALUE},
{LED_TYPE_LOC,  LED_MODE_BLUE,	LED_TYPE_LOC_REG_MASK,   LED_MODE_LOC_ON_VALUE},
{LED_TYPE_DIAG, LED_MODE_OFF,   LED_TYPE_DIAG_REG_MASK,  LED_MODE_DIAG_OFF_VALUE},
{LED_TYPE_DIAG, LED_MODE_GREEN, LED_TYPE_DIAG_REG_MASK,  LED_MODE_DIAG_GREEN_VALUE},
{LED_TYPE_DIAG, LED_MODE_RED,   LED_TYPE_DIAG_REG_MASK,  LED_MODE_DIAG_RED_VALUE},
{LED_TYPE_DIAG, LED_MODE_ORANGE,LED_TYPE_DIAG_REG_MASK,  LED_MODE_DIAG_AMBER_VALUE},
};

static void accton_as7712_32x_led_set(struct led_classdev *led_cdev,
									  enum led_brightness led_light_mode, enum led_type type);

static int accton_getLedReg(enum led_type type, u8 *reg)
{	 
	int i;
	for (i = 0; i < ARRAY_SIZE(led_reg_map); i++) {	
		if(led_reg_map[i].types & (type<<1)){
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

static int accton_as7712_32x_led_read_value(u8 reg)
{
	return accton_i2c_cpld_read(LED_CNTRLER_I2C_ADDRESS, reg);
}

static int accton_as7712_32x_led_write_value(u8 reg, u8 value)
{
	return accton_i2c_cpld_write(LED_CNTRLER_I2C_ADDRESS, reg, value);
}

static void accton_as7712_32x_led_update(void)
{
	mutex_lock(&ledctl->update_lock);

	if (time_after(jiffies, ledctl->last_updated + HZ + HZ / 2)
		|| !ledctl->valid) {
		int i;

		dev_dbg(&ledctl->pdev->dev, "Starting accton_as7712_32x_led update\n");

		/* Update LED data
		 */
		for (i = 0; i < ARRAY_SIZE(ledctl->reg_val); i++) {
			int status = accton_as7712_32x_led_read_value(led_reg_map[i].reg_addr);
			
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

static void accton_as7712_32x_led_set(struct led_classdev *led_cdev,
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
	
	reg_val = accton_as7712_32x_led_read_value(reg);
	
	if (reg_val < 0) {
		dev_dbg(&ledctl->pdev->dev, "reg %d, err %d\n", reg, reg_val);
		goto exit;
	}
	reg_val = led_light_mode_to_reg_val(type, led_light_mode, reg_val);  
	accton_as7712_32x_led_write_value(reg, reg_val);

	/* to prevent the slow-update issue */
	ledctl->valid = 0;

exit:
	mutex_unlock(&ledctl->update_lock);
}


static void accton_as7712_32x_led_diag_set(struct led_classdev *led_cdev,
										   enum led_brightness led_light_mode)
{
	accton_as7712_32x_led_set(led_cdev, led_light_mode,  LED_TYPE_DIAG);
}

static enum led_brightness accton_as7712_32x_led_diag_get(struct led_classdev *cdev)
{
	accton_as7712_32x_led_update();
	return led_reg_val_to_light_mode(LED_TYPE_DIAG, ledctl->reg_val[0]);
}

static void accton_as7712_32x_led_loc_set(struct led_classdev *led_cdev,
										  enum led_brightness led_light_mode)
{
	accton_as7712_32x_led_set(led_cdev, led_light_mode, LED_TYPE_LOC);
}

static enum led_brightness accton_as7712_32x_led_loc_get(struct led_classdev *cdev)
{
	accton_as7712_32x_led_update();
	return led_reg_val_to_light_mode(LED_TYPE_LOC, ledctl->reg_val[0]);
}

static void accton_as7712_32x_led_auto_set(struct led_classdev *led_cdev,
										   enum led_brightness led_light_mode)
{
}

static enum led_brightness accton_as7712_32x_led_auto_get(struct led_classdev *cdev)
{
	return LED_MODE_AUTO;
}

#if (ENABLE_PORT_LED == 1)
#define PORT_LED_COLOR_MASK		(0x7 << 2)
#define PORT_LED_COLOR1_REG_VAL (0x0 << 2)
#define PORT_LED_COLOR2_REG_VAL (0x1 << 2)
#define PORT_LED_COLOR3_REG_VAL (0x2 << 2)
#define PORT_LED_COLOR4_REG_VAL (0x3 << 2)
#define PORT_LED_COLOR5_REG_VAL (0x4 << 2)
#define PORT_LED_COLOR6_REG_VAL (0x5 << 2)
#define PORT_LED_COLOR7_REG_VAL (0x6 << 2)
#define PORT_LED_COLOR8_REG_VAL (0x7 << 2)

static int accton_as7712_32x_port_led_read_value(unsigned short cpld_addr, u8 reg)
{
	return accton_i2c_cpld_read(cpld_addr, reg);
}

static int accton_as7712_32x_port_led_write_value(unsigned short cpld_addr, u8 reg, u8 value)
{
	return accton_i2c_cpld_write(cpld_addr, reg, value);
}

static int port_led_mode_to_cpld_val(int mode)
{
	u8 color    = 0;
	u8 blinking = 0;
	u8 on       = 1 << 0;

	switch (mode) {
		case LED_MODE_WHITE_BLINKING:  blinking = 1 << 1; /* fall through */
		case LED_MODE_WHITE:  color = 0x0 << 2;
			break;
		case LED_MODE_YELLOW_BLINKING: blinking = 1 << 1; /* fall through */
		case LED_MODE_YELLOW: color = 0x1 << 2;
			break;
		case LED_MODE_ORANGE_BLINKING: blinking = 1 << 1; /* fall through */
		case LED_MODE_ORANGE: color = 0x2 << 2;
			break;
		case LED_MODE_PURPLE_BLINKING: blinking = 1 << 1; /* fall through */
		case LED_MODE_PURPLE: color = 0x3 << 2;
			break;
		case LED_MODE_CYAN_BLINKING:   blinking = 1 << 1; /* fall through */
		case LED_MODE_CYAN:   color = 0x4 << 2;
			break;
		case LED_MODE_RED_BLINKING:    blinking = 1 << 1; /* fall through */
		case LED_MODE_RED:    color = 0x5 << 2;
			break;
		case LED_MODE_GREEN_BLINKING:  blinking = 1 << 1; /* fall through */
		case LED_MODE_GREEN:  color = 0x6 << 2;
			break;	
		case LED_MODE_BLUE_BLINKING:   blinking = 1 << 1; /* fall through */
		case LED_MODE_BLUE:   color = 0x7 << 2;
			break;
		case LED_MODE_OFF:    on = 0 << 0;
			break;
		default:
			return -EINVAL;
	}

	return (color | blinking | on);
}

static int cpld_val_to_port_led_mode(uint8_t value)
{
	int on 		 = (value & 0x1);
	int blinking = (value & 0x2);
	int color    = (value & PORT_LED_COLOR_MASK) ;

	if (!on) {
		return LED_MODE_OFF;
	}

	switch (color) {
		case PORT_LED_COLOR1_REG_VAL:
			return blinking ? LED_MODE_WHITE_BLINKING : LED_MODE_WHITE;
		case PORT_LED_COLOR2_REG_VAL:
			return blinking ? LED_MODE_YELLOW_BLINKING : LED_MODE_YELLOW;
		case PORT_LED_COLOR3_REG_VAL:
			return blinking ? LED_MODE_ORANGE_BLINKING : LED_MODE_ORANGE;
		case PORT_LED_COLOR4_REG_VAL:
			return blinking ? LED_MODE_PURPLE_BLINKING : LED_MODE_PURPLE;
		case PORT_LED_COLOR5_REG_VAL:
			return blinking ? LED_MODE_CYAN_BLINKING : LED_MODE_CYAN;
		case PORT_LED_COLOR6_REG_VAL:
			return blinking ? LED_MODE_RED_BLINKING : LED_MODE_RED;
		case PORT_LED_COLOR7_REG_VAL:
			return blinking ? LED_MODE_GREEN_BLINKING : LED_MODE_GREEN;
		case PORT_LED_COLOR8_REG_VAL:
			return blinking ? LED_MODE_BLUE_BLINKING : LED_MODE_BLUE;
		default:
			return -EINVAL;;
	}
}


static void accton_as7712_32x_port_led_set(struct led_classdev *cdev,
										   enum led_brightness led_light_mode)
{
	unsigned int port, lid;
	unsigned short cpld_addr;
	u8 reg, value;
	sscanf(cdev->name, "accton_as7712_32x_led::port%u_led%u", &port, &lid);

	if (port > 32 || lid > 4) {
		dev_dbg(&ledctl->pdev->dev, "Port(%u), Led_id(%u) not match\n", port, lid);
		return;
	}

	cpld_addr = (port < 16) ? 0x64 : 0x62;
	reg       = (0x50 + (port % 16) * 4 + lid);
	value	  = port_led_mode_to_cpld_val(led_light_mode);

	if (value < 0) {
		dev_dbg(&ledctl->pdev->dev, "Unknow port led mode(%d)\n", led_light_mode);
		return;
	}

	accton_as7712_32x_port_led_write_value(cpld_addr, reg, value);
}

static enum led_brightness accton_as7712_32x_port_led_get(struct led_classdev *cdev)
{
	unsigned int port, lid;
	unsigned short cpld_addr;
	u8 reg, value;
	sscanf(cdev->name, "accton_as7712_32x_led::port%u_led%u", &port, &lid);

	if (port > 32 || lid > 4) {
		dev_dbg(&ledctl->pdev->dev, "Port(%u), Led_id(%u) not match\n", port, lid);
		return -EINVAL;
	}

	cpld_addr = (port < 16) ? 0x64 : 0x62;
	reg       = (0x50 + (port % 16) * 4 + lid);
	value 	  = accton_as7712_32x_port_led_read_value(cpld_addr, reg);
	
	if (value < 0) {
		dev_dbg(&ledctl->pdev->dev, "Unable to read reg value from cpld(0x%x), reg(0x%x)\n", cpld_addr, reg);
		return value;
	}

	return cpld_val_to_port_led_mode(value);
}

#define _PORT_LED_CLASSDEV(port, lid)									\
	[LED_TYPE_PORT##port##_LED##lid] = {								\
		.name			 = "accton_as7712_32x_led::port"#port"_led"#lid,\
		.default_trigger = "unused",									\
		.brightness_set	 = accton_as7712_32x_port_led_set,				\
		.brightness_get	 = accton_as7712_32x_port_led_get,				\
		.max_brightness	 = LED_MODE_CYAN_BLINKING,						\
	}

#define PORT_LED_CLASSDEV(port)	\
	_PORT_LED_CLASSDEV(port, 0),\
	_PORT_LED_CLASSDEV(port, 1),\
	_PORT_LED_CLASSDEV(port, 2),\
	_PORT_LED_CLASSDEV(port, 3)
#endif

static struct led_classdev accton_as7712_32x_leds[] = {
	[LED_TYPE_DIAG] = {
		.name			 = "accton_as7712_32x_led::diag",
		.default_trigger = "unused",
		.brightness_set	 = accton_as7712_32x_led_diag_set,
		.brightness_get	 = accton_as7712_32x_led_diag_get,
		.flags			 = LED_CORE_SUSPENDRESUME,
		.max_brightness	 = LED_MODE_RED,
	},
	[LED_TYPE_LOC] = {
		.name			 = "accton_as7712_32x_led::loc",
		.default_trigger = "unused",
		.brightness_set	 = accton_as7712_32x_led_loc_set,
		.brightness_get	 = accton_as7712_32x_led_loc_get,
		.flags			 = LED_CORE_SUSPENDRESUME,
		.max_brightness	 = LED_MODE_BLUE,
	},
	[LED_TYPE_FAN] = {
		.name			 = "accton_as7712_32x_led::fan",
		.default_trigger = "unused",
		.brightness_set	 = accton_as7712_32x_led_auto_set,
		.brightness_get  = accton_as7712_32x_led_auto_get,
		.flags			 = LED_CORE_SUSPENDRESUME,
		.max_brightness  = LED_MODE_AUTO,
	},
	[LED_TYPE_PSU1] = {
		.name			 = "accton_as7712_32x_led::psu1",
		.default_trigger = "unused",
		.brightness_set	 = accton_as7712_32x_led_auto_set,
		.brightness_get  = accton_as7712_32x_led_auto_get,
		.flags			 = LED_CORE_SUSPENDRESUME,
		.max_brightness  = LED_MODE_AUTO,
	},
	[LED_TYPE_PSU2] = {
		.name			 = "accton_as7712_32x_led::psu2",
		.default_trigger = "unused",
		.brightness_set	 = accton_as7712_32x_led_auto_set,
		.brightness_get  = accton_as7712_32x_led_auto_get,
		.flags			 = LED_CORE_SUSPENDRESUME,
		.max_brightness  = LED_MODE_AUTO,
	},
#if (ENABLE_PORT_LED == 1)
	PORT_LED_CLASSDEV(0),
	PORT_LED_CLASSDEV(1),
	PORT_LED_CLASSDEV(2),
	PORT_LED_CLASSDEV(3),
	PORT_LED_CLASSDEV(4),
	PORT_LED_CLASSDEV(5),
	PORT_LED_CLASSDEV(6),
	PORT_LED_CLASSDEV(7),
	PORT_LED_CLASSDEV(8),
	PORT_LED_CLASSDEV(9),
	PORT_LED_CLASSDEV(10),
	PORT_LED_CLASSDEV(11),
	PORT_LED_CLASSDEV(12),
	PORT_LED_CLASSDEV(13),
	PORT_LED_CLASSDEV(14),
	PORT_LED_CLASSDEV(15),
	PORT_LED_CLASSDEV(16),
	PORT_LED_CLASSDEV(17),
	PORT_LED_CLASSDEV(18),
	PORT_LED_CLASSDEV(19),
	PORT_LED_CLASSDEV(20),
	PORT_LED_CLASSDEV(21),
	PORT_LED_CLASSDEV(22),
	PORT_LED_CLASSDEV(23),
	PORT_LED_CLASSDEV(24),
	PORT_LED_CLASSDEV(25),
	PORT_LED_CLASSDEV(26),
	PORT_LED_CLASSDEV(27),
	PORT_LED_CLASSDEV(28),
	PORT_LED_CLASSDEV(29),
	PORT_LED_CLASSDEV(30),
	PORT_LED_CLASSDEV(31),
#endif
};

static int accton_as7712_32x_led_suspend(struct platform_device *dev,
		pm_message_t state)
{
	int i = 0;
	
	for (i = 0; i < ARRAY_SIZE(accton_as7712_32x_leds); i++) {
		led_classdev_suspend(&accton_as7712_32x_leds[i]);
	}

	return 0;
}

static int accton_as7712_32x_led_resume(struct platform_device *dev)
{
	int i = 0;
	
	for (i = 0; i < ARRAY_SIZE(accton_as7712_32x_leds); i++) {
		led_classdev_resume(&accton_as7712_32x_leds[i]);
	}

	return 0;
}

static int accton_as7712_32x_led_probe(struct platform_device *pdev)
{
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(accton_as7712_32x_leds); i++) {
		ret = led_classdev_register(&pdev->dev, &accton_as7712_32x_leds[i]);
		
		if (ret < 0)
			break;
	}
	
	/* Check if all LEDs were successfully registered */
	if (i != ARRAY_SIZE(accton_as7712_32x_leds)){
		int j;
		
		/* only unregister the LEDs that were successfully registered */
		for (j = 0; j < i; j++) {
			led_classdev_unregister(&accton_as7712_32x_leds[i]);
		}
	}

	return ret;
}

static void accton_as7712_32x_led_remove(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(accton_as7712_32x_leds); i++) {
		led_classdev_unregister(&accton_as7712_32x_leds[i]);
	}
}

static struct platform_driver accton_as7712_32x_led_driver = {
	.probe	  = accton_as7712_32x_led_probe,
	.remove	 = accton_as7712_32x_led_remove,
	.suspend	= accton_as7712_32x_led_suspend,
	.resume	 = accton_as7712_32x_led_resume,
	.driver	 = {
	.name   = DRVNAME,
	.owner  = THIS_MODULE,
	},
};

static int __init accton_as7712_32x_led_init(void)
{
	int ret;

	ret = platform_driver_register(&accton_as7712_32x_led_driver);
	if (ret < 0) {
		goto exit;
	}

	ledctl = kzalloc(sizeof(struct accton_as7712_32x_led_data), GFP_KERNEL);
	if (!ledctl) {
		ret = -ENOMEM;
		platform_driver_unregister(&accton_as7712_32x_led_driver);
		goto exit;
	}

	mutex_init(&ledctl->update_lock);

	ledctl->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
	if (IS_ERR(ledctl->pdev)) {
		ret = PTR_ERR(ledctl->pdev);
		platform_driver_unregister(&accton_as7712_32x_led_driver);
		kfree(ledctl);
		goto exit;
	}

exit:
	return ret;
}

static void __exit accton_as7712_32x_led_exit(void)
{
	platform_device_unregister(ledctl->pdev);
	platform_driver_unregister(&accton_as7712_32x_led_driver);
	kfree(ledctl);
}

module_init(accton_as7712_32x_led_init);
module_exit(accton_as7712_32x_led_exit);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("accton_as7712_32x_led driver");
MODULE_LICENSE("GPL");

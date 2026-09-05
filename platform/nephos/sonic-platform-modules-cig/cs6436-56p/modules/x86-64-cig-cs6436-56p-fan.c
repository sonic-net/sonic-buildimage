/*
 * A hwmon driver for the CIG cs6436-56P fan
 *
 * Copyright (C) 2018 Cambridge, Inc.
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
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/kthread.h>
#include <linux/device.h>
#include <linux/platform_device.h>



#define  FAN_SPEED_DUTY_TO_CPLD_STEP 10 

static struct cs6436_56p_fan_data *cs6436_56p_fan_update_device(struct device *dev);
static ssize_t fan_show_value(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_duty_cycle(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count);
static ssize_t set_fan_direction(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count);


extern  int cig_cpld_write_register(u8 reg_off, u8 val);
extern int cig_cpld_read_register(u8 reg_off, u8 *val);

/* fan related data, the index should match sysfs_fan_attributes
 */
static const u8 fan_reg[] = {
    0x41,       /*  fan enable/disable */
    0x40,       /* fan PWM(for all fan) */
    0x42,       /* front fan 1 speed(rpm) */
    0x44,       /* front fan 2 speed(rpm) */
    0x46,       /* front fan 3 speed(rpm) */
    0x48,       /* front fan 4 speed(rpm) */
    0x4a,       /* front fan 5 speed(rpm) */
    0x43,       /* rear fan 1 speed(rpm) */
    0x45,       /* rear fan 2 speed(rpm) */
    0x47,       /* rear fan 3 speed(rpm) */
    0x49,       /* rear fan 4 speed(rpm) */
    0x4b,       /* rear fan 5 speed(rpm) */
    0x4c,       /* fan direction rear to front or front to rear */
};


/* Each client has this additional data */
struct cs6436_56p_fan_data {
    struct platform_device *pdev;
    struct device   *hwmon_dev;
    struct mutex     update_lock;
    char             valid;           /* != 0 if registers are valid */
    unsigned long    last_updated;    /* In jiffies */
    u8               reg_val[ARRAY_SIZE(fan_reg)]; /* Register value */
};

static struct cs6436_56p_fan_data  *fan_data = NULL;

enum fan_id {
    FAN1_ID,
    FAN2_ID,
    FAN3_ID,
    FAN4_ID,
    FAN5_ID,
};

enum sysfs_fan_attributes {
    FAN_STATE_REG,
    FAN_DUTY_CYCLE_PERCENTAGE, /* Only one CPLD register to control duty cycle for all fans */
    FAN1_FRONT_SPEED_RPM,
    FAN2_FRONT_SPEED_RPM,
    FAN3_FRONT_SPEED_RPM,
    FAN4_FRONT_SPEED_RPM,
    FAN5_FRONT_SPEED_RPM,
    FAN1_REAR_SPEED_RPM,
    FAN2_REAR_SPEED_RPM,
    FAN3_REAR_SPEED_RPM,
    FAN4_REAR_SPEED_RPM,
    FAN5_REAR_SPEED_RPM,
    FAN_DIRECTION,
    FAN1_STATE,
    FAN2_STATE,
    FAN3_STATE,
    FAN4_STATE,
    FAN5_STATE,
    FAN1_FAULT,
    FAN2_FAULT,
    FAN3_FAULT,
    FAN4_FAULT,
    FAN5_FAULT,
	FAN1_DIRECTION,
	FAN2_DIRECTION,
	FAN3_DIRECTION,
	FAN4_DIRECTION,
	FAN5_DIRECTION,
};

/* Define attributes
 */
#define DECLARE_FAN_STATE_SENSOR_DEV_ATTR(index) \
	static SENSOR_DEVICE_ATTR(fan##index##_state, S_IRUGO, fan_show_value, NULL, FAN##index##_STATE)
#define DECLARE_FAN_STATE_ATTR(index)	  &sensor_dev_attr_fan##index##_state.dev_attr.attr

#define DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_fault, S_IRUGO, fan_show_value, NULL, FAN##index##_FAULT)
#define DECLARE_FAN_FAULT_ATTR(index)      &sensor_dev_attr_fan##index##_fault.dev_attr.attr

#define DECLARE_FAN_DUTY_CYCLE_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_duty_cycle_percentage, S_IWUSR | S_IRUGO, fan_show_value, set_duty_cycle, FAN##index##_DUTY_CYCLE_PERCENTAGE)
#define DECLARE_FAN_DUTY_CYCLE_ATTR(index) &sensor_dev_attr_fan##index##_duty_cycle_percentage.dev_attr.attr

#define DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_front_speed_rpm, S_IRUGO, fan_show_value, NULL, FAN##index##_FRONT_SPEED_RPM);\
    static SENSOR_DEVICE_ATTR(fan##index##_rear_speed_rpm, S_IRUGO, fan_show_value, NULL, FAN##index##_REAR_SPEED_RPM)
#define DECLARE_FAN_SPEED_RPM_ATTR(index)  &sensor_dev_attr_fan##index##_front_speed_rpm.dev_attr.attr, \
                                           &sensor_dev_attr_fan##index##_rear_speed_rpm.dev_attr.attr

#define DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(index) \
	static SENSOR_DEVICE_ATTR(fan##index##_direction, S_IWUSR | S_IRUGO, fan_show_value, set_fan_direction, FAN##index##_DIRECTION)
#define DECLARE_FAN_DIRECTION_ATTR(index)	  &sensor_dev_attr_fan##index##_direction.dev_attr.attr


/* 5 fan state attributes in this platform */
DECLARE_FAN_STATE_SENSOR_DEV_ATTR(1);
DECLARE_FAN_STATE_SENSOR_DEV_ATTR(2);
DECLARE_FAN_STATE_SENSOR_DEV_ATTR(3);
DECLARE_FAN_STATE_SENSOR_DEV_ATTR(4);
DECLARE_FAN_STATE_SENSOR_DEV_ATTR(5);


/* 5 fan fault attributes in this platform */
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(1);
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(2);
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(3);
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(4);
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(5);

/* 5 fan speed(rpm) attributes in this platform */
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(1);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(2);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(3);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(4);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(5);

/* 1 fan duty cycle attribute in this platform */
DECLARE_FAN_DUTY_CYCLE_SENSOR_DEV_ATTR();

DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(1);
DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(2);
DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(3);
DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(4);
DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(5);


static struct attribute *cs6436_56p_fan_attributes[] = {
    /* fan related attributes */
    DECLARE_FAN_STATE_ATTR(1),
    DECLARE_FAN_STATE_ATTR(2),
    DECLARE_FAN_STATE_ATTR(3),
    DECLARE_FAN_STATE_ATTR(4),
    DECLARE_FAN_STATE_ATTR(5),
    DECLARE_FAN_FAULT_ATTR(1),
    DECLARE_FAN_FAULT_ATTR(2),
    DECLARE_FAN_FAULT_ATTR(3),
    DECLARE_FAN_FAULT_ATTR(4),
    DECLARE_FAN_FAULT_ATTR(5),
    DECLARE_FAN_SPEED_RPM_ATTR(1),
    DECLARE_FAN_SPEED_RPM_ATTR(2),
    DECLARE_FAN_SPEED_RPM_ATTR(3),
    DECLARE_FAN_SPEED_RPM_ATTR(4),
    DECLARE_FAN_SPEED_RPM_ATTR(5),
    DECLARE_FAN_DUTY_CYCLE_ATTR(),
	DECLARE_FAN_DIRECTION_ATTR(1),
	DECLARE_FAN_DIRECTION_ATTR(2),
	DECLARE_FAN_DIRECTION_ATTR(3),
	DECLARE_FAN_DIRECTION_ATTR(4),
	DECLARE_FAN_DIRECTION_ATTR(5),
    NULL
};

#define FAN_MAX_DUTY_CYCLE              100
#define FAN_REG_VAL_TO_SPEED_RPM_STEP   100

/* fan utility functions
 */
static u32 reg_val_to_duty_cycle(u8 reg_val)
{
	if (reg_val 
== 0xFF) {
		return 100;
	}
	return ((u32)(reg_val) * 100)/ 255;
}

static u8 duty_cycle_to_reg_val(u8 duty_cycle)
{
	if (duty_cycle >= FAN_MAX_DUTY_CYCLE) {
		return 0xFF;
	}

	return 255 / 10 * (duty_cycle / 10);
}

static u32 reg_val_to_speed_rpm(u8 reg_val)
{
    return (u32)reg_val * FAN_REG_VAL_TO_SPEED_RPM_STEP;
}

static u8 reg_val_to_is_state(u8 reg_val, enum fan_id id)
{
    u8 mask = (1 << id);

    reg_val &= mask;

    return reg_val ? 0 : 1;
}

static u8 is_fan_fault(struct cs6436_56p_fan_data *data, enum fan_id id)
{
    u8 ret = 1;
    int front_fan_index = FAN1_FRONT_SPEED_RPM + id;
    int rear_fan_index  = FAN1_REAR_SPEED_RPM  + id;

    /* Check if the speed of front or rear fan is ZERO,
     */
    if (reg_val_to_speed_rpm(data->reg_val[front_fan_index]) &&
            reg_val_to_speed_rpm(data->reg_val[rear_fan_index]))  {
        ret = 0;
    }

    return ret;
}


static ssize_t set_duty_cycle(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count)
{
    int error, value;

    error = kstrtoint(buf, 10, &value);
    if (error)
        return error;

    if (value <= 0 || value > FAN_MAX_DUTY_CYCLE)
        return -EINVAL;

    cig_cpld_write_register(fan_reg[FAN_DUTY_CYCLE_PERCENTAGE], duty_cycle_to_reg_val(value));
    
    return count;
}

static ssize_t set_fan_direction(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count)
{
    int error, value,fan_index;
	u8 mask,reg_val;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	fan_index = attr->index - FAN1_DIRECTION;
    error = kstrtoint(buf, 10, &value);
    if (error)
        return error;
	
    if (!(value == 0 || value == 1))
        return -EINVAL;
		

	cig_cpld_read_register(fan_reg[FAN_DIRECTION],&reg_val);

	if(value == 1)
	{
		reg_val |= (1 << fan_index);
	}
	else
	{
		reg_val &= ~(1 << fan_index);
	}

    cig_cpld_write_register(fan_reg[FAN_DIRECTION], reg_val);
    
    return count;
}

							  

						  

static ssize_t fan_show_value(struct device *dev, struct device_attribute *da,
                              char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    cs6436_56p_fan_update_device(dev);
	
    struct cs6436_56p_fan_data *data = fan_data;
	
    ssize_t ret = 0;

    if (data->valid) {
	switch (attr->index) {
			
	    case FAN1_STATE:
	    case FAN2_STATE:
	    case FAN3_STATE:
	    case FAN4_STATE:
	    case FAN5_STATE:
 //printk("FAN_STATE_REG: 0x%x\n", data->reg_val[FAN_STATE_REG]);
 //printk("index: %d\n", attr->index);
	        ret = sprintf(buf, "%d\n",
	                      reg_val_to_is_state(data->reg_val[FAN_STATE_REG],
	                      attr->index - FAN1_STATE));
	        break;
	    case FAN_DUTY_CYCLE_PERCENTAGE:
	    {
	        u32 duty_cycle = reg_val_to_duty_cycle(data->reg_val[FAN_DUTY_CYCLE_PERCENTAGE]);
	        ret = sprintf(buf, "%u\n", duty_cycle);
	        break;
	    }
	    case FAN1_FRONT_SPEED_RPM:
	    case FAN2_FRONT_SPEED_RPM:
	    case FAN3_FRONT_SPEED_RPM:
	    case FAN4_FRONT_SPEED_RPM:
	    case FAN5_FRONT_SPEED_RPM:
	    case FAN1_REAR_SPEED_RPM:
	    case FAN2_REAR_SPEED_RPM:
	    case FAN3_REAR_SPEED_RPM:
	    case FAN4_REAR_SPEED_RPM:
	    case FAN5_REAR_SPEED_RPM:
// printk("FAN_seed_REG: 0x%x\n", data->reg_val[attr->index]);
// printk("index: %d\n", attr->index);
	        ret = sprintf(buf, "%u\n", reg_val_to_speed_rpm(data->reg_val[attr->index]));
	        break;

	    case FAN1_FAULT:
	    case FAN2_FAULT:
	    case FAN3_FAULT:
	    case FAN4_FAULT:
	    case FAN5_FAULT:
	        ret = sprintf(buf, "%d\n", is_fan_fault(data, attr->index - FAN1_FAULT));
	        break;
		case FAN1_DIRECTION:
	    case FAN2_DIRECTION:
	    case FAN3_DIRECTION:
	    case FAN4_DIRECTION:
	    case FAN5_DIRECTION:
	       	ret = sprintf(buf, "%d\n",reg_val_to_is_state(data->reg_val[FAN_DIRECTION],attr->index - FAN1_DIRECTION));
	        break;
	    default:
	        break;
        }
    }

    return ret;
}

static const struct attribute_group cs6436_56p_fan_group = {
    .attrs = cs6436_56p_fan_attributes,
};

static struct cs6436_56p_fan_data *cs6436_56p_fan_update_device(struct device *dev)
{
    struct cs6436_56p_fan_data *data = fan_data;

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + HZ + HZ / 2) ||
            !data->valid) {
        int i;

        data->valid = 0;

        /* Update fan data
         */
        for (i = 0; i < ARRAY_SIZE(data->reg_val); i++) {
            u8 status;
            (void)cig_cpld_read_register(fan_reg[i], &status);

            if (status < 0) {
                data->valid = 0;
                mutex_unlock(&data->update_lock);
                return data;
            }
            else {
                data->reg_val[i] = status;
            }
        }

        data->last_updated = jiffies;
        data->valid = 1;
    }

    mutex_unlock(&data->update_lock);

    return data;
}

static int cs6436_56p_fan_probe(struct platform_device *pdev)
{
    int status = -1;
    /* Register sysfs hooks */
    status = sysfs_create_group(&pdev->dev.kobj, &cs6436_56p_fan_group);
    if (status) {
        goto exit;

    }
    
	fan_data->hwmon_dev = hwmon_device_register(&pdev->dev);
	if (IS_ERR(fan_data->hwmon_dev)) {
		status = PTR_ERR(fan_data->hwmon_dev);
		goto exit_remove;
	}

    dev_info(&pdev->dev, "cs6436_56p_fan\n");
    
    return 0;
    
exit_remove:
    sysfs_remove_group(&pdev->dev.kobj, &cs6436_56p_fan_group);
exit:
    return status;
}

static int cs6436_56p_fan_remove(struct platform_device *pdev)
{
    hwmon_device_unregister(fan_data->hwmon_dev);
    sysfs_remove_group(&fan_data->pdev->dev.kobj, &cs6436_56p_fan_group);
    
    return 0;
}

#define DRVNAME "cs6436_56p_fan"

static struct platform_driver cs6436_56p_fan_driver = {
    .probe      = cs6436_56p_fan_probe,
    .remove     = cs6436_56p_fan_remove,
    .driver     = {
        .name   = DRVNAME,
        .owner  = THIS_MODULE,
    },
};





static int __init cs6436_56p_fan_init(void)
{
    int ret;

    cig_cpld_write_register(0x40, duty_cycle_to_reg_val(50));

    ret = platform_driver_register(&cs6436_56p_fan_driver);
    if (ret < 0) {
        goto exit;
    }

    fan_data = kzalloc(sizeof(struct cs6436_56p_fan_data), GFP_KERNEL);
    if (!fan_data) {
        ret = -ENOMEM;
        platform_driver_unregister(&cs6436_56p_fan_driver);
        goto exit;
    }

	mutex_init(&fan_data->update_lock);
    fan_data->valid = 0;
	
    fan_data->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
    if (IS_ERR(fan_data->pdev)) {
        ret = PTR_ERR(fan_data->pdev);
        platform_driver_unregister(&cs6436_56p_fan_driver);
        kfree(fan_data);
        goto exit;
    }

exit:
    return ret;
}

static void __exit cs6436_56p_fan_exit(void)
{
    platform_device_unregister(fan_data->pdev);
    platform_driver_unregister(&cs6436_56p_fan_driver);
    kfree(fan_data);
}

MODULE_AUTHOR("CIG");
MODULE_DESCRIPTION("cs6436_56p_fan driver");
MODULE_LICENSE("GPL");

module_init(cs6436_56p_fan_init);
module_exit(cs6436_56p_fan_exit);

MODULE_AUTHOR("Zhang Peng <zhangpeng@cigtech.com>");
MODULE_DESCRIPTION("cs6436_56p_fan driver");
MODULE_LICENSE("GPL");


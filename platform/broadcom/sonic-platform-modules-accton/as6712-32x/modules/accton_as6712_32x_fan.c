/*
 * A hwmon driver for the Accton as6712 32x fan contrl
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

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/kthread.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#define DRVNAME "as6712_32x_fan"

#define FAN_MAX_NUMBER                   5
#define FAN_SPEED_CPLD_TO_RPM_STEP       150
#define FAN_SPEED_PRECENT_TO_CPLD_STEP   5
#define FAN_DUTY_CYCLE_MIN               0   /* 10% ??*/
#define FAN_DUTY_CYCLE_MAX               100  /* 100% */

#define CPLD_REG_FAN_STATUS_OFFSET        0xC
#define CPLD_REG_FANR_STATUS_OFFSET       0x17
#define CPLD_REG_FAN_DIRECTION_OFFSET     0x1E

#define CPLD_FAN1_REG_SPEED_OFFSET       0x10
#define CPLD_FAN2_REG_SPEED_OFFSET       0x11
#define CPLD_FAN3_REG_SPEED_OFFSET       0x12
#define CPLD_FAN4_REG_SPEED_OFFSET       0x13
#define CPLD_FAN5_REG_SPEED_OFFSET       0x14

#define CPLD_FANR1_REG_SPEED_OFFSET      0x18
#define CPLD_FANR2_REG_SPEED_OFFSET      0x19
#define CPLD_FANR3_REG_SPEED_OFFSET      0x1A
#define CPLD_FANR4_REG_SPEED_OFFSET      0x1B
#define CPLD_FANR5_REG_SPEED_OFFSET      0x1C

#define CPLD_REG_FAN_PWM_CYCLE_OFFSET      0xD

#define CPLD_FAN1_INFO_BIT_MASK           0x1
#define CPLD_FAN2_INFO_BIT_MASK           0x2
#define CPLD_FAN3_INFO_BIT_MASK           0x4
#define CPLD_FAN4_INFO_BIT_MASK           0x8
#define CPLD_FAN5_INFO_BIT_MASK           0x10

#define PROJECT_NAME                      

#define DEBUG_MODE 0

#if (DEBUG_MODE == 1)
    #define DEBUG_PRINT(format, ...)   printk(format, __VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)  
#endif

static struct accton_as6712_32x_fan  *fan_data = NULL;

struct accton_as6712_32x_fan {
    struct platform_device *pdev;
    struct device   *hwmon_dev;
    struct mutex     update_lock;
    char             valid;           /* != 0 if registers are valid */
    unsigned long    last_updated;    /* In jiffies */
    u8               status[FAN_MAX_NUMBER];     /* inner first fan status */
    u32              speed[FAN_MAX_NUMBER];      /* inner first fan speed */
    u8               direction[FAN_MAX_NUMBER];  /* reconrd the direction of inner first and second fans */
    u32              duty_cycle[FAN_MAX_NUMBER]; /* control the speed of inner first and second fans */
    u8               r_status[FAN_MAX_NUMBER];   /* inner second fan status */
    u32              r_speed[FAN_MAX_NUMBER];    /* inner second fan speed */
};

/*******************/
#define MAKE_FAN_MASK_OR_REG(name,type) \
    CPLD_FAN##type##1_##name,      \
    CPLD_FAN##type##2_##name,      \
    CPLD_FAN##type##3_##name,      \
    CPLD_FAN##type##4_##name,      \
    CPLD_FAN##type##5_##name,          

/* fan related data
 */
static const u8 fan_info_mask[] = {
    MAKE_FAN_MASK_OR_REG(INFO_BIT_MASK,)
};

static const u8 fan_speed_reg[] = {
    MAKE_FAN_MASK_OR_REG(REG_SPEED_OFFSET,)
};

static const u8 fanr_speed_reg[] = {
    MAKE_FAN_MASK_OR_REG(REG_SPEED_OFFSET,R)
};

/*******************/
#define DEF_FAN_SET(id) \
    FAN##id##_FAULT,      \
    FAN##id##_SPEED,      \
    FAN##id##_DUTY_CYCLE, \
    FAN##id##_DIRECTION, \
    FANR##id##_FAULT,    \
    FANR##id##_SPEED,
    
enum sysfs_fan_attributes {
    DEF_FAN_SET(1)
    DEF_FAN_SET(2)
    DEF_FAN_SET(3)
    DEF_FAN_SET(4)
    DEF_FAN_SET(5)
};
/*******************/
static void accton_as6712_32x_fan_update_device(struct device *dev);
static int accton_as6712_32x_fan_read_value(u8 reg);
static int accton_as6712_32x_fan_write_value(u8 reg, u8 value);
                                             
static ssize_t fan_set_duty_cycle(struct device *dev, 
                    struct device_attribute *da,const char *buf, size_t count);
static ssize_t fan_show_value(struct device *dev, 
                    struct device_attribute *da, char *buf);
static ssize_t show_name(struct device *dev, 
                    struct device_attribute *da, char *buf);

extern int as6712_32x_cpld_read(unsigned short cpld_addr, u8 reg);
extern int as6712_32x_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

                    
/*******************/
#define _MAKE_SENSOR_DEVICE_ATTR(prj, id, id2) \
    static SENSOR_DEVICE_ATTR(prj##fan##id##_speed_rpm, S_IRUGO, fan_show_value, NULL, FAN##id##_SPEED); \
    static SENSOR_DEVICE_ATTR(prj##fan##id##_duty_cycle_percentage, S_IWUSR | S_IRUGO, fan_show_value,          \
                                            fan_set_duty_cycle, FAN##id##_DUTY_CYCLE);          \
    static SENSOR_DEVICE_ATTR(prj##fan##id##_direction, S_IRUGO, fan_show_value, NULL, FAN##id##_DIRECTION); \
    static SENSOR_DEVICE_ATTR(prj##fanr##id##_fault, S_IRUGO, fan_show_value, NULL, FANR##id##_FAULT); \
    static SENSOR_DEVICE_ATTR(prj##fanr##id##_speed_rpm, S_IRUGO, fan_show_value, NULL, FANR##id##_SPEED); \
    static SENSOR_DEVICE_ATTR(prj##fan##id##_input, S_IRUGO, fan_show_value, NULL, FAN##id##_SPEED); \
    static SENSOR_DEVICE_ATTR(prj##fan##id2##_input, S_IRUGO, fan_show_value, NULL, FANR##id##_SPEED); \
    static SENSOR_DEVICE_ATTR(prj##fan##id##_fault, S_IRUGO, fan_show_value, NULL, FAN##id##_FAULT); \
    static SENSOR_DEVICE_ATTR(prj##fan##id2##_fault, S_IRUGO, fan_show_value, NULL, FAN##id##_FAULT); 

#define MAKE_SENSOR_DEVICE_ATTR(prj,id, id2) _MAKE_SENSOR_DEVICE_ATTR(prj,id, id2) 

MAKE_SENSOR_DEVICE_ATTR(PROJECT_NAME ,1 ,11)                  
MAKE_SENSOR_DEVICE_ATTR(PROJECT_NAME ,2 ,12)
MAKE_SENSOR_DEVICE_ATTR(PROJECT_NAME ,3 ,13)
MAKE_SENSOR_DEVICE_ATTR(PROJECT_NAME ,4 ,14)
MAKE_SENSOR_DEVICE_ATTR(PROJECT_NAME ,5 ,15)

static SENSOR_DEVICE_ATTR(name, S_IRUGO, show_name, NULL, 0);
/*******************/

#define _MAKE_FAN_ATTR(prj, id, id2) \
    &sensor_dev_attr_##prj##fan##id##_speed_rpm.dev_attr.attr,     \
    &sensor_dev_attr_##prj##fan##id##_duty_cycle_percentage.dev_attr.attr,\
    &sensor_dev_attr_##prj##fan##id##_direction.dev_attr.attr, \
    &sensor_dev_attr_##prj##fanr##id##_fault.dev_attr.attr,   \
    &sensor_dev_attr_##prj##fanr##id##_speed_rpm.dev_attr.attr, \
    &sensor_dev_attr_##prj##fan##id##_input.dev_attr.attr, \
    &sensor_dev_attr_##prj##fan##id2##_input.dev_attr.attr, \
    &sensor_dev_attr_##prj##fan##id##_fault.dev_attr.attr,     \
    &sensor_dev_attr_##prj##fan##id2##_fault.dev_attr.attr,


#define MAKE_FAN_ATTR(prj, id, id2) _MAKE_FAN_ATTR(prj, id, id2) 

static struct attribute *accton_as6712_32x_fan_attributes[] = {
    /* fan related attributes */
    MAKE_FAN_ATTR(PROJECT_NAME,1 ,11)                  
    MAKE_FAN_ATTR(PROJECT_NAME,2 ,12)
    MAKE_FAN_ATTR(PROJECT_NAME,3 ,13)                  
    MAKE_FAN_ATTR(PROJECT_NAME,4 ,14)
    MAKE_FAN_ATTR(PROJECT_NAME,5 ,15)                  
    &sensor_dev_attr_name.dev_attr.attr,  
    NULL
};
/*******************/

/* fan related functions
 */
static ssize_t fan_show_value(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct  sensor_device_attribute *attr = to_sensor_dev_attr(da);
    ssize_t ret = 0;
    int     data_index, type_index;
    
    accton_as6712_32x_fan_update_device(dev);

    if (fan_data->valid == 0) {
        return ret;
    }

    type_index = attr->index%FAN2_FAULT;
    data_index = attr->index/FAN2_FAULT;
     
    switch (type_index) {
        case FAN1_FAULT:
            ret = sprintf(buf, "%d\n", fan_data->status[data_index]);
            DEBUG_PRINT("[Check !!][%s][%d][type->index=%d][data->index=%d]\n", __FUNCTION__, __LINE__, type_index, data_index);  
            break;
        case FAN1_SPEED:
            ret = sprintf(buf, "%d\n", fan_data->speed[data_index]);
            DEBUG_PRINT("[Check !!][%s][%d][type->index=%d][data->index=%d]\n", __FUNCTION__, __LINE__, type_index, data_index);  
            break;
        case FAN1_DUTY_CYCLE:
            ret = sprintf(buf, "%d\n", fan_data->duty_cycle[data_index]);
            DEBUG_PRINT("[Check !!][%s][%d][type->index=%d][data->index=%d]\n", __FUNCTION__, __LINE__, type_index, data_index);  
            break;
        case FAN1_DIRECTION:
            ret = sprintf(buf, "%d\n", fan_data->direction[data_index]);   /* presnet, need to modify*/
            DEBUG_PRINT("[Check !!][%s][%d][type->index=%d][data->index=%d]\n", __FUNCTION__, __LINE__, type_index, data_index);  
            break;  
        case FANR1_FAULT:
            ret = sprintf(buf, "%d\n", fan_data->r_status[data_index]);
            DEBUG_PRINT("[Check !!][%s][%d][type->index=%d][data->index=%d]\n", __FUNCTION__, __LINE__, type_index, data_index);  
            break;
        case FANR1_SPEED:
            ret = sprintf(buf, "%d\n", fan_data->r_speed[data_index]);
            DEBUG_PRINT("[Check !!][%s][%d][type->index=%d][data->index=%d]\n", __FUNCTION__, __LINE__, type_index, data_index);  
            break;
        default:
            DEBUG_PRINT("[Check !!][%s][%d] \n", __FUNCTION__, __LINE__);      
            break;
    }
    
    return ret;
}

static ssize_t show_name(struct device *dev, struct device_attribute *da,
             char *buf)
{
    return sprintf(buf, "%s\n", DRVNAME);
} 
/*******************/
static ssize_t fan_set_duty_cycle(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count) {

    int error, value;
    
    error = kstrtoint(buf, 10, &value);
    if (error)
        return error;
        
    if (value < FAN_DUTY_CYCLE_MIN || value > FAN_DUTY_CYCLE_MAX)
        return -EINVAL;

    accton_as6712_32x_fan_write_value(CPLD_REG_FAN_PWM_CYCLE_OFFSET, value/FAN_SPEED_PRECENT_TO_CPLD_STEP);
    
    fan_data->valid = 0;

    return count;
}

static const struct attribute_group accton_as6712_32x_fan_group = {
    .attrs = accton_as6712_32x_fan_attributes,
};

static int accton_as6712_32x_fan_read_value(u8 reg)
{
    return as6712_32x_cpld_read(0x60, reg);
}

static int accton_as6712_32x_fan_write_value(u8 reg, u8 value)
{
    return as6712_32x_cpld_write(0x60, reg, value);
}

static void accton_as6712_32x_fan_update_device(struct device *dev)
{
    int speed, r_speed, fault, r_fault, direction, ctrl_speed;
    int i;
    int retry_count;

    mutex_lock(&fan_data->update_lock);

    DEBUG_PRINT("Starting accton_as6712_32x_fan update \n");    

    if (!(time_after(jiffies, fan_data->last_updated + HZ + HZ / 2) || !fan_data->valid)) {
        /* do nothing */
        goto _exit; 
    }
        
    fan_data->valid = 0;
        
    DEBUG_PRINT("Starting accton_as6712_32x_fan update 2 \n");    

    fault      = accton_as6712_32x_fan_read_value(CPLD_REG_FAN_STATUS_OFFSET);
    r_fault    = accton_as6712_32x_fan_read_value(CPLD_REG_FANR_STATUS_OFFSET);    
    direction  = accton_as6712_32x_fan_read_value(CPLD_REG_FAN_DIRECTION_OFFSET);
    ctrl_speed = accton_as6712_32x_fan_read_value(CPLD_REG_FAN_PWM_CYCLE_OFFSET);
    
    if ( (fault < 0) || (r_fault < 0) || (ctrl_speed < 0) )
    {        
        DEBUG_PRINT("[Error!!][%s][%d] \n", __FUNCTION__, __LINE__);            
        goto _exit; /* error */ 
    }

    DEBUG_PRINT("[fan:] fault:%d, r_fault=%d, ctrl_speed=%d \n",fault, r_fault, ctrl_speed);    

    for (i = 0; i < FAN_MAX_NUMBER; i++)
    {
        /* Update fan data
         */

        /* fan fault 
         * 0: normal, 1:abnormal
         * Each FAN-tray module has two fans.
         */
        fan_data->status[i]     = (fault     & fan_info_mask[i]) >> i;
        DEBUG_PRINT("[fan%d:] fail=%d \n",i, fan_data->status[i]);    
        
        fan_data->r_status[i]   = (r_fault   & fan_info_mask[i]) >> i;
        fan_data->direction[i]  = (direction & fan_info_mask[i]) >> i;
        fan_data->duty_cycle[i] = ctrl_speed * FAN_SPEED_PRECENT_TO_CPLD_STEP;
        
        /* fan speed 
         */
        speed = 0;
        r_speed = 0;
        retry_count = 3;
        while (retry_count) {
            retry_count--;
            speed   = accton_as6712_32x_fan_read_value(fan_speed_reg[i]);
            r_speed = accton_as6712_32x_fan_read_value(fanr_speed_reg[i]);
            if ( (speed < 0) || (r_speed < 0) )
            {
                DEBUG_PRINT("[Error!!][%s][%d] \n", __FUNCTION__, __LINE__);
                goto _exit; /* error */
            }
            if ( (speed == 0) || (r_speed == 0) )
            {
                msleep(50);
                continue;
            }
            break;
        }

        DEBUG_PRINT("[fan%d:] speed:%d, r_speed=%d \n", i, speed, r_speed);    
        
        fan_data->speed[i]   = speed   * FAN_SPEED_CPLD_TO_RPM_STEP;
        fan_data->r_speed[i] = r_speed * FAN_SPEED_CPLD_TO_RPM_STEP;
    }
    
    /* finish to update */
    fan_data->last_updated = jiffies;
    fan_data->valid = 1;

_exit:    
    mutex_unlock(&fan_data->update_lock);
}

static int accton_as6712_32x_fan_probe(struct platform_device *pdev)
{
    int status = -1;

    /* Register sysfs hooks */
    status = sysfs_create_group(&pdev->dev.kobj, &accton_as6712_32x_fan_group);
    if (status) {
        goto exit;

    }
    
	fan_data->hwmon_dev = hwmon_device_register(&pdev->dev);
	if (IS_ERR(fan_data->hwmon_dev)) {
		status = PTR_ERR(fan_data->hwmon_dev);
		goto exit_remove;
	}

    dev_info(&pdev->dev, "accton_as6712_32x_fan\n");
    
    return 0;
    
exit_remove:
    sysfs_remove_group(&pdev->dev.kobj, &accton_as6712_32x_fan_group);
exit:
    return status;
}

static int accton_as6712_32x_fan_remove(struct platform_device *pdev)
{
    hwmon_device_unregister(fan_data->hwmon_dev);
    sysfs_remove_group(&fan_data->pdev->dev.kobj, &accton_as6712_32x_fan_group);
    
    return 0;
}



static struct platform_driver accton_as6712_32x_fan_driver = {
    .probe      = accton_as6712_32x_fan_probe,
    .remove     = accton_as6712_32x_fan_remove,
    .driver     = {
        .name   = DRVNAME,
        .owner  = THIS_MODULE,
    },
};

static int __init accton_as6712_32x_fan_init(void)
{
    int ret;

    ret = platform_driver_register(&accton_as6712_32x_fan_driver);
    if (ret < 0) {
        goto exit;
    }
        
    fan_data = kzalloc(sizeof(struct accton_as6712_32x_fan), GFP_KERNEL);
    if (!fan_data) {
        ret = -ENOMEM;
        platform_driver_unregister(&accton_as6712_32x_fan_driver);
        goto exit;
    }

	mutex_init(&fan_data->update_lock);
    fan_data->valid = 0;
	
    fan_data->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
    if (IS_ERR(fan_data->pdev)) {
        ret = PTR_ERR(fan_data->pdev);
        platform_driver_unregister(&accton_as6712_32x_fan_driver);
        kfree(fan_data);
        goto exit;
    }

exit:
    return ret;
}

static void __exit accton_as6712_32x_fan_exit(void)
{
    platform_device_unregister(fan_data->pdev);
    platform_driver_unregister(&accton_as6712_32x_fan_driver);
    kfree(fan_data);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("accton_as6712_32x_fan driver");
MODULE_LICENSE("GPL");

module_init(accton_as6712_32x_fan_init);
module_exit(accton_as6712_32x_fan_exit);


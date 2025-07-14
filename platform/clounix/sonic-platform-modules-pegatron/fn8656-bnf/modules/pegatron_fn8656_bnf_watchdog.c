#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include "linux/kthread.h"
#include <linux/delay.h>
#include<asm/unistd.h>
#include<asm/fcntl.h>
#include<linux/ioctl.h>
#include<linux/jiffies.h>
#include<linux/time.h>
#include "pegatron_pub.h"

enum watchdog_sensor_sysfs_attributes
{
    ARM,
    DISARM,
    IS_ARMED,
    FEED_DOG,
    IDENTIFY,
    STATE,
    TIMELEFT,
    TIMEOUT,
    RESET,
    WDT_ENABLE,
    WDT_DEBUG
};

#define MODULE_NAME       "pegatron_fn8656_watchdog"
#define ERROR_SUCCESS 0

struct kobject *kobj_switch = NULL;

extern int pegatron_fn8656_bnf_cpld_read(unsigned short cpld_addr, u8 reg);
extern int pegatron_fn8656_bnf_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);


static uint loglevel = LOG_INFO | LOG_WARNING | LOG_ERR;
static struct kobject * kobj_watchdog_root = NULL;

unsigned long  armed_jiffies;
unsigned long  curr_jiffies;
u32 timeout = 32;

#define WATCHDOG_CPLD_ADDRESS 0x18

#define WATCHDOG_CPLD_WDT_REG 0x12
#define WATCHDOG_CPLD_WDT_ENB 0x1 /*0: Enable 1: Disable*/
#define WATCHDOG_CPLD_WDT_ENB_MASK 0x1
#define WATCHDOG_CPLD_WDT_FEED 0x80 /*WDT_CPU_FLAG*/
#define WATCHDOG_CPLD_WDT_TIMEOUT_MASK 0xe
#define WATCHDOG_CPLD_WDT_TIMEOUT_1S (0 << 1)
#define WATCHDOG_CPLD_WDT_TIMEOUT_3S (1 << 1)
#define WATCHDOG_CPLD_WDT_TIMEOUT_5S (2 << 1)
#define WATCHDOG_CPLD_WDT_TIMEOUT_10S (3 << 1)
#define WATCHDOG_CPLD_WDT_TIMEOUT_30S (4 << 1)
#define WATCHDOG_CPLD_WDT_TIMEOUT_60S (5 << 1)
#define WATCHDOG_CPLD_WDT_TIMEOUT_120S (6 << 1)
#define WATCHDOG_CPLD_WDT_TIMEOUT_180S (7 << 1)

#define WATCHDOG_CPLD_WDT_TIMELEFT_REG 0x14


#define timeout_set(timeout, set_value) \
do { \
 set_value &= (~WATCHDOG_CPLD_WDT_TIMEOUT_MASK); \
 if (timeout <= 5) \
     set_value |= (WATCHDOG_CPLD_WDT_TIMEOUT_5S & WATCHDOG_CPLD_WDT_TIMEOUT_MASK); \
 else if (timeout <= 10) \
     set_value |= (WATCHDOG_CPLD_WDT_TIMEOUT_10S & WATCHDOG_CPLD_WDT_TIMEOUT_MASK); \
 else if (timeout <= 30) \
     set_value |= (WATCHDOG_CPLD_WDT_TIMEOUT_30S & WATCHDOG_CPLD_WDT_TIMEOUT_MASK); \
 else if (timeout <= 60) \
     set_value |= (WATCHDOG_CPLD_WDT_TIMEOUT_60S & WATCHDOG_CPLD_WDT_TIMEOUT_MASK); \
 else if (timeout <= 120) \
     set_value |= (WATCHDOG_CPLD_WDT_TIMEOUT_120S & WATCHDOG_CPLD_WDT_TIMEOUT_MASK); \
 else \
     set_value |= (WATCHDOG_CPLD_WDT_TIMEOUT_180S & WATCHDOG_CPLD_WDT_TIMEOUT_MASK); \
}while(0)


static ssize_t bsp_cpu_cpld_write_byte(u8 data, u8 reg)
{
    return pegatron_fn8656_bnf_cpld_write(WATCHDOG_CPLD_ADDRESS, reg, data);
}

static ssize_t bsp_cpu_cpld_read_byte(u8 *data, u8 reg)
{
    *data = pegatron_fn8656_bnf_cpld_read(WATCHDOG_CPLD_ADDRESS, reg);
    return 0; 
}

static ssize_t bsp_sysfs_watchdog_custom_set_attr(struct device *kobject, struct device_attribute *da, const char *buf, size_t count)
{
    int temp = 0;
    int ret = ERROR_SUCCESS;
    u8 set_value = 0;
    u8 get_value = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    switch (attr->index)
    {
        case ARM:
        {
            if (sscanf(buf, "%d", &temp) <= 0)
            {
                pega_print(ERR, "Format '%s' error, integer expected!", buf);
                ret = -EINVAL;
            }
            else
            {
                if (0 == temp)
                {
                    set_value |= WATCHDOG_CPLD_WDT_ENB;  
                    pega_print(DEBUG,"watchdog timer disabled!");
                }
                else
                {
                    set_value &= (~WATCHDOG_CPLD_WDT_ENB);  
                    pega_print(DEBUG,"watchdog timer enabled!");
                }

                armed_jiffies = jiffies;
                if (temp > 255)
                    temp = 255;
                timeout_set(temp, set_value);
                timeout = temp;
                ret = bsp_cpu_cpld_write_byte(set_value, WATCHDOG_CPLD_WDT_REG);
            }
            break;
        }
        case TIMEOUT:
        {
            if (sscanf(buf, "%d", &temp) <= 0)
            {
                pega_print(DEBUG, "Format '%s' error, integer expected!", buf);
                ret = -EINVAL;
            }
            else
            {
                if (temp > 255)
                    temp = 255;
                ret = bsp_cpu_cpld_read_byte(&get_value, WATCHDOG_CPLD_WDT_REG);
                set_value = get_value;
                timeout_set(temp, set_value);
                timeout = temp;
                ret = bsp_cpu_cpld_write_byte(set_value, WATCHDOG_CPLD_WDT_REG);
            }
            break;
        }
        case DISARM:
        {
            if (sscanf(buf, "%d", &temp) <= 0)
            {
                pega_print(DEBUG, "Format '%s' error, integer expected!", buf);
                ret = -EINVAL;
            }
            else
            {
                timeout_set(temp, set_value);
                set_value |= WATCHDOG_CPLD_WDT_ENB;  
                ret = bsp_cpu_cpld_write_byte(set_value, WATCHDOG_CPLD_WDT_REG);
                pega_print(DEBUG,"watchdog timer disabled!");
            }
            break;
        }
        case WDT_ENABLE:
        {
            if (sscanf(buf, "%d", &temp) <= 0)
            {
                pega_print(DEBUG, "Format '%s' error, integer expected!", buf);
                ret = -EINVAL;
            }
            else
            {
                ret = bsp_cpu_cpld_read_byte(&get_value, WATCHDOG_CPLD_WDT_REG);
                if(((get_value & WATCHDOG_CPLD_WDT_ENB_MASK) == 1) && (temp == 1))
                {
                    armed_jiffies = jiffies;
                    set_value = get_value & (~WATCHDOG_CPLD_WDT_ENB);  
                    ret = bsp_cpu_cpld_write_byte(set_value, WATCHDOG_CPLD_WDT_REG);
                    pega_print(DEBUG,"watchdog timer enabled!");
                }
                else if(((get_value & WATCHDOG_CPLD_WDT_ENB_MASK) == 0) && (temp == 0))
                {
                    set_value = get_value | WATCHDOG_CPLD_WDT_ENB;  
                    ret = bsp_cpu_cpld_write_byte(set_value, WATCHDOG_CPLD_WDT_REG);
                    pega_print(DEBUG,"watchdog timer disabled!");
                }
            }
            break;
        }
        case FEED_DOG:
        case RESET:
        {
            ret = bsp_cpu_cpld_read_byte(&get_value, WATCHDOG_CPLD_WDT_REG);
            armed_jiffies = jiffies;
            pega_print(DEBUG,"feed dog!");
            break;
        }
        default:
        {
            pega_print(ERR, "Not found attr %d for watchdog", attr->index);
            ret = -ENOSYS;
            break;
        }
    }
    
exit:
    if (ret != ERROR_SUCCESS)
    {
        count = ret;
    }
    return count;
}

static ssize_t bsp_sysfs_watchdog_custom_get_attr(struct device *kobj, struct device_attribute *da, char *buf)
{
    ssize_t index = 0;
    int ret = ERROR_SUCCESS;
    u8 get_value;
    int int_value =0;
    //int float_value =0;
    unsigned int run_time = 0;
    int left_time = 0;
    u8 timeout_data[8] = {1, 3, 5, 10, 30, 60, 120, 180};

    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    switch(attr->index)
    {
        case ARM:
        case TIMEOUT:
        {
            ret = bsp_cpu_cpld_read_byte(&get_value, WATCHDOG_CPLD_WDT_REG);
            index = scnprintf(buf, PAGE_SIZE, "%d\n", timeout_data[(get_value >> 1) & 0x7]);
            break;
        }
        case WDT_ENABLE:
        case IS_ARMED:
        {
            ret = bsp_cpu_cpld_read_byte(&get_value, WATCHDOG_CPLD_WDT_REG);
            if((get_value & WATCHDOG_CPLD_WDT_ENB_MASK) == 1)
            {
                index = sprintf(buf, "%d\n", 0);
            }
            else
            {
                index = sprintf(buf, "%d\n", 1);
            }
            break;
        }
        case DISARM:
        {
            ret = bsp_cpu_cpld_read_byte(&get_value, WATCHDOG_CPLD_WDT_REG);
            if((get_value & WATCHDOG_CPLD_WDT_ENB_MASK) == 1)
            {
                index = scnprintf(buf, PAGE_SIZE, "%d\n", 1);
            }
            else
            {
                index = scnprintf(buf, PAGE_SIZE, "%d\n", 0);
            }
            break;
        }
        case IDENTIFY:
        {
            index = scnprintf(buf, PAGE_SIZE, "pegatron_cpld_wdt\n");
            break;
        }
        case STATE:
        {
            ret = bsp_cpu_cpld_read_byte(&get_value, WATCHDOG_CPLD_WDT_REG);
            if((get_value & WATCHDOG_CPLD_WDT_ENB_MASK) == 1)
            {
                index = scnprintf(buf, PAGE_SIZE, "inactive\n");
            }
            else
            {
                index = scnprintf(buf, PAGE_SIZE, "active\n");
            }
            break;
        }
        case TIMELEFT:
        {
#if 0
            curr_jiffies = jiffies;

            run_time = jiffies_to_msecs(curr_jiffies - armed_jiffies) / 1000;

            left_time = timeout - run_time;
            if (left_time < 0)
                   left_time = 0;
#else

            ret = bsp_cpu_cpld_read_byte(&get_value, WATCHDOG_CPLD_WDT_TIMELEFT_REG);
            left_time = get_value;
#endif

            index = sprintf(buf, "%d\n", left_time);
            break;
        }
        case WDT_DEBUG:
        {
            ret = bsp_cpu_cpld_read_byte(&get_value, WATCHDOG_CPLD_WDT_REG);
            int_value = timeout_data[(get_value >> 1) & 0x7];

            if((get_value & WATCHDOG_CPLD_WDT_ENB_MASK) == 0)
            {
                index = scnprintf(buf, PAGE_SIZE, "enable: 1\ntimeout:%d\n", int_value);
            }
            else
            {
                index = scnprintf(buf, PAGE_SIZE, "enable: 0\ntimeout:%d\n", int_value);
            }
            index += scnprintf(buf + index, PAGE_SIZE - index, "%s\n", "BSP DEBUG WDT");
            break;
        }
        default:
        {
            pega_print(DEBUG, "watchdog attribte %d is invalid\n", attr->index);
            break;
        }
    }
exit:
    return index;
}

//debug node
static SENSOR_DEVICE_ATTR(arm,            S_IRUGO|S_IWUSR,  bsp_sysfs_watchdog_custom_get_attr, bsp_sysfs_watchdog_custom_set_attr, ARM);
static SENSOR_DEVICE_ATTR(disarm,         S_IRUGO|S_IWUSR,  bsp_sysfs_watchdog_custom_get_attr, bsp_sysfs_watchdog_custom_set_attr, DISARM);
static SENSOR_DEVICE_ATTR(is_armed, S_IRUGO,                bsp_sysfs_watchdog_custom_get_attr, NULL, IS_ARMED);
static SENSOR_DEVICE_ATTR(feed_dog,       S_IWUSR,          NULL, bsp_sysfs_watchdog_custom_set_attr, FEED_DOG);

static SENSOR_DEVICE_ATTR(identify,       S_IRUGO, bsp_sysfs_watchdog_custom_get_attr, NULL, IDENTIFY);
static SENSOR_DEVICE_ATTR(state,          S_IRUGO, bsp_sysfs_watchdog_custom_get_attr, NULL, STATE);
static SENSOR_DEVICE_ATTR(timeleft,       S_IRUGO, bsp_sysfs_watchdog_custom_get_attr, NULL, TIMELEFT);
static SENSOR_DEVICE_ATTR(timeout,        S_IRUGO|S_IWUSR, bsp_sysfs_watchdog_custom_get_attr, bsp_sysfs_watchdog_custom_set_attr, TIMEOUT);
static SENSOR_DEVICE_ATTR(reset,          S_IWUSR,         NULL, bsp_sysfs_watchdog_custom_set_attr, RESET);
static SENSOR_DEVICE_ATTR(enable,         S_IRUGO|S_IWUSR, bsp_sysfs_watchdog_custom_get_attr, bsp_sysfs_watchdog_custom_set_attr, WDT_ENABLE);

static SENSOR_DEVICE_ATTR(debug, S_IRUGO, bsp_sysfs_watchdog_custom_get_attr, NULL, WDT_DEBUG);
//BSPMODULE_DEBUG_RW_ATTR_DEF(loglevel, BSP_WATCHDOG_MODULE);

static struct attribute *watchdog_debug_attributes[] =
{
    &sensor_dev_attr_arm.dev_attr.attr,
    &sensor_dev_attr_disarm.dev_attr.attr,
    &sensor_dev_attr_is_armed.dev_attr.attr,
    &sensor_dev_attr_feed_dog.dev_attr.attr,

    &sensor_dev_attr_identify.dev_attr.attr,
    &sensor_dev_attr_state.dev_attr.attr,
    &sensor_dev_attr_timeleft.dev_attr.attr,
    &sensor_dev_attr_timeout.dev_attr.attr,
    &sensor_dev_attr_reset.dev_attr.attr,
    &sensor_dev_attr_enable.dev_attr.attr,
    &sensor_dev_attr_debug.dev_attr.attr,
//    &bspmodule_loglevel.attr,
    NULL
};

static const struct attribute_group watchdog_switch_attribute_group =
{
    .attrs = watchdog_debug_attributes,
};

void watchdog_sysfs_exit(void)
{
    if (kobj_watchdog_root != NULL && kobj_watchdog_root->state_initialized)
    {
        sysfs_remove_group(kobj_watchdog_root, &watchdog_switch_attribute_group);
        kobject_put(kobj_watchdog_root);
    }
    return;
}

int watchdog_sysfs_init(void)
{
    int ret = ERROR_SUCCESS;
    kobj_watchdog_root = kobject_create_and_add("watchdog", kobj_switch);

    if (kobj_watchdog_root == NULL)
    {
        pega_print(ERR, "create kobj_watchdog_root failed!");
        ret = -ENOMEM;
        goto exit;
    }

    ret = sysfs_create_group(kobj_watchdog_root, &watchdog_switch_attribute_group);

exit:
    if (ret != 0)
    {
        pega_print(ERR, "watchdog sysfs init failed!\n");
        watchdog_sysfs_exit();
    }

    return ret;
}

int  bsp_sysfs_init(void)
{
    int ret = ERROR_SUCCESS;

    kobj_switch = kobject_create_and_add("clx", kernel_kobj->parent);


    return ret;
}


void bsp_sysfs_release_kobjs(void)
{
    if (kobj_switch != NULL)
    {
        kobject_put(kobj_switch);
    }
    return;
}



static int __init watchdog_init(void)
{
    int ret = ERROR_SUCCESS;

    bsp_sysfs_init();
    ret = watchdog_sysfs_init();
    if (ERROR_SUCCESS == ret)
    {
        pega_print(INFO,"pegatron_fn8656_watchdog module finished and success!");
    }
    else
    {
        pega_print(INFO,"pegatron_fn8656_watchdog module finished and failed!");
    }

    return ret;
}


static void __exit watchdog_exit(void)
{
    watchdog_sysfs_exit();
    bsp_sysfs_release_kobjs();
    pega_print(INFO,"pegatron_fn8656_watchdog module uninstalled !\n");
    return;
}

module_param(loglevel,uint,0644);

MODULE_PARM_DESC(loglevel,"0x01-LOG_ERR,0x02-LOG_WARNING,0x04-LOG_INFO,0x08-LOG_DEBUG");


module_init(watchdog_init);
module_exit(watchdog_exit);

MODULE_AUTHOR("Junde Zhou <zhoujd@clounix.com>");
MODULE_DESCRIPTION("pegatron watchdog driver");
MODULE_LICENSE("Dual BSD/GPL");

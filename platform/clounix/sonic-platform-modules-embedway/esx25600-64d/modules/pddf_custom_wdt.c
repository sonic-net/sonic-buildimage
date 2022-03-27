#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include "pddf_client_defs.h"

static int *log_level = &wdt_log_level;

#define WATCHDOG_BASE_ADDRESS           (0x0100)

#define WATCHDOG_CONFIG1          0x49
#define WATCHDOG_CONFIG2          0x4A
#define WATCHDOG_STATUS1          0x4B
#define WATCHDOG_STATUS2          0x4C
#define WATCHDOG_FEED              0x4D

/*bit field in WATCHDOG_CONFIG1*/
#define WATCHDOG_CONFIG1_ENABLE_OFFSET 0
#define WATCHDOG_CONFIG1_ENABLE_SIZE 1
#define WATCHDOG_CONFIG1_RST_OFFSET 1
#define WATCHDOG_CONFIG1_RST_SIZE 1
#define WATCHDOG_CONFIG1_CLEAR_OFFSET 2
#define WATCHDOG_CONFIG1_CLEAR_SIZE 1

/*bit field in WATCHDOG_CONFIG2*/
#define WATCHDOG_CONFIG2_TIMEOUT_OFFSET 0
#define WATCHDOG_CONFIG2_TIMEOUT_SIZE 8

/*bit field in WATCHDOG_STATUS1*/
#define WATCHDOG_STATUS1_REBOOT_OFFSET 0
#define WATCHDOG_STATUS1_REBOOT_SIZE 1

/*bit field in WATCHDOG_STATUS2*/
#define WATCHDOG_STATUS2_CNT_OFFSET 0
#define WATCHDOG_STATUS2_CNT_SIZE 8

/*bit field in WATCHDOG_FEED*/
#define WATCHDOG_FEED_SET_OFFSET 0
#define WATCHDOG_FEED_SET_SIZE 1

/* Bit manipulation macros */
#define WATCHDOG_BIT(name)					\
	(1 << WATCHDOG_##name##_OFFSET)
#define WATCHDOG_BF(name,value)				\
	(((value) & ((1 << WATCHDOG_##name##_SIZE) - 1))	\
	 << WATCHDOG_##name##_OFFSET)
#define WATCHDOG_BFEXT(name,value) \
	(((value) >> WATCHDOG_##name##_OFFSET) \
     & ((1 << WATCHDOG_##name##_SIZE) - 1))
#define WATCHDOG_BFINS(name,value,old)			\
	(((old) & ~(((1 << WATCHDOG_##name##_SIZE) - 1)	\
		    << WATCHDOG_##name##_OFFSET))		\
	 | WATCHDOG_BF(name,value))

#define CPU_CPLD_I2C_ADDR  (0x62)
#define CPU_CPLD_NAME  "CPLD-CB"

extern int board_i2c_cpld_read_new(unsigned short cpld_addr, char *name, u8 reg);
extern int board_i2c_cpld_write_new(unsigned short cpld_addr, char *name, u8 reg, u8 value);
struct kobject  *kobj_watchdog_root = NULL;

static int cpu_cpld_read(u8 offset)
{
    int ret;

    ret =  board_i2c_cpld_read_new(CPU_CPLD_I2C_ADDR, CPU_CPLD_NAME, offset);
    if (unlikely(ret < 0))
    {
        pddf_err(WDT, "board_i2c_cpld_read_new failed,err=%d\r\n",ret);
    }

    return ret;
}

static int cpu_cpld_write(u8 val, u8 offset)
{
    int ret;
    
    ret =  board_i2c_cpld_write_new(CPU_CPLD_I2C_ADDR, CPU_CPLD_NAME, offset, val);
    if (unlikely(ret < 0))
    {
        pddf_err(WDT, "board_i2c_cpld_write_new failed,err=%d\r\n",ret);
    }

    return ret;
}

/*
 * clx_get_watchdog_identify - Used to get watchdog identify, such as iTCO_wdt
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_watchdog_identify(struct device *dev,
			struct device_attribute *dattr, char *buf)
{
    ssize_t ret = -1;
    /* add vendor codes here */
    ret = scnprintf(buf, PAGE_SIZE,"embedway_fpga_wdt\n");
    return ret;
}

/*
 * clx_get_watchdog_state - Used to get watchdog state,
 * filled the value to buf, 0: inactive, 1: active
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_watchdog_state(struct device *dev,
			struct device_attribute *attr, char *buf)
{
    unsigned int  data = 0 ;
    ssize_t ret = -1;

    data= cpu_cpld_read(WATCHDOG_CONFIG1);
    if(data & WATCHDOG_BIT(CONFIG1_ENABLE))
    {
        ret = scnprintf(buf, PAGE_SIZE, "active\n");
    }
    else
    {
        ret = scnprintf(buf, PAGE_SIZE, "inactive\n");
    }

    return ret;
}

/*
 * clx_get_watchdog_timeleft - Used to get watchdog timeleft,
 * filled the value to buf
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_watchdog_timeleft(struct device *dev,
			struct device_attribute *attr, char *buf)
{
    /* add vendor codes here */
    ssize_t ret = -1;
    unsigned int  data = 0 ;
    unsigned int  timeleft ;

    data = cpu_cpld_read(WATCHDOG_CONFIG2);
    data = WATCHDOG_BFEXT(CONFIG2_TIMEOUT,data) ;
    timeleft = cpu_cpld_read(WATCHDOG_STATUS2);
    timeleft = WATCHDOG_BFEXT(STATUS2_CNT,timeleft);
    timeleft = data - timeleft ;

    ret = scnprintf(buf, PAGE_SIZE,"%d\n",(int)timeleft);
    return ret;
}

/*
 * clx_get_watchdog_timeout - Used to get watchdog timeout,
 * filled the value to buf
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_watchdog_timeout(struct device *dev,
			struct device_attribute *attr, char *buf)
{
    /* add vendor codes here */
    ssize_t ret = -1;
    unsigned int  timeout ;

    timeout = cpu_cpld_read(WATCHDOG_CONFIG2);
    timeout = WATCHDOG_BFEXT(CONFIG2_TIMEOUT,timeout); 
    //printk(KERN_INFO  "get timeout is %d.\r\n", timeout);
    ret = scnprintf(buf, PAGE_SIZE,"%d\n",(int)timeout);

    return ret;
}

/*
 * clx_set_watchdog_timeout - Used to set watchdog timeout,
 * @value: timeout value
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_set_watchdog_timeout(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
    /* add vendor codes here */
    unsigned int  timeout, value ;

    if (kstrtouint(buf, 10, &value))
    {
        return -EINVAL;
    }
    pddf_dbg(WDT, "set timeout is %d.\r\n", value);
    timeout = cpu_cpld_read(WATCHDOG_CONFIG2);
    cpu_cpld_write(WATCHDOG_BFINS(CONFIG2_TIMEOUT,value,timeout), WATCHDOG_CONFIG2);

    return count;
}

/*
 * clx_get_watchdog_enable_status - Used to get watchdog enable status,
 * filled the value to buf, 0: disable, 1: enable
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_watchdog_enable_status(struct device *dev,
			struct device_attribute *dattr, char *buf)
{
    /* add vendor codes here */
    unsigned int  data = 0 ;
    unsigned char value = 0;
    ssize_t ret = -1;

    data= cpu_cpld_read(WATCHDOG_CONFIG1);
    value = WATCHDOG_BFEXT(CONFIG1_ENABLE,data);
    ret = scnprintf(buf,PAGE_SIZE,"%d\n",value);

    return ret;
}

/*
 * clx_set_watchdog_enable_status - Used to set watchdog enable status,
 * @value: enable status value, 0: disable, 1: enable
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_set_watchdog_enable_status(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
    /* add vendor codes here */
    unsigned int  data = 0 , value = 0;

    if (kstrtouint(buf, 10, &value))
    {
        return -EINVAL;
    }

    data= cpu_cpld_read(WATCHDOG_CONFIG1);
    cpu_cpld_write(WATCHDOG_BFINS(CONFIG1_ENABLE,value,data), WATCHDOG_CONFIG1);
    return count;
}

/*
   reset watchdog and disable it when module exit
*/
static void reset_watchdog(void)
{
    unsigned int data, timeout;

    data= cpu_cpld_read(WATCHDOG_CONFIG1);
    cpu_cpld_write(WATCHDOG_BFINS(CONFIG1_ENABLE, 0, data), WATCHDOG_CONFIG1);

    timeout = cpu_cpld_read(WATCHDOG_CONFIG2);
    cpu_cpld_write(WATCHDOG_BFINS(CONFIG2_TIMEOUT, 90, timeout), WATCHDOG_CONFIG2);
    return;
}

/*
 * drv_set_watchdog_reset - Used to set watchdog reset(wdi),
 * @value: any value is ok
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_set_watchdog_reset(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
    unsigned int  data = 0 ;

    data = WATCHDOG_BIT(FEED_SET);
    cpu_cpld_write(data, WATCHDOG_FEED);
    return count;
}


/*
 * drv_get_watchdog_rst_occur - Used to get watchdog rst_occur,
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_watchdog_rst_occur(struct device *dev,
			struct device_attribute *dattr, char *buf)
{
    /* add vendor codes here */
    unsigned int  data = 0 ;
    unsigned char value = 0;
    ssize_t ret = -1;

    data= cpu_cpld_read(WATCHDOG_STATUS1);
    value = WATCHDOG_BFEXT(STATUS1_REBOOT,data);
    ret = scnprintf(buf,PAGE_SIZE,"%d\n",value);

    return ret;
}

/*
 * drv_set_watchdog_rst_occur - Used to clear watchdog rst_occur,
 * @value:rst_occur value, 0: disable, 1: enable
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_set_watchdog_rst_occur(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
    /* add vendor codes here */
    unsigned int  data = 0 , value = 0;

    if (kstrtouint(buf, 10, &value))
    {
        return -EINVAL;
    }

    data= cpu_cpld_read(WATCHDOG_CONFIG1);
    cpu_cpld_write(WATCHDOG_BFINS(CONFIG1_CLEAR,value,data), WATCHDOG_CONFIG1);
    return count;
}

static ssize_t drv_get_loglevel(struct device *dev,
			struct device_attribute *dattr, char *buf)
{
    int value = *log_level;
    ssize_t ret = -1;

    ret = scnprintf(buf,PAGE_SIZE,"%d\n",value);

    return ret;
}

static ssize_t drv_set_loglevel(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
    /* add vendor codes here */
    int value = 0;

    if (kstrtouint(buf, 10, &value))
    {
        return -EINVAL;
    }

    if (value >= LOG_OFF && value <= LOG_DEBUG) {
        *log_level = value;
    } else {
        return -EINVAL;
    }

    return count;
}

static DEVICE_ATTR(identify,   S_IRUGO, drv_get_watchdog_identify, NULL);
static DEVICE_ATTR(state,      S_IRUGO, drv_get_watchdog_state, NULL);
static DEVICE_ATTR(timeleft,   S_IRUGO, drv_get_watchdog_timeleft, NULL);
static DEVICE_ATTR(timeout,    S_IRUGO|S_IWUSR, drv_get_watchdog_timeout, drv_set_watchdog_timeout);
static DEVICE_ATTR(reset,      S_IWUSR,         NULL, drv_set_watchdog_reset);
static DEVICE_ATTR(enable,     S_IRUGO|S_IWUSR, drv_get_watchdog_enable_status, drv_set_watchdog_enable_status);
static DEVICE_ATTR(rst_occur,  S_IRUGO|S_IWUSR, drv_get_watchdog_rst_occur, drv_set_watchdog_rst_occur);
static DEVICE_ATTR(loglevel,   S_IRUGO|S_IWUSR, drv_get_loglevel, drv_set_loglevel);

static struct attribute *watchdog_debug_attributes[] =
{
    &dev_attr_identify.attr,
    &dev_attr_state.attr,
    &dev_attr_timeleft.attr,
    &dev_attr_timeout.attr,
    &dev_attr_reset.attr,
    &dev_attr_enable.attr,
    &dev_attr_rst_occur.attr,
    &dev_attr_loglevel.attr,
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
    int ret = 0;

    kobj_watchdog_root = kobject_create_and_add("watchdog", kernel_kobj->parent);

    if (kobj_watchdog_root == NULL)
    {
        pddf_err(WDT, "create kobj_watchdog_root failed!");
        ret = -ENOMEM;
        goto exit;
    }

    ret = sysfs_create_group(kobj_watchdog_root, &watchdog_switch_attribute_group);

exit:
    if (ret != 0)
    {
        pddf_err(WDT, "watchdog sysfs init failed!\n");
        watchdog_sysfs_exit();
    }

    return ret;
}

static int __init embedway_watchdog_init(void)
{
    int ret = 0;

    ret = watchdog_sysfs_init();
    if (ret)
    {
       pddf_err(WDT, "embedway fpga module finished and failed!\n");
    }
    else
    {
        pddf_err(WDT, "embedway fpga watchdog module finished and success!\n");    
    }

    return ret;
}


static void __exit embedway_watchdog_exit(void)
{
    watchdog_sysfs_exit();
    reset_watchdog();
    pddf_info(WDT, "embedway fpga watchdog module uninstalled !\n");
    return;
}

MODULE_AUTHOR("Embedway");
MODULE_DESCRIPTION("fpga watchdog driver");
MODULE_LICENSE("GPL");

module_init(embedway_watchdog_init);
module_exit(embedway_watchdog_exit);

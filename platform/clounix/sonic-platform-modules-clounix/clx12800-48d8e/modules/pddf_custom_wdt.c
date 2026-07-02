/*******************************************************************************

*  Copyright©[2020-2024] [Hangzhou Clounix Technology Limited]. 

*  All rights reserved.



*  This program is free software: you can redistribute it and/or modify

*  it under the terms of the GNU General Public License as published by

*  the Free Software Foundation, either version 3 of the License, or

*  (at your option) any later version.



*  This program is distributed in the hope that it will be useful,

*  but WITHOUT ANY WARRANTY; without even the implied warranty of

*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the

*  GNU General Public License for more details.



*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 

*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 

*  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 

*  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 

*  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 

*  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 

*  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 

*  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 

*  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 

*  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 

*  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include <linux/kernel.h>
#include <linux/reboot.h>
#include <linux/notifier.h>
#include <linux/panic_notifier.h>
#include <linux/kdebug.h>

#define WATCHDOG_BASE_ADDRESS           (0x0100)

#define WATCHDOG_CONFIG          0x0110
#define WATCHDOG_STATUS          0x0114
#define WATCHDOG_FEED            0x0118

/*bit field in WATCHDOG_CONFIG*/
#define WATCHDOG_CONFIG_ENABLE_OFFSET 31
#define WATCHDOG_CONFIG_ENABLE_SIZE 1
#define WATCHDOG_CONFIG_RST_OFFSET 30
#define WATCHDOG_CONFIG_RST_SIZE 1
#define WATCHDOG_CONFIG_CLEAR_OFFSET 24
#define WATCHDOG_CONFIG_CLEAR_SIZE 1
#define WATCHDOG_CONFIG_TIMEOUT_OFFSET 0
#define WATCHDOG_CONFIG_TIMEOUT_SIZE 8

/*bit field in WATCHDOG_STATUS*/
#define WATCHDOG_STATUS_REBOOT_OFFSET 31
#define WATCHDOG_STATUS_REBOOT_SIZE 1
#define WATCHDOG_STATUS_CNT_OFFSET 0
#define WATCHDOG_STATUS_CNT_SIZE 8
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

#define DEFAULT_TIMEOUT (240)

extern void __iomem * fpga_ctl_addr;
struct kobject  *kobj_watchdog_root = NULL;

static struct notifier_block panic_nb = {0};

static int prolong_wdt_work(struct notifier_block *nb, unsigned long action, void *data)
{
    unsigned int reg_data = 0;

    if (fpga_ctl_addr == NULL)
        return NOTIFY_DONE;

    reg_data = readl(fpga_ctl_addr + WATCHDOG_CONFIG);
    writel(WATCHDOG_BFINS(CONFIG_TIMEOUT, DEFAULT_TIMEOUT, reg_data), fpga_ctl_addr + WATCHDOG_CONFIG);
  //reg_data = readl(fpga_ctl_addr + WATCHDOG_CONFIG);
  //writel(WATCHDOG_BFINS(CONFIG_ENABLE, 1, reg_data), fpga_ctl_addr + WATCHDOG_CONFIG);

    printk(KERN_EMERG "%s[%d]\n", __func__, __LINE__);
    return NOTIFY_DONE;
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
    ret = scnprintf(buf, PAGE_SIZE,"clounix_fpga_wdt\n");
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

    if (NULL != fpga_ctl_addr) {
        data= readl(fpga_ctl_addr + WATCHDOG_CONFIG);
        if(data & WATCHDOG_BIT(CONFIG_ENABLE))
        {
            ret = scnprintf(buf, PAGE_SIZE, "active\n");
        }
        else
        {
            ret = scnprintf(buf, PAGE_SIZE, "inactive\n");
        }
    }
    else {
        printk(KERN_ERR "fpga resource is not available.\r\n");
        ret = -1;
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
    if (fpga_ctl_addr == NULL) {
        printk(KERN_ERR  "fpga resource is not available.\r\n");
        return -ENXIO;
    }
    data = readl(fpga_ctl_addr + WATCHDOG_CONFIG);
    data = WATCHDOG_BFEXT(CONFIG_TIMEOUT,data) ;
    timeleft = readl(fpga_ctl_addr + WATCHDOG_STATUS);
    timeleft = WATCHDOG_BFEXT(STATUS_CNT,timeleft);
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
    if (fpga_ctl_addr == NULL) {
        printk(KERN_ERR  "fpga resource is not available.\r\n");
        return -ENXIO;
    }
    timeout = readl(fpga_ctl_addr + WATCHDOG_CONFIG);
    timeout = WATCHDOG_BFEXT(CONFIG_TIMEOUT,timeout); 
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
    if (fpga_ctl_addr == NULL) {
        printk(KERN_ERR  "fpga resource is not available.\r\n");
        return -ENXIO;
    }
     if (kstrtouint(buf, 10, &value))
    {
        return -EINVAL;
    }
   // printk(KERN_INFO  "set timeout is %d.\r\n", value);
    timeout = readl(fpga_ctl_addr + WATCHDOG_CONFIG);
    writel(WATCHDOG_BFINS(CONFIG_TIMEOUT,value,timeout), fpga_ctl_addr + WATCHDOG_CONFIG);

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

    if(NULL != fpga_ctl_addr){
        data= readl(fpga_ctl_addr + WATCHDOG_CONFIG);
        value = WATCHDOG_BFEXT(CONFIG_ENABLE,data);
        ret = scnprintf(buf,PAGE_SIZE,"%d\n",value);
    }
    else{
        printk(KERN_ERR  "fpga resource is not available.\r\n");
        ret = -1;
    }
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
    if (fpga_ctl_addr == NULL) {
        printk(KERN_ERR  "fpga resource is not available.\r\n");
        return -ENXIO;
    }
    if (kstrtouint(buf, 10, &value))
    {
        return -EINVAL;
    }

    data= readl(fpga_ctl_addr + WATCHDOG_CONFIG);
    writel(WATCHDOG_BFINS(CONFIG_ENABLE,value,data), fpga_ctl_addr + WATCHDOG_CONFIG);
    return count;
}

/*
   reset watchdog and disable it when module exit
*/
static void reset_watchdog(void)
{
    unsigned int data, timeout;

    data= readl(fpga_ctl_addr + WATCHDOG_CONFIG);
    writel(WATCHDOG_BFINS(CONFIG_ENABLE, 0, data), fpga_ctl_addr + WATCHDOG_CONFIG);

    timeout = readl(fpga_ctl_addr + WATCHDOG_CONFIG);
    writel(WATCHDOG_BFINS(CONFIG_TIMEOUT, 90, timeout), fpga_ctl_addr + WATCHDOG_CONFIG);
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

    if (fpga_ctl_addr == NULL) {
        printk(KERN_ERR  "fpga resource is not available.\r\n");
        return -ENXIO;
    }
    data = WATCHDOG_BIT(FEED_SET);
    writel(data, fpga_ctl_addr + WATCHDOG_FEED);
    return count;
}

static ssize_t drv_get_watchdog_rst_flag(struct device *dev,
			struct device_attribute *attr, char *buf)
{
    unsigned int  data = 0 ;

    if (fpga_ctl_addr == NULL) {
        printk(KERN_ERR  "fpga resource is not available.\r\n");
        return -ENXIO;
    }

    data = readl(fpga_ctl_addr + WATCHDOG_STATUS);
    if ((data & (1 << WATCHDOG_STATUS_REBOOT_OFFSET)) != 0)
        return sprintf(buf, "1\n");
    else
        return sprintf(buf, "0\n");
}

static ssize_t drv_set_watchdog_rst_flag(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
    unsigned int  data = 0 ;

    if (fpga_ctl_addr == NULL) {
        printk(KERN_ERR "fpga resource is not available.\r\n");
        return -ENXIO;
    }

    data = readl(fpga_ctl_addr + WATCHDOG_CONFIG);
    data |= 1 << WATCHDOG_CONFIG_CLEAR_OFFSET;
    writel(data, fpga_ctl_addr + WATCHDOG_CONFIG);

    return count;
}

static DEVICE_ATTR(identify,   S_IRUGO, drv_get_watchdog_identify, NULL);
static DEVICE_ATTR(state,      S_IRUGO, drv_get_watchdog_state, NULL);
static DEVICE_ATTR(timeleft,   S_IRUGO, drv_get_watchdog_timeleft, NULL);
static DEVICE_ATTR(timeout,    S_IRUGO|S_IWUSR, drv_get_watchdog_timeout, drv_set_watchdog_timeout);
static DEVICE_ATTR(reset,      S_IWUSR,         NULL, drv_set_watchdog_reset);
static DEVICE_ATTR(enable,     S_IRUGO|S_IWUSR, drv_get_watchdog_enable_status, drv_set_watchdog_enable_status);
static DEVICE_ATTR(rst_occur,     S_IRUGO|S_IWUSR, drv_get_watchdog_rst_flag, drv_set_watchdog_rst_flag);


static struct attribute *watchdog_debug_attributes[] =
{
    &dev_attr_identify.attr,
    &dev_attr_state.attr,
    &dev_attr_timeleft.attr,
    &dev_attr_timeout.attr,
    &dev_attr_reset.attr,
    &dev_attr_enable.attr,
    &dev_attr_rst_occur.attr,
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
        printk(KERN_ERR "create kobj_watchdog_root failed!");
        ret = -ENOMEM;
        goto exit;
    }

    ret = sysfs_create_group(kobj_watchdog_root, &watchdog_switch_attribute_group);

exit:
    if (ret != 0)
    {
        printk(KERN_ERR  "watchdog sysfs init failed!\n");
        watchdog_sysfs_exit();
    }

    return ret;
}

static int __init clounix_watchdog_init(void)
{
    int ret = 0;

    ret = watchdog_sysfs_init();
    if (ret)
    {
       printk(KERN_ERR "clounix fpga module finished and failed!\n");
    }
    else
    {
        printk(KERN_INFO "clounix fpga watchdog module finished and success!\n");    
        panic_nb.notifier_call = prolong_wdt_work;
        atomic_notifier_chain_register(&panic_notifier_list, &panic_nb);
    }

    return ret;
}


static void __exit clounix_watchdog_exit(void)
{
    atomic_notifier_chain_unregister(&panic_notifier_list, &panic_nb);

    watchdog_sysfs_exit();
    reset_watchdog();
    printk(KERN_INFO "clounix fpga watchdog module uninstalled !\n");
    return;
}

MODULE_AUTHOR("Clounix");
MODULE_DESCRIPTION("fpga watchdog driver");
MODULE_LICENSE("GPL");

module_init(clounix_watchdog_init);
module_exit(clounix_watchdog_exit);

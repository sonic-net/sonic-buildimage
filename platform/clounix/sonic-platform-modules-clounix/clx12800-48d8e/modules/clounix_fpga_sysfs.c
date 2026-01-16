#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/hwmon-sysfs.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include "clounix_fpga.h"

static ssize_t get_sys_fpga_power_cycle(struct device *dev, struct device_attribute *da,
             char *buf)
{  
    uint32_t data = 0;

    if(NULL != fpga_ctl_addr){
        data= readl(fpga_ctl_addr + FPGA_RESET_CFG_BASE);
    }
    return sprintf(buf, "0x%x\n", (data >> P12V_STBY_EN) & 0x1);
}
static ssize_t set_sys_fpga_power_cycle(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    uint32_t value = 0;
    uint32_t data = 0;

    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    if(NULL != fpga_ctl_addr){
        data= readl(fpga_ctl_addr + FPGA_RESET_CFG_BASE);
        if(1 == value)
            SET_BIT(data, P12V_STBY_EN);
        else
            CLEAR_BIT(data, P12V_STBY_EN);
        writel(data, fpga_ctl_addr + FPGA_RESET_CFG_BASE);
    }
    return count;
}

static ssize_t get_sys_fpga_pvt_temp_input(struct device *dev, struct device_attribute *da,
             char *buf)
{  
    uint32_t data = 0;

    if(NULL != fpga_ctl_addr){
        data= readl(fpga_ctl_addr + FPGA_PVT_MGR_DATA);
    }
    return sprintf(buf, "%d\n", data & FPGA_PVT_MGR_DATA_MASK);
}

static ssize_t get_sys_fpga_pvt_temp_label(struct device *dev, struct device_attribute *da,
             char *buf)
{  
    return sprintf(buf, "%s\n", "fpga pvt temp");
}
static int fpga_pvt_temp_max = 95000;
static ssize_t get_sys_fpga_pvt_temp_max(struct device *dev, struct device_attribute *da,
             char *buf)
{  
    return sprintf(buf, "%d\n", fpga_pvt_temp_max);
}

static ssize_t set_sys_fpga_pvt_temp_max(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
    /* add vendor codes here */
    unsigned int value ;
    if (fpga_ctl_addr == NULL) {
        printk(KERN_ERR  "fpga resource is not available.\r\n");
        return -ENXIO;
    }
     if (kstrtouint(buf, 10, &value))
    {
        return -EINVAL;
    }
    fpga_pvt_temp_max = value;

    return count;
}
static int fpga_pvt_temp_crit = 105000;
static ssize_t get_sys_fpga_pvt_temp_crit(struct device *dev, struct device_attribute *da,
             char *buf)
{  
    return sprintf(buf, "%d\n", fpga_pvt_temp_crit);
}
static ssize_t set_sys_fpga_pvt_temp_crit(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
    /* add vendor codes here */
    unsigned int value ;
    if (fpga_ctl_addr == NULL) {
        printk(KERN_ERR  "fpga resource is not available.\r\n");
        return -ENXIO;
    }
    if (kstrtouint(buf, 10, &value))
    {
        return -EINVAL;
    }
    fpga_pvt_temp_crit = value;

    return count;
}
static DEVICE_ATTR(power_cycle,S_IRUGO | S_IWUSR, get_sys_fpga_power_cycle, set_sys_fpga_power_cycle);
static SENSOR_DEVICE_ATTR(pvt_temp1_input,S_IRUGO, get_sys_fpga_pvt_temp_input, NULL,0);
static SENSOR_DEVICE_ATTR(pvt_temp1_max,S_IRUGO | S_IWUSR , get_sys_fpga_pvt_temp_max, set_sys_fpga_pvt_temp_max,0);
static SENSOR_DEVICE_ATTR(pvt_temp1_crit,S_IRUGO | S_IWUSR, get_sys_fpga_pvt_temp_crit, set_sys_fpga_pvt_temp_crit,0);
static SENSOR_DEVICE_ATTR(pvt_temp1_label,S_IRUGO | S_IWUSR, get_sys_fpga_pvt_temp_label, NULL,0);
static struct attribute *fpga_attributes[] =
{
    &dev_attr_power_cycle.attr,
    &sensor_dev_attr_pvt_temp1_input.dev_attr.attr,
    &sensor_dev_attr_pvt_temp1_max.dev_attr.attr,
    &sensor_dev_attr_pvt_temp1_crit.dev_attr.attr,
    &sensor_dev_attr_pvt_temp1_label.dev_attr.attr,
    NULL
};

static const struct attribute_group fpga_attribute_group =
{
    .attrs = fpga_attributes,
};
static int __init clounix_fpga_sysfs_init(void)
{
    int err = 0;
    struct pci_dev *pdev = pci_get_device(FPGA_VENDOR_ID, FPGA_DEVICE_ID, NULL);
    if(pdev) {
        err = sysfs_create_group(&pdev->dev.kobj,&fpga_attribute_group);
        if (err) {
	         printk(KERN_ERR  "sysfs_create_file error status %d\n", err);
        }
    }else {
        printk(KERN_ERR "no fpga device vendorid:0x%x  deviceid:0x%x\n", FPGA_VENDOR_ID,FPGA_DEVICE_ID);
        return -1;
    }
    return err;
}

static void __exit clounix_fpga_sysfs_exit(void)
{
    struct pci_dev *pdev = pci_get_device(FPGA_VENDOR_ID, FPGA_DEVICE_ID, NULL);
    if(pdev) {
        sysfs_remove_group(&pdev->dev.kobj, &fpga_attribute_group);
    }
    return;
}

MODULE_AUTHOR("Clounix");
MODULE_DESCRIPTION("clounix fpga ");
MODULE_LICENSE("GPL");

module_init(clounix_fpga_sysfs_init);
module_exit(clounix_fpga_sysfs_exit);

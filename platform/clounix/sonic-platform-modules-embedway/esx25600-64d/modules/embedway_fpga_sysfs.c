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
#include "embedway_fpga.h"

#define CPU_CPLD_I2C_ADDR  (0x62)
#define CPU_CPLD_NAME  "CPLD-CB"
#define POWER_CYCLE_REG 0x54
#define POWER_CYCLE_BIT 0
#define CPI_LED_CFG_REG 0x340
#define CPI_LED_CFG_BIT_OFFSET 8
#define CPI_LED_CFG_BIT_MASK 0x7

#define REBOOT_CAUSE_REG 0x55
#define CAUSE_POWER_LOSS_BIT 0
#define CAUSE_HW_OTHER_BIT 1
#define CAUSE_COLD_RESET_BIT 2
#define CAUSE_BIOS_RESET_BIT 3
#define CAUSE_PSU_SHUTDOWN_BIT 4
#define CAUSE_BMC_SHUTDOWN_BIT 5

extern int board_i2c_cpld_read_new(unsigned short cpld_addr, char *name, u8 reg);
extern int board_i2c_cpld_write_new(unsigned short cpld_addr, char *name, u8 reg, u8 value);

static int cpu_cpld_read(u8 offset)
{
    int ret;

    ret =  board_i2c_cpld_read_new(CPU_CPLD_I2C_ADDR, CPU_CPLD_NAME, offset);
    if (unlikely(ret < 0))
    {
        printk(KERN_ERR  "board_i2c_cpld_read_new failed,err=%d\r\n",ret);
    }

    return ret;
}

static int cpu_cpld_write(u8 val, u8 offset)
{
    int ret;
    
    ret =  board_i2c_cpld_write_new(CPU_CPLD_I2C_ADDR, CPU_CPLD_NAME, offset, val);
    if (unlikely(ret < 0))
    {
        printk(KERN_ERR  "board_i2c_cpld_write_new failed,err=%d\r\n",ret);
    }

    return ret;
}

static ssize_t get_sys_fpga_power_cycle(struct device *dev, struct device_attribute *da,
             char *buf)
{  
    unsigned int  data = 0 ;
    data= cpu_cpld_read(POWER_CYCLE_REG);
    return sprintf(buf, "0x%x\n", data & POWER_CYCLE_BIT);
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

    data= cpu_cpld_read(POWER_CYCLE_REG);
    if(1 == value)
        SET_BIT(data, POWER_CYCLE_BIT);
    else
        CLEAR_BIT(data, POWER_CYCLE_BIT);
    cpu_cpld_write(data, POWER_CYCLE_REG);

    return count;
}
/* cpi_led_cfg
0— 熄灭(default)
1— 绿灯亮
2— 红灯亮
3— 黄灯亮
4— 绿灯闪烁
5— 红灯闪烁
6— 黄灯闪烁
*/
static ssize_t get_cpi_led_cfg(struct device *dev, struct device_attribute *da,
             char *buf)
{  
    uint32_t data = 0;

    if(NULL != fpga_ctl_addr){
        data= readl(fpga_ctl_addr + CPI_LED_CFG_REG);
    }
    return sprintf(buf, "%d\n", ((data >> CPI_LED_CFG_BIT_OFFSET) & CPI_LED_CFG_BIT_MASK));
}
static ssize_t set_cpi_led_cfg(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    uint32_t value = 0;
    uint32_t data = 0;

    if (kstrtouint(buf, 0, &value))
    {
        return -EINVAL;
    }

    if(NULL != fpga_ctl_addr){
        data= readl(fpga_ctl_addr + CPI_LED_CFG_REG);
        data &= ~(CPI_LED_CFG_BIT_MASK << CPI_LED_CFG_BIT_OFFSET);//clear old cpi led cfg
        data |= ((value & CPI_LED_CFG_BIT_MASK) << CPI_LED_CFG_BIT_OFFSET);//set new cpi led cfg
        writel(data, fpga_ctl_addr + CPI_LED_CFG_REG);
    }
    return count;
}

static ssize_t get_cause_pwr_loss(struct device *dev, struct device_attribute *da,
             char *buf)
{  
    unsigned int  data = 0, bitval;
    data= cpu_cpld_read(REBOOT_CAUSE_REG);
    GET_BIT(data, CAUSE_POWER_LOSS_BIT, bitval);
    return sprintf(buf, "%d\n", bitval);
}

static ssize_t set_cause_pwr_loss(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    uint32_t value = 0;
    uint32_t data = 0;

    if (kstrtouint(buf, 10, &value))
    {
        return -EINVAL;
    }

    data= cpu_cpld_read(REBOOT_CAUSE_REG);
    if(1 == value)
        SET_BIT(data, CAUSE_POWER_LOSS_BIT);
    else
        CLEAR_BIT(data, CAUSE_POWER_LOSS_BIT);
    cpu_cpld_write(data, REBOOT_CAUSE_REG);

    return count;
}

static ssize_t get_cause_hw_other(struct device *dev, struct device_attribute *da,
             char *buf)
{  
    unsigned int  data = 0, bitval;
    data= cpu_cpld_read(REBOOT_CAUSE_REG);
    GET_BIT(data, CAUSE_HW_OTHER_BIT, bitval);
    return sprintf(buf, "%d\n", bitval);
}

static ssize_t set_cause_hw_other(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    uint32_t value = 0;
    uint32_t data = 0;

    if (kstrtouint(buf, 10, &value))
    {
        return -EINVAL;
    }

    data= cpu_cpld_read(REBOOT_CAUSE_REG);
    if(1 == value)
        SET_BIT(data, CAUSE_HW_OTHER_BIT);
    else
        CLEAR_BIT(data, CAUSE_HW_OTHER_BIT);
    cpu_cpld_write(data, REBOOT_CAUSE_REG);

    return count;
}

static ssize_t get_cause_cold_reset(struct device *dev, struct device_attribute *da,
             char *buf)
{  
    unsigned int  data = 0, bitval;
    data= cpu_cpld_read(REBOOT_CAUSE_REG);
    GET_BIT(data, CAUSE_COLD_RESET_BIT, bitval);
    return sprintf(buf, "%d\n", bitval);
}

static ssize_t set_cause_cold_reset(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    uint32_t value = 0;
    uint32_t data = 0;

    if (kstrtouint(buf, 10, &value))
    {
        return -EINVAL;
    }

    data= cpu_cpld_read(REBOOT_CAUSE_REG);
    if(1 == value)
        SET_BIT(data, CAUSE_COLD_RESET_BIT);
    else
        CLEAR_BIT(data, CAUSE_COLD_RESET_BIT);
    cpu_cpld_write(data, REBOOT_CAUSE_REG);

    return count;
}

static ssize_t get_cause_bios_reset(struct device *dev, struct device_attribute *da,
             char *buf)
{  
    unsigned int  data = 0, bitval;
    data= cpu_cpld_read(REBOOT_CAUSE_REG);
    GET_BIT(data, CAUSE_BIOS_RESET_BIT, bitval);
    return sprintf(buf, "%d\n", bitval);
}

static ssize_t set_cause_bios_reset(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    uint32_t value = 0;
    uint32_t data = 0;

    if (kstrtouint(buf, 10, &value))
    {
        return -EINVAL;
    }

    data= cpu_cpld_read(REBOOT_CAUSE_REG);
    if(1 == value)
        SET_BIT(data, CAUSE_BIOS_RESET_BIT);
    else
        CLEAR_BIT(data, CAUSE_BIOS_RESET_BIT);
    cpu_cpld_write(data, REBOOT_CAUSE_REG);

    return count;
}

static ssize_t get_cause_psu_shutdown(struct device *dev, struct device_attribute *da,
             char *buf)
{  
    unsigned int  data = 0, bitval;
    data= cpu_cpld_read(REBOOT_CAUSE_REG);
    GET_BIT(data, CAUSE_PSU_SHUTDOWN_BIT, bitval);
    return sprintf(buf, "%d\n", bitval);
}

static ssize_t set_cause_psu_shutdown(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    uint32_t value = 0;
    uint32_t data = 0;

    if (kstrtouint(buf, 10, &value))
    {
        return -EINVAL;
    }

    data= cpu_cpld_read(REBOOT_CAUSE_REG);
    if(1 == value)
        SET_BIT(data, CAUSE_PSU_SHUTDOWN_BIT);
    else
        CLEAR_BIT(data, CAUSE_PSU_SHUTDOWN_BIT);
    cpu_cpld_write(data, REBOOT_CAUSE_REG);

    return count;
}

static ssize_t get_cause_bmc_shutdown(struct device *dev, struct device_attribute *da,
             char *buf)
{  
    unsigned int  data = 0, bitval;
    data= cpu_cpld_read(REBOOT_CAUSE_REG);
    GET_BIT(data, CAUSE_BMC_SHUTDOWN_BIT, bitval);
    return sprintf(buf, "%d\n", bitval);
}

static ssize_t set_cause_bmc_shutdown(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    uint32_t value = 0;
    uint32_t data = 0;

    if (kstrtouint(buf, 10, &value))
    {
        return -EINVAL;
    }

    data= cpu_cpld_read(REBOOT_CAUSE_REG);
    if(1 == value)
        SET_BIT(data, CAUSE_BMC_SHUTDOWN_BIT);
    else
        CLEAR_BIT(data, CAUSE_BMC_SHUTDOWN_BIT);
    cpu_cpld_write(data, REBOOT_CAUSE_REG);

    return count;
}
static void enable_pvt_temp(void)
{   
    uint32_t data = 0;
    data = readl(fpga_ctl_addr + FPGA_PVT_BASE);
    if((data &(1 << FPGA_PVT_MGR_CFG_EN_BIT)) == 0)
    {    /*unreset PVT*/
        data &= ~(1 << FPGA_PVT_MGR_CFG_RST_BIT);
        /*enable PVT*/
        data |= (1 << FPGA_PVT_MGR_CFG_EN_BIT);
        //printk("data is 0x%x\n",data);
        writel(data, fpga_ctl_addr + FPGA_PVT_BASE);
    }
    return;
}

static ssize_t get_sys_fpga_pvt_temp_input(struct device *dev, struct device_attribute *da,
             char *buf)
{  
    uint32_t data = 0;

    if(NULL != fpga_ctl_addr){
        enable_pvt_temp();
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

static ssize_t set_fpga_cpld_reset(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    uint32_t value = 0;
    uint32_t data = 0;

    if (kstrtouint(buf, 0, &value))
    {
        return -EINVAL;
    }
    if (value != 1) return -EINVAL;
    if(NULL != fpga_ctl_addr){
        data = readl(fpga_ctl_addr + CPLD_BASE_ADDRESS);
        /*reset CPLD0*/
        data |= (1 << CPLD0_RST_BIT);
        /*enable CPLD0*/
        data |= (1 << CPLD0_EN_BIT);
        /*reset CPLD1*/
        data |= (1 << CPLD1_RST_BIT);
        /*enable CPLD1*/
        data |= (1 << CPLD1_EN_BIT);
        //printk("data is 0x%x\n",data);
        writel(data, fpga_ctl_addr + CPLD_BASE_ADDRESS);
    }
    return count;
}

static DEVICE_ATTR(power_cycle,S_IRUGO | S_IWUSR, get_sys_fpga_power_cycle, set_sys_fpga_power_cycle);
static DEVICE_ATTR(cpi_led_cfg,S_IRUGO | S_IWUSR, get_cpi_led_cfg, set_cpi_led_cfg);
static DEVICE_ATTR(fpga_cpld_reset,S_IRUGO | S_IWUSR, NULL, set_fpga_cpld_reset);
static SENSOR_DEVICE_ATTR(pvt_temp1_input,S_IRUGO, get_sys_fpga_pvt_temp_input, NULL,0);
static SENSOR_DEVICE_ATTR(pvt_temp1_max,S_IRUGO | S_IWUSR , get_sys_fpga_pvt_temp_max, set_sys_fpga_pvt_temp_max,0);
static SENSOR_DEVICE_ATTR(pvt_temp1_crit,S_IRUGO | S_IWUSR, get_sys_fpga_pvt_temp_crit, set_sys_fpga_pvt_temp_crit,0);
static SENSOR_DEVICE_ATTR(pvt_temp1_label,S_IRUGO | S_IWUSR, get_sys_fpga_pvt_temp_label, NULL,0);
static DEVICE_ATTR(cause_pwr_loss,S_IRUGO | S_IWUSR, get_cause_pwr_loss, set_cause_pwr_loss);
static DEVICE_ATTR(cause_hw_other,S_IRUGO | S_IWUSR, get_cause_hw_other, set_cause_hw_other);
static DEVICE_ATTR(cause_cold_reset,S_IRUGO | S_IWUSR, get_cause_cold_reset, set_cause_cold_reset);
static DEVICE_ATTR(cause_bios_reset,S_IRUGO | S_IWUSR, get_cause_bios_reset, set_cause_bios_reset);
static DEVICE_ATTR(cause_psu_shutdown,S_IRUGO | S_IWUSR, get_cause_psu_shutdown, set_cause_psu_shutdown);
static DEVICE_ATTR(cause_bmc_shutdown,S_IRUGO | S_IWUSR, get_cause_bmc_shutdown, set_cause_bmc_shutdown);
static struct attribute *fpga_attributes[] =
{
    &dev_attr_power_cycle.attr,
    &dev_attr_cpi_led_cfg.attr,
    &dev_attr_fpga_cpld_reset.attr,
    &sensor_dev_attr_pvt_temp1_input.dev_attr.attr,
    &sensor_dev_attr_pvt_temp1_max.dev_attr.attr,
    &sensor_dev_attr_pvt_temp1_crit.dev_attr.attr,
    &sensor_dev_attr_pvt_temp1_label.dev_attr.attr,
    &dev_attr_cause_pwr_loss.attr,
    &dev_attr_cause_hw_other.attr,
    &dev_attr_cause_cold_reset.attr,
    &dev_attr_cause_bios_reset.attr,
    &dev_attr_cause_psu_shutdown.attr,
    &dev_attr_cause_bmc_shutdown.attr,
    NULL
};

static const struct attribute_group fpga_attribute_group =
{
    .attrs = fpga_attributes,
};
static int __init embedway_fpga_sysfs_init(void)
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

static void __exit embedway_fpga_sysfs_exit(void)
{
    struct pci_dev *pdev = pci_get_device(FPGA_VENDOR_ID, FPGA_DEVICE_ID, NULL);
    if(pdev) {
        sysfs_remove_group(&pdev->dev.kobj, &fpga_attribute_group);
    }
    return;
}

MODULE_AUTHOR("Embedway");
MODULE_DESCRIPTION("embedway fpga ");
MODULE_LICENSE("GPL");

module_init(embedway_fpga_sysfs_init);
module_exit(embedway_fpga_sysfs_exit);

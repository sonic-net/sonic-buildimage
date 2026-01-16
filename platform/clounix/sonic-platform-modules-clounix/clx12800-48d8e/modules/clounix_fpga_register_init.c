#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include "clounix_fpga.h"

static int clounix_fpga_cpld_init(void)
{
	int data;
    if(NULL != fpga_ctl_addr){
        data = readl(fpga_ctl_addr + CPLD_BASE_ADDRESS);
        /*unreset CPLD0*/
        data &= ~(1 << CPLD0_RST_BIT);
        /*enable CPLD0*/
        data |= (1 << CPLD0_EN_BIT);
        /*unreset CPLD1*/
        data &= ~(1 << CPLD1_RST_BIT);
        /*enable CPLD1*/
        data |= (1 << CPLD1_EN_BIT);
        //printk("data is 0x%x\n",data);
        writel(data, fpga_ctl_addr + CPLD_BASE_ADDRESS);
    }else {
        printk(KERN_INFO "fpga resource is not available.\r\n");
        return -1;
    }
    return 0;
}
static int clounix_fpga_pvt_init(void)
{
	int data;
    if(NULL != fpga_ctl_addr){
        data = readl(fpga_ctl_addr + FPGA_PVT_BASE);
        /*unreset PVT*/
        data &= ~(1 << FPGA_PVT_MGR_CFG_RST_BIT);
        /*enable PVT*/
        data |= (1 << FPGA_PVT_MGR_CFG_EN_BIT);
        //printk("data is 0x%x\n",data);
        writel(data, fpga_ctl_addr + FPGA_PVT_BASE);
    }else {
        printk(KERN_INFO "fpga resource is not available.\r\n");
        return -1;
    }
    return 0;
}
static int __init clounix_fpga_register_init(void)
{
    int ret = 0;
    ret = clounix_fpga_cpld_init();
    ret |= clounix_fpga_pvt_init();
    if(ret){
        printk(KERN_INFO "clounix_fpga_register_init fail(0x%x).\r\n",ret);
    }
      return ret;
}
static void __exit clounix_fpga_register_exit(void)
{
    return;
}

MODULE_AUTHOR("Clounix");
MODULE_DESCRIPTION("clounix cpld");
MODULE_LICENSE("GPL");

module_init(clounix_fpga_register_init);
module_exit(clounix_fpga_register_exit);

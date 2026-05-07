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

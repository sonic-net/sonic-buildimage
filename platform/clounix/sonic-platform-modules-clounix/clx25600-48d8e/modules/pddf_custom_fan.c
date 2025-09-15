/*******************************************************************************

*  Copyright©[2020-2024] [Hangzhou Embedway Technology Limited]. 

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

#define __STDC_WANT_LIB_EXT1__ 1
#include <linux/string.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/dmi.h>

#include "pddf_client_defs.h"
#include "pddf_fan_defs.h"
#include "pddf_fan_driver.h"

extern void *get_device_table(char *name);

int sonic_i2c_set_fan_pwm_custom(void *client, FAN_DATA_ATTR *udata, void *info);

extern FAN_SYSFS_ATTR_DATA data_fan1_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan2_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan3_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan4_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan5_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan6_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan7_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan8_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan9_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan10_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan11_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan12_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan12_pwm;

int sonic_i2c_set_fan_pwm_custom(void *client, FAN_DATA_ATTR *udata, void *info)
{
    int status = 0;
    int val = 0;
    int reg_val = 0;
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;

    val = painfo->val.intval & udata->mask;

    if (val > 255)
    {
        return -EINVAL;
    }

    /*clounix EVB max PWM is 100*/
    if(val > 100)
    {
        val = 100;
    }

    if (strncmp(udata->devtype, "fpgapci", strlen("fpgapci")) == 0)
    {
        reg_val = ptr_fpgapci_read(udata->devaddr);
        reg_val &= ~(udata->mask << udata->offset);
        reg_val |= (val << udata->offset);
        status = ptr_fpgapci_write(udata->devaddr,reg_val);
    }
    else
    {
        if (udata->len == 1)
            status = i2c_smbus_write_byte_data(client, udata->offset, val);
        else if (udata->len == 2)
        {
            uint8_t val_lsb = val & 0xFF;
            uint8_t val_hsb = (val >> 8) & 0xFF;
            /* TODO: Check this logic for LE and BE */
            status = i2c_smbus_write_byte_data(client, udata->offset, val_lsb);
            if (status == 0) status = i2c_smbus_write_byte_data(client, udata->offset+1, val_hsb);
        }
        else
        {
            printk(KERN_ERR "PDDF_FAN_ERROR: %s: pwm should be of len 1/2 bytes. Not setting the pwm as the length is %d\n", __FUNCTION__, udata->len);
        }
    }

    return status;
}

static int __init pddf_custom_fan_init(void)
{
    data_fan1_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan2_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan3_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan4_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan5_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan6_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan7_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan8_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan9_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan10_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan11_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan12_pwm.do_set = sonic_i2c_set_fan_pwm_custom;

    return 0;
}

static void __exit pddf_custom_fan_exit(void)
{
    return;
}

MODULE_AUTHOR("Clounix");
MODULE_DESCRIPTION("pddf custom fan api");
MODULE_LICENSE("GPL");

module_init(pddf_custom_fan_init);
module_exit(pddf_custom_fan_exit);
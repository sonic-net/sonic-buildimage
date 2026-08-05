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

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pmbus.h>
#include <linux/sysfs.h>
#include <linux/stat.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include "pmbus.h"


#define TPS_VOUT_MARGIN 0xD5
#define TPS_VOM_STEP_NUM 13
const char *margin_up[] = {"0", "+0.9", "+1.8", "+2.8", "+3.7", "+4.7", "+5.7", "+6.7", "+7.7", "+8.8", "+9.9", "+10.9", "+12.0"};
const char *margin_down[] = {"0", "-1.1", "-2.1", "-3.2", "-4.2", "-5.2", "-6.2", "-7.1", "-8.1", "-9.0", "-9.9", "-10.7", "-11.6"};

ssize_t vout_margin_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int status = 0;
    unsigned int vom_val_h, vom_val_l;
    struct i2c_client *client = to_i2c_client(dev->parent);

    status = i2c_smbus_read_byte_data(client, TPS_VOUT_MARGIN);
    if (status >= 0)
    {
        vom_val_h = (status & 0xF0) >> 4;
        vom_val_l = status & 0xF;

        if(vom_val_h > 0)
        {
            if(vom_val_h >= (TPS_VOM_STEP_NUM - 1))
            {
                return sprintf(buf, "%s%%\n", margin_up[TPS_VOM_STEP_NUM - 1]);
            }
            else
            {
                return sprintf(buf, "%s%%\n", margin_up[vom_val_h]);
            }
        }
        else
        {
            if(vom_val_l >= (TPS_VOM_STEP_NUM - 1))
            {
                return sprintf(buf, "%s%%\n", margin_down[TPS_VOM_STEP_NUM - 1]);
            }
            else
            {
                return sprintf(buf, "%s%%\n", margin_down[vom_val_l]);
            }
        }
    }

    return status;
}

SENSOR_DEVICE_ATTR(vout_margin, S_IWUSR|S_IRUGO, vout_margin_show, NULL, TPS_VOUT_MARGIN);

static struct attribute *tps_psu_attrs[] = {
    &sensor_dev_attr_vout_margin.dev_attr.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = tps_psu_attrs,
};

static const struct attribute_group *attr_groups[] = {
    &attr_group,
    NULL,
};
/*
static int tps_read_byte_data(struct i2c_client *client, int page, int reg)
{
    int rv = 0;
    switch(reg) {
        case TPS_VOUT_MARGIN:
            rv = pmbus_read_byte_data(client, page, reg);
            return (rv&0xFF);

        default:
            break;
    }

    return -ENODATA;
}
*/
static struct pmbus_platform_data pdata = {
    .flags = PMBUS_SKIP_STATUS_CHECK,
};
static int tps_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    struct device *dev = &client->dev;
    struct device *hwmon_dev;

    printk(KERN_INFO "load tps53915 driver\n");
    client->dev.platform_data = &pdata;
 
    hwmon_dev = devm_hwmon_device_register_with_groups(dev, client->name,
                                                       client, attr_groups);
    if (IS_ERR(hwmon_dev))
        return PTR_ERR(hwmon_dev);

    return 0;
}
static void tps_remove(struct i2c_client *client)
{
    return;
}
static const struct i2c_device_id tps_id[] = {
    {"tps53915", 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, tps_id);

static const struct of_device_id __maybe_unused tps_of_match[] = {
    {.compatible = "TI,tps53915"},
    {}
};
MODULE_DEVICE_TABLE(of, tps_of_match);

static struct i2c_driver tps_driver = {
    .driver = {
        .name = "Clounix tps",
        .of_match_table = of_match_ptr(tps_of_match),
    },
    .probe = tps_probe,
    .remove = tps_remove,
    .id_table = tps_id,
};
module_i2c_driver(tps_driver);

MODULE_AUTHOR("daiaq@clounix.com");
MODULE_DESCRIPTION("PMBus driver for TPS53915");
MODULE_LICENSE("GPL");

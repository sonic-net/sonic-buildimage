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
#include "pddf_fan_defs.h"
#include "pddf_fan_driver.h"

extern FAN_SYSFS_ATTR_DATA data_fan1_input;
extern FAN_SYSFS_ATTR_DATA data_fan2_input;
extern FAN_SYSFS_ATTR_DATA data_fan3_input;
extern FAN_SYSFS_ATTR_DATA data_fan4_input;
extern FAN_SYSFS_ATTR_DATA data_fan5_input;
extern FAN_SYSFS_ATTR_DATA data_fan6_input;
extern FAN_SYSFS_ATTR_DATA data_fan7_input;
extern FAN_SYSFS_ATTR_DATA data_fan8_input;

extern void *get_device_table(char *name);

int sonic_i2c_get_fan_rpm_custom(void *client, FAN_DATA_ATTR *udata, void *info)
{
    int status = 0;
    uint32_t val = 0, reg_val = 0;
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;
    struct i2c_client *client_ptr=NULL;

    client_ptr = (struct i2c_client *)client;
    if (client_ptr)
    {
        reg_val = i2c_smbus_read_byte_data(client_ptr, udata->offset);
        if (reg_val == 0 || reg_val == 255) {
            val = 0;
        }
        else
        {
            u64 f, dv;
            dv = 2 * 2 * 40960 * (u64)(255 - reg_val);
            f = 60000000000 / dv;
            val = (u32)f;
        }
    }
    else
        printk(KERN_ERR "Unable to get the client handle\n");

    painfo->val.intval = val;
    /*printk(KERN_ERR "INSIDE CUSTOM GET FAN RPM FUNC... Get value from offset 0x%x ... value is 0x%x\n", udata->offset, val);*/
    return status;
}
EXPORT_SYMBOL(sonic_i2c_get_fan_rpm_custom);


static int __init pddf_custom_fan_init(void)
{
    data_fan1_input.do_get = sonic_i2c_get_fan_rpm_custom;
    data_fan2_input.do_get = sonic_i2c_get_fan_rpm_custom;
    data_fan3_input.do_get = sonic_i2c_get_fan_rpm_custom;
    data_fan4_input.do_get = sonic_i2c_get_fan_rpm_custom;
    data_fan5_input.do_get = sonic_i2c_get_fan_rpm_custom;
    data_fan6_input.do_get = sonic_i2c_get_fan_rpm_custom;
    data_fan7_input.do_get = sonic_i2c_get_fan_rpm_custom;
    data_fan8_input.do_get = sonic_i2c_get_fan_rpm_custom;
    return 0;
}

static void __exit pddf_custom_fan_exit(void)
{
	return;
}

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("pddf custom fan api");
MODULE_LICENSE("GPL");

module_init(pddf_custom_fan_init);
module_exit(pddf_custom_fan_exit);


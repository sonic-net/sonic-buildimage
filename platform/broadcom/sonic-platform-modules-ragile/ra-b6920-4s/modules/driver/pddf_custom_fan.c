#include <linux/module.h>
#include "../../../../../pddf/i2c/modules/include/pddf_client_defs.h"
#include "../../../../../pddf/i2c/modules/include/pddf_fan_defs.h"
#include "../../../../../pddf/i2c/modules/include/pddf_fan_driver.h"
#include "../../../../../pddf/i2c/modules/include/pddf_fan_api.h"

extern void *get_device_table(char *name);

extern FAN_SYSFS_ATTR_DATA data_fan1_present;
extern FAN_SYSFS_ATTR_DATA data_fan2_present;
extern FAN_SYSFS_ATTR_DATA data_fan3_present;
extern FAN_SYSFS_ATTR_DATA data_fan4_present;
extern FAN_SYSFS_ATTR_DATA data_fan5_present;
extern FAN_SYSFS_ATTR_DATA data_fan6_present;
extern FAN_SYSFS_ATTR_DATA data_fan7_present;
extern FAN_SYSFS_ATTR_DATA data_fan8_present;
extern FAN_SYSFS_ATTR_DATA data_fan9_present;
extern FAN_SYSFS_ATTR_DATA data_fan10_present;
extern FAN_SYSFS_ATTR_DATA data_fan11_present;
extern FAN_SYSFS_ATTR_DATA data_fan12_present;

extern FAN_SYSFS_ATTR_DATA data_fan1_input;
extern FAN_SYSFS_ATTR_DATA data_fan2_input;
extern FAN_SYSFS_ATTR_DATA data_fan3_input;
extern FAN_SYSFS_ATTR_DATA data_fan4_input;
extern FAN_SYSFS_ATTR_DATA data_fan5_input;
extern FAN_SYSFS_ATTR_DATA data_fan6_input;
extern FAN_SYSFS_ATTR_DATA data_fan7_input;
extern FAN_SYSFS_ATTR_DATA data_fan8_input;
extern FAN_SYSFS_ATTR_DATA data_fan9_input;
extern FAN_SYSFS_ATTR_DATA data_fan10_input;
extern FAN_SYSFS_ATTR_DATA data_fan11_input;
extern FAN_SYSFS_ATTR_DATA data_fan12_input;

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

extern FAN_SYSFS_ATTR_DATA data_fan1_fault;
extern FAN_SYSFS_ATTR_DATA data_fan2_fault;
extern FAN_SYSFS_ATTR_DATA data_fan3_fault;
extern FAN_SYSFS_ATTR_DATA data_fan4_fault;
extern FAN_SYSFS_ATTR_DATA data_fan5_fault;
extern FAN_SYSFS_ATTR_DATA data_fan6_fault;
extern FAN_SYSFS_ATTR_DATA data_fan7_fault;
extern FAN_SYSFS_ATTR_DATA data_fan8_fault;
extern FAN_SYSFS_ATTR_DATA data_fan9_fault;
extern FAN_SYSFS_ATTR_DATA data_fan10_fault;
extern FAN_SYSFS_ATTR_DATA data_fan11_fault;
extern FAN_SYSFS_ATTR_DATA data_fan12_fault;

int pddf_custom_fan_present(void *client, FAN_DATA_ATTR *udata, void *info);
int pddf_custom_fan_input(void *client, FAN_DATA_ATTR *udata, void *info);
int pddf_custom_fan_fault(void *client, FAN_DATA_ATTR *udata, void *info);
int pddf_custom_fan_pwm(void *client, FAN_DATA_ATTR *udata, void *info);
int pddf_custom_fan_set_pwm(void *client, FAN_DATA_ATTR *udata, void *info);

int pddf_custom_fan_present(void *client, FAN_DATA_ATTR *udata, void *info)
{
    uint32_t val;
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;

    val = 0;

    if (strcmp(udata->devtype, "cpld") == 0) {
        if (udata->devname) {
            client = (struct i2c_client *)get_device_table(udata->devname);
        }
    }
    val = i2c_smbus_read_byte_data((struct i2c_client *)client, udata->offset);
    /* printk("%s read data %x\n", __FUNCTION__, val); */

    painfo->val.intval = ((val & udata->mask) == udata->cmpval);

    return 0;
}

int pddf_custom_fan_input(void *client, FAN_DATA_ATTR *udata, void *info)
{
    uint32_t val;
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;

    val = 0;

    if (strcmp(udata->devtype, "cpld") == 0) {
        if (udata->devname) {
            client = (struct i2c_client *)get_device_table(udata->devname);
        }
    }
    if (udata->len == 1) {
        val = i2c_smbus_read_byte_data(client, udata->offset);
    } else if (udata->len == 2) {
        val = i2c_smbus_read_word_swapped((struct i2c_client *)client, udata->offset);
    }

    /* printk("%s read data %x\n", __FUNCTION__, val); */

    if (udata->is_divisor)
        painfo->val.intval = udata->mult / (val >> 3);
    else
        painfo->val.intval = udata->mult * val;

    return 0;
}

int pddf_custom_fan_pwm(void *client, FAN_DATA_ATTR *udata, void *info)
{
    uint32_t val;
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;

    val = 0;

    if (strcmp(udata->devtype, "cpld") == 0) {
        if (udata->devname) {
            client = (struct i2c_client *)get_device_table(udata->devname);
        }
    }
    if (udata->len == 1) {
        val = i2c_smbus_read_byte_data(client, udata->offset);
    } else if (udata->len == 2) {
        val = i2c_smbus_read_word_swapped((struct i2c_client *)client, udata->offset);
    }
    /* printk("%s read data %x\n", __FUNCTION__, val); */

    val = val & udata->mask;
    painfo->val.intval = val;

    return 0;
}

int pddf_custom_fan_set_pwm(void *client, FAN_DATA_ATTR *udata, void *info)
{
    uint32_t val;
    struct fan_attr_info *painfo;

    val = 0;
    painfo = (struct fan_attr_info *)info;

    val = painfo->val.intval & udata->mask;

    if (val > 255) {
        return -EINVAL;
    }

    if (strcmp(udata->devtype, "cpld") == 0 && udata->devname) {
        client = (struct i2c_client *)get_device_table(udata->devname);
        i2c_smbus_write_byte_data(client, udata->offset, val);
    }

    return 0;
}


int pddf_custom_fan_fault(void *client, FAN_DATA_ATTR *udata, void *info)
{
    uint32_t val;
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;

    val = 0;

    if (strcmp(udata->devtype, "cpld") == 0) {
        if (udata->devname) {
            client = (struct i2c_client *)get_device_table(udata->devname);
        }
    }
    val = i2c_smbus_read_byte_data((struct i2c_client *)client, udata->offset);
    /* printk("%s read data %x\n", __FUNCTION__, val); */

    val = val & udata->mask;
    painfo->val.intval = val;

    return 0;
}

int __init pddf_custom_fan_init(void)
{
    printk(KERN_ERR "pddf_custom_fan_init\n");

    data_fan1_present.do_get = pddf_custom_fan_present;
    data_fan2_present.do_get = pddf_custom_fan_present;
    data_fan3_present.do_get = pddf_custom_fan_present;
    data_fan4_present.do_get = pddf_custom_fan_present;
    data_fan5_present.do_get = pddf_custom_fan_present;
    data_fan6_present.do_get = pddf_custom_fan_present;
    data_fan7_present.do_get = pddf_custom_fan_present;
    data_fan8_present.do_get = pddf_custom_fan_present;
    data_fan9_present.do_get = pddf_custom_fan_present;
    data_fan10_present.do_get = pddf_custom_fan_present;
    data_fan11_present.do_get = pddf_custom_fan_present;
    data_fan12_present.do_get = pddf_custom_fan_present;

    data_fan1_input.do_get = pddf_custom_fan_input;
    data_fan2_input.do_get = pddf_custom_fan_input;
    data_fan3_input.do_get = pddf_custom_fan_input;
    data_fan4_input.do_get = pddf_custom_fan_input;
    data_fan5_input.do_get = pddf_custom_fan_input;
    data_fan6_input.do_get = pddf_custom_fan_input;
    data_fan7_input.do_get = pddf_custom_fan_input;
    data_fan8_input.do_get = pddf_custom_fan_input;
    data_fan9_input.do_get = pddf_custom_fan_input;
    data_fan10_input.do_get = pddf_custom_fan_input;
    data_fan11_input.do_get = pddf_custom_fan_input;
    data_fan12_input.do_get = pddf_custom_fan_input;

    data_fan1_pwm.do_get = pddf_custom_fan_pwm;
    data_fan2_pwm.do_get = pddf_custom_fan_pwm;
    data_fan3_pwm.do_get = pddf_custom_fan_pwm;
    data_fan4_pwm.do_get = pddf_custom_fan_pwm;
    data_fan5_pwm.do_get = pddf_custom_fan_pwm;
    data_fan6_pwm.do_get = pddf_custom_fan_pwm;
    data_fan7_pwm.do_get = pddf_custom_fan_pwm;
    data_fan8_pwm.do_get = pddf_custom_fan_pwm;
    data_fan9_pwm.do_get = pddf_custom_fan_pwm;
    data_fan10_pwm.do_get = pddf_custom_fan_pwm;
    data_fan11_pwm.do_get = pddf_custom_fan_pwm;
    data_fan12_pwm.do_get = pddf_custom_fan_pwm;

    data_fan1_pwm.do_set = pddf_custom_fan_set_pwm;
    data_fan2_pwm.do_set = pddf_custom_fan_set_pwm;
    data_fan3_pwm.do_set = pddf_custom_fan_set_pwm;
    data_fan4_pwm.do_set = pddf_custom_fan_set_pwm;
    data_fan5_pwm.do_set = pddf_custom_fan_set_pwm;
    data_fan6_pwm.do_set = pddf_custom_fan_set_pwm;
    data_fan7_pwm.do_set = pddf_custom_fan_set_pwm;
    data_fan8_pwm.do_set = pddf_custom_fan_set_pwm;
    data_fan9_pwm.do_set = pddf_custom_fan_set_pwm;
    data_fan10_pwm.do_set = pddf_custom_fan_set_pwm;
    data_fan11_pwm.do_set = pddf_custom_fan_set_pwm;
    data_fan12_pwm.do_set = pddf_custom_fan_set_pwm;

    data_fan1_fault.do_get = pddf_custom_fan_fault;
    data_fan2_fault.do_get = pddf_custom_fan_fault;
    data_fan3_fault.do_get = pddf_custom_fan_fault;
    data_fan4_fault.do_get = pddf_custom_fan_fault;
    data_fan5_fault.do_get = pddf_custom_fan_fault;
    data_fan6_fault.do_get = pddf_custom_fan_fault;
    data_fan7_fault.do_get = pddf_custom_fan_fault;
    data_fan8_fault.do_get = pddf_custom_fan_fault;
    data_fan9_fault.do_get = pddf_custom_fan_fault;
    data_fan10_fault.do_get = pddf_custom_fan_fault;
    data_fan11_fault.do_get = pddf_custom_fan_fault;
    data_fan12_fault.do_get = pddf_custom_fan_fault;

    return 0;
}

void __exit pddf_custom_fan_exit(void)
{
    printk(KERN_ERR "pddf_custom_fan_exit\n");
    return;
}

MODULE_AUTHOR("support <support@ragile.com>");
MODULE_DESCRIPTION("pddf custom fan api");
MODULE_LICENSE("GPL");

module_init(pddf_custom_fan_init);
module_exit(pddf_custom_fan_exit);


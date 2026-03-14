#include <linux/module.h>
#include <linux/i2c.h>
#include "../../../../../pddf/i2c/modules/include/pddf_xcvr_defs.h"
#include "../../../../../pddf/i2c/modules/include/pddf_xcvr_api.h"

extern void *get_device_table(char *name);
extern XCVR_SYSFS_ATTR_OPS xcvr_ops[];

int pddf_custom_xcvr_pres(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data);
int pddf_custom_xcvr_reset(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data);

int pddf_custom_xcvr_pres(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status;
    uint32_t modpres;

    status = 0;
    modpres = 0;
    if (strcmp(info->devtype, "cpld") == 0 && info->devname) {
        client = (struct i2c_client *)get_device_table(info->devname);
        status = i2c_smbus_read_byte_data(client, info->offset);
        if (status < 0) {
            return status;
        } else {
            modpres = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            /* printk(KERN_INFO "\nMod presence :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", modpres, status, info->devaddr, info->mask, info->offset); */
        }
    }

    data->modpres = modpres;
    return 0;
}

int pddf_custom_xcvr_reset(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status;
    uint32_t modreset;

    status = 0;
    modreset = 0;
    if (strcmp(info->devtype, "cpld") == 0 && info->devname) {
        client = (struct i2c_client *)get_device_table(info->devname);
        status = i2c_smbus_read_byte_data(client, info->offset);
        if (status < 0) {
            return status;
        } else {
            modreset = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            /* printk(KERN_INFO "\nMod Reset :0x%x, reg_value = 0x%x\n", modreset, status); */
        }
    }

    data->reset = modreset;
    return 0;
}

int __init pddf_custom_xcvr_init(void)
{
    xcvr_ops[0].do_get = pddf_custom_xcvr_pres;
    xcvr_ops[1].do_get = pddf_custom_xcvr_reset;

    printk(KERN_ERR "pddf_custom_xcvr_init\n");
    return 0;
}

void __exit pddf_custom_xcvr_exit(void)
{
    printk(KERN_ERR "pddf_custom_xcvr_exit\n");
    return;
}

MODULE_AUTHOR("support <support@ragile.com>");
MODULE_DESCRIPTION("pddf custom xcvr api");
MODULE_LICENSE("GPL");

module_init(pddf_custom_xcvr_init);
module_exit(pddf_custom_xcvr_exit);


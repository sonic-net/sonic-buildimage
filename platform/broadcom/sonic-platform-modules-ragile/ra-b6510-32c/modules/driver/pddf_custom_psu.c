#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/ioport.h>
#include "../../../../../pddf/i2c/modules/include/pddf_psu_defs.h"
#include "../../../../../pddf/i2c/modules/include/pddf_psu_driver.h"

static int pddf_custom_psu_present(void *client, PSU_DATA_ATTR *adata, void *data);
extern void *get_device_table(char *name);
extern PSU_SYSFS_ATTR_DATA access_psu_present;
extern PSU_SYSFS_ATTR_DATA access_psu_power_good;

static int pddf_custom_psu_present(void *client, PSU_DATA_ATTR *adata, void *data)
{
	int ret;
	struct i2c_client *client_ptr;
	struct psu_attr_info *padata;

	ret = -1;
	client_ptr = NULL;
	padata = (struct psu_attr_info *)data;

	if (strncmp(adata->devtype, "io", strlen("io")) == 0) {
		ret = inb(adata->offset);

		if (ret < 0) {
			return ret;
		}

		padata->val.intval = ((ret & adata->mask) == adata->cmpval);
	}

	else if (strncmp(adata->devtype, "cpld", strlen("cpld")) == 0) {
		client_ptr = (struct i2c_client *)get_device_table(adata->devname);
		if (client_ptr) {
			ret = i2c_smbus_read_byte_data(client_ptr, adata->offset);
		}

		if (ret < 0) {
			return ret;
		}

		padata->val.intval = ((ret & adata->mask) == adata->cmpval);
	}

	return 0;
}

static int pddf_custom_psu_power_good(void *client, PSU_DATA_ATTR *adata, void *data)
{
	int ret;
	struct i2c_client *client_ptr;
	struct psu_attr_info *padata;

	ret = -1;
	client_ptr = NULL;
	padata = (struct psu_attr_info *)data;

	if (strncmp(adata->devtype, "io", strlen("io")) == 0) {
		ret = inb(adata->offset);

		if (ret < 0) {
			return ret;
		}

		padata->val.intval = ((ret & adata->mask) == adata->cmpval);
	}

	else if (strncmp(adata->devtype, "cpld", strlen("cpld")) == 0) {
		client_ptr = (struct i2c_client *)get_device_table(adata->devname);
		if (client_ptr) {
			ret = i2c_smbus_read_byte_data(client_ptr, adata->offset);
		}

		if (ret < 0) {
			return ret;
		}

		padata->val.intval = ((ret & adata->mask) == adata->cmpval);
	}

	return 0;
}

static int __init pddf_custom_psu_init(void)
{
	access_psu_present.do_get = pddf_custom_psu_present;
	access_psu_power_good.do_get = pddf_custom_psu_power_good;
	printk(KERN_ERR "pddf_custom_psu_init\n");
	return 0;
}

static void __exit pddf_custom_psu_exit(void)
{
	printk(KERN_ERR "pddf_custom_psu_exit\n");
	return;
}

MODULE_AUTHOR("support <support@ragile.com>");
MODULE_DESCRIPTION("pddf custom psu api");
MODULE_LICENSE("GPL");

module_init(pddf_custom_psu_init);
module_exit(pddf_custom_psu_exit);


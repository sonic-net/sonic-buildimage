#include "linux/kobject.h"
#include "linux/sysfs.h"
#include "pddf_client_defs.h"
#include "pddf_multifpgapci_defs.h"
#include <linux/spi/spi.h>
#include <linux/string.h>

static ssize_t create_spi_device(struct device *dev,
								 struct device_attribute *da, const char *buf,
								 size_t count);
static ssize_t delete_spi_device(struct device *dev,
								 struct device_attribute *da, const char *buf,
								 size_t count);
extern void *get_device_table(char *name);
extern void delete_device_table(char *name);

/* SPI CLIENT DATA */
PDDF_DATA_ATTR(create_spi_device, S_IWUSR | S_IRUGO, show_pddf_data,
			   create_spi_device, PDDF_CHAR, 116, NULL, NULL);
PDDF_DATA_ATTR(delete_spi_device, S_IWUSR | S_IRUGO, show_pddf_data,
			   delete_spi_device, PDDF_CHAR, NAME_SIZE, NULL, NULL);

static struct kobject *spi_kobj = NULL;

static struct attribute *spi_attributes[] = {
	&attr_create_spi_device.dev_attr.attr,
	&attr_delete_spi_device.dev_attr.attr, NULL};

static const struct attribute_group pddf_spi_client_data_group = {
	.attrs = spi_attributes,
};

ssize_t create_spi_device(struct device *dev, struct device_attribute *da,
						  const char *buf, size_t count) {
	char device_name[GEN_NAME_SIZE];
	char spi_controller_name[GEN_NAME_SIZE];
	char modalias[SPI_NAME_SIZE];
	u32 max_speed_hz;
	u16 chip_select;

	struct spi_board_info *sbi = NULL;
	struct spi_controller *controller = NULL;
	struct spi_device *spi_slave = NULL;

	// Expect the following format
	// echo "$device_name $spi_controller_name $modalias $max_speed_hz
	// $chip_select" > create_spi_device
	int sscanf_result =
		sscanf(buf, "%31s %31s %31s %u %hu", device_name, spi_controller_name,
			   modalias, &max_speed_hz, &chip_select);
	if (sscanf_result != 5) {
		printk("%s: [%s] Failed to register create spi device, %s\n", SPI,
			   __FUNCTION__, buf);
		return -EINVAL;
	}

	controller = get_device_table(spi_controller_name);
	if (controller == NULL) {
		printk(
			"%s: [%s] Unable to get the spi controller spi_controller_name: %s "
			"for device_name: %s\n",
			SPI, __FUNCTION__, spi_controller_name, device_name);
		return -ENODEV;
	}

	sbi = kzalloc(sizeof(*sbi), GFP_KERNEL);
	if (sbi == NULL) {
		printk("%s: [%s](%d) kzalloc error\n", SPI, __FUNCTION__, __LINE__);
		return -ENOMEM;
	}
	strscpy(sbi->modalias, modalias, sizeof(sbi->modalias));
	sbi->max_speed_hz = max_speed_hz;
	sbi->chip_select = chip_select;

	spi_slave = spi_new_device(controller, sbi);
	if (spi_slave == NULL) {
		printk("%s: [%s](%d) spi_new_device error\n", SPI, __FUNCTION__,
			   __LINE__);
		kfree(sbi);
		return -ENOMEM;
	}

	add_device_table(device_name, spi_slave);
	return count;
}

static ssize_t delete_spi_device(struct device *dev,
								 struct device_attribute *da, const char *buf,
								 size_t count) {
	char device_name[GEN_NAME_SIZE];

	// Expect the following format
	// echo $device_name  > delete_spi_device
	if (sscanf(buf, "%31s", device_name) != 1) {
		printk("%s: %s Failed to register create spi device, %s\n", SPI,
			   __FUNCTION__, buf);
		return -EINVAL;
	}

	struct spi_device *spi_slave = get_device_table(device_name);
	if (spi_slave == NULL) {
		printk("%s: [%s] Unable to get the spi slave device by name: %s\n", SPI,
			   __FUNCTION__, device_name);
		return -EINVAL;
	}
	spi_unregister_device(spi_slave);
	delete_device_table(device_name);
	return count;
}

int __init pddf_spi_module_init(void) {
	struct kobject *device_kobj;
	int ret = 0;

	pddf_dbg(SPI, "SPI MODULE.. init\n");

	device_kobj = get_device_i2c_kobj();
	if (!device_kobj)
		return -ENOMEM;

	spi_kobj = kobject_create_and_add("spi", device_kobj);
	if (!spi_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(spi_kobj, &pddf_spi_client_data_group);
	if (ret) {
		kobject_put(spi_kobj);
		pddf_dbg(
			SPI,
			"sysfs_create_group pddf_spi_client_data_group failed. err: %d\n",
			ret);
		return ret;
	}
	pddf_dbg(SPI, "CREATED PDDF SPI SYSFS GROUP\n");

	return ret;
}

void __exit pddf_spi_module_exit(void) {
	pddf_dbg(SPI, KERN_INFO "SPI MODULE.. exit\n");
	sysfs_remove_group(spi_kobj, &pddf_spi_client_data_group);
	kobject_put(spi_kobj);
	pddf_dbg(MUX, KERN_INFO "%s: Removed the kobjects\n", __FUNCTION__);
}

module_init(pddf_spi_module_init);
module_exit(pddf_spi_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nexthop Systems");
MODULE_DESCRIPTION("PDDF module for creating and deleting SPI devices");

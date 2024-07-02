// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/acpi.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include "pca954x.h"


/* i2c / mux */
#include "moonstone_fpga_i2c_tree_cfg.h"

static char *i2c_core_sel = "fpga-xiic-i2c";


/* SFF (SFP/QSFP/OSFP) */
// #include "silverstoneDP_sff_cfg.h"
// #include "moonstone_sff_cfg.h"

#define MOD_VERSION "0.2.0"
#define CLASS_NAME "cls_fpga"
#define DRV_NAME "cls_sw_fpga"
#define FPGA_PCI_BAR_NUM        0

#define FPGA_PCIE_DEVICE_ID	0x7021
#define MMIO_BAR		0


#define VIRTUAL_I2C_OSFP_PORT   64
#define VIRTUAL_I2C_SFP_PORT    2
#define CPLD1_SLAVE_ADDR        0x30
#define CPLD2_SLAVE_ADDR        0x31
#define NUM_I2C_CLIENT          2
#define SFF_PORT_TOTAL		(VIRTUAL_I2C_OSFP_PORT + VIRTUAL_I2C_SFP_PORT)

/* MISC       */
#define FPGA_VERSION            0x0000
#define FPGA_VERSION_MJ_MSK     0xff00
#define FPGA_VERSION_MN_MSK     0x00ff
#define FPGA_SCRATCH            0x0004
#define FPGA_PORT_XCVR_READY    0x000c
#define TH5_MAX_TEMP_REG        0x0078
#define TH5_MIN_TEMP_REG        0x0080

/* SFP CTR/STAT BASE ADDR*/
#define SFP_PORT_CTRL_STAT_BASE     0x1000

// SFP Stat/Ctrl bit# via SW_FPGA
#define SFP_RXLOS_BIT_POS           0
#define SFP_ABSMOD_BIT_POS          1
#define SFP_TXFAULT_BIT_POS         2
#define SFP_TXDIS_BIT_POS           3


// OSFP Stat/ctrl bit# via SW_CPLD
#define PORT_AMNT_PER_SW_CPLD       32
#define PORT_SEL_SW_CPLD_REG_ADDR   0x10
#define PORT_CTRL_SW_CPLD_REG_ADDR  0x11
#define PORT_STAT_SW_CPLD_REG_ADDR  0x12
#define INT_L_BIT_POS               0
#define PRSNT_BIT_POS               4
#define LPMOD_BIT_POS               0
#define RST_L_BIT_POS               4

#define B_CPLD_BMC_PRESENCE_STAT_REG	0xA108

enum PORT_TYPE {
		NONE = 0,
		SFP,
		QSFP,
		OSFP
};


/* switchbrd_priv - switchboard private data */
struct switchbrd_priv {
	unsigned long base;
	int num_i2c_bus;
	struct platform_device **i2cbuses_pdev;
	struct platform_device *spiflash_pdev;
};

struct sff_device_data {
	int portid;
	enum PORT_TYPE port_type;
};

struct fpga_device {
	/* data mmio region */
	void __iomem *data_base_addr;
	resource_size_t data_mmio_start;
	resource_size_t data_mmio_len;
};

struct moonstone_fpga_data {
	struct device *osfp_devices[VIRTUAL_I2C_OSFP_PORT];
	struct device *sfp_devices[VIRTUAL_I2C_SFP_PORT];
	struct i2c_client *cpld_i2c_clients[NUM_I2C_CLIENT];
	struct mutex fpga_lock;         // For FPGA internal lock
	void __iomem *fpga_read_addr;
	uint8_t cpld1_read_addr;
	uint8_t cpld2_read_addr;
};

static struct fpga_device fpga_dev = {
	.data_base_addr = 0,
	.data_mmio_start = 0,
	.data_mmio_len = 0,
};

static struct kobject *fpga;
static struct kobject *cpld1;
static struct kobject *cpld2;
static struct class *fpgafwclass;
static struct moonstone_fpga_data *fpga_data;
static struct device *sff_dev;

/* I2C bus speed param */
static int bus_clock_master_1 = 100;
module_param(bus_clock_master_1, int, 0660);
MODULE_PARM_DESC(bus_clock_master_1,
	"I2C master 1 bus speed in KHz 50/80/100/200/400");

static int bus_clock_master_2 = 100;
module_param(bus_clock_master_2, int, 0660);
MODULE_PARM_DESC(bus_clock_master_2,
	"I2C master 2 bus speed in KHz 50/80/100/200/400");

static int bus_clock_master_3 = 100;
module_param(bus_clock_master_3, int, 0660);
MODULE_PARM_DESC(bus_clock_master_3,
	"I2C master 3 bus speed in KHz 50/80/100/200/400");

static int bus_clock_master_4 = 100;
module_param(bus_clock_master_4, int, 0660);
MODULE_PARM_DESC(bus_clock_master_4,
	"I2C master 4 bus speed in KHz 50/80/100/200/400");

static int bus_clock_master_5 = 100;
module_param(bus_clock_master_5, int, 0660);
MODULE_PARM_DESC(bus_clock_master_5,
	"I2C master 5 bus speed in KHz 50/80/100/200/400");

int cpld_i2c_write_byte(uint8_t devaddr, uint8_t regoffset, uint8_t value)
{
	struct i2c_adapter *adap;
	int err;
	struct i2c_msg msg[1];
	unsigned char data[2];

	adap = i2c_get_adapter(SWITCH_CPLD_ADAP_NUM);

	if (!adap)
		return -ENODEV;

	msg->addr = devaddr; /* I2C address of chip */
	msg->flags = 0;
	msg->len = 2;
	msg->buf = data;

	data[0] = regoffset; /* register num */
	data[1] = value; /* register data */
	err = i2c_transfer(adap, msg, 1);
	i2c_put_adapter(adap);

	if (err >= 0)
		return 0;
	return err;
}

int cpld_i2c_read_byte(uint8_t devaddr, uint8_t regoffset, uint8_t *value)
{
	struct i2c_adapter *adap;
	int err;
	struct i2c_msg msg[1];
	unsigned char data[2];

	adap = i2c_get_adapter(SWITCH_CPLD_ADAP_NUM);
	if (!adap)
		return -ENODEV;

	msg->addr = devaddr; /* I2C address of chip */
	msg->flags = 0;
	msg->len = 1;
	msg->buf = data;
	data[0] = regoffset; /* register num */
	err = i2c_transfer(adap, msg, 1);

	msg->addr = devaddr; /* I2C address */
	msg->flags = I2C_M_RD;
	msg->len = 1;
	msg->buf = data;
	err = i2c_transfer(adap, msg, 1);
	*value = data[0];

	i2c_put_adapter(adap);

	if (err >= 0)
		return 0;
		
	return err;
}	

int cls_fpgapci_read(uint32_t fpga_read_addr)
{
	uint32_t data;
	
	mutex_lock(&fpga_data->fpga_lock);
	data = ioread32((void __iomem *)(fpga_dev.data_base_addr + fpga_read_addr));
	mutex_unlock(&fpga_data->fpga_lock);
	
	return data;
}
EXPORT_SYMBOL(cls_fpgapci_read);

int cls_fpgapci_write(uint32_t fpga_write_addr, uint32_t value)
{
	mutex_lock(&fpga_data->fpga_lock);
	iowrite32(value, fpga_dev.data_base_addr + fpga_write_addr);
	mutex_unlock(&fpga_data->fpga_lock);
	return 0;	
}
EXPORT_SYMBOL(cls_fpgapci_write);

static struct mutex sysfs_osfp_mutex;

int sysfs_osfp_mutex_lock(void)
{
    mutex_lock(&sysfs_osfp_mutex);
    return 0;
}

int sysfs_osfp_mutex_unlock(void)
{
    mutex_unlock(&sysfs_osfp_mutex);
    return 0;
}

int (*ptr_osfp_mutex_lock)(void);
int (*ptr_osfp_mutex_unlock)(void);
EXPORT_SYMBOL(ptr_osfp_mutex_lock);
EXPORT_SYMBOL(ptr_osfp_mutex_unlock);

static void moonstone_dev_release(struct device *dev)
{
	return;
}

static int fpga_pci_probe(struct pci_dev *pdev)
{
	int err;
	struct device *dev = &pdev->dev;
	uint32_t fpga_version;

	/* Skip the reqions request and mmap the resource */
	/* bar0: data mmio region */
	fpga_dev.data_mmio_start = pci_resource_start(pdev, FPGA_PCI_BAR_NUM);
	fpga_dev.data_mmio_len = pci_resource_len(pdev, FPGA_PCI_BAR_NUM);
	fpga_dev.data_base_addr = ioremap_cache(fpga_dev.data_mmio_start,
												fpga_dev.data_mmio_len);
	if (!fpga_dev.data_base_addr) {
		dev_err(dev, "cannot iomap region of size %lu\n",
				(unsigned long)fpga_dev.data_mmio_len);
		err = PTR_ERR(fpga_dev.data_base_addr);
		goto err_exit;
	}
	dev_info(dev, "data_mmio iomap base = 0x%lx\n",
			 (unsigned long)fpga_dev.data_base_addr);
	dev_info(dev, "data_mmio_start = 0x%lx data_mmio_len = %lu\n",
			 (unsigned long)fpga_dev.data_mmio_start,
			 (unsigned long)fpga_dev.data_mmio_len);

	pr_info("FPGA PCIe driver probe OK.\n");
	pr_info("FPGA ioremap registers of size %lu\n",
			(unsigned long)fpga_dev.data_mmio_len);
	pr_info("FPGA Virtual BAR %d at %8.8lx - %8.8lx\n",
			FPGA_PCI_BAR_NUM,
			(unsigned long)fpga_dev.data_base_addr,
			(unsigned long)(fpga_dev.data_base_addr + fpga_dev.data_mmio_len));
	pr_info("");
	fpga_version = ioread32(fpga_dev.data_base_addr);
	pr_info("FPGA VERSION : %8.8x\n", fpga_version);

	fpgafwclass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(fpgafwclass)) {
		pr_alert("Failed to register device class\n");
		err = PTR_ERR(fpgafwclass);
		goto mem_unmap;
	}

	return 0;

mem_unmap:
	iounmap(fpga_dev.data_base_addr);
err_exit:
	return err;
}

static ssize_t getreg_show(struct device *dev,
				  struct device_attribute *devattr,
				  char *buf)
{
	// read data from the address
	uint32_t data;

	mutex_lock(&fpga_data->fpga_lock);
	data = ioread32((void __iomem *)(fpga_data->fpga_read_addr));
	mutex_unlock(&fpga_data->fpga_lock);
	return sprintf(buf, "0x%8.8x\n", data);
}

static ssize_t getreg_store(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	uint32_t addr;
	char *last;

	addr = (uint32_t)strtoul(buf, &last, 16);
	if (addr == 0 && buf == last)
		return -EINVAL;

	mutex_lock(&fpga_data->fpga_lock);	
	fpga_data->fpga_read_addr = fpga_dev.data_base_addr + addr;
	mutex_unlock(&fpga_data->fpga_lock);	
	return count;
}

static ssize_t scratch_show(struct device *dev,
				struct device_attribute *devattr,
				char *buf)
{
	return sprintf(buf, "0x%8.8x\n", ioread32(fpga_dev.data_base_addr
						  + FPGA_SCRATCH)
						  & 0xffffffff);
}

static ssize_t scratch_store(struct device *dev,
				struct device_attribute *devattr,
				const char *buf, size_t count)
{
	uint32_t data;
	char *last;


	data = (uint32_t)strtoul(buf, &last, 16);

	if (data == 0 && buf == last)
		return -EINVAL;

	mutex_lock(&fpga_data->fpga_lock);	
	iowrite32(data, fpga_dev.data_base_addr + FPGA_SCRATCH);
	mutex_unlock(&fpga_data->fpga_lock);	
	return count;
}

static ssize_t setreg_store(struct device *dev,
				  struct device_attribute *devattr,
				  const char *buf, size_t count)
{
	//register is 4 bytes
	uint32_t addr;
	uint32_t value;
	uint32_t mode = 8;
	char *tok;
	char clone[count+1];
	char *pclone = clone;
	char *last;

	memset(clone, 0x00, count+1);
    size_t buf_len = strlen(buf);
    size_t len = buf_len > count ? count : buf_len;
	strncpy(clone, buf, count); // nosemgrep

	mutex_lock(&fpga_data->fpga_lock);
	tok = strsep((char **)&pclone, " ");
	if (tok == NULL) {
		mutex_unlock(&fpga_data->fpga_lock);
		return -EINVAL;
	}
	addr = (uint32_t)strtoul(tok, &last, 16);
	if (addr == 0 && tok == last) {
		mutex_unlock(&fpga_data->fpga_lock);
		return -EINVAL;
	}
	tok = strsep((char **)&pclone, " ");
	if (tok == NULL) {
		mutex_unlock(&fpga_data->fpga_lock);
		return -EINVAL;
	}
	value = (uint32_t)strtoul(tok, &last, 16);
	if (value == 0 && tok == last) {
		mutex_unlock(&fpga_data->fpga_lock);
		return -EINVAL;
	}
	tok = strsep((char **)&pclone, " ");
	if (tok == NULL) {
		mode = 32;
	} else {
		mode = (uint32_t)strtoul(tok, &last, 10);
		if (mode == 0 && tok == last) {
			mutex_unlock(&fpga_data->fpga_lock);
			return -EINVAL;
		}
	}
	if (mode == 32) {
		iowrite32(value, fpga_dev.data_base_addr+addr);
	} else if (mode == 8) {
		iowrite8(value, fpga_dev.data_base_addr+addr);
	} else {
		mutex_unlock(&fpga_data->fpga_lock);
		return -EINVAL;
	}
	mutex_unlock(&fpga_data->fpga_lock);
	return count;
}

static ssize_t version_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	u32 data;


	mutex_lock(&fpga_data->fpga_lock);
	data = ioread32(fpga_dev.data_base_addr+FPGA_VERSION);
	mutex_unlock(&fpga_data->fpga_lock);
	return sprintf(buf, "0x%8.8x\n", data);
}

static ssize_t TH5_max_temp_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	u32 data;
	s64 temp;
	ssize_t ret_val = 0;


	mutex_lock(&fpga_data->fpga_lock);
	data = ioread32(fpga_dev.data_base_addr + TH5_MAX_TEMP_REG);

	// protect deviding by 0
	if (data == 0) {
		ret_val = sprintf(buf, "invalid raw data\n");
	} else {
		/* original calculation */
		/* temp = ((1000000000 / data / 40 / 2 - 1) * (-0.23734)) + 356.07; */
		temp = (((1000000000 / data / 40 / 2 - 1) * (-23734)) + 35607000) / 100;
		ret_val = sprintf(buf, "%lld\n", temp);
	}

	mutex_unlock(&fpga_data->fpga_lock);

	return ret_val;
}

static ssize_t TH5_max_temp_raw_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	u32 data;


	mutex_lock(&fpga_data->fpga_lock);
	data = ioread32(fpga_dev.data_base_addr + TH5_MAX_TEMP_REG);
	mutex_unlock(&fpga_data->fpga_lock);
	return sprintf(buf, "0x%x\n", data);
}

static ssize_t TH5_min_temp_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{	
	u32 data;
	s64 temp;
	ssize_t ret_val = 0;


	mutex_lock(&fpga_data->fpga_lock);
	data = ioread32(fpga_dev.data_base_addr + TH5_MIN_TEMP_REG);

	// protect deviding by 0
	if (data == 0) {
		ret_val = sprintf(buf, "invalid raw data\n");
	} else {
		/* original calculation */
		/* temp = ((1000000000 / data / 40 / 2 - 1) * (-0.23734)) + 356.07; */
		temp = (((1000000000 / data / 40 / 2 - 1) * (-23734)) + 35607000) / 100;
		ret_val = sprintf(buf, "%lld\n", temp);
	}

	mutex_unlock(&fpga_data->fpga_lock);

	return ret_val;	
}

static ssize_t TH5_min_temp_raw_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	u32 data;


	mutex_lock(&fpga_data->fpga_lock);
	data = ioread32(fpga_dev.data_base_addr + TH5_MIN_TEMP_REG);
	mutex_unlock(&fpga_data->fpga_lock);
	return sprintf(buf, "0x%x\n", data);
}
/* FPGA attributes */
static DEVICE_ATTR_RW(getreg);
static DEVICE_ATTR_RW(scratch);
static DEVICE_ATTR_WO(setreg);
static DEVICE_ATTR_RO(version);
static DEVICE_ATTR_RO(TH5_max_temp);
static DEVICE_ATTR_RO(TH5_max_temp_raw);
static DEVICE_ATTR_RO(TH5_min_temp);
static DEVICE_ATTR_RO(TH5_min_temp_raw);
// static BIN_ATTR_RO(dump, PORT_XCVR_REGISTER_SIZE);

// static struct bin_attribute *fpga_bin_attrs[] = {
//		&bin_attr_dump,
//		NULL,
// };

static struct attribute *fpga_attrs[] = {
		&dev_attr_getreg.attr,
		&dev_attr_scratch.attr,
		&dev_attr_setreg.attr,
		&dev_attr_version.attr,
		&dev_attr_TH5_max_temp.attr,
		&dev_attr_TH5_max_temp_raw.attr,
		&dev_attr_TH5_min_temp.attr,
		&dev_attr_TH5_min_temp_raw.attr,		
		NULL,
};

static struct attribute_group fpga_attr_grp = {
		.attrs = fpga_attrs,
		// .bin_attrs = fpga_bin_attrs,
};

/* SW CPLDs attributes */
static ssize_t cpld1_getreg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// CPLD register is one byte
	uint8_t data;

	cpld_i2c_read_byte(CPLD1_SLAVE_ADDR, fpga_data->cpld1_read_addr, &data);

	return sprintf(buf, "0x%2.2x\n", (unsigned int)data);
}

static ssize_t cpld1_getreg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	uint8_t addr;
	char *last;


	addr = (uint8_t)(strtoul(buf, &last, 16));

	if (addr == 0 && buf == last)
		return -EINVAL;

	fpga_data->cpld1_read_addr = addr;
	return size;
}

struct device_attribute dev_attr_cpld1_getreg = __ATTR(getreg, 0600, cpld1_getreg_show, cpld1_getreg_store);

static ssize_t cpld1_scratch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// CPLD register is one byte
	uint8_t data;

	cpld_i2c_read_byte(CPLD1_SLAVE_ADDR, 0x1, &data);
	
	return sprintf(buf, "0x%.2x\n", data);
}

static ssize_t cpld1_scratch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	// CPLD register is one byte

	u8 value;
	ssize_t status;

	status = kstrtou8(buf, 0, &value);
	if (status != 0)
		return status;
	status = cpld_i2c_write_byte(CPLD1_SLAVE_ADDR, 0x1, value);
	if (status == 0)
		status = size;
	return status;
}
struct device_attribute dev_attr_cpld1_scratch = __ATTR(scratch, 0600, cpld1_scratch_show, cpld1_scratch_store);

static ssize_t cpld1_setreg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{

	uint8_t addr, value;
	char *tok;
	char clone[size+1];
	char *pclone = clone;
	char *last;
	int err;

    size_t buf_len = strlen(buf);
    size_t len = buf_len > size ? size : buf_len;
	strncpy(clone, buf, len); // nosemgrep

	tok = strsep((char **)&pclone, " ");
	if (tok == NULL)
		return -EINVAL;

	addr = (uint8_t)(strtoul(tok, &last, 16));
	if (addr == 0 && tok == last)
		return -EINVAL;

	tok = strsep((char **)&pclone, " ");
	if (tok == NULL)
		return -EINVAL;

	value = (uint8_t)(strtoul(tok, &last, 16));
	if (value == 0 && tok == last)
		return -EINVAL;

	err = cpld_i2c_write_byte(CPLD1_SLAVE_ADDR, addr, value);
	if (err < 0)
		return err;

	return size;
}
struct device_attribute dev_attr_cpld1_setreg = __ATTR(setreg, 0200, NULL, cpld1_setreg_store);

static ssize_t cpld1_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// CPLD register is one byte
	u8 value;

	cpld_i2c_read_byte(CPLD1_SLAVE_ADDR, 0, &value);

	return sprintf(buf, "0x%.2x\n", value);
}
struct device_attribute dev_attr_cpld1_version = __ATTR(version, 0444, cpld1_version_show, NULL);

static struct attribute *cpld1_attrs[] = {
	&dev_attr_cpld1_version.attr,
	&dev_attr_cpld1_getreg.attr,
	&dev_attr_cpld1_scratch.attr,
	&dev_attr_cpld1_setreg.attr,
	NULL,
};

static struct attribute_group cpld1_attr_grp = {
	.attrs = cpld1_attrs,
};

static ssize_t cpld2_getreg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// CPLD register is one byte
	uint8_t data;

	cpld_i2c_read_byte(CPLD2_SLAVE_ADDR, fpga_data->cpld2_read_addr, &data);
		
	return sprintf(buf, "0x%2.2x\n", (unsigned int)data);
}

static ssize_t cpld2_getreg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	// CPLD register is one byte
	uint8_t addr;
	char *last;


	addr = (uint8_t)(strtoul(buf, &last, 16));

	if (addr == 0 && buf == last)
		return -EINVAL;

	fpga_data->cpld2_read_addr = addr;
	return size;
}
struct device_attribute dev_attr_cpld2_getreg = __ATTR(getreg, 0600, cpld2_getreg_show, cpld2_getreg_store);

static ssize_t cpld2_scratch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// CPLD register is one byte
	uint8_t data;

	cpld_i2c_read_byte(CPLD2_SLAVE_ADDR, 0x1, &data);
	
	return sprintf(buf, "0x%.2x\n", data);
}

static ssize_t cpld2_scratch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	// CPLD register is one byte
	u8 value;
	ssize_t status;

	status = kstrtou8(buf, 0, &value);
	if (status != 0)
		return status;
	status = cpld_i2c_write_byte(CPLD2_SLAVE_ADDR, 0x1, value);
	if (status == 0)
		status = size;
	return status;
}
struct device_attribute dev_attr_cpld2_scratch = __ATTR(scratch, 0600, cpld2_scratch_show, cpld2_scratch_store);

static ssize_t cpld2_setreg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	uint8_t addr, value;
	char *tok;
	char clone[size+1];
	char *pclone = clone;
	char *last;
	int err;

    size_t buf_len = strlen(buf);
    size_t len = buf_len > size ? size : buf_len;
	strncpy(clone, buf, len); // nosemgrep

	tok = strsep((char **)&pclone, " ");
	if (tok == NULL)
		return -EINVAL;

	addr = (uint8_t)(strtoul(tok, &last, 16));
	if (addr == 0 && tok == last)
		return -EINVAL;

	tok = strsep((char **)&pclone, " ");
	if (tok == NULL)
		return -EINVAL;

	value = (uint8_t)(strtoul(tok, &last, 16));
	if (value == 0 && tok == last)
		return -EINVAL;

	err = cpld_i2c_write_byte(CPLD2_SLAVE_ADDR, addr, value);
	if (err < 0)
		return err;

	return size;
}
struct device_attribute dev_attr_cpld2_setreg = __ATTR(setreg, 0200, NULL, cpld2_setreg_store);

static ssize_t cpld2_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// CPLD register is one byte
	u8 value;

	cpld_i2c_read_byte(CPLD2_SLAVE_ADDR, 0, &value);

	return sprintf(buf, "0x%.2x\n", value);
}
struct device_attribute dev_attr_cpld2_version = __ATTR(version, 0444, cpld2_version_show, NULL);

static struct attribute *cpld2_attrs[] = {
	&dev_attr_cpld2_version.attr,
	&dev_attr_cpld2_getreg.attr,
	&dev_attr_cpld2_scratch.attr,
	&dev_attr_cpld2_setreg.attr,
	NULL,
};

static struct attribute_group cpld2_attr_grp = {
	.attrs = cpld2_attrs,
};

/* OSFP attributes */
static ssize_t osfp_int_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 data;
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	int cpld_addr = CPLD1_SLAVE_ADDR;
	uint8_t value;
	int err;

	mutex_lock(&fpga_data->fpga_lock);
	if (ptr_osfp_mutex_lock){
		ptr_osfp_mutex_lock();
	}

	value = portid;
	if (portid > PORT_AMNT_PER_SW_CPLD) {
		cpld_addr = CPLD2_SLAVE_ADDR;
		value = portid - PORT_AMNT_PER_SW_CPLD;
	}
	
	err = cpld_i2c_write_byte(cpld_addr, PORT_SEL_SW_CPLD_REG_ADDR, value - 1);
	if (err < 0){
		if (ptr_osfp_mutex_unlock){
			ptr_osfp_mutex_unlock();
		}		
		mutex_unlock(&fpga_data->fpga_lock);
		return err;
	}

	err = cpld_i2c_read_byte(cpld_addr, PORT_STAT_SW_CPLD_REG_ADDR, &data);
	if (err < 0){
		if (ptr_osfp_mutex_unlock){
			ptr_osfp_mutex_unlock();
		}		
		mutex_unlock(&fpga_data->fpga_lock);
		return err;
	}

	if (ptr_osfp_mutex_unlock){
		ptr_osfp_mutex_unlock();
	}
	mutex_unlock(&fpga_data->fpga_lock);

	return sprintf(buf, "%d\n", (data >> INT_L_BIT_POS) & 1U);
}
struct device_attribute dev_attr_osfp_int = __ATTR(INT_L, 0444, osfp_int_show, NULL);

static ssize_t osfp_modprs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 data;
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	int cpld_addr = CPLD1_SLAVE_ADDR;
	uint8_t value;
	int err;

	mutex_lock(&fpga_data->fpga_lock);
	if (ptr_osfp_mutex_lock){
		ptr_osfp_mutex_lock();
	}
	
	value = portid;
	if (portid > PORT_AMNT_PER_SW_CPLD) {
		cpld_addr = CPLD2_SLAVE_ADDR;
		value = portid - PORT_AMNT_PER_SW_CPLD;
	}
	
	err = cpld_i2c_write_byte(cpld_addr, PORT_SEL_SW_CPLD_REG_ADDR, value - 1);
	if (err < 0){
		if (ptr_osfp_mutex_unlock){
			ptr_osfp_mutex_unlock();
		}
		mutex_unlock(&fpga_data->fpga_lock);
		return err;
	}

	err = cpld_i2c_read_byte(cpld_addr, PORT_STAT_SW_CPLD_REG_ADDR, &data);
	if (err < 0){
		if (ptr_osfp_mutex_unlock){
			ptr_osfp_mutex_unlock();
		}
		mutex_unlock(&fpga_data->fpga_lock);
		return err;
	}

	if (ptr_osfp_mutex_unlock){
		ptr_osfp_mutex_unlock();
	}
	mutex_unlock(&fpga_data->fpga_lock);

	return sprintf(buf, "%d\n", (data >> PRSNT_BIT_POS) & 1U);	
}
struct device_attribute dev_attr_osfp_modprs = __ATTR(MODPRS_L, 0444, osfp_modprs_show, NULL);

static ssize_t osfp_reset_show(struct device *dev, struct device_attribute *attr, char *buf)
{		
	u8 data;
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	int cpld_addr = CPLD1_SLAVE_ADDR;
	uint8_t value;
	int err;

	mutex_lock(&fpga_data->fpga_lock);
	if (ptr_osfp_mutex_lock){
		ptr_osfp_mutex_lock();
	}
	
	value = portid;
	if (portid > PORT_AMNT_PER_SW_CPLD) {
		cpld_addr = CPLD2_SLAVE_ADDR;
		value = portid - PORT_AMNT_PER_SW_CPLD;
	}
	
	err = cpld_i2c_write_byte(cpld_addr, PORT_SEL_SW_CPLD_REG_ADDR, value - 1);
	if (err < 0){
		if (ptr_osfp_mutex_unlock){
			ptr_osfp_mutex_unlock();
		}		
		mutex_unlock(&fpga_data->fpga_lock);
		return err;
	}

	err = cpld_i2c_read_byte(cpld_addr, PORT_CTRL_SW_CPLD_REG_ADDR, &data);
	if (err < 0){
		if (ptr_osfp_mutex_unlock){
			ptr_osfp_mutex_unlock();
		}		
		mutex_unlock(&fpga_data->fpga_lock);
		return err;
	}

	if (ptr_osfp_mutex_unlock){
		ptr_osfp_mutex_unlock();
	}
	mutex_unlock(&fpga_data->fpga_lock);

	return sprintf(buf, "%d\n", (data >> RST_L_BIT_POS) & 1U);
}

static ssize_t osfp_reset_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	long value;
	u8 data;
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	int cpld_addr = CPLD1_SLAVE_ADDR;
	int err;

	status = kstrtol(buf, 0, &value);

	if (status != 0)
		return status;

	if ((value != 0) && (value != 1))
		return -ERANGE;

	mutex_lock(&fpga_data->fpga_lock);
	if (ptr_osfp_mutex_lock){
		ptr_osfp_mutex_lock();
	}

	value = portid;
	if (portid > PORT_AMNT_PER_SW_CPLD) {
		cpld_addr = CPLD2_SLAVE_ADDR;
		value = portid - PORT_AMNT_PER_SW_CPLD;
	}

	err = cpld_i2c_write_byte(cpld_addr, PORT_SEL_SW_CPLD_REG_ADDR, value - 1);
	if (err < 0) {
		if (ptr_osfp_mutex_unlock){
			ptr_osfp_mutex_unlock();
		}		
		mutex_unlock(&fpga_data->fpga_lock);
		return err;
	}
	
	err = cpld_i2c_read_byte(cpld_addr, PORT_CTRL_SW_CPLD_REG_ADDR, &data);
	if (err < 0) {
		if (ptr_osfp_mutex_unlock){
			ptr_osfp_mutex_unlock();
		}		
		mutex_unlock(&fpga_data->fpga_lock);
		return data;
	}

	if (value)
		data |= (0x01 << RST_L_BIT_POS);
	else
		data &= ~(0x01 << RST_L_BIT_POS);

	err = cpld_i2c_write_byte(cpld_addr, PORT_CTRL_SW_CPLD_REG_ADDR, data);
	if (err < 0) {
		if (ptr_osfp_mutex_unlock){
			ptr_osfp_mutex_unlock();
		}		
		mutex_unlock(&fpga_data->fpga_lock);
		return err;
	}

	status = size;

	if (ptr_osfp_mutex_unlock){
		ptr_osfp_mutex_unlock();
	}
	mutex_unlock(&fpga_data->fpga_lock);

	return status;
}
struct device_attribute dev_attr_osfp_reset = __ATTR(RST_L, 0600, osfp_reset_show, osfp_reset_store);

static ssize_t osfp_lpmod_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 data;
	long value;
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	int cpld_addr = CPLD1_SLAVE_ADDR;
	int err;


	mutex_lock(&fpga_data->fpga_lock);
	if (ptr_osfp_mutex_lock){
		ptr_osfp_mutex_lock();
	}

	value = portid;
	if (portid > PORT_AMNT_PER_SW_CPLD) {
		cpld_addr = CPLD2_SLAVE_ADDR;
		value = portid - PORT_AMNT_PER_SW_CPLD;
	}

	err = cpld_i2c_write_byte(cpld_addr, PORT_SEL_SW_CPLD_REG_ADDR, value - 1);
	if (err < 0) {
		if (ptr_osfp_mutex_unlock){
			ptr_osfp_mutex_unlock();
		}
		mutex_unlock(&fpga_data->fpga_lock);
		return err;
	}

	err = cpld_i2c_read_byte(cpld_addr, PORT_CTRL_SW_CPLD_REG_ADDR, &data);
	if (err < 0) {
		if (ptr_osfp_mutex_unlock){
			ptr_osfp_mutex_unlock();
		}
		mutex_unlock(&fpga_data->fpga_lock);
		return data;
	}	

	if (ptr_osfp_mutex_unlock){
		ptr_osfp_mutex_unlock();
	}
	mutex_unlock(&fpga_data->fpga_lock);

	return sprintf(buf, "%d\n", (data >> LPMOD_BIT_POS) & 1U);
}
static ssize_t osfp_lpmod_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	long value;
	u8 data;
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	int cpld_addr = CPLD1_SLAVE_ADDR;
	int err;

	status = kstrtol(buf, 0, &value);

	if (status != 0)
		return status;

	if ((value != 0) && (value != 1))
		return -ERANGE;

	mutex_lock(&fpga_data->fpga_lock);
	if (ptr_osfp_mutex_lock){
		ptr_osfp_mutex_lock();
	}

	value = portid;
	if (portid > PORT_AMNT_PER_SW_CPLD) {
		cpld_addr = CPLD2_SLAVE_ADDR;
		value = portid - PORT_AMNT_PER_SW_CPLD;
	}

	err = cpld_i2c_write_byte(cpld_addr, PORT_SEL_SW_CPLD_REG_ADDR, value - 1);
	if (err < 0) {
		if (ptr_osfp_mutex_unlock){
			ptr_osfp_mutex_unlock();
		}
		mutex_unlock(&fpga_data->fpga_lock);
		return err;
	}

	err = cpld_i2c_read_byte(cpld_addr, PORT_CTRL_SW_CPLD_REG_ADDR, &data);
	if (err < 0) {
		if (ptr_osfp_mutex_unlock){
			ptr_osfp_mutex_unlock();
		}
		mutex_unlock(&fpga_data->fpga_lock);
		return data;
	}

	if (value)
		data |= (0x01 << LPMOD_BIT_POS);
	else
		data &= ~(0x01 << LPMOD_BIT_POS);

	err = cpld_i2c_write_byte(cpld_addr, PORT_CTRL_SW_CPLD_REG_ADDR, data);
	if (err < 0) {
		if (ptr_osfp_mutex_unlock){
			ptr_osfp_mutex_unlock();
		}
		mutex_unlock(&fpga_data->fpga_lock);
		return err;
	}	
	
	status = size;

	if (ptr_osfp_mutex_unlock){
		ptr_osfp_mutex_unlock();
	}
	mutex_unlock(&fpga_data->fpga_lock);

	return status;
}
struct device_attribute dev_attr_osfp_lpmod = __ATTR(LPMOD, 0600, osfp_lpmod_show, osfp_lpmod_store);

static struct attribute *osfp_attrs[] = {
	&dev_attr_osfp_int.attr,
	&dev_attr_osfp_modprs.attr,
	&dev_attr_osfp_lpmod.attr,
	&dev_attr_osfp_reset.attr,
	NULL,
};

static struct attribute_group osfp_attr_grp = {
	.attrs = osfp_attrs,
};

static const struct attribute_group *osfp_attr_grps[] = {
	&osfp_attr_grp,
	NULL
};

static ssize_t sfp_txfault_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u32 data;
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	unsigned int REGISTER = SFP_PORT_CTRL_STAT_BASE + (portid * 0x04);


	mutex_lock(&fpga_data->fpga_lock);
	data = ioread32(fpga_dev.data_base_addr + REGISTER);
	mutex_unlock(&fpga_data->fpga_lock);
	return sprintf(buf, "%d\n", (data >> SFP_TXFAULT_BIT_POS) & 1U);
}
DEVICE_ATTR_RO(sfp_txfault);

static ssize_t sfp_rxlos_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u32 data;
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	unsigned int REGISTER = SFP_PORT_CTRL_STAT_BASE + (portid * 0x04);


	mutex_lock(&fpga_data->fpga_lock);
	data = ioread32(fpga_dev.data_base_addr + REGISTER);
	mutex_unlock(&fpga_data->fpga_lock);
	return sprintf(buf, "%d\n", (data >> SFP_RXLOS_BIT_POS) & 1U);
}
DEVICE_ATTR_RO(sfp_rxlos);

static ssize_t sfp_absmod_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u32 data;
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	unsigned int REGISTER = SFP_PORT_CTRL_STAT_BASE + (portid * 0x04);

	
	mutex_lock(&fpga_data->fpga_lock);
	data = ioread32(fpga_dev.data_base_addr + REGISTER);
	mutex_unlock(&fpga_data->fpga_lock);
	return sprintf(buf, "%d\n", (data >> SFP_ABSMOD_BIT_POS) & 1U);
}
DEVICE_ATTR_RO(sfp_absmod);

static ssize_t sfp_txdisable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u32 data;
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	unsigned int REGISTER = SFP_PORT_CTRL_STAT_BASE + (portid * 0x04);

	
	mutex_lock(&fpga_data->fpga_lock);
	data = ioread32(fpga_dev.data_base_addr + REGISTER);
	mutex_unlock(&fpga_data->fpga_lock);
	return sprintf(buf, "%d\n", (data >> SFP_TXDIS_BIT_POS) & 1U);
}
static ssize_t sfp_txdisable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	long value;
	u32 data;
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	unsigned int REGISTER = SFP_PORT_CTRL_STAT_BASE + (portid * 0x04);
	

	status = kstrtol(buf, 0, &value);

	if (status != 0)
		return status;

	if ((value != 0) && (value != 1))
		return -ERANGE;
	
	mutex_lock(&fpga_data->fpga_lock);

	data = ioread32(fpga_dev.data_base_addr + REGISTER);
	if (value)
		data |= ((u32)0x1 << SFP_TXDIS_BIT_POS);
	else
		data &= ~((u32)0x1 << SFP_TXDIS_BIT_POS);
	
	iowrite32(data, fpga_dev.data_base_addr + REGISTER);
	
	status = size;

	mutex_unlock(&fpga_data->fpga_lock);

	return status;
}
DEVICE_ATTR_RW(sfp_txdisable);

static struct attribute *sfp_attrs[] = {
	&dev_attr_sfp_txfault.attr,
	&dev_attr_sfp_rxlos.attr,
	&dev_attr_sfp_absmod.attr,
	&dev_attr_sfp_txdisable.attr,
	NULL,
};

static struct attribute_group sfp_attr_grp = {
	.attrs = sfp_attrs,
};

static const struct attribute_group *sfp_attr_grps[] = {
	&sfp_attr_grp,
	NULL
};

static struct device *sff_init(enum PORT_TYPE type, int portid)
{
	struct sff_device_data *new_data;
	struct device *new_device = NULL;

	new_data = kzalloc(sizeof(*new_data), GFP_KERNEL);
	if (!new_data) {
		pr_alert("Cannot alloc sff device data @port%d\n",
									portid);
		return NULL;
	}
	/* The OSFP port ID start from 1 */
	new_data->portid = portid;
	mutex_init(&sysfs_osfp_mutex);
	ptr_osfp_mutex_lock = sysfs_osfp_mutex_lock;
	ptr_osfp_mutex_unlock = sysfs_osfp_mutex_unlock;

	if (type == OSFP) {

		new_device = device_create_with_groups(fpgafwclass,
							sff_dev,
							MKDEV(0, 0),
							new_data,
							osfp_attr_grps,
							"OSFP%d",
							portid);

	} else if (type == SFP) {
		new_device = device_create_with_groups(fpgafwclass,
							sff_dev,
							MKDEV(0, 0),
							new_data,
							sfp_attr_grps,
							"SFP%d",
							portid);
	}

	if (IS_ERR(new_device)) {
		pr_alert("Cannot create sff device @port%d\n", portid);
		kfree(new_data);
		return NULL;
	}
	return new_device;
}

static void fpga_pci_remove(void)
{
	iounmap(fpga_dev.data_base_addr);
	class_unregister(fpgafwclass);
	class_destroy(fpgafwclass);
};

static int moonstone_drv_remove(struct platform_device *pdev)
{
	int portid_count;
	struct sff_device_data *rem_data;
	struct i2c_client *rem_cli;


	rem_cli = fpga_data->cpld_i2c_clients[0];
	if (rem_cli)
		i2c_unregister_device(rem_cli);

	rem_cli = fpga_data->cpld_i2c_clients[1];
	if (rem_cli)
		i2c_unregister_device(rem_cli);

	for (portid_count = 0; portid_count < VIRTUAL_I2C_OSFP_PORT; portid_count++) {
		if (fpga_data->osfp_devices[portid_count] != NULL) {
			rem_data = dev_get_drvdata(fpga_data->osfp_devices[portid_count]);
			device_unregister(fpga_data->osfp_devices[portid_count]);
			put_device(fpga_data->osfp_devices[portid_count]);
			kfree(rem_data);
		}
	}

	for (portid_count = 0; portid_count < VIRTUAL_I2C_SFP_PORT; portid_count++) {
		if (fpga_data->sfp_devices[portid_count] != NULL) {
			rem_data = dev_get_drvdata(fpga_data->sfp_devices[portid_count]);
			device_unregister(fpga_data->sfp_devices[portid_count]);
			put_device(fpga_data->sfp_devices[portid_count]);
			kfree(rem_data);
		}
	}

	sysfs_remove_group(fpga, &fpga_attr_grp);
	sysfs_remove_group(cpld1, &cpld1_attr_grp);
	sysfs_remove_group(cpld2, &cpld2_attr_grp);
	// sysfs_remove_group(&sff_dev->kobj, &sff_led_test_grp);
	kobject_put(fpga);
	kobject_put(cpld1);
	kobject_put(cpld2);
	device_destroy(fpgafwclass, MKDEV(0, 0));
	fpga_pci_remove();
	return 0;
}

static int moonstone_drv_probe(struct platform_device *pdev)
{
	int ret = 0;
	int portid_count;
	struct i2c_client *cpld1_client;
	struct i2c_client *cpld2_client;

	struct pci_dev *pci_dev = pci_get_device(PCI_VENDOR_ID_XILINX,
											 FPGA_PCIE_DEVICE_ID,
											 NULL);
	if (pci_dev) {
		fpga_pci_probe(pci_dev);
		pci_dev_put(pci_dev);
	} else {
		ret = -ENODEV;
		goto err_exit;
	}

	fpga_data = devm_kzalloc(&pdev->dev, sizeof(struct moonstone_fpga_data),
							 GFP_KERNEL);

	if (!fpga_data) {
		ret = -ENOMEM;
		goto err_exit;
	}

	/* The device class need to be instantiated before this function called */
	BUG_ON(fpgafwclass == NULL);

	fpga = kobject_create_and_add("FPGA", &pdev->dev.kobj);
	if (!fpga) {
		ret = -ENOMEM;
		goto err_exit;
	}

	ret = sysfs_create_group(fpga, &fpga_attr_grp);
	if (ret != 0) {
		pr_err("Cannot create FPGA sysfs attributes\n");
		goto err_remove_fpga;
	}

	cpld1 = kobject_create_and_add("CPLD1", &pdev->dev.kobj);
	if (!cpld1) {
		ret = -ENOMEM;
		goto err_free_cli_clpd2;
	}
	ret = sysfs_create_group(cpld1, &cpld1_attr_grp);
	if (ret != 0) {
		pr_err("Cannot create CPLD1 sysfs attributes\n");
		goto err_remove_cpld1;
	}

	cpld2 = kobject_create_and_add("CPLD2", &pdev->dev.kobj);
	if (!cpld2) {
		ret = -ENOMEM;
		goto err_remove_grp_cpld1;
	}
	ret = sysfs_create_group(cpld2, &cpld2_attr_grp);
	if (ret != 0) {
		pr_err("Cannot create CPLD2 sysfs attributes\n");
		goto err_remove_cpld2;
	}
	/* */

	sff_dev = device_create(fpgafwclass, NULL, MKDEV(0, 0), NULL, "sff_device");
	if (IS_ERR(sff_dev)) {
		ret = PTR_ERR(sff_dev);
		goto err_remove_grp_cpld2;
	}

	/*
	 * ret = sysfs_create_group(&sff_dev->kobj, &sff_led_test_grp);
	 * if (ret != 0) {
	 *	goto err_remove_sff;
	 * }
	 */

	ret = sysfs_create_link(&pdev->dev.kobj, &sff_dev->kobj, "SFF");
	if (ret != 0)
		goto err_remove_grp_sff;


	/* Init SFF devices */
	for (portid_count = 1; portid_count <= VIRTUAL_I2C_OSFP_PORT; portid_count++)
		fpga_data->osfp_devices[portid_count - 1] = sff_init(OSFP, portid_count);


	for (portid_count = 1; portid_count <= VIRTUAL_I2C_SFP_PORT; portid_count++)
		fpga_data->sfp_devices[portid_count - 1] = sff_init(SFP, portid_count);


	// Set default read address to VERSION
	fpga_data->fpga_read_addr = fpga_dev.data_base_addr + FPGA_VERSION;
	fpga_data->cpld1_read_addr = 0x00;
	fpga_data->cpld2_read_addr = 0x00;
	fpga_data->cpld_i2c_clients[0] = cpld1_client;
	fpga_data->cpld_i2c_clients[1] = cpld2_client;

	mutex_init(&fpga_data->fpga_lock);

return 0;

err_remove_grp_sff:
	// sysfs_remove_group(&sff_dev->kobj, &sff_led_test_grp);
// err_remove_sff:
	device_destroy(fpgafwclass, MKDEV(0, 0));
err_remove_grp_cpld2:
	sysfs_remove_group(cpld2, &cpld2_attr_grp);
err_remove_cpld2:
	kobject_put(cpld2);
err_remove_grp_cpld1:
	sysfs_remove_group(cpld1, &cpld1_attr_grp);
err_remove_cpld1:
	kobject_put(cpld1);
err_free_cli_clpd2:
	i2c_unregister_device(cpld2_client);
err_free_cli_clpd1:
	i2c_unregister_device(cpld1_client);
err_remove_grp_fpga:
	sysfs_remove_group(fpga, &fpga_attr_grp);
err_remove_fpga:
	kobject_put(fpga);
err_exit:
	return ret;
}

static struct platform_driver moonstone_drv = {
	.probe  = moonstone_drv_probe,
	.remove = __exit_p(moonstone_drv_remove),
	.driver = {
		.name   = DRV_NAME,
	},
};

static struct platform_device moonstone_dev = {
	.name           = DRV_NAME,
	.id             = -1,
	.num_resources  = 0,
	.dev = {
		.release = moonstone_dev_release,
	}
};

static int cls_fpga_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	struct switchbrd_priv *priv;
	struct platform_device **i2cbuses_pdev;
	unsigned long rstart;
	int num_i2c_bus, i;
	int err;
	bool b_cpld_bmc_presence;
	uint8_t b_cpld_bmc_presence_raw;

	err = pci_enable_device(dev);
	if (err) {
		dev_err(&dev->dev,  "Failed to enable PCI device\n");
		goto err_exit;
	}

	/* Check for valid MMIO address */
	rstart = pci_resource_start(dev, MMIO_BAR);
	if (!rstart) {
		dev_err(&dev->dev,
			"Switchboard base address uninitialized, check FPGA\n");
		err = -ENODEV;
		goto err_disable_device;
	}

	dev_dbg(&dev->dev, "BAR%d res: 0x%lx-0x%llx\n", MMIO_BAR,
		rstart, pci_resource_end(dev, MMIO_BAR));

	priv = devm_kzalloc(&dev->dev,
				sizeof(struct switchbrd_priv), GFP_KERNEL);
	if (!priv) {
		err = -ENOMEM;
		goto err_disable_device;
	}

	pci_set_drvdata(dev, priv);

	/* i2c_ocore device creation */
	b_cpld_bmc_presence_raw = inb(B_CPLD_BMC_PRESENCE_STAT_REG);
	b_cpld_bmc_presence = (b_cpld_bmc_presence_raw & 0x01)? false:true;
	//dev_info(&dev->dev, "BMC Presence Status from B_CPLD @ 0x%X: [0x%02x] [%s] [%s]\n",
	dev_info(&dev->dev, "BMC Presence Status from B_CPLD @ 0x%X: [0x%02x] [%s]\n",
			B_CPLD_BMC_PRESENCE_STAT_REG,
			b_cpld_bmc_presence_raw,
			/*b_cpld_bmc_presence? "true":"false",*/
			b_cpld_bmc_presence? "Present":"Absent");

	num_i2c_bus = b_cpld_bmc_presence? 6:13;
	i2cbuses_pdev = devm_kzalloc(
			&dev->dev,
			num_i2c_bus * sizeof(struct platform_device *),
			GFP_KERNEL);

	dev_info(&dev->dev, "total SW_FPGA_I2C: [%d]\n", num_i2c_bus);

	for (i = 0; i < num_i2c_bus; i++) {

		i2c_bus_configs[i].res[0].start += rstart;
		i2c_bus_configs[i].res[0].end += rstart;

		dev_info(&dev->dev, "i2c-bus.%d: 0x%llx - 0x%llx\n",
			i2c_bus_configs[i].id,
			i2c_bus_configs[i].res[0].start,
			i2c_bus_configs[i].res[0].end);

		i2cbuses_pdev[i] = platform_device_register_resndata(
					&dev->dev, i2c_core_sel,
					i2c_bus_configs[i].id,
					i2c_bus_configs[i].res,
					i2c_bus_configs[i].num_res,
					NULL,
					0);

		if (IS_ERR(i2cbuses_pdev[i])) {
			dev_err(&dev->dev, "Failed to register ocores-i2c.%d\n",
				i2c_bus_configs[i].id);
			err = PTR_ERR(i2cbuses_pdev[i]);
			goto err_unregister_ocore;
		}
	}

	platform_device_register(&moonstone_dev);
	platform_driver_register(&moonstone_drv);

	priv->base = rstart;
	priv->num_i2c_bus = num_i2c_bus;
	priv->i2cbuses_pdev = i2cbuses_pdev;
	return 0;

err_unregister_ocore:
	for (i = 0; i < num_i2c_bus; i++) {
		if (priv->i2cbuses_pdev[i])
			platform_device_unregister(priv->i2cbuses_pdev[i]);
	}

err_disable_device:
	pci_disable_device(dev);
err_exit:
	return err;
}

static const struct pci_device_id pci_clsswbrd[] = {
	{  PCI_VDEVICE(XILINX, FPGA_PCIE_DEVICE_ID) },
	{0, }
};

static void cls_fpga_remove(struct pci_dev *dev);

static struct pci_driver clsswbrd_pci_driver = {
	.name = DRV_NAME,
	.id_table = pci_clsswbrd,
	.probe = cls_fpga_probe,
	.remove = cls_fpga_remove,
};

static void cls_fpga_remove(struct pci_dev *dev)
{
	int i;
	struct switchbrd_priv *priv = pci_get_drvdata(dev);


	platform_driver_unregister(&moonstone_drv);
	platform_device_unregister(&moonstone_dev);

	for (i = 0; i < priv->num_i2c_bus; i++) {
		if (priv->i2cbuses_pdev[i])
			platform_device_unregister(priv->i2cbuses_pdev[i]);
	}

	// pci_release_regions(dev);
	pci_disable_device(dev);

	dev_info(&dev->dev, "cls_sw_fpga module removed\n");

	return;
};

MODULE_DEVICE_TABLE(pci, pci_clsswbrd);

module_pci_driver(clsswbrd_pci_driver);

MODULE_AUTHOR("Raywat Polpakar <rpolpa@celestica.com>");
MODULE_DESCRIPTION("Celestica Moonstone sw_fpga driver");
MODULE_VERSION(MOD_VERSION);
MODULE_LICENSE("GPL");

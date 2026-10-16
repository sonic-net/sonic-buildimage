// SPDX-License-Identifier: GPL-2.0+
/*
 * Based on Linux kernel version 6.1.94
 * Original file: drivers/hwmon/pmbus/isl68137.c
 *
 * Hardware monitoring driver for Renesas Digital Multiphase Voltage Regulators
 *
 * Copyright (c) 2017 Google Inc
 * Copyright (c) 2020 Renesas Electronics America
 *
 * Modifcations by Nexthop Systems Inc., 2025:
 *   - Prepend nh_ to usages of pmbus.h
 *   - Expose sysfs attribute "mfr_revision" for programmable version.
 *   - Blackbox/fault-log support for RAA Gen3 parts (raa_dmpvr3_2rail_nontc):
 *     228234, 228236, 228244. Records live in NVM and RAM and are walked via
 * 	   the chip's DMA registers (set DMA_ADDR, read DMA_DATA / DMA_SEQ_DATA).
 *   - Expose sysfs attribute "blackbox_ram" and nvmem sysfs attribute at
 *     /sys/bus/nvmem/devices/<bus>-<device><cell>/nvmem to read the NVM records
 *
 * Copyright (C) 2025 Nexthop Systems Inc.
 */

#include <linux/err.h>
#include <linux/hwmon-sysfs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/nvmem-provider.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/version.h>

#include "nh_pmbus.h"

#define ISL68137_VOUT_AVS	0x30

/* RAA DMA definitions */
#define RAA_DMP_DMA_DATA_REG_BYTES	4
#define RAA_DMP_DMA_DATA_FIX_CMD	0xC5
#define RAA_DMP_DMA_DATA_SEQ_CMD	0xC6
#define RAA_DMP_DMA_ADDR_CMD		0xC7


/* RAA digital multiphase Gen2 definitions */
#define RAA_DMPVR2_READ_VMON	0xc8
#define RAA_DMPVR2_MIN_DEVICE_REV	0x02000000

/* RAA Gen2 blackbox RAM definitions */
#define RAA_DMPVR2_BLACKBOX_RAM_PTR_ADDR	0x00C5
#define RAA_DMPVR2_BLACKBOX_RAM_RECORD_SIZE	38
#define RAA_DMPVR2_BLACKBOX_RAM_NUM_RECORDS	1
#define RAA_DMPVR2_BLACKBOX_RAM_TOTAL_SIZE	(RAA_DMPVR2_BLACKBOX_RAM_RECORD_SIZE * RAA_DMPVR2_BLACKBOX_RAM_NUM_RECORDS * RAA_DMP_DMA_DATA_REG_BYTES)

/* RAA Gen2 blackbox NVM definitions */
#define RAA_DMPVR2_BLACKBOX_NVM_BASE_ADDR	0x0500
#define RAA_DMPVR2_BLACKBOX_NVM_RECORD_SIZE	40
#define RAA_DMPVR2_BLACKBOX_NVM_NUM_RECORDS	10
#define RAA_DMPVR2_BLACKBOX_NVM_TOTAL_SIZE	(RAA_DMPVR2_BLACKBOX_NVM_RECORD_SIZE * RAA_DMPVR2_BLACKBOX_NVM_NUM_RECORDS * RAA_DMP_DMA_DATA_REG_BYTES)


/* RAA digital multiphase Gen3 definitions */
#define RAA_DMPVR3_MIN_DEVICE_REV	0x00000003
#define RAA_DMPVR3_BLACKBOX_OTP_R_EN_ADDR	0xECF0
#define RAA_DMPVR3_BLACKBOX_OTP_R_EN_BIT	0x0001

/* RAA Gen3 blackbox RAM definitions */
#define RAA_DMPVR3_BLACKBOX_RAM_PTR_ADDR	0x0049
#define RAA_DMPVR3_BLACKBOX_RAM_RECORD_SIZE	44
#define RAA_DMPVR3_BLACKBOX_RAM_NUM_RECORDS	1
#define RAA_DMPVR3_BLACKBOX_RAM_TOTAL_SIZE	(RAA_DMPVR3_BLACKBOX_RAM_RECORD_SIZE * RAA_DMPVR3_BLACKBOX_RAM_NUM_RECORDS * RAA_DMP_DMA_DATA_REG_BYTES)

/* RAA Gen3 blackbox NVM definitions */
#define RAA_DMPVR3_BLACKBOX_NVM_PTR_ADDR	0x0048
#define RAA_DMPVR3_BLACKBOX_NVM_RECORD_SIZE	46
#define RAA_DMPVR3_BLACKBOX_NVM_NUM_RECORDS	10
#define RAA_DMPVR3_BLACKBOX_NVM_TOTAL_SIZE	(RAA_DMPVR3_BLACKBOX_NVM_RECORD_SIZE * RAA_DMPVR3_BLACKBOX_NVM_NUM_RECORDS * RAA_DMP_DMA_DATA_REG_BYTES)


enum chips {
	isl68137,
	isl68220,
	isl68221,
	isl68222,
	isl68223,
	isl68224,
	isl68225,
	isl68226,
	isl68227,
	isl68229,
	isl68233,
	isl68239,
	isl69222,
	isl69223,
	isl69224,
	isl69225,
	isl69227,
	isl69228,
	isl69234,
	isl69236,
	isl69239,
	isl69242,
	isl69243,
	isl69247,
	isl69248,
	isl69254,
	isl69255,
	isl69256,
	isl69259,
	isl69260,
	isl69268,
	isl69269,
	isl69298,
	raa228000,
	raa228004,
	raa228006,
	raa228228,
	raa228234,
	raa228236,
	raa229001,
	raa229004,
};

enum variants {
	raa_dmpvr1_2rail,
	raa_dmpvr2_1rail,
	raa_dmpvr2_2rail,
	raa_dmpvr2_2rail_nontc,
	raa_dmpvr2_3rail,
	raa_dmpvr2_hv,
	raa_dmpvr3_2rail_nontc,
};

/* Per-variant NVM blackbox description; reg_write unset means read-only */
struct raa_dmp_nvmem_info {
	const struct nvmem_cell_info *cells;
	int ncells;
	nvmem_reg_read_t reg_read;
	nvmem_reg_write_t reg_write;
};

struct isl68137_data {
	struct pmbus_driver_info info;
	struct i2c_client *client;
	unsigned long variant;

	/* NVM blackbox */
	const struct raa_dmp_nvmem_info *nvmem_info;
	struct nvmem_config nvmem_config;
	struct nvmem_device *nvmem;
	u8 *dev_mem;
};

static const struct nvmem_cell_info raa_dmpvr2_nvmem_cells[] = {
	{
		.name   = "blackbox_nvm",
		.offset = 0,
		.bytes  = RAA_DMPVR2_BLACKBOX_NVM_TOTAL_SIZE,
	},
};

static const struct nvmem_cell_info raa_dmpvr3_nvmem_cells[] = {
	{
		.name   = "blackbox_nvm",
		.offset = 0,
		.bytes  = RAA_DMPVR3_BLACKBOX_NVM_TOTAL_SIZE,
	},
};

static const struct i2c_device_id raa_dmpvr_id[];

static ssize_t isl68137_avs_enable_show_page(struct i2c_client *client,
					     int page,
					     char *buf)
{
	int val = nh_pmbus_read_byte_data(client, page, PMBUS_OPERATION);

	return sprintf(buf, "%d\n",
		       (val & ISL68137_VOUT_AVS) == ISL68137_VOUT_AVS ? 1 : 0);
}

static ssize_t isl68137_avs_enable_store_page(struct i2c_client *client,
					      int page,
					      const char *buf, size_t count)
{
	int rc, op_val;
	bool result;

	rc = kstrtobool(buf, &result);
	if (rc)
		return rc;

	op_val = result ? ISL68137_VOUT_AVS : 0;

	/*
	 * Writes to VOUT setpoint over AVSBus will persist after the VRM is
	 * switched to PMBus control. Switching back to AVSBus control
	 * restores this persisted setpoint rather than re-initializing to
	 * PMBus VOUT_COMMAND. Writing VOUT_COMMAND first over PMBus before
	 * enabling AVS control is the workaround.
	 */
	if (op_val == ISL68137_VOUT_AVS) {
		rc = nh_pmbus_read_word_data(client, page, 0xff,
					  PMBUS_VOUT_COMMAND);
		if (rc < 0)
			return rc;

		rc = nh_pmbus_write_word_data(client, page, PMBUS_VOUT_COMMAND,
					   rc);
		if (rc < 0)
			return rc;
	}

	rc = nh_pmbus_update_byte_data(client, page, PMBUS_OPERATION,
				    ISL68137_VOUT_AVS, op_val);

	return (rc < 0) ? rc : count;
}

static ssize_t isl68137_avs_enable_show(struct device *dev,
					struct device_attribute *devattr,
					char *buf)
{
	struct i2c_client *client = to_i2c_client(dev->parent);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);

	return isl68137_avs_enable_show_page(client, attr->index, buf);
}

static ssize_t isl68137_avs_enable_store(struct device *dev,
				struct device_attribute *devattr,
				const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev->parent);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);

	return isl68137_avs_enable_store_page(client, attr->index, buf, count);
}

static SENSOR_DEVICE_ATTR_RW(avs0_enable, isl68137_avs_enable, 0);
static SENSOR_DEVICE_ATTR_RW(avs1_enable, isl68137_avs_enable, 1);

static struct attribute *enable_attrs[] = {
	&sensor_dev_attr_avs0_enable.dev_attr.attr,
	&sensor_dev_attr_avs1_enable.dev_attr.attr,
	NULL,
};

static const struct attribute_group enable_group = {
	.attrs = enable_attrs,
};

static ssize_t mfr_revision_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev->parent);
	u8 read_buf[5];
	int ret;

	// Read the block data from PMBUS_MFR_REVISION register
	ret = i2c_smbus_read_i2c_block_data(client, PMBUS_MFR_REVISION, 5, read_buf);
	if (ret < 0)
		return ret;

	if (ret != 5)
		return -EIO;

	// Byte 0: Length of the MFR_REVISION data.
	// Byte [4:1]: 4 bytes of space with no defined format.
	// NH usage:
	// Byte 1: Minor revision
	// Byte 2: Major revision
	return sprintf(buf, "%d.%d\n", read_buf[2], read_buf[1]);
}

static DEVICE_ATTR_RO(mfr_revision);

static struct attribute *mfr_attrs[] = {
	&dev_attr_mfr_revision.attr,
	NULL,
};

static const struct attribute_group mfr_group = {
	.attrs = mfr_attrs,
};

static int raa_dmpvr2_read_word_data(struct i2c_client *client, int page,
				     int phase, int reg)
{
	int ret;

	switch (reg) {
	case PMBUS_VIRT_READ_VMON:
		ret = nh_pmbus_read_word_data(client, page, phase,
					   RAA_DMPVR2_READ_VMON);
		break;
	default:
		ret = -ENODATA;
		break;
	}

	return ret;
}

static struct pmbus_driver_info raa_dmpvr_info = {
	.pages = 3,
	.format[PSC_VOLTAGE_IN] = direct,
	.format[PSC_VOLTAGE_OUT] = direct,
	.format[PSC_CURRENT_IN] = direct,
	.format[PSC_CURRENT_OUT] = direct,
	.format[PSC_POWER] = direct,
	.format[PSC_TEMPERATURE] = direct,
	.m[PSC_VOLTAGE_IN] = 1,
	.b[PSC_VOLTAGE_IN] = 0,
	.R[PSC_VOLTAGE_IN] = 2,
	.m[PSC_VOLTAGE_OUT] = 1,
	.b[PSC_VOLTAGE_OUT] = 0,
	.R[PSC_VOLTAGE_OUT] = 3,
	.m[PSC_CURRENT_IN] = 1,
	.b[PSC_CURRENT_IN] = 0,
	.R[PSC_CURRENT_IN] = 2,
	.m[PSC_CURRENT_OUT] = 1,
	.b[PSC_CURRENT_OUT] = 0,
	.R[PSC_CURRENT_OUT] = 1,
	.m[PSC_POWER] = 1,
	.b[PSC_POWER] = 0,
	.R[PSC_POWER] = 0,
	.m[PSC_TEMPERATURE] = 1,
	.b[PSC_TEMPERATURE] = 0,
	.R[PSC_TEMPERATURE] = 0,
	.func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_IIN | PMBUS_HAVE_PIN
	    | PMBUS_HAVE_STATUS_INPUT | PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2
	    | PMBUS_HAVE_TEMP3 | PMBUS_HAVE_STATUS_TEMP
	    | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
	    | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_POUT
		| PMBUS_HAVE_VMON,
	.func[1] = PMBUS_HAVE_IIN | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT
	    | PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP3 | PMBUS_HAVE_STATUS_TEMP
	    | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT | PMBUS_HAVE_IOUT
	    | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_POUT,
	.func[2] = PMBUS_HAVE_IIN | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT
	    | PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP3 | PMBUS_HAVE_STATUS_TEMP
	    | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT | PMBUS_HAVE_IOUT
	    | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_POUT,
};

static int raa_dmp_write_dma_addr(struct i2c_client *client, u16 addr)
{
	int ret;

	ret = i2c_smbus_write_word_data(client, RAA_DMP_DMA_ADDR_CMD, addr);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to set DMA address register: ret=%d, addr=0x%x\n", ret, addr);
		return ret;
	}

	return 0;
}

static int raa_dmp_write_dma_data(struct i2c_client *client, u16 addr, u32 dma_data)
{
	u8 buf[RAA_DMP_DMA_DATA_REG_BYTES];
	int ret;

	ret = raa_dmp_write_dma_addr(client, addr);
	if (ret < 0)
		return ret;

	for (int i = 0; i < RAA_DMP_DMA_DATA_REG_BYTES; i++)
		buf[i] = (dma_data >> (i * 8)) & 0xFF;

	ret = i2c_smbus_write_i2c_block_data(client, RAA_DMP_DMA_DATA_FIX_CMD, RAA_DMP_DMA_DATA_REG_BYTES, buf);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to write DMA data register: ret=%d, addr=0x%x\n", ret, addr);
		return ret;
	}

	return 0;
}

static int raa_dmp_read_dma_data(struct i2c_client *client, u8 cmd, u32 *dma_data)
{
	u8 buf[RAA_DMP_DMA_DATA_REG_BYTES];
	int ret;

	ret = i2c_smbus_read_i2c_block_data(client, cmd, RAA_DMP_DMA_DATA_REG_BYTES, buf);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to read DMA data register: ret=%d, cmd=0x%x\n", ret, cmd);
		return ret;
	}
	if (ret != 4) {
		dev_err(&client->dev, "Read an unexpected amount of bytes from DMA data register: bytes=%d\n", ret);
		return -EIO;
	}

	*dma_data = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
	return 0;
}

static int raa_dmp_read_dma_data_fix(struct i2c_client *client, u16 addr, u32 *dma_data)
{
	int ret;

	ret = raa_dmp_write_dma_addr(client, addr);
	if (ret < 0)
		return ret;

	ret = raa_dmp_read_dma_data(client, RAA_DMP_DMA_DATA_FIX_CMD, dma_data);
	if (ret < 0)
		return ret;

	return 0;
}

static int raa_dmp_read_dma_data_seq(struct i2c_client *client, u16 addr, u32 *dma_data, size_t len)
{
	int ret;
	int i;

	ret = raa_dmp_write_dma_addr(client, addr);
	if (ret < 0)
		return ret;

	for (i = 0; i < len; i++) {
		ret = raa_dmp_read_dma_data(client, RAA_DMP_DMA_DATA_SEQ_CMD, &dma_data[i]);
		if (ret < 0)
			return ret;
	}

	ret = i2c_smbus_read_word_data(client, RAA_DMP_DMA_ADDR_CMD);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to read DMA address register: ret=%d\n", ret);
		return ret;
	}
	if (ret != (addr + len)) {
		dev_err(&client->dev, "Sequential read failed to increment DMA address correctly: addr=0x%x\n", ret);
		return -EIO;
	}

	return 0;
}

static int device_revision_read(struct i2c_client *client, u32 *rev_num)
{
	u8 buf[5];
	int ret;

	// Fixed-length read: block reads with PEC fail on some adapters (xiic)
	ret = i2c_smbus_read_i2c_block_data(client, PMBUS_IC_DEVICE_REV, 5, buf);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to read PMBus IC_DEVICE_REV: ret=%d\n", ret);
		return ret;
	}
	if (ret != 5) {
		dev_err(&client->dev, "Failed to read IC_DEVICE_REV, expected 5 bytes and received %d\n", ret);
		return -EIO;
	}
	// Byte 0: Length of the IC_DEVICE_REV data.
	if (buf[0] != 4) {
		dev_err(&client->dev, "Unexpected IC_DEVICE_REV length: bytes=%d, wanted=4\n", buf[0]);
		return -EIO;
	}

	*rev_num = (buf[4] << 24) | (buf[3] << 16) | (buf[2] << 8) | buf[1];
	return 0;
}

static int raa_dmpvr2_ram_read_blackbox(struct i2c_client *client, u8 *out)
{
	u32 *buf = (u32 *) out;
	u16 bb_addr;
	int ret;
	u32 val;

	ret = device_revision_read(client, &val);
	if (ret < 0)
		return ret;
	if (val < RAA_DMPVR2_MIN_DEVICE_REV) {
		dev_err(&client->dev, "Device revision less than required minimum for blackbox reading: rev=%u.%u.%u.%u\n",
									(val >> 24) & 0xff, (val >> 16) & 0xff, (val >> 8) & 0xff, val & 0xff);
		return -ENOTSUPP;
	}

	// Read byte offset of RAM blackbox
	ret = raa_dmp_read_dma_data_fix(client, RAA_DMPVR2_BLACKBOX_RAM_PTR_ADDR, &val);
	if (ret < 0)
		return ret;

	// Transform byte offset into address (no OTP-segment bit for RAM)
	if ((val & 0x3) != 0)
		dev_warn(&client->dev, "RAM blackbox byte offset is not divisible by 4: offset=0x%x\n", val & 0x0000FFFF);
	bb_addr = ((val & 0x0000FFFF) / 4);

	// Read the blackbox record
	ret = raa_dmp_read_dma_data_seq(client, bb_addr, buf, RAA_DMPVR2_BLACKBOX_RAM_RECORD_SIZE);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to read blackbox record from RAM: ret=%d\n", ret);
		return ret;
	}

	return 0;
}

static int raa_dmpvr3_ram_read_blackbox(struct i2c_client *client, u8 *out)
{
	u32 *buf = (u32 *) out;
	u16 bb_addr;
	int ret;
	u32 val;

	ret = device_revision_read(client, &val);
	if (ret < 0)
		return ret;
	if (val < RAA_DMPVR3_MIN_DEVICE_REV) {
		dev_err(&client->dev, "Device revision less than required minimum for blackbox reading: rev=%u.%u.%u.%u\n",
									(val >> 24) & 0xff, (val >> 16) & 0xff, (val >> 8) & 0xff, val & 0xff);
		return -ENOTSUPP;
	}

	// Read byte offset of RAM blackbox
	ret = raa_dmp_read_dma_data_fix(client, RAA_DMPVR3_BLACKBOX_RAM_PTR_ADDR, &val);
	if (ret < 0)
		return ret;

	// Transform byte offset into address (no OTP-segment bit for RAM)
	if ((val & 0x3) != 0)
		dev_warn(&client->dev, "RAM blackbox byte offset is not divisible by 4: offset=0x%x\n", val & 0x0000FFFF);
	bb_addr = ((val & 0x0000FFFF) / 4);

	// Read the blackbox record
	ret = raa_dmp_read_dma_data_seq(client, bb_addr, buf, RAA_DMPVR3_BLACKBOX_RAM_RECORD_SIZE);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to read blackbox record from RAM: ret=%d\n", ret);
		return ret;
	}

	return 0;
}

static ssize_t blackbox_ram_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev->parent);
	const struct pmbus_driver_info *info = nh_pmbus_get_driver_info(client);
	struct isl68137_data *data = container_of(info, struct isl68137_data, info);
	size_t size = 0;
	int ret;

	ret = nh_pmbus_lock_interruptible(client);
	if (ret)
		return ret;

	switch (data->variant) {
	case raa_dmpvr2_1rail:
	case raa_dmpvr2_2rail:
	case raa_dmpvr2_2rail_nontc:
	case raa_dmpvr2_3rail:
	case raa_dmpvr2_hv:
		ret = raa_dmpvr2_ram_read_blackbox(data->client, buf);
		size = RAA_DMPVR2_BLACKBOX_RAM_TOTAL_SIZE;
		break;
	case raa_dmpvr3_2rail_nontc:
		ret = raa_dmpvr3_ram_read_blackbox(data->client, buf);
		size = RAA_DMPVR3_BLACKBOX_RAM_TOTAL_SIZE;
		break;
	default:
		ret = -EOPNOTSUPP;
		break;
	}

	nh_pmbus_unlock(client);
	if (ret < 0) {
		dev_err(&client->dev, "Could not read RAM blackbox\n");
		return ret;
	}

	return size;
}

static DEVICE_ATTR_RO(blackbox_ram);

/*
 blackbox_ram attribute may be more easily accessible from the debugfs,
 currently unimplemented due to debugfs being disabled in nh_pmbus.
*/
static struct attribute *blackbox_attrs[] = {
	&dev_attr_blackbox_ram.attr,
	NULL,
};

static const struct attribute_group blackbox_group = {
	.attrs = blackbox_attrs,
};

static int raa_dmpvr2_expose_nvm_blackbox(struct i2c_client *client)
{
	int ret;
	u32 val;

	ret = raa_dmp_read_dma_data_fix(client, 0x00C4, &val);
	if (ret < 0)
		return ret;

	// Send NVM blackbox data to device RAM
	ret = raa_dmp_write_dma_data(client, 0xEC58, val);
	if (ret < 0)
		return ret;

	ret = raa_dmp_write_dma_data(client, 0xEC59, 0x00001400);
	if (ret < 0)
		return ret;

	ret = raa_dmp_write_dma_data(client, 0xEC5B, 0x0C480190);
	if (ret < 0)
		return ret;

	ret = raa_dmp_write_dma_data(client, 0xEC5C, 0x00000001);
	if (ret < 0)
		return ret;

	return 0;
}

static int raa_dmpvr2_nvmem_read_blackbox(struct i2c_client *client, u8 *out)
{
	u32 *buf = (u32 *) out;
	int record;
	int offset;
	int ret;
	u32 val;

	ret = device_revision_read(client, &val);
	if (ret < 0)
		return ret;
	if (val < RAA_DMPVR2_MIN_DEVICE_REV) {
		dev_err(&client->dev, "Device revision less than required minimum for blackbox reading: rev=%u.%u.%u.%u\n",
									(val >> 24) & 0xff, (val >> 16) & 0xff, (val >> 8) & 0xff, val & 0xff);
		return -ENOTSUPP;
	}

	// Move NVM Blackbox data to device RAM
	ret = raa_dmpvr2_expose_nvm_blackbox(client);
	if (ret < 0)
		return ret;

	// Set DMA address to the starting address of the blackbox record
	ret = raa_dmp_write_dma_addr(client, RAA_DMPVR2_BLACKBOX_NVM_BASE_ADDR);
	if (ret < 0)
		return ret;

	// Read the blackbox records
	for (record = 0; record < RAA_DMPVR2_BLACKBOX_NVM_NUM_RECORDS; record++) {
		offset = RAA_DMPVR2_BLACKBOX_NVM_RECORD_SIZE * record;

		// Read blackbox header
		ret = raa_dmp_read_dma_data(client, RAA_DMP_DMA_DATA_FIX_CMD, &val);
		if (ret < 0)
			return ret;

		// Skip to next record if header is not valid
		if (val != 0xFFFF0000) {
			memset(buf + offset, 0, RAA_DMPVR2_BLACKBOX_NVM_RECORD_SIZE * RAA_DMP_DMA_DATA_REG_BYTES);
			if (record < RAA_DMPVR2_BLACKBOX_NVM_NUM_RECORDS - 1) {
				ret = raa_dmp_write_dma_addr(client, RAA_DMPVR2_BLACKBOX_NVM_BASE_ADDR + offset + RAA_DMPVR2_BLACKBOX_NVM_RECORD_SIZE);
				if (ret < 0)
					return ret;
			}
		} else {
			ret = raa_dmp_read_dma_data_seq(client, RAA_DMPVR2_BLACKBOX_NVM_BASE_ADDR + offset, &buf[offset], RAA_DMPVR2_BLACKBOX_NVM_RECORD_SIZE);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static int raa_dmpvr3_enable_otp_read(struct i2c_client *client)
{
	int ret;
	u32 val;

	ret = raa_dmp_read_dma_data_fix(client, RAA_DMPVR3_BLACKBOX_OTP_R_EN_ADDR, &val);
	if (ret < 0)
		return ret;

	// Check if OTP reading is already enabled
	if ((val & RAA_DMPVR3_BLACKBOX_OTP_R_EN_BIT) == 0) {
		val = val | RAA_DMPVR3_BLACKBOX_OTP_R_EN_BIT;
		ret = raa_dmp_write_dma_data(client, RAA_DMPVR3_BLACKBOX_OTP_R_EN_ADDR, val);
		if (ret < 0) {
			dev_err(&client->dev, "Failed to enable OTP memory read: ret=%d\n", ret);
			return ret;
		}
	}

	return 0;
}

static int raa_dmpvr3_nvmem_read_blackbox(struct i2c_client *client, u8 *out)
{
	u32 *buf = (u32 *) out;
	u16 bb_addr;
	int record;
	int offset;
	int ret;
	u32 val;

	ret = device_revision_read(client, &val);
	if (ret < 0)
		return ret;
	if (val < RAA_DMPVR3_MIN_DEVICE_REV) {
		dev_err(&client->dev, "Device revision less than required minimum for blackbox reading: rev=%u.%u.%u.%u\n",
									(val >> 24) & 0xff, (val >> 16) & 0xff, (val >> 8) & 0xff, val & 0xff);
		return -ENOTSUPP;
	}

	// Read byte offset of NVM blackbox
	ret = raa_dmp_read_dma_data_fix(client, RAA_DMPVR3_BLACKBOX_NVM_PTR_ADDR, &val);
	if (ret < 0)
		return ret;

	// Transform byte offset into address; bit 15 selects the OTP segment.
	if ((val & 0x3) != 0)
		dev_warn(&client->dev, "NVM blackbox byte offset is not divisible by 4: offset=0x%x\n", val & 0x0000FFFF);
	bb_addr = ((val & 0x0000FFFF) / 4) | (1 << 15);

	ret = raa_dmpvr3_enable_otp_read(client);
	if (ret < 0)
		return ret;

	// Set DMA address to the starting address of the blackbox record
	ret = raa_dmp_write_dma_addr(client, bb_addr);
	if (ret < 0)
		return ret;

	// Read the blackbox records
	for (record = 0; record < RAA_DMPVR3_BLACKBOX_NVM_NUM_RECORDS; record++) {
		offset = RAA_DMPVR3_BLACKBOX_NVM_RECORD_SIZE * record;

		// Read blackbox header
		ret = raa_dmp_read_dma_data(client, RAA_DMP_DMA_DATA_FIX_CMD, &val);
		if (ret < 0)
			return ret;

		// Skip to next record if header is empty
		if (val == 0) {
			memset(buf + offset, 0, RAA_DMPVR3_BLACKBOX_NVM_RECORD_SIZE * RAA_DMP_DMA_DATA_REG_BYTES);
			if (record < RAA_DMPVR3_BLACKBOX_NVM_NUM_RECORDS - 1) {
				ret = raa_dmp_write_dma_addr(client, bb_addr + offset + RAA_DMPVR3_BLACKBOX_NVM_RECORD_SIZE);
				if (ret < 0)
					return ret;
			}
		} else {
			ret = raa_dmp_read_dma_data_seq(client, bb_addr + offset, &buf[offset], RAA_DMPVR3_BLACKBOX_NVM_RECORD_SIZE);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static int raa_dmp_nvmem_read(void *priv, unsigned int offset, void *val, size_t bytes)
{
	struct isl68137_data *data = priv;
	int ret;

	if (offset + bytes > data->nvmem_config.size)
		return -EINVAL;

	ret = nh_pmbus_lock_interruptible(data->client);
	if (ret)
		return ret;

	if (offset == 0) {
		memset(data->dev_mem, 0, data->nvmem_config.size);

		switch (data->variant) {
		case raa_dmpvr2_1rail:
		case raa_dmpvr2_2rail:
		case raa_dmpvr2_2rail_nontc:
		case raa_dmpvr2_3rail:
		case raa_dmpvr2_hv:
			ret = raa_dmpvr2_nvmem_read_blackbox(data->client, data->dev_mem);
			break;
		case raa_dmpvr3_2rail_nontc:
			ret = raa_dmpvr3_nvmem_read_blackbox(data->client, data->dev_mem);
			break;
		default:
			ret = -EOPNOTSUPP;
			break;
		}

		if (ret < 0) {
			nh_pmbus_unlock(data->client);
			dev_err(&data->client->dev, "Could not read NVM blackbox\n");
			return ret;
		}
	}

	memcpy(val, data->dev_mem + offset, bytes);
	nh_pmbus_unlock(data->client);
	return 0;
}

static const struct attribute_group *isl68137_attribute_groups[] = {
	&enable_group,
	&mfr_group,
	NULL,
};

static const struct attribute_group *raa_dmpvr2_attribute_groups[] = {
	&mfr_group,
	&blackbox_group,
	NULL,
};

static const struct raa_dmp_nvmem_info raa_dmpvr2_nvmem_info = {
	.cells    = raa_dmpvr2_nvmem_cells,
	.ncells   = ARRAY_SIZE(raa_dmpvr2_nvmem_cells),
	.reg_read = raa_dmp_nvmem_read,
	.reg_write = NULL,
};

static const struct attribute_group *raa_dmpvr3_attribute_groups[] = {
	&mfr_group,
	&blackbox_group,
	NULL,
};

static const struct raa_dmp_nvmem_info raa_dmpvr3_nvmem_info = {
	.cells    = raa_dmpvr3_nvmem_cells,
	.ncells   = ARRAY_SIZE(raa_dmpvr3_nvmem_cells),
	.reg_read = raa_dmp_nvmem_read,
	.reg_write = NULL,
};

static int raa_dmp_config_nvmem(struct isl68137_data *data)
{
	const struct raa_dmp_nvmem_info *nvmem_info = data->nvmem_info;

	data->nvmem_config.name      = dev_name(&data->client->dev);
	data->nvmem_config.dev       = &data->client->dev;
	data->nvmem_config.root_only = true;
	data->nvmem_config.read_only = !nvmem_info->reg_write;
	data->nvmem_config.owner     = THIS_MODULE;
	data->nvmem_config.reg_read  = nvmem_info->reg_read;
	data->nvmem_config.reg_write = nvmem_info->reg_write;
	data->nvmem_config.cells     = nvmem_info->cells;
	data->nvmem_config.ncells    = nvmem_info->ncells;
	data->nvmem_config.priv      = data;
	data->nvmem_config.stride    = 1;
	data->nvmem_config.word_size = 1;
	data->nvmem_config.size      = nvmem_info->cells[0].bytes;

	data->dev_mem = devm_kzalloc(&data->client->dev, data->nvmem_config.size, GFP_KERNEL);
	if (!data->dev_mem)
		return -ENOMEM;

	data->nvmem = devm_nvmem_register(&data->client->dev, &data->nvmem_config);
	if (IS_ERR(data->nvmem)) {
		dev_err(&data->client->dev, "Could not register nvmem!\n");
		return PTR_ERR(data->nvmem);
	}

	return 0;
}

static int isl68137_probe(struct i2c_client *client)
{
	struct isl68137_data *data;
	struct pmbus_driver_info *info;
	unsigned long variant;
	int ret;

	data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	data->client = client;

	info = &data->info;
	memcpy(info, &raa_dmpvr_info, sizeof(*info));

	variant = i2c_match_id(raa_dmpvr_id, client)->driver_data;
	data->variant = variant;
	switch (variant) {
	case raa_dmpvr1_2rail:
		info->pages = 2;
		info->R[PSC_VOLTAGE_IN] = 3;
		info->func[0] &= ~PMBUS_HAVE_VMON;
		info->func[1] = PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
		    | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT
		    | PMBUS_HAVE_POUT;
		info->groups = isl68137_attribute_groups;
		break;
	case raa_dmpvr2_1rail:
		info->pages = 1;
		info->read_word_data = raa_dmpvr2_read_word_data;
		info->groups = raa_dmpvr2_attribute_groups;
		data->nvmem_info = &raa_dmpvr2_nvmem_info;
		break;
	case raa_dmpvr2_2rail_nontc:
		info->func[0] &= ~PMBUS_HAVE_TEMP3;
		info->func[1] &= ~PMBUS_HAVE_TEMP3;
		fallthrough;
	case raa_dmpvr2_2rail:
		info->pages = 2;
		info->read_word_data = raa_dmpvr2_read_word_data;
		info->groups = raa_dmpvr2_attribute_groups;
		data->nvmem_info = &raa_dmpvr2_nvmem_info;
		break;
	case raa_dmpvr2_3rail:
		info->read_word_data = raa_dmpvr2_read_word_data;
		info->groups = raa_dmpvr2_attribute_groups;
		data->nvmem_info = &raa_dmpvr2_nvmem_info;
		break;
	case raa_dmpvr2_hv:
		info->pages = 1;
		info->R[PSC_VOLTAGE_IN] = 1;
		info->m[PSC_VOLTAGE_OUT] = 2;
		info->R[PSC_VOLTAGE_OUT] = 2;
		info->m[PSC_CURRENT_IN] = 2;
		info->m[PSC_POWER] = 2;
		info->R[PSC_POWER] = -1;
		info->read_word_data = raa_dmpvr2_read_word_data;
		info->groups = raa_dmpvr2_attribute_groups;
		data->nvmem_info = &raa_dmpvr2_nvmem_info;
		break;
	case raa_dmpvr3_2rail_nontc:
		info->func[0] &= ~PMBUS_HAVE_TEMP3;
		info->func[1] &= ~PMBUS_HAVE_TEMP3;
		info->pages = 2;
		info->read_word_data = raa_dmpvr2_read_word_data;
		info->groups = raa_dmpvr3_attribute_groups;
		data->nvmem_info = &raa_dmpvr3_nvmem_info;
		break;
	default:
		return -ENODEV;
	}

	ret = nh_pmbus_do_probe(client, info);
	if (ret)
		return ret;

	if (data->nvmem_info) {
		ret = raa_dmp_config_nvmem(data);
		if (ret)
			return ret;
	}

	return 0;
}

static const struct i2c_device_id raa_dmpvr_id[] = {
	{"nh_isl68137", raa_dmpvr1_2rail},
	{"nh_isl68220", raa_dmpvr2_2rail},
	{"nh_isl68221", raa_dmpvr2_3rail},
	{"nh_isl68222", raa_dmpvr2_2rail},
	{"nh_isl68223", raa_dmpvr2_2rail},
	{"nh_isl68224", raa_dmpvr2_3rail},
	{"nh_isl68225", raa_dmpvr2_2rail},
	{"nh_isl68226", raa_dmpvr2_3rail},
	{"nh_isl68227", raa_dmpvr2_1rail},
	{"nh_isl68229", raa_dmpvr2_3rail},
	{"nh_isl68233", raa_dmpvr2_2rail},
	{"nh_isl68239", raa_dmpvr2_3rail},

	{"nh_isl69222", raa_dmpvr2_2rail},
	{"nh_isl69223", raa_dmpvr2_3rail},
	{"nh_isl69224", raa_dmpvr2_2rail},
	{"nh_isl69225", raa_dmpvr2_2rail},
	{"nh_isl69227", raa_dmpvr2_3rail},
	{"nh_isl69228", raa_dmpvr2_3rail},
	{"nh_isl69234", raa_dmpvr2_2rail},
	{"nh_isl69236", raa_dmpvr2_2rail},
	{"nh_isl69239", raa_dmpvr2_3rail},
	{"nh_isl69242", raa_dmpvr2_2rail},
	{"nh_isl69243", raa_dmpvr2_1rail},
	{"nh_isl69247", raa_dmpvr2_2rail},
	{"nh_isl69248", raa_dmpvr2_2rail},
	{"nh_isl69254", raa_dmpvr2_2rail},
	{"nh_isl69255", raa_dmpvr2_2rail},
	{"nh_isl69256", raa_dmpvr2_2rail},
	{"nh_isl69259", raa_dmpvr2_2rail},
	{"nh_isl69260", raa_dmpvr2_2rail},
	{"nh_isl69268", raa_dmpvr2_2rail},
	{"nh_isl69269", raa_dmpvr2_3rail},
	{"nh_isl69298", raa_dmpvr2_2rail},

	{"nh_raa228000", raa_dmpvr2_hv},
	{"nh_raa228004", raa_dmpvr2_hv},
	{"nh_raa228006", raa_dmpvr2_hv},
	{"nh_raa228228", raa_dmpvr2_2rail_nontc},
	{"nh_raa228234", raa_dmpvr3_2rail_nontc},
	{"nh_raa228236", raa_dmpvr3_2rail_nontc},
	{"nh_raa228244", raa_dmpvr3_2rail_nontc},
	{"nh_raa229001", raa_dmpvr2_2rail},
	{"nh_raa229004", raa_dmpvr2_2rail},
	{}
};

MODULE_DEVICE_TABLE(i2c, raa_dmpvr_id);

/* This is the driver that will be inserted */
static struct i2c_driver isl68137_driver = {
	.driver = {
		   .name = "nh_isl68137",
		   },
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
	.probe_new = isl68137_probe,
#else
	.probe = isl68137_probe,
#endif
	.id_table = raa_dmpvr_id,
};

module_i2c_driver(isl68137_driver);

MODULE_AUTHOR("Maxim Sloyko <maxims@google.com>");
MODULE_DESCRIPTION("PMBus driver for Renesas digital multiphase voltage regulators");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(PMBUS);

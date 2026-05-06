// SPDX-License-Identifier: GPL-2.0
/*
 * ADM1266 - Cascadable Super Sequencer with Margin
 * Control and Fault Recording
 *
 * Copyright 2020 Analog Devices Inc.
 */
/*
 * Nexthop changes for BLACKBOX SUPPORT:
 *
 * Original kernel driver version: v6.16.2
 *
 * Reading Blackbox:
 * -----------------
 * The standard linux driver had many i2c message correctness issues while accessing
 * the black box. This resulted in the driver returning -EPROTO to userspace.
 *
 * The Nexthop driver performs the following steps using the i2c_transfer() with
 * atomic write-then-read transactions:
 * - Step 1: Read blackbox info [0xE6] -> 4 bytes to get record count
 * - Step 2: For each record, atomic transaction [0xDE, 0x01, index] -> 64 bytes
 *
 * Clear Blackbox:
 * ---------------
 * Added support to clear the blackbox via the standard driver.
 *
 * The Nexthop driver uses the following command sequence to clear blackbox entries:
 *   - Command: [0xDE, 0xFE, 0x00] sent via I2C transaction
 * The userspace access is provided via the same nvmem sysfs interface.
 *   - Usage: echo "anything" > /sys/bus/nvmem/devices/<bus>-<device><cell>/nvmem
 * 
 * RTC setting:
 * ------------
 * The ADM1266 RTC is configured to count time from a custom epoch (2024-01-01 00:00:00 UTC).
 * This is necessary because the ADM1266 RTC only uses 4 bytes to store seconds, which
 * rollover every ~136 years. Using a custom epoch maximizes the usable time range.
 *
 * The custom epoch is hardcoded in this driver (ADM1266_CUSTOM_EPOCH_OFFSET) and
 * is automatically applied during the device probe.
 *
 * For flexibility, userspace can override the epoch offset via sysfs:
 *   - Usage: echo "<epoch_offset_sec>" > /sys/bus/i2c/devices/<bus>-<device>/rtc_epoch_offset
 *   - <epoch_offset_sec> must be a non-negative integer seconds since 1970.
 *   - A daemon can periodically write to this to sync RTC with the system time to avoid
 *     clocks' drift.
 * If the ADM1266 is power cycled, the RTC resets to zero and will be reconfigured on next
 * device probe.
 *
 * Extra sysfs attributes:
 * ------------------------
 * - rtc_epoch_offset: Read-write attribute to get/set the epoch offset (since 1970) in seconds.
 * - powerup_counter: Read-only attribute to get the current powerup counter.
 * - firmware_revision: Read-only attribute to get the firmware revision.
 * - mfr_revision: Read-only attribute to get the manufacturer revision.
 */

#include <linux/bitfield.h>
#include <linux/crc8.h>
#include <linux/debugfs.h>
#include <linux/gpio/driver.h>
#include <linux/i2c.h>
#include <linux/i2c-smbus.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/nvmem-provider.h>
#include "nh_pmbus.h"
#include <linux/slab.h>
#include <linux/timekeeping.h>
#include <linux/version.h>

/*
 * Nexthop's custom epoch: 2024-01-01 00:00:00 UTC
 * This is represented as the elapsed seconds since the UNIX epoch (1970-01-01 00:00:00 UTC).
 */
#define ADM1266_NH_CUSTOM_EPOCH_OFFSET 1704067200ULL

#define ADM1266_MFR_REVISION   0x9B
#define ADM1266_IC_DEVICE_REV  0xAE
#define ADM1266_BLACKBOX_CONFIG	0xD3
#define ADM1266_PDIO_CONFIG	0xD4
#define ADM1266_READ_STATE	0xD9
#define ADM1266_READ_BLACKBOX	0xDE
#define ADM1266_SET_RTC		0xDF
#define ADM1266_GPIO_CONFIG	0xE1
#define ADM1266_POWERUP_COUNTER 0xE4
#define ADM1266_BLACKBOX_INFO	0xE6
#define ADM1266_PDIO_STATUS	0xE9
#define ADM1266_GPIO_STATUS	0xEA

/* ADM1266 GPIO defines */
#define ADM1266_GPIO_NR			9
#define ADM1266_GPIO_FUNCTIONS(x)	FIELD_GET(BIT(0), x)
#define ADM1266_GPIO_INPUT_EN(x)	FIELD_GET(BIT(2), x)
#define ADM1266_GPIO_OUTPUT_EN(x)	FIELD_GET(BIT(3), x)
#define ADM1266_GPIO_OPEN_DRAIN(x)	FIELD_GET(BIT(4), x)

/* ADM1266 PDIO defines */
#define ADM1266_PDIO_NR			16
#define ADM1266_PDIO_PIN_CFG(x)		FIELD_GET(GENMASK(15, 13), x)
#define ADM1266_PDIO_GLITCH_FILT(x)	FIELD_GET(GENMASK(12, 9), x)
#define ADM1266_PDIO_OUT_CFG(x)		FIELD_GET(GENMASK(2, 0), x)

#define ADM1266_BLACKBOX_OFFSET		0
#define ADM1266_BLACKBOX_SIZE		64
#define ADM1266_BLACKBOX_MAX_RECORDS	32

#define ADM1266_PMBUS_BLOCK_MAX		255

struct nh_adm1266_data {
	struct pmbus_driver_info info;
	struct gpio_chip gc;
	const char *gpio_names[ADM1266_GPIO_NR + ADM1266_PDIO_NR];
	struct i2c_client *client;
	struct dentry *debugfs_dir;
	struct nvmem_config nvmem_config;
	struct nvmem_device *nvmem;
	u8 *dev_mem;
	struct mutex buf_mutex;
	u8 write_buf[ADM1266_PMBUS_BLOCK_MAX + 1] ____cacheline_aligned;
	u8 read_buf[ADM1266_PMBUS_BLOCK_MAX + 1] ____cacheline_aligned;
	u64 rtc_epoch_offset;  // User-provided offset from the UNIX epoch (Jan 1st, 1970) in seconds.
};

static const struct nvmem_cell_info nh_adm1266_nvmem_cells[] = {
	{
		.name           = "blackbox",
		.offset         = ADM1266_BLACKBOX_OFFSET,
		.bytes          = 2048,
	},
};

DECLARE_CRC8_TABLE(pmbus_crc_table);

/*
 * Different from Block Read as it sends data and waits for the slave to
 * return a value dependent on that data. The protocol is simply a Write Block
 * followed by a Read Block without the Read-Block command field and the
 * Write-Block STOP bit.
 */
static int nh_adm1266_pmbus_block_xfer(struct nh_adm1266_data *data, u8 cmd, u8 w_len, u8 *data_w,
				    u8 *data_r)
{
	struct i2c_client *client = data->client;
	struct i2c_msg msgs[2] = {
		{
			.addr = client->addr,
			.flags = I2C_M_DMA_SAFE,
			.buf = data->write_buf,
			.len = w_len + 2,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD | I2C_M_DMA_SAFE,
			.buf = data->read_buf,
			.len = ADM1266_PMBUS_BLOCK_MAX + 2,
		}
	};
	u8 addr;
	u8 crc;
	int ret;

	mutex_lock(&data->buf_mutex);

	msgs[0].buf[0] = cmd;
	msgs[0].buf[1] = w_len;
	memcpy(&msgs[0].buf[2], data_w, w_len);

	ret = i2c_transfer(client->adapter, msgs, 2);
	if (ret != 2) {
		if (ret >= 0)
			ret = -EPROTO;

		mutex_unlock(&data->buf_mutex);

		return ret;
	}

	if (client->flags & I2C_CLIENT_PEC) {
		addr = i2c_8bit_addr_from_msg(&msgs[0]);
		crc = crc8(pmbus_crc_table, &addr, 1, 0);
		crc = crc8(pmbus_crc_table, msgs[0].buf,  msgs[0].len, crc);

		addr = i2c_8bit_addr_from_msg(&msgs[1]);
		crc = crc8(pmbus_crc_table, &addr, 1, crc);
		crc = crc8(pmbus_crc_table, msgs[1].buf,  msgs[1].buf[0] + 1, crc);

		if (crc != msgs[1].buf[msgs[1].buf[0] + 1]) {
			mutex_unlock(&data->buf_mutex);
			return -EBADMSG;
		}
	}

	memcpy(data_r, &msgs[1].buf[1], msgs[1].buf[0]);

	ret = msgs[1].buf[0];
	mutex_unlock(&data->buf_mutex);

	return ret;
}

static const unsigned int nh_adm1266_gpio_mapping[ADM1266_GPIO_NR][2] = {
	{1, 0},
	{2, 1},
	{3, 2},
	{4, 8},
	{5, 9},
	{6, 10},
	{7, 11},
	{8, 6},
	{9, 7},
};

static const char *nh_adm1266_names[ADM1266_GPIO_NR + ADM1266_PDIO_NR] = {
	"GPIO1", "GPIO2", "GPIO3", "GPIO4", "GPIO5", "GPIO6", "GPIO7", "GPIO8",
	"GPIO9", "PDIO1", "PDIO2", "PDIO3", "PDIO4", "PDIO5", "PDIO6",
	"PDIO7", "PDIO8", "PDIO9", "PDIO10", "PDIO11", "PDIO12", "PDIO13",
	"PDIO14", "PDIO15", "PDIO16",
};

static int nh_adm1266_gpio_get(struct gpio_chip *chip, unsigned int offset)
{
	struct nh_adm1266_data *data = gpiochip_get_data(chip);
	u8 read_buf[I2C_SMBUS_BLOCK_MAX + 1];
	unsigned long pins_status;
	unsigned int pmbus_cmd;
	int ret;

	if (offset < ADM1266_GPIO_NR)
		pmbus_cmd = ADM1266_GPIO_STATUS;
	else
		pmbus_cmd = ADM1266_PDIO_STATUS;

	ret = i2c_smbus_read_block_data(data->client, pmbus_cmd, read_buf);
	if (ret < 0)
		return ret;

	pins_status = read_buf[0] + (read_buf[1] << 8);
	if (offset < ADM1266_GPIO_NR)
		return test_bit(nh_adm1266_gpio_mapping[offset][1], &pins_status);

	return test_bit(offset - ADM1266_GPIO_NR, &pins_status);
}

static int nh_adm1266_gpio_get_multiple(struct gpio_chip *chip, unsigned long *mask,
				     unsigned long *bits)
{
	struct nh_adm1266_data *data = gpiochip_get_data(chip);
	u8 read_buf[ADM1266_PMBUS_BLOCK_MAX + 1];
	unsigned long status;
	unsigned int gpio_nr;
	int ret;

	ret = i2c_smbus_read_block_data(data->client, ADM1266_GPIO_STATUS, read_buf);
	if (ret < 0)
		return ret;

	status = read_buf[0] + (read_buf[1] << 8);

	*bits = 0;
	for_each_set_bit(gpio_nr, mask, ADM1266_GPIO_NR) {
		if (test_bit(nh_adm1266_gpio_mapping[gpio_nr][1], &status))
			set_bit(gpio_nr, bits);
	}

	ret = i2c_smbus_read_block_data(data->client, ADM1266_PDIO_STATUS, read_buf);
	if (ret < 0)
		return ret;

	status = read_buf[0] + (read_buf[1] << 8);

	*bits = 0;
	for_each_set_bit_from(gpio_nr, mask, ADM1266_GPIO_NR + ADM1266_PDIO_STATUS) {
		if (test_bit(gpio_nr - ADM1266_GPIO_NR, &status))
			set_bit(gpio_nr, bits);
	}

	return 0;
}

static void nh_adm1266_gpio_dbg_show(struct seq_file *s, struct gpio_chip *chip)
{
	struct nh_adm1266_data *data = gpiochip_get_data(chip);
	u8 read_buf[ADM1266_PMBUS_BLOCK_MAX + 1];
	unsigned long gpio_config;
	unsigned long pdio_config;
	unsigned long pin_cfg;
	u8 write_cmd;
	int ret;
	int i;

	for (i = 0; i < ADM1266_GPIO_NR; i++) {
		write_cmd = nh_adm1266_gpio_mapping[i][1];
		ret = nh_adm1266_pmbus_block_xfer(data, ADM1266_GPIO_CONFIG, 1, &write_cmd, read_buf);
		if (ret != 2)
			return;

		gpio_config = read_buf[0];
		seq_puts(s, nh_adm1266_names[i]);

		seq_puts(s, " ( ");
		if (!ADM1266_GPIO_FUNCTIONS(gpio_config)) {
			seq_puts(s, "high-Z )\n");
			continue;
		}
		if (ADM1266_GPIO_INPUT_EN(gpio_config))
			seq_puts(s, "input ");
		if (ADM1266_GPIO_OUTPUT_EN(gpio_config))
			seq_puts(s, "output ");
		if (ADM1266_GPIO_OPEN_DRAIN(gpio_config))
			seq_puts(s, "open-drain )\n");
		else
			seq_puts(s, "push-pull )\n");
	}

	write_cmd = 0xFF;
	ret = nh_adm1266_pmbus_block_xfer(data, ADM1266_PDIO_CONFIG, 1, &write_cmd, read_buf);
	if (ret != 32)
		return;

	for (i = 0; i < ADM1266_PDIO_NR; i++) {
		seq_puts(s, nh_adm1266_names[ADM1266_GPIO_NR + i]);

		pdio_config = read_buf[2 * i];
		pdio_config += (read_buf[2 * i + 1] << 8);
		pin_cfg = ADM1266_PDIO_PIN_CFG(pdio_config);

		seq_puts(s, " ( ");
		if (!pin_cfg || pin_cfg > 5) {
			seq_puts(s, "high-Z )\n");
			continue;
		}

		if (pin_cfg & BIT(0))
			seq_puts(s, "output ");

		if (pin_cfg & BIT(1))
			seq_puts(s, "input ");

		seq_puts(s, ")\n");
	}
}

static int nh_adm1266_config_gpio(struct nh_adm1266_data *data)
{
	const char *name = dev_name(&data->client->dev);
	char *gpio_name;
	int ret;
	int i;

	for (i = 0; i < ARRAY_SIZE(data->gpio_names); i++) {
		gpio_name = devm_kasprintf(&data->client->dev, GFP_KERNEL, "nh_adm1266-%x-%x-%s",
					   data->client->adapter->nr, data->client->addr, nh_adm1266_names[i]);
		if (!gpio_name)
			return -ENOMEM;

		data->gpio_names[i] = gpio_name;
	}

	data->gc.label = name;
	data->gc.parent = &data->client->dev;
	data->gc.owner = THIS_MODULE;
	data->gc.can_sleep = true;
	data->gc.base = -1;
	data->gc.names = data->gpio_names;
	data->gc.ngpio = ARRAY_SIZE(data->gpio_names);
	data->gc.get = nh_adm1266_gpio_get;
	data->gc.get_multiple = nh_adm1266_gpio_get_multiple;
	data->gc.dbg_show = nh_adm1266_gpio_dbg_show;

	ret = devm_gpiochip_add_data(&data->client->dev, &data->gc, data);
	if (ret)
		dev_err(&data->client->dev, "GPIO registering failed (%d)\n", ret);

	return ret;
}

static int nh_adm1266_state_read(struct seq_file *s, void *pdata)
{
	struct device *dev = s->private;
	struct i2c_client *client = to_i2c_client(dev);
	int ret;

	ret = i2c_smbus_read_word_data(client, ADM1266_READ_STATE);
	if (ret < 0)
		return ret;

	seq_printf(s, "%d\n", ret);

	return 0;
}

static void nh_adm1266_init_debugfs(struct nh_adm1266_data *data)
{
	struct dentry *root;

	root = nh_pmbus_get_debugfs_dir(data->client);
	if (!root)
		return;

	data->debugfs_dir = debugfs_create_dir(data->client->name, root);

	debugfs_create_devm_seqfile(&data->client->dev, "sequencer_state", data->debugfs_dir,
				    nh_adm1266_state_read);
}

/*
 * Performs an atomic write-then-read transaction, by bundling
 * a write of the PMBus command (with optional write data) and
 * a subsequent read of the response data, into a single I2C transaction.
 *
 * This is Nexthop-specific change, which works for blackbox reading. The original
 * driver used `nh_adm1266_pmbus_block_xfer()`, which was not working.
 */
static int nh_adm1266_i2c_atomic_write_then_read(
	struct nh_adm1266_data *data,
	u8 cmd,
	u16 w_len,
	u8 *w_data,
	u16 r_len,
	u8 *r_data)
{
	u8 response_len;
	int ret;
	struct i2c_msg msgs[2] = {
		{
			.addr = data->client->addr,
			.flags = 0,
			.buf = data->write_buf,
			.len = w_len ? w_len + 2 : 1,
		},
		{
			.addr = data->client->addr,
			.flags = I2C_M_RD,
			.buf = data->read_buf,
			.len = r_len + 1,
		}
	};

	if (msgs[0].len > sizeof(data->write_buf) || msgs[1].len > sizeof(data->read_buf))
		return -EMSGSIZE;

	mutex_lock(&data->buf_mutex);

	// Prepare Write message with command and optional write data.
	msgs[0].buf[0] = cmd;
	if (w_len > 0) {
		msgs[0].buf[1] = w_len;
		memcpy(&msgs[0].buf[2], w_data, w_len);
	}

	// Send messages.
	ret = i2c_transfer(data->client->adapter, msgs, 2);
	if (ret != 2) {
		mutex_unlock(&data->buf_mutex);
		return ret < 0 ? ret : -EIO;
	}

	// Verify response length.
	response_len = msgs[1].buf[0];
	if (response_len != r_len) {
		mutex_unlock(&data->buf_mutex);
		return -EIO;
	}

	// Return the response data via the output param.
	memcpy(r_data, &msgs[1].buf[1], response_len);

	mutex_unlock(&data->buf_mutex);

	return 0;
}


static int nh_adm1266_nvmem_read_blackbox(struct nh_adm1266_data *data, u8 *read_buff)
{
	u8 blackbox_info_buf[4];
	u8 first_index;
	u8 latest_index;
	u8 record_count;
	int ret;
	int i;

	// Step 1: Read blackbox info.
	ret = nh_adm1266_i2c_atomic_write_then_read(
		data,
		ADM1266_BLACKBOX_INFO,
		/*w_len=*/0,
		/*w_data=*/NULL,
		/*r_len=*/sizeof(blackbox_info_buf),
		/*r_data=*/blackbox_info_buf);
	if (ret) {
		dev_err(&data->client->dev, "Failed to read BLACKBOX_INFORMATION: ret=%d", ret);
		return ret;
	}
	latest_index = blackbox_info_buf[2];
	record_count = blackbox_info_buf[3];
	first_index = (latest_index - record_count + 1 + ADM1266_BLACKBOX_MAX_RECORDS) % ADM1266_BLACKBOX_MAX_RECORDS;

	// Step 2: Iterate over and read all blackbox records.
	for (i = 0; i < record_count; i++) {
		u8 index = (first_index + i) % ADM1266_BLACKBOX_MAX_RECORDS;

		ret = nh_adm1266_i2c_atomic_write_then_read(
			data,
			ADM1266_READ_BLACKBOX,
			/*w_len=*/1,
			/*w_data=*/&index,
			/*r_len=*/ADM1266_BLACKBOX_SIZE,
			/*r_data=*/read_buff);
		if (ret) {
			dev_err(&data->client->dev, "Failed to read blackbox record index=%d: ret=%d", index, ret);
			return ret;
		}
		read_buff += ADM1266_BLACKBOX_SIZE;
	}

	return 0;
}

static int nh_adm1266_nvmem_read(void *priv, unsigned int offset, void *val, size_t bytes)
{
	struct nh_adm1266_data *data = priv;
	int ret;

	if (offset + bytes > data->nvmem_config.size)
		return -EINVAL;

	if (offset == 0) {
		memset(data->dev_mem, 0, data->nvmem_config.size);

		ret = nh_adm1266_nvmem_read_blackbox(data, data->dev_mem);
		if (ret) {
			dev_err(&data->client->dev, "Could not read blackbox!");
			return ret;
		}
	}

	memcpy(val, data->dev_mem + offset, bytes);

	return 0;
}

static int nh_adm1266_nvmem_clear(void *priv, unsigned int offset, void *val,
				  size_t bytes)
{
	struct nh_adm1266_data *data = priv;
	int ret;
	struct i2c_msg msgs[1];
	u8 write_buf[4];

	if (data == NULL) {
		return -1;
	}

	write_buf[0] = ADM1266_READ_BLACKBOX; // 0xDE
	write_buf[1] = 2; // The number of bytes
	write_buf[2] = 0xFE; // Clear command
	write_buf[3] = 0x00; // Parameter

	msgs[0].addr = data->client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 4;
	msgs[0].buf = write_buf;

	ret = i2c_transfer(data->client->adapter, msgs, 1);
	if (ret != 1) {
		dev_err(&data->client->dev, "Failed to clear blackbox: %d\n",
			ret);
		ret = ret < 0 ? ret : -EIO;
	} else {
		dev_info(&data->client->dev, "Blackbox cleared successfully\n");
		ret = 0;
	}
	return ret;
}

static int nh_adm1266_config_nvmem(struct nh_adm1266_data *data)
{
	data->nvmem_config.name = dev_name(&data->client->dev);
	data->nvmem_config.dev = &data->client->dev;
	data->nvmem_config.root_only = true;
	data->nvmem_config.read_only = false; /* enable clear */
	data->nvmem_config.owner = THIS_MODULE;
	data->nvmem_config.reg_read = nh_adm1266_nvmem_read;
	data->nvmem_config.reg_write = nh_adm1266_nvmem_clear;
	data->nvmem_config.cells = nh_adm1266_nvmem_cells;
	data->nvmem_config.ncells = ARRAY_SIZE(nh_adm1266_nvmem_cells);
	data->nvmem_config.priv = data;
	data->nvmem_config.stride = 1;
	data->nvmem_config.word_size = 1;
	data->nvmem_config.size = nh_adm1266_nvmem_cells[0].bytes;

	data->dev_mem = devm_kzalloc(&data->client->dev, data->nvmem_config.size, GFP_KERNEL);
	if (!data->dev_mem)
		return -ENOMEM;

	data->nvmem = devm_nvmem_register(&data->client->dev, &data->nvmem_config);
	if (IS_ERR(data->nvmem)) {
		dev_err(&data->client->dev, "Could not register nvmem!");
		return PTR_ERR(data->nvmem);
	}

	return 0;
}

static int nh_adm1266_set_rtc(struct nh_adm1266_data *data, u64 sec, u64 nsec)
{
	char write_buf[6];
	u16 fraction;
	int i;

	memset(write_buf, 0, sizeof(write_buf));

	// Transform nanoseconds to 16-bit data (LSB of ADM1266 timestamp represents 1/2^16 second).
	fraction = (u16)((nsec * 65536) / 1000000000);

	if (sec > 0xFFFFFFFF) {
		dev_warn(&data->client->dev,
				 "sec=%llu is too large. ADM1266 uses 32-bit to store secs and will report incorrect timestamps.\n",
				 sec);
	}

	for (i = 0; i < 4; i++)
		write_buf[2 + i] = (sec >> (i * 8)) & 0xFF;
	for (i = 0; i < 2; i++)
		write_buf[i] = (fraction >> (i * 8)) & 0xFF;

	return i2c_smbus_write_block_data(data->client, ADM1266_SET_RTC, sizeof(write_buf), write_buf);
}

static int nh_adm1266_set_rtc_relative_to_epoch(struct nh_adm1266_data *data, u64 epoch_offset_sec)
{
	struct timespec64 now;
	u64 rtc_sec;
	int ret;

	ktime_get_real_ts64(&now);

	if (epoch_offset_sec > (u64)now.tv_sec) {
		dev_warn(&data->client->dev,
				 "User-provided epoch_offset_sec=%llu is in the future.\n",
				 epoch_offset_sec);
		return -EINVAL;
	}
	rtc_sec = (u64)now.tv_sec - epoch_offset_sec;

	ret = nh_adm1266_set_rtc(data, rtc_sec, now.tv_nsec);
	if (ret) {
		dev_err(&data->client->dev, "Failed to set RTC: ret=%d\n", ret);
		return ret;
	}

	data->rtc_epoch_offset = epoch_offset_sec;
	return 0;
}

/*
 * Returns a pointer to the nh_adm1266_data struct, given a device.
 *
 * nh_pmbus_do_probe() set i2c clientdata with pmbus_driver_info*.
 * Since pmbus_driver_info is stored in nh_adm1266_data, this function
 * uses the container_of() to get the nh_adm1266_data*.
 */
static struct nh_adm1266_data *to_nh_adm1266_data(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	const struct pmbus_driver_info *info = nh_pmbus_get_driver_info(client);
	return container_of(info, struct nh_adm1266_data, info);
}

static ssize_t rtc_epoch_offset_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct nh_adm1266_data *data = to_nh_adm1266_data(dev);
	u64 epoch_offset_sec;
	int ret;

	ret = kstrtoull(buf, 10, &epoch_offset_sec);
	if (ret) {
		dev_warn(dev, "Failed to convert '%s' to u64, ret=%d\n", buf, ret);
		return -EINVAL;
	}

	ret = nh_adm1266_set_rtc_relative_to_epoch(data, epoch_offset_sec);
	if (ret)
		return ret;

	return count;
}

static ssize_t rtc_epoch_offset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct nh_adm1266_data *data = to_nh_adm1266_data(dev);
	return sprintf(buf, "%llu\n", data->rtc_epoch_offset);
}

static DEVICE_ATTR_RW(rtc_epoch_offset);

static ssize_t powerup_counter_show(struct device *dev, struct device_attribute *attr, char *buf) {
	struct nh_adm1266_data *data = to_nh_adm1266_data(dev);
	u8 read_buf[3];
	int ret;
	u16 powerup_counter;

	ret = i2c_smbus_read_i2c_block_data(data->client, ADM1266_POWERUP_COUNTER, 3, read_buf);
	if (ret < 0)
		return ret;

	if (ret != 3)
		return -EIO;

	// Byte 0: Length of the powerup counter data.
	// Byte [2:1]: Powerup counter value (in little-endian format).
	powerup_counter = read_buf[1] | (read_buf[2] << 8);
	return sprintf(buf, "%u\n", powerup_counter);
}

static DEVICE_ATTR_RO(powerup_counter);

static ssize_t firmware_revision_show(struct device *dev, struct device_attribute *attr, char *buf) {
	struct nh_adm1266_data *data = to_nh_adm1266_data(dev);
	u8 read_buf[9];
	int ret;

	ret = i2c_smbus_read_i2c_block_data(data->client, ADM1266_IC_DEVICE_REV, 9, read_buf);
	if (ret < 0)
		return ret;

	if (ret != 9)
		return -EIO;

	// Byte 0: Length of the IC_DEVICE_REV data.
	// Byte [3:1]: Firmware revision in the format of "major.minor.patch".
	// Byte [6:4]: Bootloader revision in the format of "major.minor.patch".
	// Byte [8:7]: Chip revision in the format of 2 ASCII characters.
	return sprintf(buf, "%d.%d.%d\n", read_buf[1], read_buf[2], read_buf[3]);
}

static DEVICE_ATTR_RO(firmware_revision);

static ssize_t mfr_revision_show(struct device *dev, struct device_attribute *attr, char *buf) {
	struct nh_adm1266_data *data = to_nh_adm1266_data(dev);

	int ret;
	u8 mfr_revision_buf[8];
	u8 mfr_revision_length_to_read = sizeof(mfr_revision_buf);

	ret = nh_adm1266_i2c_atomic_write_then_read(
		data,
		ADM1266_MFR_REVISION,
		/*w_len=*/1,
		/*w_data=*/&mfr_revision_length_to_read,
		/*r_len=*/mfr_revision_length_to_read,
		/*r_data=*/mfr_revision_buf);
	if (ret) {
		dev_err(&data->client->dev, "Failed to read MFR_REVISION: ret=%d", ret);
		return ret;
	}

	// Print up to 8 ASCII characters or up to the first null character, whichever comes first.
	return sprintf(buf, "%.*s\n", mfr_revision_length_to_read, mfr_revision_buf);
}

static DEVICE_ATTR_RO(mfr_revision);

static int nh_adm1266_probe(struct i2c_client *client)
{
	struct nh_adm1266_data *data;
	int ret;
	int i;

	data = devm_kzalloc(&client->dev, sizeof(struct nh_adm1266_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->client = client;
	data->info.pages = 17;
	data->info.format[PSC_VOLTAGE_OUT] = linear;
	for (i = 0; i < data->info.pages; i++)
		data->info.func[i] = PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT;

	crc8_populate_msb(pmbus_crc_table, 0x7);
	mutex_init(&data->buf_mutex);

	ret = nh_adm1266_config_gpio(data);
	if (ret < 0)
		return ret;

	ret = nh_adm1266_set_rtc_relative_to_epoch(data, ADM1266_NH_CUSTOM_EPOCH_OFFSET);
	if (ret < 0)
		return ret;

	ret = nh_adm1266_config_nvmem(data);
	if (ret < 0)
		return ret;

	ret = nh_pmbus_do_probe(client, &data->info);
	if (ret)
		return ret;

	ret = device_create_file(&client->dev, &dev_attr_rtc_epoch_offset);
	if (ret) {
		dev_err(&client->dev, "Failed to create rtc_epoch_offset attribute: ret=%d\n", ret);
		return ret;
	}
	ret = device_create_file(&client->dev, &dev_attr_powerup_counter);
	if (ret) {
		dev_err(&client->dev, "Failed to create powerup_counter attribute: ret=%d\n", ret);
		return ret;
	}
	ret = device_create_file(&client->dev, &dev_attr_firmware_revision);
	if (ret) {
		dev_err(&client->dev, "Failed to create firmware_revision attribute: ret=%d\n", ret);
		return ret;
	}
	ret = device_create_file(&client->dev, &dev_attr_mfr_revision);
	if (ret) {
		dev_err(&client->dev, "Failed to create mfr_revision attribute: ret=%d\n", ret);
		return ret;
	}

	nh_adm1266_init_debugfs(data);

	return 0;
}

static void nh_adm1266_remove(struct i2c_client *client)
{
	device_remove_file(&client->dev, &dev_attr_mfr_revision);
	device_remove_file(&client->dev, &dev_attr_firmware_revision);
	device_remove_file(&client->dev, &dev_attr_rtc_epoch_offset);
	device_remove_file(&client->dev, &dev_attr_powerup_counter);
}

static const struct of_device_id nh_adm1266_of_match[] = {
	{ .compatible = "adi,nh_adm1266" },
	{ }
};
MODULE_DEVICE_TABLE(of, nh_adm1266_of_match);

static const struct i2c_device_id nh_adm1266_id[] = {
	{ "nh_adm1266" },
	{ }
};
MODULE_DEVICE_TABLE(i2c, nh_adm1266_id);

static struct i2c_driver nh_adm1266_driver = {
	.driver = {
		   .name = "nh_adm1266",
		   .of_match_table = nh_adm1266_of_match,
		  },
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
	.probe_new = nh_adm1266_probe,
#else
	.probe = nh_adm1266_probe,
#endif
	.remove = nh_adm1266_remove,
	.id_table = nh_adm1266_id,
};

module_i2c_driver(nh_adm1266_driver);

MODULE_AUTHOR("Alexandru Tachici <alexandru.tachici@analog.com>");
MODULE_DESCRIPTION("PMBus driver for Analog Devices ADM1266");
MODULE_LICENSE("GPL v2");
MODULE_IMPORT_NS(PMBUS);

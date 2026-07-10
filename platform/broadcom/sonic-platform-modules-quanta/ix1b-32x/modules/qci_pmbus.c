/*
 * QUANTA Generic PMBUS driver
 *
 *
 * Based on generic pmbus driver and ltc2978 driver
 *
 * Author: Chih-Pei Chang <Chih-Pei.Chang@qct.io>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>

enum projects { ly8, ix1, ix2, ix1b };

#define DELAY_TIME		1000	/* uS	*/

/* Pmbus reg defines are copied from drivers/hwmon/pmbus/pmbus.h*/
/*
 * Registers
 */
enum pmbus_regs {
	PMBUS_PAGE			= 0x00,
	PMBUS_OPERATION			= 0x01,
	PMBUS_ON_OFF_CONFIG		= 0x02,
	PMBUS_CLEAR_FAULTS		= 0x03,
	PMBUS_PHASE			= 0x04,

	PMBUS_CAPABILITY		= 0x19,
	PMBUS_QUERY			= 0x1A,

	PMBUS_VOUT_MODE			= 0x20,
	PMBUS_VOUT_COMMAND		= 0x21,
	PMBUS_VOUT_TRIM			= 0x22,
	PMBUS_VOUT_CAL_OFFSET		= 0x23,
	PMBUS_VOUT_MAX			= 0x24,
	PMBUS_VOUT_MARGIN_HIGH		= 0x25,
	PMBUS_VOUT_MARGIN_LOW		= 0x26,
	PMBUS_VOUT_TRANSITION_RATE	= 0x27,
	PMBUS_VOUT_DROOP		= 0x28,
	PMBUS_VOUT_SCALE_LOOP		= 0x29,
	PMBUS_VOUT_SCALE_MONITOR	= 0x2A,

	PMBUS_COEFFICIENTS		= 0x30,
	PMBUS_POUT_MAX			= 0x31,

	PMBUS_FAN_CONFIG_12		= 0x3A,
	PMBUS_FAN_COMMAND_1		= 0x3B,
	PMBUS_FAN_COMMAND_2		= 0x3C,
	PMBUS_FAN_CONFIG_34		= 0x3D,
	PMBUS_FAN_COMMAND_3		= 0x3E,
	PMBUS_FAN_COMMAND_4		= 0x3F,

	PMBUS_VOUT_OV_FAULT_LIMIT	= 0x40,
	PMBUS_VOUT_OV_FAULT_RESPONSE	= 0x41,
	PMBUS_VOUT_OV_WARN_LIMIT	= 0x42,
	PMBUS_VOUT_UV_WARN_LIMIT	= 0x43,
	PMBUS_VOUT_UV_FAULT_LIMIT	= 0x44,
	PMBUS_VOUT_UV_FAULT_RESPONSE	= 0x45,
	PMBUS_IOUT_OC_FAULT_LIMIT	= 0x46,
	PMBUS_IOUT_OC_FAULT_RESPONSE	= 0x47,
	PMBUS_IOUT_OC_LV_FAULT_LIMIT	= 0x48,
	PMBUS_IOUT_OC_LV_FAULT_RESPONSE	= 0x49,
	PMBUS_IOUT_OC_WARN_LIMIT	= 0x4A,
	PMBUS_IOUT_UC_FAULT_LIMIT	= 0x4B,
	PMBUS_IOUT_UC_FAULT_RESPONSE	= 0x4C,

	PMBUS_OT_FAULT_LIMIT		= 0x4F,
	PMBUS_OT_FAULT_RESPONSE		= 0x50,
	PMBUS_OT_WARN_LIMIT		= 0x51,
	PMBUS_UT_WARN_LIMIT		= 0x52,
	PMBUS_UT_FAULT_LIMIT		= 0x53,
	PMBUS_UT_FAULT_RESPONSE		= 0x54,
	PMBUS_VIN_OV_FAULT_LIMIT	= 0x55,
	PMBUS_VIN_OV_FAULT_RESPONSE	= 0x56,
	PMBUS_VIN_OV_WARN_LIMIT		= 0x57,
	PMBUS_VIN_UV_WARN_LIMIT		= 0x58,
	PMBUS_VIN_UV_FAULT_LIMIT	= 0x59,

	PMBUS_IIN_OC_FAULT_LIMIT	= 0x5B,
	PMBUS_IIN_OC_WARN_LIMIT		= 0x5D,

	PMBUS_POUT_OP_FAULT_LIMIT	= 0x68,
	PMBUS_POUT_OP_WARN_LIMIT	= 0x6A,
	PMBUS_PIN_OP_WARN_LIMIT		= 0x6B,

	PMBUS_STATUS_BYTE		= 0x78,
	PMBUS_STATUS_WORD		= 0x79,
	PMBUS_STATUS_VOUT		= 0x7A,
	PMBUS_STATUS_IOUT		= 0x7B,
	PMBUS_STATUS_INPUT		= 0x7C,
	PMBUS_STATUS_TEMPERATURE	= 0x7D,
	PMBUS_STATUS_CML		= 0x7E,
	PMBUS_STATUS_OTHER		= 0x7F,
	PMBUS_STATUS_MFR_SPECIFIC	= 0x80,
	PMBUS_STATUS_FAN_12		= 0x81,
	PMBUS_STATUS_FAN_34		= 0x82,

	PMBUS_READ_VIN			= 0x88,
	PMBUS_READ_IIN			= 0x89,
	PMBUS_READ_VCAP			= 0x8A,
	PMBUS_READ_VOUT			= 0x8B,
	PMBUS_READ_IOUT			= 0x8C,
	PMBUS_READ_TEMPERATURE_1	= 0x8D,
	PMBUS_READ_TEMPERATURE_2	= 0x8E,
	PMBUS_READ_TEMPERATURE_3	= 0x8F,
	PMBUS_READ_FAN_SPEED_1		= 0x90,
	PMBUS_READ_FAN_SPEED_2		= 0x91,
	PMBUS_READ_FAN_SPEED_3		= 0x92,
	PMBUS_READ_FAN_SPEED_4		= 0x93,
	PMBUS_READ_DUTY_CYCLE		= 0x94,
	PMBUS_READ_FREQUENCY		= 0x95,
	PMBUS_READ_POUT			= 0x96,
	PMBUS_READ_PIN			= 0x97,

	PMBUS_REVISION			= 0x98,
	PMBUS_MFR_ID			= 0x99,
	PMBUS_MFR_MODEL			= 0x9A,
	PMBUS_MFR_REVISION		= 0x9B,
	PMBUS_MFR_LOCATION		= 0x9C,
	PMBUS_MFR_DATE			= 0x9D,
	PMBUS_MFR_SERIAL		= 0x9E,

/*
 * Virtual registers.
 * Useful to support attributes which are not supported by standard PMBus
 * registers but exist as manufacturer specific registers on individual chips.
 * Must be mapped to real registers in device specific code.
 *
 * Semantics:
 * Virtual registers are all word size.
 * READ registers are read-only; writes are either ignored or return an error.
 * RESET registers are read/write. Reading reset registers returns zero
 * (used for detection), writing any value causes the associated history to be
 * reset.
 * Virtual registers have to be handled in device specific driver code. Chip
 * driver code returns non-negative register values if a virtual register is
 * supported, or a negative error code if not. The chip driver may return
 * -ENODATA or any other error code in this case, though an error code other
 * than -ENODATA is handled more efficiently and thus preferred. Either case,
 * the calling PMBus core code will abort if the chip driver returns an error
 * code when reading or writing virtual registers.
 */
	PMBUS_VIRT_BASE			= 0x100,
	PMBUS_VIRT_READ_TEMP_AVG,
	PMBUS_VIRT_READ_TEMP_MIN,
	PMBUS_VIRT_READ_TEMP_MAX,
	PMBUS_VIRT_RESET_TEMP_HISTORY,
	PMBUS_VIRT_READ_VIN_AVG,
	PMBUS_VIRT_READ_VIN_MIN,
	PMBUS_VIRT_READ_VIN_MAX,
	PMBUS_VIRT_RESET_VIN_HISTORY,
	PMBUS_VIRT_READ_IIN_AVG,
	PMBUS_VIRT_READ_IIN_MIN,
	PMBUS_VIRT_READ_IIN_MAX,
	PMBUS_VIRT_RESET_IIN_HISTORY,
	PMBUS_VIRT_READ_PIN_AVG,
	PMBUS_VIRT_READ_PIN_MIN,
	PMBUS_VIRT_READ_PIN_MAX,
	PMBUS_VIRT_RESET_PIN_HISTORY,
	PMBUS_VIRT_READ_POUT_AVG,
	PMBUS_VIRT_READ_POUT_MIN,
	PMBUS_VIRT_READ_POUT_MAX,
	PMBUS_VIRT_RESET_POUT_HISTORY,
	PMBUS_VIRT_READ_VOUT_AVG,
	PMBUS_VIRT_READ_VOUT_MIN,
	PMBUS_VIRT_READ_VOUT_MAX,
	PMBUS_VIRT_RESET_VOUT_HISTORY,
	PMBUS_VIRT_READ_IOUT_AVG,
	PMBUS_VIRT_READ_IOUT_MIN,
	PMBUS_VIRT_READ_IOUT_MAX,
	PMBUS_VIRT_RESET_IOUT_HISTORY,
	PMBUS_VIRT_READ_TEMP2_AVG,
	PMBUS_VIRT_READ_TEMP2_MIN,
	PMBUS_VIRT_READ_TEMP2_MAX,
	PMBUS_VIRT_RESET_TEMP2_HISTORY,

	PMBUS_VIRT_READ_VMON,
	PMBUS_VIRT_VMON_UV_WARN_LIMIT,
	PMBUS_VIRT_VMON_OV_WARN_LIMIT,
	PMBUS_VIRT_VMON_UV_FAULT_LIMIT,
	PMBUS_VIRT_VMON_OV_FAULT_LIMIT,
	PMBUS_VIRT_STATUS_VMON,
};

enum pmbus_sensor_classes {
	PSC_VOLTAGE_IN = 0,
	PSC_VOLTAGE_OUT,
	PSC_CURRENT_IN,
	PSC_CURRENT_OUT,
	PSC_POWER,
	PSC_TEMPERATURE,
	PSC_FAN,
	PSC_NUM_CLASSES		/* Number of power sensor classes */
};

#define PMBUS_PAGES	32	/* Per PMBus specification */

/* Functionality bit mask */
#define PMBUS_HAVE_VIN		BIT(0)
#define PMBUS_HAVE_VCAP		BIT(1)
#define PMBUS_HAVE_VOUT		BIT(2)
#define PMBUS_HAVE_IIN		BIT(3)
#define PMBUS_HAVE_IOUT		BIT(4)
#define PMBUS_HAVE_PIN		BIT(5)
#define PMBUS_HAVE_POUT		BIT(6)
#define PMBUS_HAVE_FAN12	BIT(7)
#define PMBUS_HAVE_FAN34	BIT(8)
#define PMBUS_HAVE_TEMP		BIT(9)
#define PMBUS_HAVE_TEMP2	BIT(10)
#define PMBUS_HAVE_TEMP3	BIT(11)
#define PMBUS_HAVE_STATUS_VOUT	BIT(12)
#define PMBUS_HAVE_STATUS_IOUT	BIT(13)
#define PMBUS_HAVE_STATUS_INPUT	BIT(14)
#define PMBUS_HAVE_STATUS_TEMP	BIT(15)
#define PMBUS_HAVE_STATUS_FAN12	BIT(16)
#define PMBUS_HAVE_STATUS_FAN34	BIT(17)
#define PMBUS_HAVE_VMON		BIT(18)
#define PMBUS_HAVE_STATUS_VMON	BIT(19)

enum pmbus_data_format { linear = 0, direct, vid };
enum vrm_version { vr11 = 0, vr12, vr13 };

struct pmbus_driver_info {
	int pages;		/* Total number of pages */
	enum pmbus_data_format format[PSC_NUM_CLASSES];
	enum vrm_version vrm_version;
	/*
	 * Support one set of coefficients for each sensor type
	 * Used for chips providing data in direct mode.
	 */
	int m[PSC_NUM_CLASSES];	/* mantissa for direct data format */
	int b[PSC_NUM_CLASSES];	/* offset */
	int R[PSC_NUM_CLASSES];	/* exponent */

	u32 func[PMBUS_PAGES];	/* Functionality, per page */
	/*
	 * The following functions map manufacturing specific register values
	 * to PMBus standard register values. Specify only if mapping is
	 * necessary.
	 * Functions return the register value (read) or zero (write) if
	 * successful. A return value of -ENODATA indicates that there is no
	 * manufacturer specific register, but that a standard PMBus register
	 * may exist. Any other negative return value indicates that the
	 * register does not exist, and that no attempt should be made to read
	 * the standard register.
	 */
	int (*read_byte_data)(struct i2c_client *client, int page, int reg);
	int (*read_word_data)(struct i2c_client *client, int page, int reg);
	int (*write_word_data)(struct i2c_client *client, int page, int reg,
			       u16 word);
	int (*write_byte)(struct i2c_client *client, int page, u8 value);
	/*
	 * The identify function determines supported PMBus functionality.
	 * This function is only necessary if a chip driver supports multiple
	 * chips, and the chip functionality is not pre-determined.
	 */
	int (*identify)(struct i2c_client *client,
			struct pmbus_driver_info *info);

	/* Regulator functionality, if supported by this chip driver. */
	int num_regulators;
	const struct regulator_desc *reg_desc;
};

extern int pmbus_set_page(struct i2c_client *client, u8 page);
extern int pmbus_read_byte_data(struct i2c_client *client, int page, u8 reg);
extern bool pmbus_check_byte_register(struct i2c_client *client, int page, int reg);
extern bool pmbus_check_word_register(struct i2c_client *client, int page, int reg);
extern int pmbus_do_probe(struct i2c_client *client, const struct i2c_device_id *id,
		   struct pmbus_driver_info *info);
extern int pmbus_do_remove(struct i2c_client *client);

/* Needed to access the mutex. Copied from pmbus_core.c */
#define PB_STATUS_BASE		0
#define PB_STATUS_VOUT_BASE	(PB_STATUS_BASE + PMBUS_PAGES)
#define PB_STATUS_IOUT_BASE	(PB_STATUS_VOUT_BASE + PMBUS_PAGES)
#define PB_STATUS_FAN_BASE	(PB_STATUS_IOUT_BASE + PMBUS_PAGES)
#define PB_STATUS_FAN34_BASE	(PB_STATUS_FAN_BASE + PMBUS_PAGES)
#define PB_STATUS_TEMP_BASE	(PB_STATUS_FAN34_BASE + PMBUS_PAGES)
#define PB_STATUS_INPUT_BASE	(PB_STATUS_TEMP_BASE + PMBUS_PAGES)
#define PB_STATUS_VMON_BASE	(PB_STATUS_INPUT_BASE + 1)
#define PB_NUM_STATUS_REG	(PB_STATUS_VMON_BASE + 1)
struct pmbus_data {
	struct device *dev;
	struct device *hwmon_dev;

	u32 flags;		/* from platform data */

	int exponent[PMBUS_PAGES];
				/* linear mode: exponent for output voltages */

	const struct pmbus_driver_info *info;

	int max_attributes;
	int num_attributes;
	struct attribute_group group;
	const struct attribute_group *groups[2];

	struct pmbus_sensor *sensors;

	struct mutex update_lock;
	bool valid;
	unsigned long last_updated;	/* in jiffies */

	/*
	 * A single status register covers multiple attributes,
	 * so we keep them all together.
	 */
	u8 status[PB_NUM_STATUS_REG];
	u8 status_register;

	u8 currpage;
};

static int qci_pmbus_read_block(struct i2c_client *client, u8 command, int data_len, u8 *data)
{
    int result = 0;
    int retry_count = 3;

    while (retry_count) {
        retry_count--;

        result = i2c_smbus_read_i2c_block_data(client, command, data_len, data);

        if (result < 0) {
            msleep(10);
            continue;
        }

        result = 0;
        break;
    }

    return result;
}

static ssize_t qci_pmbus_show_mfr_id(struct device *dev,
                struct device_attribute *da, char *buf)
{
    int ret, len;
    u8 block_buffer[I2C_SMBUS_BLOCK_MAX + 1], *str;
    struct i2c_client *client = container_of(dev, struct i2c_client, dev);

    ret = qci_pmbus_read_block(client, PMBUS_MFR_ID, I2C_SMBUS_BLOCK_MAX, block_buffer);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to read Manufacturer ID\n");
        return ret;
    }
    len = block_buffer[0];
    block_buffer[(len+1)] = '\0';
    str = &(block_buffer[1]);

    return snprintf(buf, PAGE_SIZE, "%s\n", str);
}

static ssize_t qci_pmbus_show_mfr_model(struct device *dev,
                struct device_attribute *da, char *buf)
{
    int ret, len;
    u8 block_buffer[I2C_SMBUS_BLOCK_MAX + 1], *str;
    struct i2c_client *client = container_of(dev, struct i2c_client, dev);

    ret = qci_pmbus_read_block(client, PMBUS_MFR_MODEL, I2C_SMBUS_BLOCK_MAX, block_buffer);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to read Manufacturer Model\n");
        return ret;
    }
    len = block_buffer[0];
    block_buffer[(len+1)] = '\0';
    str = &(block_buffer[1]);

    return snprintf(buf, PAGE_SIZE, "%s\n", str);
}

static ssize_t qci_pmbus_show_mfr_revision(struct device *dev,
                struct device_attribute *da, char *buf)
{
    int ret, len;
    u8 block_buffer[I2C_SMBUS_BLOCK_MAX + 1], *str;
    struct i2c_client *client = container_of(dev, struct i2c_client, dev);

    ret = qci_pmbus_read_block(client, PMBUS_MFR_REVISION, I2C_SMBUS_BLOCK_MAX, block_buffer);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to read Manufacturer Revision\n");
        return ret;
    }
    len = block_buffer[0];
    block_buffer[(len+1)] = '\0';
    str = &(block_buffer[1]);

    return snprintf(buf, PAGE_SIZE, "%s\n", str);
}

static ssize_t qci_pmbus_show_mfr_location(struct device *dev,
                struct device_attribute *da, char *buf)
{
    int ret, len;
    u8 block_buffer[I2C_SMBUS_BLOCK_MAX + 1], *str;
    struct i2c_client *client = container_of(dev, struct i2c_client, dev);

    ret = qci_pmbus_read_block(client, PMBUS_MFR_LOCATION, I2C_SMBUS_BLOCK_MAX, block_buffer);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to read Manufacture Location\n");
        return ret;
    }
    len = block_buffer[0];
    block_buffer[(len+1)] = '\0';
    str = &(block_buffer[1]);

    return snprintf(buf, PAGE_SIZE, "%s\n", str);
}

static ssize_t qci_pmbus_show_mfr_serial(struct device *dev,
                struct device_attribute *da, char *buf)
{
    int ret, len;
    u8 block_buffer[I2C_SMBUS_BLOCK_MAX + 1], *str;
    struct i2c_client *client = container_of(dev, struct i2c_client, dev);

    ret = qci_pmbus_read_block(client, PMBUS_MFR_SERIAL, I2C_SMBUS_BLOCK_MAX, block_buffer);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to read Manufacturer Serial\n");
        return ret;
    }
    len = block_buffer[0];
    block_buffer[(len+1)] = '\0';
    str = &(block_buffer[1]);

    return snprintf(buf, PAGE_SIZE, "%s\n", str);
}


static DEVICE_ATTR(mfr_id, S_IRUGO, qci_pmbus_show_mfr_id, NULL);
static DEVICE_ATTR(mfr_model, S_IRUGO, qci_pmbus_show_mfr_model, NULL);
static DEVICE_ATTR(mfr_revision, S_IRUGO, qci_pmbus_show_mfr_revision, NULL);
static DEVICE_ATTR(mfr_location, S_IRUGO, qci_pmbus_show_mfr_location, NULL);
static DEVICE_ATTR(mfr_serial, S_IRUGO, qci_pmbus_show_mfr_serial, NULL);


static struct attribute *qci_pmbus_inventory_attrs[] = {
    &dev_attr_mfr_id.attr,
    &dev_attr_mfr_model.attr,
    &dev_attr_mfr_revision.attr,
    &dev_attr_mfr_location.attr,
    &dev_attr_mfr_serial.attr,
    NULL
};

static struct attribute_group qci_pmbus_inventory_attr_grp = {
    .attrs = qci_pmbus_inventory_attrs
};

/* FIXME: add project specific id here */
static const struct i2c_device_id qci_pmbus_id[] = {
    {"qci_pmbus_ly8", ly8},
    {"qci_pmbus_ix1", ix1},
    {"qci_pmbus_ix2", ix2},
    {"qci_pmbus_ix1b", ix1b},
    {}
};
MODULE_DEVICE_TABLE(i2c, qci_pmbus_id);

/*
 * Find sensor groups and status registers on each page.
 */
static void qci_pmbus_find_sensor_groups(struct i2c_client *client,
				     struct pmbus_driver_info *info)
{
	int page;

	/* Sensors detected on page 0 only */
	if (pmbus_check_word_register(client, 0, PMBUS_READ_VIN))
		info->func[0] |= PMBUS_HAVE_VIN;
	if (pmbus_check_word_register(client, 0, PMBUS_READ_VCAP))
		info->func[0] |= PMBUS_HAVE_VCAP;
	if (pmbus_check_word_register(client, 0, PMBUS_READ_IIN))
		info->func[0] |= PMBUS_HAVE_IIN;
	if (pmbus_check_word_register(client, 0, PMBUS_READ_PIN))
		info->func[0] |= PMBUS_HAVE_PIN;
	if (info->func[0]
	    && pmbus_check_byte_register(client, 0, PMBUS_STATUS_INPUT))
		info->func[0] |= PMBUS_HAVE_STATUS_INPUT;
	if (pmbus_check_byte_register(client, 0, PMBUS_FAN_CONFIG_12) &&
	    pmbus_check_word_register(client, 0, PMBUS_READ_FAN_SPEED_1)) {
		info->func[0] |= PMBUS_HAVE_FAN12;
		if (pmbus_check_byte_register(client, 0, PMBUS_STATUS_FAN_12))
			info->func[0] |= PMBUS_HAVE_STATUS_FAN12;
	}
	if (pmbus_check_byte_register(client, 0, PMBUS_FAN_CONFIG_34) &&
	    pmbus_check_word_register(client, 0, PMBUS_READ_FAN_SPEED_3)) {
		info->func[0] |= PMBUS_HAVE_FAN34;
		if (pmbus_check_byte_register(client, 0, PMBUS_STATUS_FAN_34))
			info->func[0] |= PMBUS_HAVE_STATUS_FAN34;
	}
	if (pmbus_check_word_register(client, 0, PMBUS_READ_TEMPERATURE_1))
		info->func[0] |= PMBUS_HAVE_TEMP;
	if (pmbus_check_word_register(client, 0, PMBUS_READ_TEMPERATURE_2))
		info->func[0] |= PMBUS_HAVE_TEMP2;
	if (pmbus_check_word_register(client, 0, PMBUS_READ_TEMPERATURE_3))
		info->func[0] |= PMBUS_HAVE_TEMP3;
	if (info->func[0] & (PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2
			     | PMBUS_HAVE_TEMP3)
	    && pmbus_check_byte_register(client, 0,
					 PMBUS_STATUS_TEMPERATURE))
			info->func[0] |= PMBUS_HAVE_STATUS_TEMP;

	/* Sensors detected on all pages */
	for (page = 0; page < info->pages; page++) {
		if (pmbus_check_word_register(client, page, PMBUS_READ_VOUT)) {
			info->func[page] |= PMBUS_HAVE_VOUT;
			if (pmbus_check_byte_register(client, page,
						      PMBUS_STATUS_VOUT))
				info->func[page] |= PMBUS_HAVE_STATUS_VOUT;
		}
		if (pmbus_check_word_register(client, page, PMBUS_READ_IOUT)) {
			info->func[page] |= PMBUS_HAVE_IOUT;
			if (pmbus_check_byte_register(client, 0,
						      PMBUS_STATUS_IOUT))
				info->func[page] |= PMBUS_HAVE_STATUS_IOUT;
		}
		if (pmbus_check_word_register(client, page, PMBUS_READ_POUT))
			info->func[page] |= PMBUS_HAVE_POUT;
	}
}

/*
 * Identify chip parameters.
 */
static int qci_pmbus_identify(struct i2c_client *client,
			  struct pmbus_driver_info *info)
{
	int ret = 0;

	if (!info->pages) {
		/*
		 * Check if the PAGE command is supported. If it is,
		 * keep setting the page number until it fails or until the
		 * maximum number of pages has been reached. Assume that
		 * this is the number of pages supported by the chip.
		 */
		if (pmbus_check_byte_register(client, 0, PMBUS_PAGE)) {
			int page;

			for (page = 1; page < PMBUS_PAGES; page++) {
				if (pmbus_set_page(client, page) < 0)
					break;
			}
			pmbus_set_page(client, 0);
			info->pages = page;
		} else {
			info->pages = 1;
		}
	}

	if (pmbus_check_byte_register(client, 0, PMBUS_VOUT_MODE)) {
		int vout_mode;

		vout_mode = pmbus_read_byte_data(client, 0, PMBUS_VOUT_MODE);
		if (vout_mode >= 0 && vout_mode != 0xff) {
			switch (vout_mode >> 5) {
			case 0:
				break;
			case 1:
				info->format[PSC_VOLTAGE_OUT] = vid;
				info->vrm_version = vr11;
				break;
			case 2:
				info->format[PSC_VOLTAGE_OUT] = direct;
				break;
			default:
				ret = -ENODEV;
				goto abort;
			}
		}
	}

	/*
	 * We should check if the COEFFICIENTS register is supported.
	 * If it is, and the chip is configured for direct mode, we can read
	 * the coefficients from the chip, one set per group of sensor
	 * registers.
	 *
	 * To do this, we will need access to a chip which actually supports the
	 * COEFFICIENTS command, since the command is too complex to implement
	 * without testing it. Until then, abort if a chip configured for direct
	 * mode was detected.
	 */
	if (info->format[PSC_VOLTAGE_OUT] == direct) {
		ret = -ENODEV;
		goto abort;
	}

	/* if no function pre-defined, try to find sensor groups  */
	if (info->func[0] == 0) qci_pmbus_find_sensor_groups(client, info);
abort:
	return ret;
}

int qci_pmbus_set_page(struct i2c_client *client, u8 page)
{
	struct pmbus_data *data = i2c_get_clientdata(client);
	int rv = 0;
	int newpage;

	if (page != data->currpage) {
		rv = i2c_smbus_write_byte_data(client, PMBUS_PAGE, page);
		udelay(DELAY_TIME);
		newpage = i2c_smbus_read_byte_data(client, PMBUS_PAGE);
		if (newpage != page)
			rv = -EIO;
		else
			data->currpage = page;
	}
	return rv;
}

int qci_write_byte(struct i2c_client *client, int page, u8 value)
{
	int rv;

	if (page >= 0) {
		rv = qci_pmbus_set_page(client, page);
		if (rv < 0)
			return rv;
	}

	rv = i2c_smbus_write_byte(client, value);
	udelay(DELAY_TIME);
	return rv;
}

int qci_write_word_data(struct i2c_client *client, int page, int reg, u16 word)
{
	int rv;

	rv = qci_pmbus_set_page(client, page);
	if (rv < 0)
		return rv;

	rv = i2c_smbus_write_word_data(client, reg, word);
	udelay(DELAY_TIME);
	return rv;
}

static int qci_pmbus_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct pmbus_driver_info *info;
	int ret;

	dev_info(dev, "qci_pmbus_probe\n");

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_READ_WORD_DATA))
		return -ENODEV;

	info = devm_kzalloc(dev, sizeof(struct pmbus_driver_info), GFP_KERNEL);

	if (!info)
		return -ENOMEM;

	info->func[0] = 0;

	/* FIXME: add project specific function table here */
	switch (id->driver_data) {
	case ly8:
		info->pages = 1;
		info->func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT
		| PMBUS_HAVE_FAN12 | PMBUS_HAVE_STATUS_FAN12
		| PMBUS_HAVE_VOUT  | PMBUS_HAVE_STATUS_VOUT
		| PMBUS_HAVE_IOUT  | PMBUS_HAVE_STATUS_IOUT
		;
		break;
	case ix1:
	case ix2:
	case ix1b:
		info->pages = 1;
		info->func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_IIN
		  | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT
		  | PMBUS_HAVE_FAN12 | PMBUS_HAVE_STATUS_FAN12
		  | PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2
		  | PMBUS_HAVE_TEMP3 | PMBUS_HAVE_STATUS_TEMP
		  | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
		  | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT
		  | PMBUS_HAVE_POUT
		  ;
		break;
	default:
		break;
	}
	info->write_word_data = qci_write_word_data;
	info->write_byte = qci_write_byte;
	info->identify = qci_pmbus_identify;	/* FIXME: reserve for future use */

	/* Register sysfs hooks */
	ret = sysfs_create_group(&dev->kobj, &qci_pmbus_inventory_attr_grp);
	if (ret) {
		dev_err(dev, "Failed to create sysfs entries\n");
		return -1;
	}

	return pmbus_do_probe(client, id, info);
}

/* This is the driver that will be inserted */
static struct i2c_driver qci_pmbus_driver = {
	.driver = {
		   .name = "qci-pmbus",
		   },
	.probe = qci_pmbus_probe,
	.remove = pmbus_do_remove,
	.id_table = qci_pmbus_id,
};

module_i2c_driver(qci_pmbus_driver);


MODULE_AUTHOR("Quanta Computer Inc.");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("QUANTA generic PMBus driver");
MODULE_LICENSE("GPL");

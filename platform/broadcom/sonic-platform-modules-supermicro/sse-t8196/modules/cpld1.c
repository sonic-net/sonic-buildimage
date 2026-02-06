#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/watchdog.h>

static int i2c_bus = 2;
static int i2c_addr = 0x2b;

module_param(i2c_bus, int, S_IRUGO);
MODULE_PARM_DESC(i2c_bus, "I2C bus number");

module_param(i2c_addr, int, S_IRUGO);
MODULE_PARM_DESC(i2c_addr, "I2C device address");


/* Refer to SSE_T8196 HW SPEC for more details */
/* The Switch Model ID of this project is 0x02 */
#define REG_MODEL_ID            0x00
#define REG_BOARD_ID            0x01
/* CPLD_ID */
#define REG_CPLD_ID             0x02
/* CPLD version */
#define REG_CPLD_VER            0x03
#define REG_DEV_RST_CTRL        0x06
#define BITPOS_WDT_CLR          0
#define BITPOS_WDT_EN           3
#define BITPOS_RSTN_SWITCH      1
#define BITPOS_SWITCH_PWRDOWN_N 2

/*
   OSFP interrupt and mask
   bit 0 and  bit 1 for OSFP2
   bit 2 and  bit 3 for OSFP1
 */
#define REG_OSFP_INT_1         0x1a
#define REG_OSFP_INT_MASK_1    0x1b

#define REG_QSFP_INT_2         0x1e
#define REG_QSFP_INT_MASK_2    0x1f
#define REG_QSFP_INT_3         0x1c
#define REG_QSFP_INT_MASK_3    0x1d

/* SFP Interrupt and mask  */
#define REG_SFP_INT            0x26
#define REG_SFP_INT_MASK       0x27


/* Watchdog */
#define REG_WDT_MAXL 0x32
#define REG_WDT_MAXM 0x33
#define REG_WDT_CNTL 0x34
#define REG_WDT_CNTM 0x35
#define REG_WDT_REC 0x40
#define BITPOS_WDT_REC 4
#define BITPOS_WDT_REC_CFG 4
/* CPLD code released date --- Month */
#define REG_CPLD_MOT 0xFE
/* CPLD code released date --- Day */
#define REG_CPLD_DAY 0xFF

enum PORT_TYPE { NONE, OSFP,QSFP,  SFP };

struct cpld_data {
	struct cdev cdev;
	struct mutex lock;
	struct device *cpld_device;
};

struct cpld_port_data {
	struct cpld_data *parent;
	int port_id;
};

static int cpld_major;
static struct class *cpld_class = NULL;
static struct cpld_data *cpld_dev_data;

/* used to access CPLD1 registers not defined in this driver with sys filesystem */
static uint8_t testee_offset = 0;

static int read_cpld_register(u8 reg_addr, uint8_t *value)
{
	struct i2c_adapter *adapter;
	struct i2c_client client;
	int ret;

	adapter = i2c_get_adapter(i2c_bus);
	if (!adapter) {
		return -ENODEV;
	}
	client.adapter = adapter;
	client.addr = i2c_addr;
	client.flags = 0;

	ret = i2c_smbus_read_byte_data(&client, reg_addr);
	if (ret >= 0) {
		*value = ret;
	}
	i2c_put_adapter(adapter);
	return ret >= 0 ? 0 : ret;
}

static int write_cpld_register(u8 reg_addr, uint8_t value)
{
	struct i2c_adapter *adapter;
	struct i2c_client client;
	int ret;

	adapter = i2c_get_adapter(i2c_bus);
	if (!adapter) {
		return -ENODEV;
	}

	client.adapter = adapter;
	client.addr = i2c_addr;
	client.flags = 0;

	ret = i2c_smbus_write_byte_data(&client, reg_addr, value);

	i2c_put_adapter(adapter);
	return ret;
}

/* CPLD1 attributes */
static ssize_t board_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	ret = read_cpld_register(REG_BOARD_ID, &value);
	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%X\n", value);
}

struct device_attribute dev_attr_board_id = __ATTR(board_id, 0400, board_id_show, NULL);

static ssize_t cpld_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	ret = read_cpld_register(REG_CPLD_ID, &value);
	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%X\n", value);
}
struct device_attribute dev_attr_cpld_id = __ATTR(cpld_id, 0400, cpld_id_show, NULL);

static ssize_t cpld_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t cpld_version;
	int ret;

	ret = read_cpld_register(REG_CPLD_VER, &cpld_version);
	if (ret < 0)
		return ret;

	return sprintf(buf, "%02X\n", cpld_version);
}

struct device_attribute dev_attr_cpld_version = __ATTR(cpld_version, 0400, cpld_version_show, NULL);


// GPIO  expander interrupt
static ssize_t gpio_expander_interrupt_1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	ret = read_cpld_register(REG_OSFP_INT_1, &value);
	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}
struct device_attribute dev_attr_gpio_expander_interrupt_1 = __ATTR(gpio_expander_interrupt_1, 0400, gpio_expander_interrupt_1_show, NULL);



// GPIO  expander interrupt
static ssize_t gpio_expander_interrupt_2_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	ret = read_cpld_register(REG_QSFP_INT_2, &value);
	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}
struct device_attribute dev_attr_gpio_expander_interrupt_2 = __ATTR(gpio_expander_interrupt_2, 0400, gpio_expander_interrupt_2_show, NULL);


// GPIO  expander interrupt/reset
static ssize_t gpio_expander_interrupt_3_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	ret = read_cpld_register(REG_QSFP_INT_3, &value);
	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}
struct device_attribute dev_attr_gpio_expander_interrupt_3 = __ATTR(gpio_expander_interrupt_3, 0400, gpio_expander_interrupt_3_show, NULL);


// GPIO  expander interrupt/reset
static ssize_t gpio_expander_interrupt_sfp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	ret = read_cpld_register(REG_SFP_INT, &value);
	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}
struct device_attribute dev_attr_gpio_expander_interrupt_sfp = __ATTR(gpio_expander_interrupt_sfp, 0400, gpio_expander_interrupt_sfp_show, NULL);
/********************************************************************************************************************************************************/
// GPIO  expander interrupt mask
static ssize_t gpio_expander_mask_1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_OSFP_INT_MASK_1 , &value);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}

static ssize_t gpio_expander_mask_1_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long value;
	int err;

	err = kstrtoul(buf, 0, &value);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = write_cpld_register(REG_OSFP_INT_MASK_1 , (u8)value);
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}
struct device_attribute dev_attr_gpio_expander_mask_1 = __ATTR(gpio_expander_mask_1, 0600, gpio_expander_mask_1_show, gpio_expander_mask_1_store);

// GPIO  expander interrupt mask
static ssize_t gpio_expander_mask_2_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_QSFP_INT_MASK_2 , &value);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}

static ssize_t gpio_expander_mask_2_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long value;
	int err;

	err = kstrtoul(buf, 0, &value);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = write_cpld_register(REG_QSFP_INT_MASK_2, (u8)value);
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}
struct device_attribute dev_attr_gpio_expander_mask_2 = __ATTR(gpio_expander_mask_2, 0600, gpio_expander_mask_2_show, gpio_expander_mask_2_store);

// GPIO  expander interrupt mask
static ssize_t gpio_expander_mask_3_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_QSFP_INT_MASK_3 , &value);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}

static ssize_t gpio_expander_mask_3_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long value;
	int err;

	err = kstrtoul(buf, 0, &value);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = write_cpld_register(REG_QSFP_INT_MASK_3, (u8)value);
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}
struct device_attribute dev_attr_gpio_expander_mask_3 = __ATTR(gpio_expander_mask_3, 0600, gpio_expander_mask_3_show, gpio_expander_mask_3_store);



// GPIO  expander interrupt mask
static ssize_t gpio_expander_mask_sfp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_SFP_INT_MASK , &value);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}

static ssize_t gpio_expander_mask_sfp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long value;
	int err;

	err = kstrtoul(buf, 0, &value);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = write_cpld_register(REG_SFP_INT_MASK, (u8)value);
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}
struct device_attribute dev_attr_gpio_expander_mask_sfp = __ATTR(gpio_expander_mask_sfp, 0600, gpio_expander_mask_sfp_show, gpio_expander_mask_sfp_store);

/********************************************************************************************************************************************************/

// FW release date
static ssize_t jed_rel_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint16_t date = 0;
	uint8_t month, day;
	int ret;

	ret = read_cpld_register(REG_CPLD_MOT, &month);
	if (ret < 0)
		return ret;

	ret = read_cpld_register(REG_CPLD_DAY, &day);
	if (ret < 0)
		return ret;

	date = (uint16_t)day | ((uint16_t)month << 8);
	return sprintf(buf, "%4.4hx\n", date);
}
struct device_attribute dev_attr_jed_rel = __ATTR(jed_rel, 0400, jed_rel_show, NULL);

// Switch Model ID
static ssize_t model_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t sw_id;
	int ret;

	ret = read_cpld_register(REG_MODEL_ID, &sw_id);
	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", sw_id);
}
struct device_attribute dev_attr_model_id = __ATTR(model_id, 0400, model_id_show, NULL);


// Reset Switch Asic
static ssize_t reset_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t reg;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_DEV_RST_CTRL, &reg);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", (reg >> BITPOS_RSTN_SWITCH) & 0x01);
}

static ssize_t reset_switch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	uint8_t reg;
	int err;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(REG_DEV_RST_CTRL, &reg);
	if (!err) {
		if (state)
			reg |= (1 << BITPOS_RSTN_SWITCH);
		else
			reg &= ~(1 << BITPOS_RSTN_SWITCH);

		err = write_cpld_register(REG_DEV_RST_CTRL, reg);
	}
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_reset_switch = __ATTR(reset_switch, 0600, reset_switch_show, reset_switch_store);




// Powerdown Switch Asic
static ssize_t switch_powerdown_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t reg;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_DEV_RST_CTRL, &reg);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", (reg >> BITPOS_SWITCH_PWRDOWN_N) & 0x01);
}

static ssize_t switch_powerdown_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	uint8_t reg;
	int err;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(REG_DEV_RST_CTRL, &reg);
	if (!err) {
		if (state)
			reg |= (1 << BITPOS_SWITCH_PWRDOWN_N);
		else
			reg &= ~(1 << BITPOS_SWITCH_PWRDOWN_N);

		err = write_cpld_register(REG_DEV_RST_CTRL, reg);
	}
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_switch_powerdown = __ATTR(switch_powerdown, 0600, switch_powerdown_show, switch_powerdown_store);

// Testee
static ssize_t testee_offset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%2.2x\n", testee_offset);
}

static ssize_t testee_offset_store(struct device *dev, struct device_attribute *attr, const char *buf,
		size_t count)
{
	ssize_t status = 0;
	uint8_t data_byte;

	status = kstrtou8(buf, 0, &data_byte);
	if (status == 0) {
		testee_offset = data_byte;
		status = count;
	}

	return status;
}
struct device_attribute dev_attr_testee_offset =
__ATTR(testee_offset, 0600, testee_offset_show, testee_offset_store);

static ssize_t testee_value_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t data_byte;
	int err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(testee_offset, &data_byte);
	mutex_unlock(&cpld_dev_data->lock);

	if (err < 0)
		return err;

	return sprintf(buf, "0x%2.2x\n", data_byte);
}

static ssize_t testee_value_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	uint8_t data_byte;
	ssize_t status;

	status = kstrtou8(buf, 0, &data_byte);
	if (status == 0) {
		mutex_lock(&cpld_dev_data->lock);
		status = write_cpld_register(testee_offset, data_byte) == 0 ? count : -EIO;
		mutex_unlock(&cpld_dev_data->lock);
	}

	return status;
}

struct device_attribute dev_attr_testee_value =
__ATTR(testee_value, 0600, testee_value_show, testee_value_store);

static struct attribute *cpld_attrs[] = {
	&dev_attr_board_id.attr,
	&dev_attr_cpld_id.attr,
	&dev_attr_cpld_version.attr,
	&dev_attr_gpio_expander_interrupt_1.attr,
	&dev_attr_gpio_expander_interrupt_2.attr,
	&dev_attr_gpio_expander_interrupt_3.attr,
	&dev_attr_gpio_expander_mask_1.attr,
	&dev_attr_gpio_expander_mask_2.attr,
	&dev_attr_gpio_expander_mask_3.attr,
	&dev_attr_gpio_expander_interrupt_sfp.attr,
	&dev_attr_gpio_expander_mask_sfp.attr,

	&dev_attr_jed_rel.attr,
	&dev_attr_model_id.attr,
	&dev_attr_reset_switch.attr,
	&dev_attr_switch_powerdown.attr,

	&dev_attr_testee_offset.attr,
	&dev_attr_testee_value.attr,
	NULL,
};

static const struct attribute_group cpld_attr_group = {
	.attrs = cpld_attrs,
};

static int cpld_open(struct inode *inode, struct file *file)
{
	file->private_data = cpld_dev_data;
	return 0;
}

static int cpld_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations cpld_fops = {
	.owner = THIS_MODULE,
	.open = cpld_open,
	.release = cpld_release,
};


/*
   Enable CPLD wathcdog
 */
static int cpld_wdt_start(struct watchdog_device *wdt_dev)
{
	uint8_t val;

	mutex_lock(&cpld_dev_data->lock);
	if (read_cpld_register(REG_DEV_RST_CTRL, &val) != 0) {
		mutex_unlock(&cpld_dev_data->lock);
		return -EIO;
	}
	/* Enable watchdog */
	val |= (1 << BITPOS_WDT_EN);
	if (write_cpld_register(REG_DEV_RST_CTRL, val) != 0) {
		mutex_unlock(&cpld_dev_data->lock);
		return -EIO;
	}
	mutex_unlock(&cpld_dev_data->lock);
	return 0;
}
/*
   Disable CPLD wathcdog
 */
static int cpld_wdt_stop(struct watchdog_device *wdt_dev)
{
	uint8_t val;

	mutex_lock(&cpld_dev_data->lock);
	if (read_cpld_register(REG_DEV_RST_CTRL, &val) != 0) {
		mutex_unlock(&cpld_dev_data->lock);
		return -EIO;
	}
	val &= ~(1 << BITPOS_WDT_EN);
	if (write_cpld_register(REG_DEV_RST_CTRL, val) != 0) {
		mutex_unlock(&cpld_dev_data->lock);
		return -EIO;
	}
	mutex_unlock(&cpld_dev_data->lock);
	return 0;
}

static int cpld_wdt_set_timeout(struct watchdog_device *wdt_dev, unsigned int timeout)
{
	uint8_t val;
	int is_enabled = 0;

	mutex_lock(&cpld_dev_data->lock);
	if (read_cpld_register(REG_DEV_RST_CTRL, &val) != 0) {
		mutex_unlock(&cpld_dev_data->lock);
		return -EIO;
	}
	if (val & (1 << BITPOS_WDT_EN)) {
		is_enabled = 1;
		/* save and stop */
		if (write_cpld_register(REG_DEV_RST_CTRL, val & ~(1 << BITPOS_WDT_EN)) != 0) {
			mutex_unlock(&cpld_dev_data->lock);
			return -EIO;
		}
	}
	if (write_cpld_register(REG_WDT_MAXL, timeout & 0xFF) != 0 ||
			write_cpld_register(REG_WDT_MAXM, (timeout >> 8) & 0xFF) != 0) {
		mutex_unlock(&cpld_dev_data->lock);
		return -EIO;
	}
	/* restore */
	if (is_enabled) {
		if (write_cpld_register(REG_DEV_RST_CTRL, val) != 0) {
			mutex_unlock(&cpld_dev_data->lock);
			return -EIO;
		}
	}
	wdt_dev->timeout = timeout;
	mutex_unlock(&cpld_dev_data->lock);
	return 0;
}

static int cpld_wdt_ping(struct watchdog_device *wdt_dev)
{
	uint8_t val;

	mutex_lock(&cpld_dev_data->lock);

	if (read_cpld_register(REG_DEV_RST_CTRL, &val) != 0) {
		mutex_unlock(&cpld_dev_data->lock);
		return -EIO;
	}
#if 0
	val &= ~(1 << BITPOS_WDT_EN);
	if (write_cpld_register(REG_DEV_RST_CTRL, val) != 0) {
		mutex_unlock(&cpld_dev_data->lock);
		return -EIO;
	}
#endif
	val |= (1 << BITPOS_WDT_CLR);
	if (write_cpld_register(REG_DEV_RST_CTRL, val) != 0) {
		mutex_unlock(&cpld_dev_data->lock);
		return -EIO;
	}

	mutex_unlock(&cpld_dev_data->lock);
	return 0;
}

static unsigned int cpld_wdt_get_bootstatus(struct cpld_data *data)
{
	uint8_t val;
	unsigned int bootstatus;

	mutex_lock(&cpld_dev_data->lock);
	if (read_cpld_register(REG_WDT_REC, &val) != 0) {
		mutex_unlock(&cpld_dev_data->lock);
		return -EIO;
	}

	bootstatus = (val & (1 << BITPOS_WDT_REC)) ? WDIOF_CARDRESET : 0;

	// Clear reboot record and re-enable recording
	val &= ~(1 << BITPOS_WDT_REC_CFG);
	if (write_cpld_register(REG_WDT_REC, val) != 0) {
		mutex_unlock(&cpld_dev_data->lock);
		return -EIO;
	}
	val |= (1 << BITPOS_WDT_REC_CFG);
	if (write_cpld_register(REG_WDT_REC, val) != 0) {
		mutex_unlock(&cpld_dev_data->lock);
		return -EIO;
	}
	mutex_unlock(&cpld_dev_data->lock);
	return bootstatus;
}

static unsigned int cpld_wdt_get_timeleft(struct watchdog_device *wdt_dev)
{
	uint8_t cntl, cntm, maxl, maxm;
	unsigned int current_count, max_timeout;

	mutex_lock(&cpld_dev_data->lock);

	if (read_cpld_register(REG_WDT_CNTL, &cntl) != 0 || read_cpld_register(REG_WDT_CNTM, &cntm) != 0) {
		mutex_unlock(&cpld_dev_data->lock);
		return -EIO;
	}
	if (read_cpld_register(REG_WDT_MAXL, &maxl) != 0 || read_cpld_register(REG_WDT_MAXM, &maxm) != 0) {
		mutex_unlock(&cpld_dev_data->lock);
		return -EIO;
	}

	mutex_unlock(&cpld_dev_data->lock);

	current_count = ((unsigned int)cntm << 8) | cntl;
	max_timeout = ((unsigned int)maxm << 8) | maxl;

	if (current_count >= max_timeout) {
		return 0;
	} else {
		return max_timeout - current_count;
	}
}

#define WATCHDOG_TIMEOUT 30 /* 30 sec default heartbeat */

static struct watchdog_device *pwddev;

static const struct watchdog_info cpld_wdt_info = {
	.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
	.identity = "CPLD1 Watchdog",
};

static const struct watchdog_ops cpld_wdt_ops = {
	.owner = THIS_MODULE,
	.start = cpld_wdt_start,
	.stop = cpld_wdt_stop,
	.ping = cpld_wdt_ping,
	.set_timeout = cpld_wdt_set_timeout,
	.get_timeleft = cpld_wdt_get_timeleft,
};

static int __init cpld_init(void)
{
	int ret;
	dev_t dev;

	cpld_dev_data = kzalloc(sizeof(*cpld_dev_data), GFP_KERNEL);
	if (!cpld_dev_data)
		return -ENOMEM;

	mutex_init(&cpld_dev_data->lock);

	if ((ret = alloc_chrdev_region(&dev, 0, 1, "cpld_device")) < 0)
		goto fail_alloc_chrdev;

	cpld_major = MAJOR(dev);
	cdev_init(&cpld_dev_data->cdev, &cpld_fops);
	cpld_dev_data->cdev.owner = THIS_MODULE;
	if ((ret = cdev_add(&cpld_dev_data->cdev, dev, 1)) < 0)
		goto fail_cdev_add;

	cpld_class = class_create("CPLD1");
	if (IS_ERR(cpld_class)) {
		ret = PTR_ERR(cpld_class);
		goto fail_class_create;
	}

	cpld_dev_data->cpld_device = device_create(cpld_class, NULL, dev, NULL, "cpld_device");
	if (IS_ERR(cpld_dev_data->cpld_device)) {
		ret = PTR_ERR(cpld_dev_data->cpld_device);
		goto fail_device_create;
	}

	ret = sysfs_create_group(&cpld_dev_data->cpld_device->kobj, &cpld_attr_group);
	if (ret)
		goto fail_sysfs_create_group;

	/* Watchdog */
	pwddev = kzalloc(sizeof(*pwddev), GFP_KERNEL);
	if (!pwddev) {
		pr_err("Failed to allocate memory for watchdog device\n");
		return -ENOMEM;
	}

	pwddev->info = &cpld_wdt_info;
	pwddev->ops = &cpld_wdt_ops;
	pwddev->bootstatus = cpld_wdt_get_bootstatus(cpld_dev_data);
	pwddev->timeout = WATCHDOG_TIMEOUT;
	pwddev->parent = cpld_dev_data->cpld_device;
	pwddev->min_timeout = 1;
	pwddev->max_timeout = 0xffff;

	ret = watchdog_register_device(pwddev);
	if (ret) {
		dev_err(cpld_dev_data->cpld_device, "Cannot register watchdog device (err=%d)\n", ret);
		kfree(pwddev);
		pwddev = NULL;
		goto fail_cpld_dev_create;
	} else {
		cpld_wdt_stop(pwddev);
		cpld_wdt_set_timeout(pwddev, WATCHDOG_TIMEOUT);
		dev_info(cpld_dev_data->cpld_device, "Watchdog initialized. heartbeat=%d sec (nowayout=%d)\n",
				pwddev->timeout, 0);
	}

	return 0;

fail_cpld_dev_create:
	sysfs_remove_group(&cpld_dev_data->cpld_device->kobj, &cpld_attr_group);

fail_sysfs_create_group:
	device_destroy(cpld_class, cpld_dev_data->cpld_device->devt);

fail_device_create:
	class_destroy(cpld_class);

fail_class_create:
	cdev_del(&cpld_dev_data->cdev);

fail_cdev_add:
	unregister_chrdev_region(dev, 1);

fail_alloc_chrdev:
	mutex_destroy(&cpld_dev_data->lock);
	kfree(cpld_dev_data);
	return ret;
}

static void __exit cpld_exit(void)
{
	dev_t dev = MKDEV(cpld_major, 0);

	if (pwddev) {
		watchdog_unregister_device(pwddev);
		dev_err(cpld_dev_data->cpld_device, "Watchdog device unregistered\n");
		kfree(pwddev);
		pwddev = NULL;
	}

	if (cpld_dev_data->cpld_device) {
		sysfs_remove_group(&cpld_dev_data->cpld_device->kobj, &cpld_attr_group);
		device_destroy(cpld_class, cpld_dev_data->cpld_device->devt);
	}

	if (cpld_class)
		class_destroy(cpld_class);

	if (cpld_dev_data) {
		cdev_del(&cpld_dev_data->cdev);
		mutex_destroy(&cpld_dev_data->lock);
		kfree(cpld_dev_data);
	}

	unregister_chrdev_region(dev, 1);
}

module_init(cpld_init);
module_exit(cpld_exit);

MODULE_DESCRIPTION("SuperMicro T8196 Switchboard CPLD1 Driver");
MODULE_LICENSE("GPL");


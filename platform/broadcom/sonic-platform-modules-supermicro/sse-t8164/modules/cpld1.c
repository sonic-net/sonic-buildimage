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

static int i2c_bus = 0;
static int i2c_addr = 0x2b;

module_param(i2c_bus, int, S_IRUGO);
MODULE_PARM_DESC(i2c_bus, "I2C bus number");

module_param(i2c_addr, int, S_IRUGO);
MODULE_PARM_DESC(i2c_addr, "I2C device address");

#define OSFP_PORT_TOTAL 64

/* Refer to SSE_T8164 HW SPEC for more details */
/* The Switch Model ID of this project is 0x02 */
#define REG_MODEL_ID 0x00
#define REG_BOARD_ID 0x01
/* CPLD_ID */
#define REG_CPLD_ID 0x02
/* CPLD version */
#define REG_CPLD_VER 0x03
#define REG_DEV_RST_CTRL 0x06
#define BITPOS_WDT_EN 0
#define BITPOS_RSTN_SWITCH 1
#define BITPOS_SWITCH_PWRDOWN_N 2
/* System LED */
#define REG_SYS_LED 0x04
/* Power good status */
#define REG_POWER_GOOD_1 0x07
#define REG_POWER_GOOD_2 0x08
/* UART control */
#define REG_UART_CONTROL 0x09
/* OSFP present */
#define REG_OSFP_PRESENT_1 0x10
#define REG_OSFP_PRESENT_2 0x11
#define REG_OSFP_PRESENT_3 0x12
#define REG_OSFP_PRESENT_4 0x13
#define REG_OSFP_PRESENT_5 0x14
#define REG_OSFP_PRESENT_6 0x15
#define REG_OSFP_PRESENT_7 0x16
#define REG_OSFP_PRESENT_8 0x17
/* GPIO expander interrupt */
#define REG_GPIO_EXPANDER_INT_1 0x18
/* Reset controls */
#define REG_GPIO_EXPANDER_RESET_1 0x1A
#define REG_GPIO_EXPANDER_RESET_2 0x1B
#define REG_I2C_MUX_RESET_1 0x1C
#define BITPOS_RSTN_MUX_OSFP_0104_3336 0
#define BITPOS_RSTN_MUX_OSFP_0508_3740 1
#define BITPOS_RSTN_MUX_OSFP_0912_4144 2
#define BITPOS_RSTN_MUX_OSFP_1316_4548 3
#define BITPOS_RSTN_MUX_OSFP_1720_4952 4
#define BITPOS_RSTN_MUX_OSFP_2124_5356 5
#define BITPOS_RSTN_MUX_OSFP_2528_5760 6
#define BITPOS_RSTN_MUX_OSFP_2932_6164 7
#define REG_I2C_MUX_RESET_2 0x1D
#define BITPOS_RSTN_MUX_ROOT 0
#define BITPOS_RSTN_MUX_OSFP 1
#define BITPOS_RSTN_MUX_GPIO_1732_4964 2
#define BITPOS_RSTN_MUX_GPIO_0116_3348 3
#define BITPOS_RSTN_MUX_EFUSE_0116_3348 4
#define BITPOS_RSTN_MUX_EFUSE_1732_4964 5
/* E-fuse */
#define REG_EFUSE_ENABLE_1 0x1E
#define REG_EFUSE_ENABLE_2 0x1F
#define REG_EFUSE_PWRGOOD_1 0x20
#define REG_EFUSE_PWRGOOD_2 0x21
#define REG_EFUSE_FAULT_1 0x22
#define REG_EFUSE_FAULT_2 0x23
#define REG_EFUSE_OVERTEMP_1 0x24
#define REG_EFUSE_OVERTEMP_2 0x25
/* SFP status */
#define REG_SFP_STATUS 0x26
#define BITPOS_SFP_VEN 1
/* Watchdog */
#define REG_WDT_MAXL 0x32
#define REG_WDT_MAXM 0x33
#define REG_WDT_CNTL 0x34
#define REG_WDT_CNTM 0x35
#define REG_WDT_REC 0x40
#define BITPOS_WDT_REC 4
#define BITPOS_WDT_REC_CFG 7
/* CPLD code released date --- Month */
#define REG_CPLD_MOT 0xFE
/* CPLD code released date --- Day */
#define REG_CPLD_DAY 0xFF

enum PORT_TYPE { NONE, OSFP, SFP };

struct cpld_data {
	struct cdev cdev;
	struct mutex lock;
	struct device *cpld_device;
	struct device *sff_devices[OSFP_PORT_TOTAL];
};

struct cpld_port_data {
	struct cpld_data *parent;
	int port_id;
};

static int cpld_major;
static struct class *cpld_class = NULL;
static struct cpld_data *cpld_dev_data;
static struct device *sff_dev = NULL; // Just for CPLD1/SFF directory, not a real device

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


// GPIO  expander interrupt/reset
static ssize_t gpio_expander_interrupt_1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	ret = read_cpld_register(REG_GPIO_EXPANDER_INT_1, &value);
	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}
struct device_attribute dev_attr_gpio_expander_interrupt_1 = __ATTR(gpio_expander_interrupt_1, 0400, gpio_expander_interrupt_1_show, NULL);

static ssize_t gpio_expander_reset_1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_GPIO_EXPANDER_RESET_1, &value);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}

static ssize_t gpio_expander_reset_1_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long value;
	int err;

	err = kstrtoul(buf, 0, &value);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = write_cpld_register(REG_GPIO_EXPANDER_RESET_1, (u8)value);
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_gpio_expander_reset_1 = __ATTR(gpio_expander_reset_1, 0600, gpio_expander_reset_1_show, gpio_expander_reset_1_store);


static ssize_t gpio_expander_reset_2_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_GPIO_EXPANDER_RESET_2, &value);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}

static ssize_t gpio_expander_reset_2_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long value;
	int err;

	err = kstrtoul(buf, 0, &value);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = write_cpld_register(REG_GPIO_EXPANDER_RESET_2, (u8)value);
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_gpio_expander_reset_2 = __ATTR(gpio_expander_reset_2, 0600, gpio_expander_reset_2_show, gpio_expander_reset_2_store);

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

// Efuse
static ssize_t osfp_efuse_enable_1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_EFUSE_ENABLE_1, &value);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}

static ssize_t osfp_efuse_enable_1_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long value;
	int err;

	err = kstrtoul(buf, 0, &value);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = write_cpld_register(REG_EFUSE_ENABLE_1, (u8)value);
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_osfp_efuse_enable_1 = __ATTR(osfp_efuse_enable_1, 0600, osfp_efuse_enable_1_show, osfp_efuse_enable_1_store);


static ssize_t osfp_efuse_enable_2_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_EFUSE_ENABLE_2, &value);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}

static ssize_t osfp_efuse_enable_2_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long value;
	int err;

	err = kstrtoul(buf, 0, &value);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = write_cpld_register(REG_EFUSE_ENABLE_2, (u8)value);
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_osfp_efuse_enable_2 = __ATTR(osfp_efuse_enable_2, 0600, osfp_efuse_enable_2_show, osfp_efuse_enable_2_store);

static ssize_t osfp_efuse_fault_1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	ret = read_cpld_register(REG_EFUSE_FAULT_1, &value);
	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}
struct device_attribute dev_attr_osfp_efuse_fault_1 = __ATTR(osfp_efuse_fault_1, 0400, osfp_efuse_fault_1_show, NULL);

static ssize_t osfp_efuse_fault_2_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	ret = read_cpld_register(REG_EFUSE_FAULT_2, &value);
	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}
struct device_attribute dev_attr_osfp_efuse_fault_2 = __ATTR(osfp_efuse_fault_2, 0400, osfp_efuse_fault_2_show, NULL);

static ssize_t osfp_efuse_overtemp_1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	ret = read_cpld_register(REG_EFUSE_OVERTEMP_1, &value);
	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}
struct device_attribute dev_attr_osfp_efuse_overtemp_1 = __ATTR(osfp_efuse_overtemp_1, 0400, osfp_efuse_overtemp_1_show, NULL);

static ssize_t osfp_efuse_overtemp_2_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	ret = read_cpld_register(REG_EFUSE_OVERTEMP_2, &value);
	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}
struct device_attribute dev_attr_osfp_efuse_overtemp_2 = __ATTR(osfp_efuse_overtemp_2, 0400, osfp_efuse_overtemp_2_show, NULL);

static ssize_t osfp_efuse_powergood_1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	ret = read_cpld_register(REG_EFUSE_PWRGOOD_1, &value);
	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}
struct device_attribute dev_attr_osfp_efuse_powergood_1 = __ATTR(osfp_efuse_powergood_1, 0400, osfp_efuse_powergood_1_show, NULL);

static ssize_t osfp_efuse_powergood_2_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	ret = read_cpld_register(REG_EFUSE_PWRGOOD_2, &value);
	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}
struct device_attribute dev_attr_osfp_efuse_powergood_2 = __ATTR(osfp_efuse_powergood_2, 0400, osfp_efuse_powergood_2_show, NULL);

// Reset Mux for EFUSEs connected to port 1-16 and 33-48
static ssize_t reset_mux_efuse_0116_3348_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t reg;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_I2C_MUX_RESET_2, &reg);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", (reg >> BITPOS_RSTN_MUX_EFUSE_0116_3348) & 0x01);
}

static ssize_t reset_mux_efuse_0116_3348_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	uint8_t reg;
	int err;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(REG_I2C_MUX_RESET_2, &reg);
	if (!err) {
		if (state)
			reg |= (1 << BITPOS_RSTN_MUX_EFUSE_0116_3348);
		else
			reg &= ~(1 << BITPOS_RSTN_MUX_EFUSE_0116_3348);

		err = write_cpld_register(REG_I2C_MUX_RESET_2, reg);
	}
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_reset_mux_efuse_0116_3348 = __ATTR(reset_mux_efuse_0116_3348, 0600, reset_mux_efuse_0116_3348_show, reset_mux_efuse_0116_3348_store);

// Reset Mux for EFUSEs connected to port 17-32 and 49-64
static ssize_t reset_mux_efuse_1732_4964_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t reg;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_I2C_MUX_RESET_2, &reg);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", (reg >> BITPOS_RSTN_MUX_EFUSE_1732_4964) & 0x01);
}

static ssize_t reset_mux_efuse_1732_4964_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	uint8_t reg;
	int err;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(REG_I2C_MUX_RESET_2, &reg);
	if (!err) {
		if (state)
			reg |= (1 << BITPOS_RSTN_MUX_EFUSE_1732_4964);
		else
			reg &= ~(1 << BITPOS_RSTN_MUX_EFUSE_1732_4964);

		err = write_cpld_register(REG_I2C_MUX_RESET_2, reg);
	}
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_reset_mux_efuse_1732_4964 = __ATTR(reset_mux_efuse_1732_4964, 0600, reset_mux_efuse_1732_4964_show, reset_mux_efuse_1732_4964_store);

// Reset Mux for GPIOs connected to port 1-16 and 33-48
static ssize_t reset_mux_gpio_0116_3348_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t reg;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_I2C_MUX_RESET_2, &reg);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", (reg >> BITPOS_RSTN_MUX_GPIO_0116_3348) & 0x01);
}

static ssize_t reset_mux_gpio_0116_3348_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	uint8_t reg;
	int err;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(REG_I2C_MUX_RESET_2, &reg);
	if (!err) {
		if (state)
			reg |= (1 << BITPOS_RSTN_MUX_GPIO_0116_3348);
		else
			reg &= ~(1 << BITPOS_RSTN_MUX_GPIO_0116_3348);

		err = write_cpld_register(REG_I2C_MUX_RESET_2, reg);
	}
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_reset_mux_gpio_0116_3348 = __ATTR(reset_mux_gpio_0116_3348, 0600, reset_mux_gpio_0116_3348_show, reset_mux_gpio_0116_3348_store);

// Reset Mux for GPIOs connected to port 17-32 and 49-64
static ssize_t reset_mux_gpio_1732_4964_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t reg;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_I2C_MUX_RESET_2, &reg);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", (reg >> BITPOS_RSTN_MUX_GPIO_1732_4964) & 0x01);
}

static ssize_t reset_mux_gpio_1732_4964_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	uint8_t reg;
	int err;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(REG_I2C_MUX_RESET_2, &reg);
	if (!err) {
		if (state)
			reg |= (1 << BITPOS_RSTN_MUX_GPIO_1732_4964);
		else
			reg &= ~(1 << BITPOS_RSTN_MUX_GPIO_1732_4964);

		err = write_cpld_register(REG_I2C_MUX_RESET_2, reg);
	}
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_reset_mux_gpio_1732_4964 = __ATTR(reset_mux_gpio_1732_4964, 0600, reset_mux_gpio_1732_4964_show, reset_mux_gpio_1732_4964_store);

// Reset Mux OSFP 1-64
static ssize_t reset_mux_osfp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t reg;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_I2C_MUX_RESET_2, &reg);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", (reg >> BITPOS_RSTN_MUX_OSFP) & 0x01);
}

static ssize_t reset_mux_osfp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	uint8_t reg;
	int err;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(REG_I2C_MUX_RESET_2, &reg);
	if (!err) {
		if (state)
			reg |= (1 << BITPOS_RSTN_MUX_OSFP);
		else
			reg &= ~(1 << BITPOS_RSTN_MUX_OSFP);

		err = write_cpld_register(REG_I2C_MUX_RESET_2, reg);
	}
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_reset_mux_osfp = __ATTR(reset_mux_osfp, 0600, reset_mux_osfp_show, reset_mux_osfp_store);

// Reset Muxes for OSFP eeprom
static ssize_t reset_mux_osfp_0104_3336_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t reg;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_I2C_MUX_RESET_1, &reg);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", (reg >> BITPOS_RSTN_MUX_OSFP_0104_3336) & 0x01);
}

static ssize_t reset_mux_osfp_0104_3336_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	uint8_t reg;
	int err;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(REG_I2C_MUX_RESET_1, &reg);
	if (!err) {
		if (state)
			reg |= (1 << BITPOS_RSTN_MUX_OSFP_0104_3336);
		else
			reg &= ~(1 << BITPOS_RSTN_MUX_OSFP_0104_3336);

		err = write_cpld_register(REG_I2C_MUX_RESET_1, reg);
	}
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_reset_mux_osfp_0104_3336 = __ATTR(reset_mux_osfp_0104_3336, 0600, reset_mux_osfp_0104_3336_show, reset_mux_osfp_0104_3336_store);


static ssize_t reset_mux_osfp_0508_3740_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t reg;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_I2C_MUX_RESET_1, &reg);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", (reg >> BITPOS_RSTN_MUX_OSFP_0508_3740) & 0x01);
}

static ssize_t reset_mux_osfp_0508_3740_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	uint8_t reg;
	int err;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(REG_I2C_MUX_RESET_1, &reg);
	if (!err) {
		if (state)
			reg |= (1 << BITPOS_RSTN_MUX_OSFP_0508_3740);
		else
			reg &= ~(1 << BITPOS_RSTN_MUX_OSFP_0508_3740);

		err = write_cpld_register(REG_I2C_MUX_RESET_1, reg);
	}
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_reset_mux_osfp_0508_3740 = __ATTR(reset_mux_osfp_0508_3740, 0600, reset_mux_osfp_0508_3740_show, reset_mux_osfp_0508_3740_store);


static ssize_t reset_mux_osfp_0912_4144_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t reg;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_I2C_MUX_RESET_1, &reg);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", (reg >> BITPOS_RSTN_MUX_OSFP_0912_4144) & 0x01);
}

static ssize_t reset_mux_osfp_0912_4144_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	uint8_t reg;
	int err;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(REG_I2C_MUX_RESET_1, &reg);
	if (!err) {
		if (state)
			reg |= (1 << BITPOS_RSTN_MUX_OSFP_0912_4144);
		else
			reg &= ~(1 << BITPOS_RSTN_MUX_OSFP_0912_4144);

		err = write_cpld_register(REG_I2C_MUX_RESET_1, reg);
	}
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_reset_mux_osfp_0912_4144 = __ATTR(reset_mux_osfp_0912_4144, 0600, reset_mux_osfp_0912_4144_show, reset_mux_osfp_0912_4144_store);


static ssize_t reset_mux_osfp_1316_4548_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t reg;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_I2C_MUX_RESET_1, &reg);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", (reg >> BITPOS_RSTN_MUX_OSFP_1316_4548) & 0x01);
}

static ssize_t reset_mux_osfp_1316_4548_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	uint8_t reg;
	int err;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(REG_I2C_MUX_RESET_1, &reg);
	if (!err) {
		if (state)
			reg |= (1 << BITPOS_RSTN_MUX_OSFP_1316_4548);
		else
			reg &= ~(1 << BITPOS_RSTN_MUX_OSFP_1316_4548);

		err = write_cpld_register(REG_I2C_MUX_RESET_1, reg);
	}
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_reset_mux_osfp_1316_4548 = __ATTR(reset_mux_osfp_1316_4548, 0600, reset_mux_osfp_1316_4548_show, reset_mux_osfp_1316_4548_store);


static ssize_t reset_mux_osfp_1720_4952_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t reg;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_I2C_MUX_RESET_1, &reg);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", (reg >> BITPOS_RSTN_MUX_OSFP_1720_4952) & 0x01);
}

static ssize_t reset_mux_osfp_1720_4952_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	uint8_t reg;
	int err;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(REG_I2C_MUX_RESET_1, &reg);
	if (!err) {
		if (state)
			reg |= (1 << BITPOS_RSTN_MUX_OSFP_1720_4952);
		else
			reg &= ~(1 << BITPOS_RSTN_MUX_OSFP_1720_4952);

		err = write_cpld_register(REG_I2C_MUX_RESET_1, reg);
	}
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_reset_mux_osfp_1720_4952 = __ATTR(reset_mux_osfp_1720_4952, 0600, reset_mux_osfp_1720_4952_show, reset_mux_osfp_1720_4952_store);

static ssize_t reset_mux_osfp_2124_5356_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t reg;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_I2C_MUX_RESET_1, &reg);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", (reg >> BITPOS_RSTN_MUX_OSFP_2124_5356) & 0x01);
}

static ssize_t reset_mux_osfp_2124_5356_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	uint8_t reg;
	int err;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(REG_I2C_MUX_RESET_1, &reg);
	if (!err) {
		if (state)
			reg |= (1 << BITPOS_RSTN_MUX_OSFP_2124_5356);
		else
			reg &= ~(1 << BITPOS_RSTN_MUX_OSFP_2124_5356);

		err = write_cpld_register(REG_I2C_MUX_RESET_1, reg);
	}
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_reset_mux_osfp_2124_5356 = __ATTR(reset_mux_osfp_2124_5356, 0600, reset_mux_osfp_2124_5356_show, reset_mux_osfp_2124_5356_store);


static ssize_t reset_mux_osfp_2528_5760_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t reg;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_I2C_MUX_RESET_1, &reg);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", (reg >> BITPOS_RSTN_MUX_OSFP_2528_5760) & 0x01);
}

static ssize_t reset_mux_osfp_2528_5760_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	uint8_t reg;
	int err;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(REG_I2C_MUX_RESET_1, &reg);
	if (!err) {
		if (state)
			reg |= (1 << BITPOS_RSTN_MUX_OSFP_2528_5760);
		else
			reg &= ~(1 << BITPOS_RSTN_MUX_OSFP_2528_5760);

		err = write_cpld_register(REG_I2C_MUX_RESET_1, reg);
	}
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_reset_mux_osfp_2528_5760 = __ATTR(reset_mux_osfp_2528_5760, 0600, reset_mux_osfp_2528_5760_show, reset_mux_osfp_2528_5760_store);


static ssize_t reset_mux_osfp_2932_6164_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t reg;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_I2C_MUX_RESET_1, &reg);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", (reg >> BITPOS_RSTN_MUX_OSFP_2932_6164) & 0x01);
}

static ssize_t reset_mux_osfp_2932_6164_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	uint8_t reg;
	int err;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(REG_I2C_MUX_RESET_1, &reg);
	if (!err) {
		if (state)
			reg |= (1 << BITPOS_RSTN_MUX_OSFP_2932_6164);
		else
			reg &= ~(1 << BITPOS_RSTN_MUX_OSFP_2932_6164);

		err = write_cpld_register(REG_I2C_MUX_RESET_1, reg);
	}
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_reset_mux_osfp_2932_6164 = __ATTR(reset_mux_osfp_2932_6164, 0600, reset_mux_osfp_2932_6164_show, reset_mux_osfp_2932_6164_store);

// Reset root I2C Mux
static ssize_t reset_mux_root_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t reg;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_I2C_MUX_RESET_2, &reg);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", (reg >> BITPOS_RSTN_MUX_ROOT) & 0x01);
}

static ssize_t reset_mux_root_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	uint8_t reg;
	int err;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(REG_I2C_MUX_RESET_2, &reg);
	if (!err) {
		if (state)
			reg |= (1 << BITPOS_RSTN_MUX_ROOT);
		else
			reg &= ~(1 << BITPOS_RSTN_MUX_ROOT);

		err = write_cpld_register(REG_I2C_MUX_RESET_2, reg);
	}
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_reset_mux_root = __ATTR(reset_mux_root, 0600, reset_mux_root_show, reset_mux_root_store);

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

// SFP Status
static ssize_t sfp_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_SFP_STATUS, &value);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}

static ssize_t sfp_status_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long value;
	int err;

	err = kstrtoul(buf, 0, &value);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = write_cpld_register(REG_SFP_STATUS, (u8)value);
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_sfp_status = __ATTR(sfp_status, 0600, sfp_status_show, sfp_status_store);

/* SFP Ven */
static ssize_t sfp_ven_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_SFP_STATUS, &value);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", (value >> BITPOS_SFP_VEN) & 0x01);
}

static ssize_t sfp_ven_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	uint8_t reg;
	int err;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = read_cpld_register(REG_SFP_STATUS, &reg);
	if (!err) {
		if (state)
			reg |= (1 << BITPOS_SFP_VEN);
		else
			reg &= ~(1 << BITPOS_SFP_VEN);

		err = write_cpld_register(REG_SFP_STATUS, reg);
	}
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}
static DEVICE_ATTR(sfp_ven, 0600, sfp_ven_show, sfp_ven_store);

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

// Switch Powergood Status
static ssize_t switch_powergood_status_1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	ret = read_cpld_register(REG_POWER_GOOD_1, &value);
	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}
struct device_attribute dev_attr_switch_powergood_status_1 = __ATTR(switch_powergood_status_1, 0400, switch_powergood_status_1_show, NULL);

static ssize_t switch_powergood_status_2_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	ret = read_cpld_register(REG_POWER_GOOD_2, &value);
	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}
struct device_attribute dev_attr_switch_powergood_status_2 = __ATTR(switch_powergood_status_2, 0400, switch_powergood_status_2_show, NULL);

// Switch UART control
static ssize_t switch_uart_control_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_UART_CONTROL, &value);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}

static ssize_t switch_uart_control_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long value;
	int err;

	err = kstrtoul(buf, 0, &value);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = write_cpld_register(REG_UART_CONTROL, (u8)value);
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_switch_uart_control = __ATTR(switch_uart_control, 0600, switch_uart_control_show, switch_uart_control_store);

// System LED
static ssize_t system_led_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t value;
	int ret;

	mutex_lock(&cpld_dev_data->lock);
	ret = read_cpld_register(REG_SYS_LED, &value);
	mutex_unlock(&cpld_dev_data->lock);

	if (ret < 0)
		return ret;

	return sprintf(buf, "0x%2.2x\n", value);
}

static ssize_t system_led_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long value;
	int err;

	err = kstrtoul(buf, 0, &value);
	if (err)
		return err;

	mutex_lock(&cpld_dev_data->lock);
	err = write_cpld_register(REG_SYS_LED, (u8)value);
	mutex_unlock(&cpld_dev_data->lock);

	return (err == 0) ? count : -EIO;
}

struct device_attribute dev_attr_system_led = __ATTR(system_led, 0600, system_led_show, system_led_store);

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
	&dev_attr_gpio_expander_reset_1.attr,
	&dev_attr_gpio_expander_reset_2.attr,
	&dev_attr_jed_rel.attr,
	&dev_attr_model_id.attr,
	&dev_attr_osfp_efuse_enable_1.attr,
	&dev_attr_osfp_efuse_enable_2.attr,
	&dev_attr_osfp_efuse_fault_1.attr,
	&dev_attr_osfp_efuse_fault_2.attr,
	&dev_attr_osfp_efuse_overtemp_1.attr,
	&dev_attr_osfp_efuse_overtemp_2.attr,
	&dev_attr_osfp_efuse_powergood_1.attr,
	&dev_attr_osfp_efuse_powergood_2.attr,
	&dev_attr_reset_mux_efuse_0116_3348.attr,
	&dev_attr_reset_mux_efuse_1732_4964.attr,
	&dev_attr_reset_mux_gpio_0116_3348.attr,
	&dev_attr_reset_mux_gpio_1732_4964.attr,
	&dev_attr_reset_mux_osfp.attr,
	&dev_attr_reset_mux_osfp_0104_3336.attr,
	&dev_attr_reset_mux_osfp_0508_3740.attr,
	&dev_attr_reset_mux_osfp_0912_4144.attr,
	&dev_attr_reset_mux_osfp_1316_4548.attr,
	&dev_attr_reset_mux_osfp_1720_4952.attr,
	&dev_attr_reset_mux_osfp_2124_5356.attr,
	&dev_attr_reset_mux_osfp_2528_5760.attr,
	&dev_attr_reset_mux_osfp_2932_6164.attr,
	&dev_attr_reset_mux_root.attr,
	&dev_attr_reset_switch.attr,
	&dev_attr_sfp_status.attr,
	&dev_attr_sfp_ven.attr,
	&dev_attr_switch_powerdown.attr,
	&dev_attr_switch_powergood_status_1.attr,
	&dev_attr_switch_powergood_status_2.attr,
	&dev_attr_switch_uart_control.attr,
	&dev_attr_system_led.attr,
	&dev_attr_testee_offset.attr,
	&dev_attr_testee_value.attr,
	NULL,
};

static const struct attribute_group cpld_attr_group = {
	.attrs = cpld_attrs,
};

/* SFF device attributes */
static ssize_t osfp_modprs_n_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct cpld_port_data *port_data = dev_get_drvdata(dev);
	u8 reg_addr;
	uint8_t value;
	int err, bit_position;
    int port_lookup[64][2] = {
        {0, 0}, {0, 2}, {0, 5}, {0, 7}, // Port 1, 2, 3, 4
        {1, 0}, {1, 2}, {1, 4}, {1, 6}, // Port 5, 6, 7, 8
        {2, 0}, {2, 2}, {2, 4}, {2, 6}, // Port 9, 10, 11, 12
        {3, 0}, {3, 2}, {3, 4}, {3, 6}, // Port 13, 14, 15, 16
        {4, 0}, {4, 2}, {4, 4}, {4, 6}, // Port 17, 18, 19, 20
        {5, 0}, {5, 2}, {5, 4}, {5, 6}, // Port 21, 22, 23, 24
        {6, 0}, {6, 2}, {6, 4}, {6, 6}, // Port 25, 26, 27, 28
        {7, 1}, {7, 3}, {7, 4}, {7, 6}, // Port 29, 30, 31, 32
        {0, 3}, {0, 1}, {0, 6}, {0, 4}, // Port 33, 34, 35, 36
        {1, 3}, {1, 1}, {1, 7}, {1, 5}, // Port 37, 38, 39, 40
        {2, 3}, {2, 1}, {2, 7}, {2, 5}, // Port 41, 42, 43, 44
        {3, 3}, {3, 1}, {3, 7}, {3, 5}, // Port 45, 46, 47, 48
        {4, 3}, {4, 1}, {4, 7}, {4, 5}, // Port 49, 50, 51, 52
        {5, 3}, {5, 1}, {5, 7}, {5, 5}, // Port 53, 54, 55, 56
        {6, 3}, {6, 1}, {6, 7}, {6, 5}, // Port 57, 58, 59, 60
        {7, 2}, {7, 0}, {7, 7}, {7, 5}  // Port 61, 62, 63, 64
    };


	if (!port_data)
		return -ENODEV;

	reg_addr = REG_OSFP_PRESENT_1 + port_lookup[port_data->port_id-1][0];

	err = read_cpld_register(reg_addr, &value);
	if (err < 0)
		return err;

	bit_position = port_lookup[port_data->port_id-1][1];

	value = (value >> bit_position) & 0x01;

	return sprintf(buf, "%d\n", value);
}
DEVICE_ATTR_RO(osfp_modprs_n);

static struct attribute *sff_osfp_attrs[] = {
	&dev_attr_osfp_modprs_n.attr,
	NULL,
};
static struct attribute_group sff_osfp_attr_grp = {
	.attrs = sff_osfp_attrs,
};
static const struct attribute_group *sff_osfp_attr_grps[] = { &sff_osfp_attr_grp, NULL };

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

static struct device *cpld_sff_init(int port_id, int port_type, struct cpld_data *parent)
{
	struct cpld_port_data *new_data;
	struct device *new_device;
	char tmpStr[20];

	new_data = kzalloc(sizeof(*new_data), GFP_KERNEL);
	if (!new_data)
		return NULL;
	new_data->port_id = port_id;
	new_data->parent = parent;

	if (!sff_dev || IS_ERR(sff_dev)) {
		printk(KERN_ERR "%s: SFF device is not created properly.\n", __FUNCTION__);
		kfree(new_data);
		return NULL;
	}

	if (port_type == OSFP) {
		sprintf(tmpStr, "OSFP%d", new_data->port_id);
		new_device = device_create_with_groups(cpld_class, sff_dev, MKDEV(0, 0), new_data, sff_osfp_attr_grps,
						       "%s", tmpStr);
	} else {
		printk(KERN_ERR "%s: Invalid port type %d.\n", __FUNCTION__, port_type);
	}

	if (IS_ERR(new_device)) {
		printk(KERN_ERR "%s: Failed to create device for port %d.\n", __FUNCTION__, port_id);
		kfree(new_data);
		return NULL;
	}

	return new_device;
}

static void cpld_sff_deinit(int port_id, struct cpld_data *data)
{
	struct cpld_port_data *dev_data;

	dev_data = dev_get_drvdata(data->sff_devices[port_id]);
	device_unregister(data->sff_devices[port_id]);
	kfree(dev_data);
}

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

	val &= ~(1 << BITPOS_WDT_EN);
	if (write_cpld_register(REG_DEV_RST_CTRL, val) != 0) {
		mutex_unlock(&cpld_dev_data->lock);
		return -EIO;
	}

	val |= (1 << BITPOS_WDT_EN);
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
	int port_count = 0;
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

	/* SFF */
	sff_dev = device_create(cpld_class, NULL, MKDEV(0, 0), NULL, "sff_device");
	if (IS_ERR(sff_dev)) {
		ret = PTR_ERR(sff_dev);
		printk(KERN_ERR "%s: Failed to create SFF device: %d\n", __FUNCTION__, ret);
		goto fail_sff_dev_create;
	}

	for (port_count = 0; port_count < OSFP_PORT_TOTAL; port_count++) {
		cpld_dev_data->sff_devices[port_count] = cpld_sff_init(port_count + 1, OSFP, cpld_dev_data);
		if (!cpld_dev_data->sff_devices[port_count]) {
			ret = -ENODEV;
			goto fail_sff_init;
		}
	}

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
		goto fail_sff_init;
	} else {
		cpld_wdt_stop(pwddev);
		cpld_wdt_set_timeout(pwddev, WATCHDOG_TIMEOUT);
		dev_info(cpld_dev_data->cpld_device, "Watchdog initialized. heartbeat=%d sec (nowayout=%d)\n",
			pwddev->timeout, 0);
	}

	return 0;

fail_sff_init:
	while (port_count-- > 0)
		cpld_sff_deinit(port_count, cpld_dev_data);

	device_destroy(cpld_class, sff_dev->devt);

fail_sff_dev_create:
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
	int port_count;
	dev_t dev = MKDEV(cpld_major, 0);

	if (pwddev) {
		watchdog_unregister_device(pwddev);
		dev_err(cpld_dev_data->cpld_device, "Watchdog device unregistered\n");
		kfree(pwddev);
		pwddev = NULL;
	}

	if (sff_dev) {
		for (port_count = 0; port_count < OSFP_PORT_TOTAL; port_count++) {
			if (cpld_dev_data->sff_devices[port_count])
				cpld_sff_deinit(port_count, cpld_dev_data);
		}
		device_destroy(cpld_class, sff_dev->devt);
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

MODULE_DESCRIPTION("SuperMicro T8164 Switchboard CPLD1 Driver");
MODULE_LICENSE("GPL");


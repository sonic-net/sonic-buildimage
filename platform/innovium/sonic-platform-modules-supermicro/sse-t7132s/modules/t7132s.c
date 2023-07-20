#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/nmi.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/bitops.h>
#include <linux/8250_pci.h>
#include <linux/interrupt.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/signal.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/watchdog.h>		/* For the watchdog specific items */
#define REG_LACKSR 0x00
#define LACKSR_CLKDIV 0x8
#define LACKSR_CLKSEL (1 << 8)
#define LACKSR_DIVEN (1 << 10)
#define LACKSR_CLKODR (1 << 12)
#define LACKSR_CLKOEN (1 << 13)
#define LACKSR_ALERA (1 << 14)

#define REG_LAS0CFGR 0x00C
#define REG_LAS0TIMR 0x010
#define REG_LAS0ADDR 0x014
#define REG_LAS1CFGR 0x018
#define REG_LAS1TIMR 0x01C
#define REG_LAS1ADDR 0x020
#define LASCFGR_INT_P (1 << 0)
#define LASCFGR_INT_L (1 << 1)
#define LASCFGR_DRQ_P (1 << 2)
#define LASCFGR_DAK_P (1 << 3)
#define LASCFGR_LSER (1 << 4)
#define LASCFGR_ENDIAN (1 << 5)
#define LASCFGR_BW16 (1 << 6)
#define REG_LIEMR 0x024
#define LIEMR_RDYPOL (1 << 4)
#define LIEMR_ALEPOL (1 << 5)
#define LIEMR_SYNCBUS (1 << 6)
#define LIEMR_MULTBUS (1 << 7)
#define LIEMR_DMA0EN (1 << 8)
#define LIEMR_DMA1EN (1 << 9)
#define LIEMR_LRST (1 << 14)
#define LIEMR_SRST (1 << 15)
#define LIEMR_L0EINTEN (1 << 16)
#define LIEMR_L0RTOIEN (1 << 17)
#define LIEMR_L1EINTEN (1 << 18)
#define LIEMR_L1RTOIEN (1 << 19)
#define LIEMR_D0DIEN (1 << 24)
#define LIEMR_D0AIEN (1 << 25)
#define LIEMR_D1DIEN (1 << 26)
#define LIEMR_D1AIEN (1 << 27)

#define DRIVER_NAME "switchboard"
#define DEVICE_NAME "fwupgrade"
#define CLASS_NAME "t7132s_cpld"
#define SFF_PORT_TOTAL 34
#define QSFP_PORT_TOTAL 32
#define SFP_PORT_TOTAL 2

/* Refer to SSE_T7132S_CPLD_spec_0820.docx for more details */
/* Switch model ID */
#define CPLD1_REG_SW_ID 0x1
/* HW/CPLD version */
#define CPLD1_REG_HWREV 0x2
/* Power sequence module status */
#define CPLD1_REG_PWR_GOOD 0x3
/* Voltage Regulator Module ALERT/Thermal */
#define CPLD1_REG_VRM 0x4
/* Enable/ Reset misc. devices */
#define CPLD1_REG_DEV_STATE_1 0x5
/* Enable/ Reset misc. devices */
#define CPLD1_REG_DEV_STATE_2 0x6
/* System reset records */
#define CPLD1_REG_SYS_RESET_REC 0x9
/* PCA9548 I2C bus switch RSTn */
#define CPLD1_REG_MUX_STATE 0xB
/* Transceiver Power Enable */
#define CPLD1_REG_X_PWR_EN_1 0xC
/* Transceiver Power Enable */
#define CPLD1_REG_X_PWR_EN_2 0xD
/* Transceiver Power Good */
#define CPLD1_REG_X_PWR_GOOD_1 0xE
/* Transceiver Power Good */
#define CPLD1_REG_X_PWR_GOOD_2 0xF
/* Watch Dog Timer maximum count setting by seconds */
#define CPLD1_REG_WDT_MAX_COUNT_1 0x22
#define CPLD1_REG_WDT_MAX_COUNT_2 0x23
/* Watch Dog Timer current count value 16 bits */
#define CPLD1_REG_WDT_CUR_COUNT_1 0x24
#define CPLD1_REG_WDT_CUR_COUNT_2 0x25
/* Version as BMC I2C Registers */
#define CPLD1_REG_VER_BMC_I2C_1 0xF0
#define CPLD1_REG_VER_BMC_I2C_2 0xF1
#define CPLD1_REG_VER_BMC_I2C_3 0xF2
#define CPLD1_REG_VER_BMC_I2C_4 0xF3
/* CPLD JED Released Date */
#define CPLD1_REG_JED_REL_MONTH 0xFE
#define CPLD1_REG_JED_REL_DAY   0xFF

/* HW/CPLD version */
#define CPLD2_REG_HWREV 0x2
/* System Ready/Reset Status */
#define CPLD2_REG_SYSRDY_RESET_STATUS 0x3
/* All xcvr LED control */
#define CPLD2_REG_ALL_LED_CTRL 0x4
/* Version as BMC I2C Registers */
#define CPLD2_REG_VER_BMC_I2C_1 0xF0
#define CPLD2_REG_VER_BMC_I2C_2 0xF1
#define CPLD2_REG_VER_BMC_I2C_3 0xF2
#define CPLD2_REG_VER_BMC_I2C_4 0xF3
/* CPLD JED Released Date */
#define CPLD2_REG_JED_REL_MONTH 0xFE
#define CPLD2_REG_JED_REL_DAY   0xFF

#define QSFP_T_RESET_LOf 0x10
#define QSFP_T_RESET_HIf 0x11

#define QSFP_T_LPMODE_LOf 0x12
#define QSFP_T_LPMODE_HIf 0x13

#define QSFP_T_INT_LOf 0x14
#define QSFP_T_INT_HIf 0x15

#define QSFP_T_MODPRS_LOf 0x16
#define QSFP_T_MODPRS_HIf 0x17

#define QSFP_B_RESET_LOf 0x18
#define QSFP_B_RESET_HIf 0x19

#define QSFP_B_LPMODE_LOf 0x1A
#define QSFP_B_LPMODE_HIf 0x1B

#define QSFP_B_INT_LOf 0x1C
#define QSFP_B_INT_HIf 0x1D

#define QSFP_B_MODPRS_LOf 0x1E
#define QSFP_B_MODPRS_HIf 0x1F

#define QSFP_REG_READ(_pbmp_, _reg_, _base_)        \
	do {								            \
		_pbmp_ = readb(_base_ + _reg_##_HIf) |   	\
		         (readb(_base_ + _reg_##_LOf) << 8); \
	} while (0);

#define QSFP_REG_WRITE(_pbmp_, _reg_, _base_)       \
	do {								            \
		writeb((_pbmp_ & 0x00ff),                   \
			_base_ + _reg_##_HIf);  	            \
		writeb(((_pbmp_ >> 8) & 0x00ff),            \
			_base_ + _reg_##_LOf);                  \
	} while (0);

enum PORT_TYPE { NONE, QSFP, SFP };

struct t7132s_cpld {
	struct mutex lock;
	unsigned char __iomem *cpld_base;
	unsigned char __iomem *cpld2_base;
	struct device *sff_devices[SFF_PORT_TOTAL];
	struct i2c_client *sff_i2c_clients[SFF_PORT_TOTAL];
};

static struct t7132s_cpld *cpld_data;
static struct class *cpld_class = NULL;

enum i2c_adapter_type {
	I2C_ADAPTER_I801 = 0,
	I2C_ADAPTER_ISMT,
	I2C_ADAPTER_CP2112,
	I2C_ADAPTER_PCA954X
};

const char *bms_i2c_adapter_names[] = { 
					"SMBus I801 adapter",
					"SMBus iSMT adapter",
					"CP2112 SMBus Bridge", 
					"PCA954X Mux" };

struct i2c_topo_node {
	int adapter_type;
	int parent_index;
	int chan_id;
	struct i2c_board_info entry;
	struct i2c_client *client;
};

static struct i2c_topo_node i2c_topo[] = {
	{ I2C_ADAPTER_CP2112, -1, -1, { I2C_BOARD_INFO("pca9548", 0x70) }, NULL },
	{ I2C_ADAPTER_PCA954X, 0, 0, { I2C_BOARD_INFO("pca9548", 0x71) }, NULL },
	{ I2C_ADAPTER_PCA954X, 0, 2, { I2C_BOARD_INFO("pca9548", 0x72) }, NULL },
	{ I2C_ADAPTER_PCA954X, 0, 1, { I2C_BOARD_INFO("pca9548", 0x73) }, NULL },
	{ I2C_ADAPTER_PCA954X, 0, 3, { I2C_BOARD_INFO("pca9548", 0x74) }, NULL },
	{ I2C_ADAPTER_PCA954X, 0, 4, { I2C_BOARD_INFO("pca9548", 0x75) }, NULL },
	{ I2C_ADAPTER_PCA954X, 5, 3, { I2C_BOARD_INFO("24c64", 0x53) }, NULL },
};

static struct i2c_board_info sff_eeprom_info[] = {
	{ I2C_BOARD_INFO("optoe3", 0x50) },
	{ I2C_BOARD_INFO("optoe2", 0x50) }
};

struct sff_device_data {
	int portid;
	enum PORT_TYPE port_type;
	int parent_index;
	int chan_id;
};

struct sff_device_data sff_device_tbl[SFF_PORT_TOTAL] = {
	{ 1, QSFP, 1, 0 },  { 2, QSFP, 3, 3 },  { 3, QSFP, 1, 1 },
	{ 4, QSFP, 3, 2 },  { 5, QSFP, 1, 2 },  { 6, QSFP, 3, 1 },
	{ 7, QSFP, 1, 3 },  { 8, QSFP, 3, 0 },  { 9, QSFP, 1, 4 },
	{ 10, QSFP, 3, 7 }, { 11, QSFP, 1, 5 }, { 12, QSFP, 3, 6 },
	{ 13, QSFP, 1, 6 }, { 14, QSFP, 3, 5 }, { 15, QSFP, 1, 7 },
	{ 16, QSFP, 3, 4 }, { 17, QSFP, 2, 0 }, { 18, QSFP, 4, 3 },
	{ 19, QSFP, 2, 1 }, { 20, QSFP, 4, 2 }, { 21, QSFP, 2, 2 },
	{ 22, QSFP, 4, 1 }, { 23, QSFP, 2, 3 }, { 24, QSFP, 4, 0 },
	{ 25, QSFP, 2, 4 }, { 26, QSFP, 4, 7 }, { 27, QSFP, 2, 5 },
	{ 28, QSFP, 4, 6 }, { 29, QSFP, 2, 6 }, { 30, QSFP, 4, 5 },
	{ 31, QSFP, 2, 7 }, { 32, QSFP, 4, 4 }, { 1, SFP, 5, 0 },
	{ 2, SFP, 5, 1 },
};

#define WATCHDOG_TIMEOUT 30	/* 30 sec default heartbeat */

static struct watchdog_device *pwddev;

static const struct watchdog_info ident = {
	.options =		WDIOF_SETTIMEOUT |
				WDIOF_KEEPALIVEPING |
				WDIOF_MAGICCLOSE,
	.firmware_version =	0,
    .identity =		"t7132s_wdt",
};

/* used to access CPLD register not defined in this driver with sys filesystem */
uint8_t cpld_testee_offset[2] = {0, 0};

static ssize_t swid_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	uint8_t data = 0;
	int err;
	char *ptr = (cpld_data->cpld_base + CPLD1_REG_SW_ID);

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld_base + CPLD1_REG_SW_ID);
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%2.2x\n", data);
}
struct device_attribute dev_attr_swid = __ATTR(swid, 0400, swid_show, NULL);
  
static ssize_t hwrev_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	uint8_t data = 0;
	int err;
	char *ptr = (cpld_data->cpld_base + CPLD1_REG_HWREV);

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld_base + CPLD1_REG_HWREV);
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%2.2x\n", data);
}
struct device_attribute dev_attr_hwrev = __ATTR(hw_rev, 0400, hwrev_show, NULL);

static ssize_t pwrgood_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	uint8_t data = 0;
	int err;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld_base + CPLD1_REG_PWR_GOOD);
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%2.2x\n", data);
}
struct device_attribute dev_attr_pwrgood =
	__ATTR(pwr_good, 0400, pwrgood_show, NULL);

static ssize_t vrm_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	uint8_t data = 0;
	int err;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld_base + CPLD1_REG_VRM);
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%2.2x\n", data);
}
struct device_attribute dev_attr_vrm = __ATTR(vrm, 0400, vrm_show, NULL);

static ssize_t devstate_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	uint8_t data = 0;
	uint8_t data2 = 0;
	long value = 0;
	int err;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld_base + CPLD1_REG_DEV_STATE_1);
	data2 = readb(cpld_data->cpld_base + CPLD1_REG_DEV_STATE_2);
	value = data;
	value |= (data2 << 8);
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%4.4lx\n", value);
}
static ssize_t devstate_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	ssize_t status = 0;
	long value = 0;
	uint8_t data = 0;
	uint8_t data2 = 0;

	mutex_lock(&cpld_data->lock);
	status = kstrtol(buf, 0, &value);
	if (status == 0) {
		data = value & 0xff;
		data2 = (value & 0xff00) >> 8;
		writeb(data, cpld_data->cpld_base + CPLD1_REG_DEV_STATE_1);
		writeb(data2, cpld_data->cpld_base + CPLD1_REG_DEV_STATE_2);
		status = count;
	}
	mutex_unlock(&cpld_data->lock);

	return status;
}
struct device_attribute dev_attr_devstate =
	__ATTR(dev_state, 0600, devstate_show, devstate_store);

static ssize_t wdtmax_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	uint8_t data1 = 0;
	uint8_t data2 = 0;
	long value = 0;

	mutex_lock(&cpld_data->lock);
	data1 = readb(cpld_data->cpld_base + CPLD1_REG_WDT_MAX_COUNT_1);
	data2 = readb(cpld_data->cpld_base + CPLD1_REG_WDT_MAX_COUNT_2);
	value = (data2 << 8) | data1;
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%4.4lx\n", value);
}
static ssize_t wdtmax_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	ssize_t status = 0;
	long value = 0;
	uint8_t data1;
	uint8_t data2;

	mutex_lock(&cpld_data->lock);
	status = kstrtol(buf, 0, &value);
	if (status == 0) {
		data1 = value & 0xff;
		data2 = (value & 0xff00) >> 8;
		writeb(data1, cpld_data->cpld_base + CPLD1_REG_WDT_MAX_COUNT_1);
		writeb(data2, cpld_data->cpld_base + CPLD1_REG_WDT_MAX_COUNT_2);
		status = count;
	}
	mutex_unlock(&cpld_data->lock);

	return status;
}
struct device_attribute dev_attr_wdtmax =
	__ATTR(wdt_max, 0600, wdtmax_show, wdtmax_store);

static ssize_t wdtcount_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	uint8_t data1 = 0;
	uint8_t data2 = 0;
	long value = 0;

	mutex_lock(&cpld_data->lock);
	data1 = readb(cpld_data->cpld_base + CPLD1_REG_WDT_CUR_COUNT_1);
	data2 = readb(cpld_data->cpld_base + CPLD1_REG_WDT_CUR_COUNT_2);
	value = (data2 << 8) | data1;
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%4.4lx\n", value);
}
struct device_attribute dev_attr_wdtcount =
	__ATTR(wdt_count, 0400, wdtcount_show, NULL);

static ssize_t sysrst_rec_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	uint8_t data = 0;
	int err;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld_base + CPLD1_REG_SYS_RESET_REC);
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%2.2x\n", data);
}
static ssize_t sysrst_rec_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	ssize_t status = 0;
	uint8_t data;

	mutex_lock(&cpld_data->lock);
	status = kstrtou8(buf, 0, &data);

	if (status == 0) {
		writeb(data, cpld_data->cpld_base + CPLD1_REG_SYS_RESET_REC);
		status = count;
	}
	mutex_unlock(&cpld_data->lock);

	return status;
}
struct device_attribute dev_attr_sysrst_rec =
	__ATTR(sysrst_rec, 0600, sysrst_rec_show, sysrst_rec_store);

static ssize_t muxstate_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	uint8_t data = 0;
	int err;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld_base + CPLD1_REG_MUX_STATE);
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%2.2x\n", data);
}
static ssize_t muxstate_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	ssize_t status = 0;
	uint8_t data;

	mutex_lock(&cpld_data->lock);
	status = kstrtou8(buf, 0, &data);
	if (status == 0) {
		writeb(data, cpld_data->cpld_base + CPLD1_REG_MUX_STATE);
		status = count;
	}
	mutex_unlock(&cpld_data->lock);

	return status;
}
struct device_attribute dev_attr_muxstate =
	__ATTR(i2c_mux_state, 0600, muxstate_show, muxstate_store);

static ssize_t xcvr_pwrstate_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	uint8_t data = 0;
	uint8_t data2 = 0;
	long value = 0;
	int err;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld_base + CPLD1_REG_X_PWR_EN_2);
	data2 = readb(cpld_data->cpld_base + CPLD1_REG_X_PWR_EN_1);
	value = data;
	value |= (data2 << 8);
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%2.2x\n", value);
}
static ssize_t xcvr_pwrstate_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	ssize_t status = 0;
	long value = 0;
	uint8_t data = 0;
	uint8_t data2 = 0;

	mutex_lock(&cpld_data->lock);
	status = kstrtol(buf, 0, &value);
	if (status == 0) {
		data = value & 0xff;
		data2 = (value & 0xff00) >> 8;
		writeb(data, cpld_data->cpld_base + CPLD1_REG_X_PWR_EN_2);
		writeb(data2, cpld_data->cpld_base + CPLD1_REG_X_PWR_EN_1);
		status = count;
	}
	mutex_unlock(&cpld_data->lock);

	return status;
}
struct device_attribute dev_attr_xcvr_pwrstate =
	__ATTR(xcvr_pwr_state, 0600, xcvr_pwrstate_show, xcvr_pwrstate_store);

static ssize_t xcvr_pwrgood_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	uint8_t data = 0;
	uint8_t data2 = 0;
	long value = 0;
	int err;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld_base + CPLD1_REG_X_PWR_GOOD_2);
	data2 = readb(cpld_data->cpld_base + CPLD1_REG_X_PWR_GOOD_1);
	value = data;
	value |= (data2 << 8);
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%2.2x\n", value);
}
struct device_attribute dev_attr_xcvr_pwrgood =
	__ATTR(xcvr_pwr_good, 0400, xcvr_pwrgood_show, NULL);

static ssize_t cpld1_ver_bmc_i2c_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	uint8_t data = 0;
	uint8_t data2 = 0;
	uint8_t data3 = 0;
	uint8_t data4 = 0;
	long value = 0;
	int err;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld_base + CPLD1_REG_VER_BMC_I2C_4);
	data2 = readb(cpld_data->cpld_base + CPLD1_REG_VER_BMC_I2C_3);
	data3 = readb(cpld_data->cpld_base + CPLD1_REG_VER_BMC_I2C_2);
	data4 = readb(cpld_data->cpld_base + CPLD1_REG_VER_BMC_I2C_1);
	value = data;
	value |= (data2 << 8);
	value |= (data3 << 16);
	value |= (data4 << 24);
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%8.8lx\n", value);
}
struct device_attribute dev_attr_cpld1_ver_bmc_i2c =
	__ATTR(ver_bmc_i2c, 0444, cpld1_ver_bmc_i2c_show, NULL);

static ssize_t cpld1_jed_rel_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	uint8_t data = 0;
	uint8_t data2 = 0;
	long value = 0;
	int err;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld_base + CPLD1_REG_JED_REL_DAY);
	data2 = readb(cpld_data->cpld_base + CPLD1_REG_JED_REL_MONTH);
	value = data;
	value |= (data2 << 8);
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%4.4lx\n", value);
}
struct device_attribute dev_attr_cpld1_jed_rel =
	__ATTR(jed_rel, 0400, cpld1_jed_rel_show, NULL);

static ssize_t cpld1_testee_offset_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	return sprintf(buf, "0x%2.2x\n", cpld_testee_offset[0]);
}
static ssize_t cpld1_testee_offset_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	ssize_t status = 0;
	uint8_t data;

	status = kstrtou8(buf, 0, &data);
	if (status == 0) {
		cpld_testee_offset[0] = data;
		status = count;
	}

	return status;
}
struct device_attribute dev_attr_cpld1_testee_offset =
	__ATTR(testee_offset, 0600, cpld1_testee_offset_show, cpld1_testee_offset_store);

static ssize_t cpld1_testee_value_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	uint8_t data = 0;
	int err;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld_base + cpld_testee_offset[0]);
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%2.2x\n", data);
}
static ssize_t cpld1_testee_value_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	ssize_t status = 0;
	uint8_t data;

	mutex_lock(&cpld_data->lock);
	status = kstrtou8(buf, 0, &data);
	if (status == 0) {
		writeb(data, cpld_data->cpld_base + cpld_testee_offset[0]);
		status = count;
	}
	mutex_unlock(&cpld_data->lock);

	return status;
}
struct device_attribute dev_attr_cpld1_testee_value =
	__ATTR(testee_value, 0600, cpld1_testee_value_show, cpld1_testee_value_store);

static struct attribute *cpld1_attrs[] = {
	&dev_attr_swid.attr,
	&dev_attr_hwrev.attr,
	&dev_attr_pwrgood.attr,
	&dev_attr_vrm.attr,
	&dev_attr_devstate.attr,
	&dev_attr_sysrst_rec.attr,
	&dev_attr_muxstate.attr,
	&dev_attr_xcvr_pwrstate.attr,
	&dev_attr_xcvr_pwrgood.attr,
	&dev_attr_wdtmax.attr,
	&dev_attr_wdtcount.attr,
	&dev_attr_cpld1_ver_bmc_i2c.attr,
	&dev_attr_cpld1_jed_rel.attr,
	&dev_attr_cpld1_testee_offset.attr,
	&dev_attr_cpld1_testee_value.attr,
	NULL,
};

static struct attribute_group cpld1_attr_grp = {
	.attrs = cpld1_attrs,
};

static ssize_t cpld2_ver_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	uint8_t data = 0;
	int err;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld2_base + CPLD2_REG_HWREV);
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%2.2x\n", data);
}
struct device_attribute dev_attr_cpld2_ver = __ATTR(cpld2_ver, 0400, cpld2_ver_show, NULL);

static ssize_t sysrdy_rst_status_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	uint8_t data = 0;
	long value = 0;
	int err;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld2_base + 
		CPLD2_REG_SYSRDY_RESET_STATUS);
	value = data;
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%2.2x\n", value);
}
static ssize_t sysrdy_rst_status_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	ssize_t status = 0;
	long value = 0;
	uint8_t data = 0;

	mutex_lock(&cpld_data->lock);
	status = kstrtol(buf, 0, &value);
	if (status == 0) {
		data = value & 0xff;
		writeb(data, cpld_data->cpld2_base + 
			CPLD2_REG_SYSRDY_RESET_STATUS);
		status = count;
	}
	mutex_unlock(&cpld_data->lock);

	return status;
}
struct device_attribute dev_attr_sysrdy_rst_status =
	__ATTR(sysrdy_rst_state, 0600, sysrdy_rst_status_show, 
		sysrdy_rst_status_store);

static ssize_t all_xcvr_led_ctrl_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	uint8_t data = 0;
	long value = 0;
	int err;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld2_base + 
		CPLD2_REG_ALL_LED_CTRL);
	value = data;
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%2.2x\n", value);
}
static ssize_t all_xcvr_led_ctrl_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	ssize_t status = 0;
	long value = 0;
	uint8_t data = 0;

	mutex_lock(&cpld_data->lock);
	status = kstrtol(buf, 0, &value);
	if (status == 0) {
		data = value & 0x1f;
		writeb(data, cpld_data->cpld2_base + 
			CPLD2_REG_ALL_LED_CTRL);
		status = count;
	}
	mutex_unlock(&cpld_data->lock);

	return status;
}
struct device_attribute dev_attr_all_xcvr_led_ctrl =
	__ATTR(all_xcvr_led_ctrl, 0600, all_xcvr_led_ctrl_show, 
		all_xcvr_led_ctrl_store);

static ssize_t cpld2_ver_bmc_i2c_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	uint8_t data = 0;
	uint8_t data2 = 0;
	uint8_t data3 = 0;
	uint8_t data4 = 0;
	long value = 0;
	int err;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld2_base + CPLD2_REG_VER_BMC_I2C_4);
	data2 = readb(cpld_data->cpld2_base + CPLD2_REG_VER_BMC_I2C_3);
	data3 = readb(cpld_data->cpld2_base + CPLD2_REG_VER_BMC_I2C_2);
	data4 = readb(cpld_data->cpld2_base + CPLD2_REG_VER_BMC_I2C_1);
	value = data;
	value |= (data2 << 8);
	value |= (data3 << 16);
	value |= (data4 << 24);
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%8.8lx\n", value);
}
struct device_attribute dev_attr_cpld2_ver_bmc_i2c =
	__ATTR(ver_bmc_i2c, 0400, cpld2_ver_bmc_i2c_show, NULL);

static ssize_t cpld2_jed_rel_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	uint8_t data = 0;
	uint8_t data2 = 0;
	long value = 0;
	int err;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld2_base + CPLD2_REG_JED_REL_DAY);
	data2 = readb(cpld_data->cpld2_base + CPLD2_REG_JED_REL_MONTH);
	value = data;
	value |= (data2 << 8);
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%4.4lx\n", value);
}
struct device_attribute dev_attr_cpld2_jed_rel =
	__ATTR(jed_rel, 0400, cpld2_jed_rel_show, NULL);

static ssize_t cpld2_testee_offset_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	return sprintf(buf, "0x%2.2x\n", cpld_testee_offset[1]);
}
static ssize_t cpld2_testee_offset_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	ssize_t status = 0;
	uint8_t data;

	status = kstrtou8(buf, 0, &data);
	if (status == 0) {
		cpld_testee_offset[1] = data;
		status = count;
	}

	return status;
}
struct device_attribute dev_attr_cpld2_testee_offset =
	__ATTR(testee_offset, 0600, cpld2_testee_offset_show, cpld2_testee_offset_store);

static ssize_t cpld2_testee_value_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	uint8_t data = 0;
	int err;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld2_base + cpld_testee_offset[1]);
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "0x%2.2x\n", data);
}
static ssize_t cpld2_testee_value_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	ssize_t status = 0;
	uint8_t data;

	mutex_lock(&cpld_data->lock);
	status = kstrtou8(buf, 0, &data);
	if (status == 0) {
		writeb(data, cpld_data->cpld2_base + cpld_testee_offset[1]);
		status = count;
	}
	mutex_unlock(&cpld_data->lock);

	return status;
}
struct device_attribute dev_attr_cpld2_testee_value =
	__ATTR(testee_value, 0600, cpld2_testee_value_show, cpld2_testee_value_store);

static struct attribute *cpld2_attrs[] = {
	&dev_attr_cpld2_ver.attr,
	&dev_attr_sysrdy_rst_status.attr,
	&dev_attr_all_xcvr_led_ctrl.attr,
	&dev_attr_cpld2_ver_bmc_i2c.attr,
	&dev_attr_cpld2_jed_rel.attr,
	&dev_attr_cpld2_testee_offset.attr,
	&dev_attr_cpld2_testee_value.attr,
	NULL,
};

static struct attribute_group cpld2_attr_grp = {
	.attrs = cpld2_attrs,
};

struct t7132s {
	unsigned char __iomem *cfg_mmio_start;
	resource_size_t cfg_mmio_len;
	unsigned char __iomem *dev_mmio_start;
	resource_size_t dev_mmio_len;
	unsigned char __iomem *dev2_mmio_start;
	resource_size_t dev2_mmio_len;
};

static struct t7132s t7132s_dev;
static struct platform_device *t7132s_platform_dev;
static struct kobject *cpld1 = NULL;
static struct kobject *cpld2 = NULL;
static struct device *sff_dev = NULL;

static ssize_t qsfp_modirq_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	u8 index = 0;
	u16 reg = 0;
	u8 value = 0;

	mutex_lock(&cpld_data->lock);
	if ((portid % 2) != 0) {
		QSFP_REG_READ (reg, QSFP_T_INT, 
			cpld_data->cpld_base);
		index = (portid + 1) / 2;
	} else {
		QSFP_REG_READ (reg, QSFP_B_INT, 
			cpld_data->cpld_base);
		index = portid / 2;
	}
	value = reg >> (index - 1) & 1;
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "%d\n", value);
}
DEVICE_ATTR_RO(qsfp_modirq);

static ssize_t qsfp_modprs_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	u8 index = 0;
	u16 reg = 0;
	u8 value = 0;

	mutex_lock(&cpld_data->lock);
	if ((portid % 2) != 0) {
		QSFP_REG_READ (reg, QSFP_T_MODPRS, 
			cpld_data->cpld_base);
		index = (portid + 1) / 2;
	} else {
		QSFP_REG_READ (reg, QSFP_B_MODPRS, 
			cpld_data->cpld_base);
		index = portid / 2;
	}
	value = reg >> (index - 1) & 1;
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "%d\n", value);
}
DEVICE_ATTR_RO(qsfp_modprs);

static ssize_t qsfp_lpmode_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	u8 index = 0;
	u16 reg = 0;
	u8 value = 0;

	mutex_lock(&cpld_data->lock);
	if ((portid % 2) != 0) {
		QSFP_REG_READ (reg, QSFP_T_LPMODE, 
			cpld_data->cpld_base);
		index = (portid + 1) / 2;
	} else {
		QSFP_REG_READ (reg, QSFP_B_LPMODE, 
			cpld_data->cpld_base);
		index = portid / 2;
	}
	value = reg >> (index - 1) & 1;
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "%d\n", value);
}
static ssize_t qsfp_lpmode_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	ssize_t status;
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	u8 value = 0;
	u8 index = 0;
	u16 reg = 0;

	mutex_lock(&cpld_data->lock);
	status = kstrtou8(buf, 0, &value);

	if ((status == 0) && (value <= 1)) {
		if ((portid % 2) != 0) {
			QSFP_REG_READ (reg, QSFP_T_LPMODE, 
				cpld_data->cpld_base);
			index = (portid + 1) / 2;
			if (value == 1) {
				reg |= (1 << (index - 1));
			} else {
				reg &= ~(1 << (index - 1));
			}
			QSFP_REG_WRITE (reg, QSFP_T_LPMODE,
				cpld_data->cpld_base);
		} else {
			QSFP_REG_READ (reg, QSFP_B_LPMODE, 
				cpld_data->cpld_base);
			index = portid / 2;
			if (value == 1) {
				reg |= (1 << (index - 1));
			} else {
				reg &= ~(1 << (index - 1));
			}
			QSFP_REG_WRITE (reg, QSFP_B_LPMODE,
				cpld_data->cpld_base);
		}
		status = count;
	}

	mutex_unlock(&cpld_data->lock);

	return status;
}
DEVICE_ATTR_RW(qsfp_lpmode);

static ssize_t qsfp_reset_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	u8 index = 0;
	u16 reg = 0;
	u8 value = 0;

	mutex_lock(&cpld_data->lock);
	if ((portid % 2) != 0) {
		QSFP_REG_READ (reg, QSFP_T_RESET, 
			cpld_data->cpld_base);
		index = (portid + 1) / 2;
	} else {
		QSFP_REG_READ (reg, QSFP_B_RESET, 
			cpld_data->cpld_base);
		index = portid / 2;
	}
	value = reg >> (index - 1) & 1;
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "%d\n", value);
}

static ssize_t qsfp_reset_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	ssize_t status;
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	u8 value = 0;
	u8 index = 0;
	u16 reg = 0;

	mutex_lock(&cpld_data->lock);
	status = kstrtou8(buf, 0, &value);

	if ((status == 0) && (value <= 1)) {
		if ((portid % 2) != 0) {
			QSFP_REG_READ (reg, QSFP_T_RESET, 
				cpld_data->cpld_base);
			index = (portid + 1) / 2;
			if (value == 1) {
				reg |= (1 << (index - 1));
			} else {
				reg &= ~(1 << (index - 1));
			}
			QSFP_REG_WRITE (reg, QSFP_T_RESET,
				cpld_data->cpld_base);
		} else {
			QSFP_REG_READ (reg, QSFP_B_RESET, 
				cpld_data->cpld_base);
			index = portid / 2;
			if (value == 1) {
				reg |= (1 << (index - 1));
			} else {
				reg &= ~(1 << (index - 1));
			}
			QSFP_REG_WRITE (reg, QSFP_B_RESET,
				cpld_data->cpld_base);
		}
		status = count;
	}
	mutex_unlock(&cpld_data->lock);

	return status;
}
DEVICE_ATTR_RW(qsfp_reset);

static struct attribute *sff_attrs[] = {
	&dev_attr_qsfp_modprs.attr,
	&dev_attr_qsfp_modirq.attr,
	&dev_attr_qsfp_lpmode.attr,
	&dev_attr_qsfp_reset.attr,
	NULL,
};
static struct attribute_group sff_attr_grp = {
	.attrs = sff_attrs,
};
static const struct attribute_group *sff_attr_grps[] = { &sff_attr_grp, NULL };

#define SFP_MOD_MASK (1 << 5)
#define SFP_LOS_MASK (1 << 4)
#define SFP_TXFAULT_MASK (1 << 3)
#define SFP_RS0_MASK (1 << 2)
#define SFP_RS1_MASK (1 << 1)
#define SFP_TXDISABLE_MASK (1)
#define CPLD1_SFP_OFFSET 0x20
#define SFP_REG_READ(_portid_, _data_, _base_)						\
	do {															\
		_data_ = readb(_base_ + CPLD1_SFP_OFFSET + _portid_ - 1);	\
	} while (0);
#define SFP_REG_WRITE(_portid_, _data_, _base_)						\
	do {															\
		writeb(_data_, _base_ + CPLD1_SFP_OFFSET + _portid_ - 1);	\
	} while (0);

static ssize_t sfp_modabs_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	u8 value = 0;

	mutex_lock(&cpld_data->lock);
	SFP_REG_READ (portid, value, cpld_data->cpld_base);	
	value = ((value & SFP_MOD_MASK) != 0) ? 1 : 0;
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "%d\n", value);
}
DEVICE_ATTR_RO(sfp_modabs);

static ssize_t sfp_txfault_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	u8 value = 0;

	mutex_lock(&cpld_data->lock);
	SFP_REG_READ (portid, value, cpld_data->cpld_base);	
	value = ((value & SFP_TXFAULT_MASK) != 0) ? 1 : 0;
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "%d\n", value);
}
DEVICE_ATTR_RO(sfp_txfault);

static ssize_t sfp_rxlos_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	u8 index = 0;
	u16 reg = 0;
	u8 value = 0;

	mutex_lock(&cpld_data->lock);
	SFP_REG_READ (portid, value, cpld_data->cpld_base);	
	value = ((value & SFP_LOS_MASK) != 0) ? 1 : 0;
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "%d\n", value);
}
DEVICE_ATTR_RO(sfp_rxlos);

static ssize_t sfp_txdisable_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	u8 value = 0;

	mutex_lock(&cpld_data->lock);
	SFP_REG_READ (portid, value, cpld_data->cpld_base);	
	value = ((value & SFP_TXDISABLE_MASK) != 0) ? 1 : 0;
	mutex_unlock(&cpld_data->lock);

	return sprintf(buf, "%d\n", value);
}

static ssize_t sfp_txdisable_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	ssize_t status;
	struct sff_device_data *dev_data = dev_get_drvdata(dev);
	unsigned int portid = dev_data->portid;
	u8 value = 0;
	u8 index = 0;
	u8 reg = 0;

	mutex_lock(&cpld_data->lock);
	status = kstrtou8(buf, 0, &value);

	if ((status == 0) && value <= 1) {
		SFP_REG_READ (portid, reg, cpld_data->cpld_base);	
		if (value == 1) {
			reg |= 1;
		} else {
			reg &= ~1;
		}
		SFP_REG_WRITE (portid, reg, cpld_data->cpld_base);
 		status = count;
	} else {
		status = -EINVAL;
	}

	mutex_unlock(&cpld_data->lock);

	return status;
}
DEVICE_ATTR_RW(sfp_txdisable);

static struct attribute *sff_sfp_attrs[] = {
    &dev_attr_sfp_txfault.attr,
    &dev_attr_sfp_rxlos.attr,
    &dev_attr_sfp_modabs.attr,
    &dev_attr_sfp_txdisable.attr,
	NULL,
};
static struct attribute_group sff_sfp_attr_grp = {
	.attrs = sff_sfp_attrs,
};
static const struct attribute_group *sff_sfp_attr_grps[] = 
	{ &sff_sfp_attr_grp, NULL };

static struct device *t7132s_sff_init(int portid)
{
	struct sff_device_data *new_data;
	struct device *new_device;
	char tmpStr[20];

	new_data = kzalloc(sizeof(*new_data), GFP_KERNEL);

	new_data->portid = sff_device_tbl[portid].portid;
	new_data->port_type = sff_device_tbl[portid].port_type;
	new_data->parent_index = sff_device_tbl[portid].parent_index;
	new_data->chan_id = sff_device_tbl[portid].chan_id;

	if (sff_device_tbl[portid].port_type == QSFP) {
		sprintf(tmpStr, "QSFP%d", new_data->portid);
		new_device = device_create_with_groups(cpld_class, sff_dev, MKDEV(0, 0),
						       new_data, sff_attr_grps, "%s",
						       tmpStr);
	} else {
		sprintf(tmpStr, "SFP%d", new_data->portid);
		new_device = device_create_with_groups(cpld_class, sff_dev, MKDEV(0, 0),
						       new_data, sff_sfp_attr_grps, "%s",
						       tmpStr);
	}

	return new_device;
}

static void t7132s_sff_deinit(int portid)
{
	struct sff_device_data *dev_data;

	dev_data = dev_get_drvdata(cpld_data->sff_devices[portid]);
	device_unregister(cpld_data->sff_devices[portid]);
	put_device(cpld_data->sff_devices[portid]);
	kfree(dev_data);

	return;
}

static int t7132s_wdt_start(struct watchdog_device *wd_dev)
{
	uint8_t data = 0;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld_base + CPLD1_REG_DEV_STATE_2);
	data |= 0x01;
	writeb(data, cpld_data->cpld_base + CPLD1_REG_DEV_STATE_2);
	mutex_unlock(&cpld_data->lock);
	return 0;
}

static int t7132s_wdt_stop(struct watchdog_device *wd_dev)
{
	uint8_t data = 0;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld_base + CPLD1_REG_DEV_STATE_2);
	data &= 0xfe;
	writeb(data, cpld_data->cpld_base + CPLD1_REG_DEV_STATE_2);
	mutex_unlock(&cpld_data->lock);
	return 0;
}

static int t7132s_wdt_ping(struct watchdog_device *wd_dev)
{
	uint8_t data = 0;

	mutex_lock(&cpld_data->lock);
	data = readb(cpld_data->cpld_base + CPLD1_REG_DEV_STATE_2);
	/* disable */
	data &= 0xfe;
	writeb(data, cpld_data->cpld_base + CPLD1_REG_DEV_STATE_2);
	/* enable */
	data |= 0x01;
	writeb(data, cpld_data->cpld_base + CPLD1_REG_DEV_STATE_2);
	mutex_unlock(&cpld_data->lock);
	return 0;
}

static int t7132s_wdt_set_timeout(struct watchdog_device *wd_dev, unsigned int t)
{
	uint8_t data1 = 0;
	uint8_t data2 = 0;
	uint8_t data = 0;
	int is_enabled = 0;

	if (t > 65535)
		return -EINVAL;

	data1 = t & 0xff;
	data2 = (t & 0xff00) >> 8;
	mutex_lock(&cpld_data->lock);
	/* save and stop */
	data = readb(cpld_data->cpld_base + CPLD1_REG_DEV_STATE_2);
	is_enabled = data & 0x01;
	if (is_enabled != 0) {
		data &= 0xfe;
		writeb(data, cpld_data->cpld_base + CPLD1_REG_DEV_STATE_2);
	}
	/* update max */
	writeb(data1, cpld_data->cpld_base + CPLD1_REG_WDT_MAX_COUNT_1);
	writeb(data2, cpld_data->cpld_base + CPLD1_REG_WDT_MAX_COUNT_2);
	/* restore */
	if (is_enabled != 0) {
		data = readb(cpld_data->cpld_base + CPLD1_REG_DEV_STATE_2);
		data |= is_enabled;
		writeb(data, cpld_data->cpld_base + CPLD1_REG_DEV_STATE_2);
	}
	mutex_unlock(&cpld_data->lock);

	wd_dev->timeout = t;
	return 0;
}

static unsigned int t7132s_wdt_get_timeleft(struct watchdog_device *wd_dev)
{
	unsigned int time_left = 0;
	uint8_t data_max1 = 0;
	uint8_t data_max2 = 0;
	long time_max = 0;
	uint8_t data_now1 = 0;
	uint8_t data_now2 = 0;
	long time_now = 0;

	mutex_lock(&cpld_data->lock);
	data_max1 = readb(cpld_data->cpld_base + CPLD1_REG_WDT_MAX_COUNT_1);
	data_max2 = readb(cpld_data->cpld_base + CPLD1_REG_WDT_MAX_COUNT_2);
	data_now1 = readb(cpld_data->cpld_base + CPLD1_REG_WDT_CUR_COUNT_1);
	data_now2 = readb(cpld_data->cpld_base + CPLD1_REG_WDT_CUR_COUNT_2);
	mutex_unlock(&cpld_data->lock);

	/* our watchdog is counting up */
	time_max = (data_max2 << 8) | data_max1;
	time_now = (data_now2 << 8) | data_now1;
	if (time_max >= time_now) {
		time_left = time_max - time_now;
	}
	else
	{
		/* for debug */
		time_left = 0;
		printk("T7132S: t7132s_wdt_get_timeleft time_max=0x%04lx time_now=0x%04lx\n",
				time_max, time_now);
	}

	return time_left;

}

static const struct watchdog_ops t7132s_wdt_ops = {
	.owner =		THIS_MODULE,
	.start =		t7132s_wdt_start,
	.stop =			t7132s_wdt_stop,
	.ping =			t7132s_wdt_ping,
	.set_timeout =		t7132s_wdt_set_timeout,
	.get_timeleft =		t7132s_wdt_get_timeleft,
};

static int __init __find_i2c_adap(struct device *dev, const void *data)
{
	const char *name = data;
	static const char *prefix = "i2c-";
	struct i2c_adapter *adapter;

	if (strncmp(dev_name(dev), prefix, strlen(prefix)) != 0) {
		return 0;
	}
	adapter = to_i2c_adapter(dev);

	return (strncmp(adapter->name, name, strlen(name)) == 0);
}

static int t7132s_drv_probe(struct platform_device *pdev)
{
	int ret = 0;
	int port_count = 0;
	struct sff_device_data *sff_data = NULL;
	const char *name = NULL;
	struct device *dev = NULL;
	struct i2c_adapter *adapter = NULL;
	struct i2c_board_info *entry = NULL;
	struct i2c_client *client = NULL;
	struct i2c_mux_core *muxc = NULL;
	int index = 0;
	int parent_idx = 0;
	int max_index = sizeof(i2c_topo) / sizeof(struct i2c_topo_node);
	int adapter_id = 0;
	int chan_id = 0;

	cpld_class = class_create(THIS_MODULE, CLASS_NAME);
	ret = PTR_ERR(cpld_class);

	cpld_data = devm_kzalloc(&pdev->dev, sizeof(struct t7132s_cpld),
				 GFP_KERNEL);
	mutex_init(&cpld_data->lock);
	cpld_data->cpld_base = t7132s_dev.dev_mmio_start;
	cpld1 = kobject_create_and_add("CPLD1", &pdev->dev.kobj);
	ret = sysfs_create_group(cpld1, &cpld1_attr_grp);
	cpld_data->cpld2_base = t7132s_dev.dev2_mmio_start;
	cpld2 = kobject_create_and_add("CPLD2", &pdev->dev.kobj);
	ret = sysfs_create_group(cpld2, &cpld2_attr_grp);

	sff_dev = device_create(cpld_class, NULL, MKDEV(0, 0), NULL, "%s",
				"sff_device");
	ret = sysfs_create_link(&pdev->dev.kobj, &sff_dev->kobj, "SFF");

	/* Setup i2c MUX devices */
	for (index = 0; index < max_index; index++) {
		if (i2c_topo[index].parent_index == -1) {
			name = bms_i2c_adapter_names[i2c_topo[index]
							     .adapter_type];
			dev = bus_find_device(&i2c_bus_type, NULL, (void *)name,
					      __find_i2c_adap);
			adapter = to_i2c_adapter(dev);
			if (adapter != NULL) {
				i2c_topo[index].client = i2c_new_client_device(
					adapter, &i2c_topo[index].entry);
				msleep(500);
			}
		} else if (i2c_topo[index].parent_index != -1) {
			parent_idx = i2c_topo[index].parent_index;
			if (i2c_topo[parent_idx].client != NULL) {
				client = i2c_topo[parent_idx].client;
				muxc = i2c_get_clientdata(client);
				if (muxc != NULL) {
					chan_id = i2c_topo[index].chan_id;
					adapter_id = muxc->adapter[chan_id]->nr;
					adapter = i2c_get_adapter(adapter_id);
					if (adapter != NULL) {
						i2c_topo[index]
							.client = i2c_new_client_device(
							adapter,
							&i2c_topo[index].entry);
						i2c_put_adapter(adapter);
						msleep(100);
					}
				}
			}
		}
		dev = NULL;
		adapter = NULL;
	}

	for (port_count = 0; port_count < SFF_PORT_TOTAL; port_count++) {
		cpld_data->sff_devices[port_count] =
			t7132s_sff_init(port_count);
		sff_data = dev_get_drvdata(cpld_data->sff_devices[port_count]);
		parent_idx = sff_data->parent_index;
		chan_id = sff_data->chan_id;
		muxc = i2c_get_clientdata(i2c_topo[parent_idx].client);
		if (muxc != NULL) {
			adapter_id = muxc->adapter[chan_id]->nr;
			adapter = i2c_get_adapter(adapter_id);
		}
		if (adapter == NULL)
			continue;
		if (sff_data->port_type == QSFP) {
			/* Initiate optoe1 device */
			cpld_data->sff_i2c_clients[port_count] =
				i2c_new_client_device(adapter, &sff_eeprom_info[0]);
		} else {
			/* Initiate optoe2 device */
			cpld_data->sff_i2c_clients[port_count] =
				i2c_new_client_device(adapter, &sff_eeprom_info[1]);
		}
		i2c_put_adapter(adapter);
		sff_data = NULL;
		adapter = NULL;
		/* Create sysfs link */
		sysfs_create_link(
			&cpld_data->sff_devices[port_count]->kobj,
			&cpld_data->sff_i2c_clients[port_count]->dev.kobj,
			"i2c");
	}

	/* watchdog */
	pwddev = devm_kzalloc(&pdev->dev, sizeof(*pwddev), GFP_KERNEL);
	if (pwddev) {
		pwddev->info = &ident,
		pwddev->ops = &t7132s_wdt_ops,
		pwddev->bootstatus = 0;
		pwddev->timeout = WATCHDOG_TIMEOUT;
		pwddev->parent = &pdev->dev;
		pwddev->min_timeout = 1;
		pwddev->max_timeout = 65535;
		//pwddev->max_hw_heartbeat_ms = 65535 * 1000;

		ret = devm_watchdog_register_device(&pdev->dev, pwddev);
		if (ret != 0) {
			printk("T7132S: cannot register watchdog device (err=%d)\n", ret);
		}
		else {
			t7132s_wdt_stop(pwddev);
			t7132s_wdt_set_timeout(pwddev, WATCHDOG_TIMEOUT);
			printk("T7132S: watchdog initialized. heartbeat=%d sec (nowayout=%d)\n",
					pwddev->timeout, 0);
		}
	}
	else {
		printk("T7132S: devm_kzalloc fail for watchdog device\n");
	}

	return 0;
}

static int t7132s_drv_remove(struct platform_device *pdev)
{
	int ret = 0;
	int port_count = 0;
	struct sff_device_data *sff_data = NULL;
	const char *name = NULL;
	struct device *dev = NULL;
	struct i2c_adapter *adapter;
	struct i2c_board_info *entry;
	int index = 0;
	int max_index = sizeof(i2c_topo) / sizeof(struct i2c_topo_node);

	for (port_count = 0; port_count < SFF_PORT_TOTAL; port_count++) {
		sysfs_remove_link(&cpld_data->sff_devices[port_count]->kobj,
				  "i2c");
		if (cpld_data->sff_i2c_clients[port_count] != NULL) {
			i2c_unregister_device(
				cpld_data->sff_i2c_clients[port_count]);
		}
	}

	for (port_count = 0; port_count < SFF_PORT_TOTAL; port_count++) {
		t7132s_sff_deinit(port_count);
	}

	for (index = max_index - 1; index >= 0; index--) {
		if (i2c_topo[index].client != NULL) {
			i2c_unregister_device(i2c_topo[index].client);
		}
	}

	sysfs_remove_link(&pdev->dev.kobj, "SFF");
	device_destroy(cpld_class, MKDEV(0, 0));
	sysfs_remove_group(cpld1, &cpld1_attr_grp);
	class_destroy(cpld_class); // remove the device class
	cpld_class = NULL;
	devm_kfree(&pdev->dev, cpld_data);
	devm_kfree(&pdev->dev, pwddev);

	return 0;
}

static struct platform_driver t7132s_drv = {
	.probe = t7132s_drv_probe,
	.remove = __exit_p(t7132s_drv_remove),
	.driver =
		{
			.name = DRIVER_NAME,
			.owner = THIS_MODULE,
		},
};

static void t7132s_remove(struct pci_dev *dev)
{
	platform_device_unregister(t7132s_platform_dev);
	platform_driver_unregister(&t7132s_drv);
	iounmap(t7132s_dev.cfg_mmio_start);
	iounmap(t7132s_dev.dev_mmio_start);
	iounmap(t7132s_dev.dev2_mmio_start);

	pci_disable_device(dev);
}

static int t7132s_probe(struct pci_dev *dev, const struct pci_device_id *ent)
{
	int retval, ret;
	unsigned long base;
	resource_size_t len;
	unsigned long val;
	char *ptr = NULL;

	retval = pci_enable_device(dev);
	pci_set_master(dev);

	len = pci_resource_len(dev, 5);
	base = pci_resource_start(dev, 5);
	t7132s_dev.cfg_mmio_start = ioremap(base, len);
	t7132s_dev.cfg_mmio_len = len;

	len = pci_resource_len(dev, 0);
	base = pci_resource_start(dev, 0);
	t7132s_dev.dev_mmio_start = ioremap(base, len);
	t7132s_dev.dev_mmio_len = len;

	len = pci_resource_len(dev, 1);
	base = pci_resource_start(dev, 1);
	t7132s_dev.dev2_mmio_start = ioremap(base, len);
	t7132s_dev.dev2_mmio_len = len;

	/* Localbus bridge reset sequence */
	val = readl(t7132s_dev.cfg_mmio_start + REG_LIEMR);
	printk("LIEMR = 0x%x", val);
	writeb(val | LIEMR_SRST | LIEMR_LRST,
	       t7132s_dev.cfg_mmio_start + REG_LIEMR);
	val = readl(t7132s_dev.cfg_mmio_start + REG_LIEMR);
	printk("LIEMR(after hw reset) = 0x%x", val);

	/* Localbus init sequence */
	val = LIEMR_RDYPOL | LIEMR_ALEPOL | LIEMR_SYNCBUS | LIEMR_MULTBUS |
	      LIEMR_DMA0EN | LIEMR_DMA1EN | LIEMR_L0EINTEN | LIEMR_L0RTOIEN |
	      LIEMR_L1EINTEN | LIEMR_L1RTOIEN | LIEMR_D0DIEN | LIEMR_D0AIEN |
	      LIEMR_D1DIEN | LIEMR_D1AIEN;
	printk("LIEMR(new) = 0x%x", val);
	writel(val, t7132s_dev.cfg_mmio_start + REG_LIEMR);
	val = readl(t7132s_dev.cfg_mmio_start + REG_LIEMR);
	printk("LIEMR(modified) = 0x%x", val);

	val = readl(t7132s_dev.cfg_mmio_start + REG_LAS0CFGR);
	val &= ~LASCFGR_BW16;
	writel(val, t7132s_dev.cfg_mmio_start + REG_LAS0CFGR);
	val = readl(t7132s_dev.cfg_mmio_start + REG_LAS0CFGR); /* flush */
	
	val = readl(t7132s_dev.cfg_mmio_start + REG_LAS1CFGR);
	val &= ~LASCFGR_BW16;
	writel(val, t7132s_dev.cfg_mmio_start + REG_LAS1CFGR);
	val = readl(t7132s_dev.cfg_mmio_start + REG_LAS1CFGR); /* flush */

	val = readl(t7132s_dev.cfg_mmio_start + REG_LACKSR);
	printk("LACKSR = 0x%x", val);
	val &= ~LACKSR_ALERA;
	val |= LACKSR_CLKOEN;
	val = 0x2405;
	writel(val, t7132s_dev.cfg_mmio_start + REG_LACKSR);
	val = readl(t7132s_dev.cfg_mmio_start + REG_LACKSR);
	printk("LACKSR(modified) = 0x%x", val);

	/* Read Switch ID from offset 0x1 */
	ptr = (t7132s_dev.dev_mmio_start + CPLD1_REG_SW_ID);
	val = readb(t7132s_dev.dev_mmio_start + CPLD1_REG_SW_ID);
	printk("Switch ID : 0x%x", val);

	platform_driver_register(&t7132s_drv);
	t7132s_platform_dev =
		platform_device_register_simple(DRIVER_NAME, -1, NULL, 0);

	return 0;
}

static int t7132s_suspend(struct pci_dev *dev, pm_message_t state)
{
	return 0;
};

static int t7132s_resume(struct pci_dev *dev)
{
	return 0;
};

static struct pci_device_id t7132s_pci_tbl[] = {
	{ 0x125B, 0x9110, 0xa000, 0x7000, 0, 0, 0 },
	{ 0x125B, 0x9100, 0xa000, 0x7000, 0, 0, 0 },
	{
		0,
	},
};

static struct pci_driver t7132s_pci_driver = {
	.name = "t7132s",
	.probe = t7132s_probe,
	.remove = t7132s_remove,
	.id_table = t7132s_pci_tbl,
	.suspend = t7132s_suspend,
	.resume = t7132s_resume,
};

static int __init t7132s_init(void)
{
	int ret;

	memset(&t7132s_dev, 0, sizeof(struct t7132s));
	ret = pci_register_driver(&t7132s_pci_driver);

	return ret;
}

static void __exit t7132s_exit(void)
{
	pci_unregister_driver(&t7132s_pci_driver);
}

module_init(t7132s_init);
module_exit(t7132s_exit);

MODULE_DEVICE_TABLE(pci, t7132s_pci_tbl);
MODULE_DESCRIPTION("SuperMicro T7132S CPLD Module");
MODULE_SUPPORTED_DEVICE("T7132S");
MODULE_LICENSE("GPL");

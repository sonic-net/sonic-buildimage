// SPDX-License-Identifier: GPL-2.0
/*
 * Virgo main-board CPLD driver.
 *
 * I2C driver for the Virgo MB CPLD (register map rev V03C) at address 0x75.
 * Exposes board status/control via flat sysfs attributes and provides a
 * 14-channel I2C mux (SFP28 P1-P12, QSFP28 P13-P14) pinned to bus 31..44.
 */

#include <linux/bits.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/mutex.h>
#include <linux/property.h>
#include <linux/sysfs.h>

/* ---- Register map (V03C) ------------------------------------------------ */
#define CPLD_REG_CCHWVR		0x00	/* CPLD code & HW version */
#define CPLD_REG_MCR1		0x01	/* misc control #1 (fan present) */
#define CPLD_REG_PSR1		0x03	/* PSU status #1 */
#define CPLD_REG_LEDCR1		0x05	/* LED control #1 (system, power) */
#define CPLD_REG_LEDCR2		0x06	/* LED control #2 (fan, SLED_EN) */
#define CPLD_REG_LEDCR3		0x07	/* LED control #3 (gnss, sync) */
#define CPLD_REG_PTPCSR1	0x08	/* PTP control status #1 */
#define CPLD_REG_ZIMCR		0x0A	/* SFP/QSFP I2C mux control */
#define CPLD_REG_MISRR1		0x0B	/* QSFP module interrupt (R/C) */
#define CPLD_REG_MISRR2		0x0C	/* SFP module interrupt #1 (R/C) */
#define CPLD_REG_MISRR3		0x0D	/* SFP module interrupt #2 (R/C) */
#define CPLD_REG_ZSQCR1		0x0E	/* QSFP reset control */
#define CPLD_REG_ZSQCR2		0x0F	/* QSFP module present */
#define CPLD_REG_ZSQCR3		0x10	/* QSFP low power mode control */
#define CPLD_REG_ZMPSCR1	0x11	/* SFP present #1 (P1-8) */
#define CPLD_REG_ZMPSCR2	0x12	/* SFP present #2 (P9-12) */
#define CPLD_REG_ZMLSCR1	0x13	/* SFP RX loss #1 (P1-8) */
#define CPLD_REG_ZMLSCR2	0x14	/* SFP RX loss #2 (P9-12) */
#define CPLD_REG_ZMTFSCR1	0x15	/* SFP TX fault #1 (P1-8) */
#define CPLD_REG_ZMTFSCR2	0x16	/* SFP TX fault #2 (P9-12) */
#define CPLD_REG_ZTDSCR1	0x17	/* SFP TX disable #1 (P1-8) */
#define CPLD_REG_ZTDSCR2	0x18	/* SFP TX disable #2 (P9-12) */
#define CPLD_REG_ZQLSR		0x22	/* QSFP link speed LED */
#define CPLD_REG_ZLSR1		0x23	/* SFP link speed LED #1 (P1-8) */
#define CPLD_REG_ZLSR2		0x24	/* SFP link speed LED #2 (P9-12) */

#define LEDCR2_SLED_EN		BIT(3)	/* serial LED interface enable */

/* Mux: 14 channels, child bus numbers 31..44 */
#define CPLD_MUX_NCHANS		14
#define CPLD_MUX_BASE_BUS	31

#define CPLDA               1
#define CPLDB               2

#define CPLDA_I2C_ADDR		0x18
#define CPLDB_I2C_ADDR		0x75

/* ---- cpldA (CPU board) register map (Function List V0.03) --------------- */
#define CPLDA_REG_CCBHVR	0x00	/* HW ver (bits[7:5]) + code ver (bits[4:0]) */
#define CPLDA_REG_WTR		0x08	/* watchdog control/status */
#define CPLDA_REG_WTD		0x09	/* watchdog remaining seconds */

#define WTR_FLAGS		(BIT(7) | BIT(6))	/* WDT_CPU_Flag, WDT_CPU_TRI (R/C) */

/* ---- Attribute descriptor ---------------------------------------------- */
enum cpld_attr_type {
	ATTR_VERSION,		/* CCHWVR -> V/T + code version */
	ATTR_HWREV,		/* CCHWVR/CCBHVR -> HW revision string */
	ATTR_BIT_RO,		/* single status bit, normalized */
	ATTR_BIT_RW,		/* single control bit, normalized */
	ATTR_LED,		/* 3-bit LED field */
	ATTR_PTP,		/* raw PTPCSR1 hex */
	ATTR_QSFP_RESET,	/* 2-bit reset field, write-1-to-pulse */
	ATTR_INTERRUPT,		/* R/C latch with software cache */
	/* cpldA (CPU board) */
	ATTR_CPU_VERSION,	/* CCBHVR -> 5-bit code version (no T/V flag) */
	ATTR_WD_ENABLE,		/* WTR bit0 (inverted): 1 = running */
	ATTR_WD_TIMEOUT,	/* WTR bits[3:1]: timeout in seconds */
	ATTR_WD_KICK,		/* WTR: rewrite to reload countdown (WO) */
	ATTR_WD_FLAG,		/* WTR bit7: timed-out latch (cached) */
	ATTR_WD_TRIGGERED,	/* WTR bit6: WD-triggered latch (cached) */
	ATTR_WD_REMAINING,	/* WTD: remaining seconds */
};

struct cpld_attr {
	struct device_attribute dev_attr;
	u8 reg;
	u8 shift;
	u8 type;
	u8 active_low;
	u8 cache_idx;		/* misrr_cache index for ATTR_INTERRUPT */
};

#define to_cpld_attr(x) container_of(x, struct cpld_attr, dev_attr)

struct virgo_cpld_data {
	struct i2c_client *client;
	struct mutex lock;		/* serializes all CPLD register access */
	struct i2c_mux_core *muxc;
	u8 misrr_cache[3];		/* [0]=MISRR1, [1]=MISRR2, [2]=MISRR3 */
	u8 wtr_cache;			/* cpldA WTR bits 7/6 software latch */
};

/* ---- sysfs show/store --------------------------------------------------- */
/* WTR WDT_Value field (bits[3:1]) -> timeout in seconds */
static const u16 wd_timeout_secs[8] = { 1, 3, 5, 10, 30, 60, 120, 180 };

static ssize_t cpld_show(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct cpld_attr *a = to_cpld_attr(da);
	struct virgo_cpld_data *data = dev_get_drvdata(dev);
	int val, ret;
	u8 field;

	mutex_lock(&data->lock);

	val = i2c_smbus_read_byte_data(data->client, a->reg);
	if (val < 0) {
		ret = val;
		goto out;
	}

	switch (a->type) {
	case ATTR_VERSION:
		ret = sysfs_emit(buf, "%c%02u\n",
				 (val & BIT(4)) ? 'T' : 'V', val & 0x0F);
		break;
	case ATTR_CPU_VERSION:
		/* cpldA: 5-bit code version, no test/release flag */
		ret = sysfs_emit(buf, "V%02u\n", val & 0x1F);
		break;
	case ATTR_HWREV:
		field = (val >> 5) & 0x7;
		if (field == 1)
			ret = sysfs_emit(buf, "R1.00\n");
		else if (field == 2)
			ret = sysfs_emit(buf, "R2.00\n");
		else if (field == 3)
			ret = sysfs_emit(buf, "R3.00\n");
		else
			ret = sysfs_emit(buf, "R?.?? (0x%x)\n", field);
		break;
	case ATTR_BIT_RO:
	case ATTR_BIT_RW:
		field = (val >> a->shift) & 0x1;
		if (a->active_low)
			field = !field;
		ret = sysfs_emit(buf, "%u\n", field);
		break;
	case ATTR_LED:
		ret = sysfs_emit(buf, "%u\n", (val >> a->shift) & 0x7);
		break;
	case ATTR_QSFP_RESET:
		/* 01 = normal operation -> 0; 00/10 -> 1 (in reset) */
		field = (val >> a->shift) & 0x3;
		ret = sysfs_emit(buf, "%u\n", field == 0x1 ? 0 : 1);
		break;
	case ATTR_PTP:
		/* reading PTPCSR1 clears the latched PTP_ALERT (bit7) */
		ret = sysfs_emit(buf, "0x%02x\n", val);
		break;
	case ATTR_INTERRUPT:
		/* register is clear-on-read and active-low; fold newly
		 * asserted (0) bits into the software latch cache, report
		 * and clear only this port's cached bit.
		 */
		data->misrr_cache[a->cache_idx] |= (u8)~val;
		field = (data->misrr_cache[a->cache_idx] >> a->shift) & 0x1;
		data->misrr_cache[a->cache_idx] &= ~BIT(a->shift);
		ret = sysfs_emit(buf, "%u\n", field);
		break;
	case ATTR_WD_ENABLE:
		/* fold R/C flags before consuming the value (bit0: 0=enable) */
		data->wtr_cache |= val & WTR_FLAGS;
		ret = sysfs_emit(buf, "%u\n", (val & BIT(0)) ? 0 : 1);
		break;
	case ATTR_WD_TIMEOUT:
		data->wtr_cache |= val & WTR_FLAGS;
		ret = sysfs_emit(buf, "%u\n", wd_timeout_secs[(val >> 1) & 0x7]);
		break;
	case ATTR_WD_FLAG:
		/* WTR read (above) cleared HW bit7; report+clear cached bit */
		data->wtr_cache |= val & WTR_FLAGS;
		field = (data->wtr_cache >> 7) & 0x1;
		data->wtr_cache &= ~BIT(7);
		ret = sysfs_emit(buf, "%u\n", field);
		break;
	case ATTR_WD_TRIGGERED:
		data->wtr_cache |= val & WTR_FLAGS;
		field = (data->wtr_cache >> 6) & 0x1;
		data->wtr_cache &= ~BIT(6);
		ret = sysfs_emit(buf, "%u\n", field);
		break;
	case ATTR_WD_REMAINING:
		/* WTD register: remaining countdown in seconds */
		ret = sysfs_emit(buf, "%u\n", val);
		break;
	default:
		ret = -EINVAL;
		break;
	}

out:
	mutex_unlock(&data->lock);
	return ret;
}

static ssize_t cpld_store(struct device *dev, struct device_attribute *da,
			  const char *buf, size_t count)
{
	struct cpld_attr *a = to_cpld_attr(da);
	struct virgo_cpld_data *data = dev_get_drvdata(dev);
	int cur, ret;
	bool en;
	u8 v;

	mutex_lock(&data->lock);

	switch (a->type) {
	case ATTR_BIT_RW:
		if (kstrtobool(buf, &en)) {
			ret = -EINVAL;
			break;
		}
		cur = i2c_smbus_read_byte_data(data->client, a->reg);
		if (cur < 0) {
			ret = cur;
			break;
		}
		if (a->active_low ? !en : en)
			cur |= BIT(a->shift);
		else
			cur &= ~BIT(a->shift);
		ret = i2c_smbus_write_byte_data(data->client, a->reg, cur);
		break;
	case ATTR_LED:
		if (kstrtou8(buf, 0, &v) || v > 6) {
			ret = -EINVAL;
			break;
		}
		cur = i2c_smbus_read_byte_data(data->client, a->reg);
		if (cur < 0) {
			ret = cur;
			break;
		}
		cur &= ~(0x7 << a->shift);
		cur |= v << a->shift;
		ret = i2c_smbus_write_byte_data(data->client, a->reg, cur);
		break;
	case ATTR_QSFP_RESET:
		if (kstrtobool(buf, &en)) {
			ret = -EINVAL;
			break;
		}
		if (!en) {
			ret = 0;	/* write 0 is a no-op */
			break;
		}
		cur = i2c_smbus_read_byte_data(data->client, a->reg);
		if (cur < 0) {
			ret = cur;
			break;
		}
		/* 10 = pulse reset, hardware auto-clears to 01 */
		cur &= ~(0x3 << a->shift);
		cur |= 0x2 << a->shift;
		ret = i2c_smbus_write_byte_data(data->client, a->reg, cur);
		break;
	case ATTR_WD_ENABLE:
		if (kstrtobool(buf, &en)) {
			ret = -EINVAL;
			break;
		}
		cur = i2c_smbus_read_byte_data(data->client, a->reg);
		if (cur < 0) {
			ret = cur;
			break;
		}
		data->wtr_cache |= cur & WTR_FLAGS;
		cur &= ~WTR_FLAGS;		/* WDT flags are read-only */
		if (en)
			cur &= ~BIT(0);		/* ENBn = 0: watchdog enabled */
		else
			cur |= BIT(0);		/* ENBn = 1: watchdog disabled */
		ret = i2c_smbus_write_byte_data(data->client, a->reg, cur);
		break;
	case ATTR_WD_TIMEOUT: {
		u32 secs;
		int idx;

		if (kstrtou32(buf, 0, &secs) || secs < 1 || secs > 180) {
			ret = -EINVAL;
			break;
		}
		/* round up to the next supported step */
		for (idx = 0; idx < 8; idx++)
			if (wd_timeout_secs[idx] >= secs)
				break;
		cur = i2c_smbus_read_byte_data(data->client, a->reg);
		if (cur < 0) {
			ret = cur;
			break;
		}
		data->wtr_cache |= cur & WTR_FLAGS;
		cur &= ~WTR_FLAGS;		/* WDT flags are read-only */
		cur &= ~(0x7 << 1);
		cur |= idx << 1;
		ret = i2c_smbus_write_byte_data(data->client, a->reg, cur);
		break;
	}
	case ATTR_WD_KICK:
		if (kstrtobool(buf, &en)) {
			ret = -EINVAL;
			break;
		}
		if (!en) {
			ret = 0;		/* write 0 is a no-op */
			break;
		}
		cur = i2c_smbus_read_byte_data(data->client, a->reg);
		if (cur < 0) {
			ret = cur;
			break;
		}
		data->wtr_cache |= cur & WTR_FLAGS;
		/* rewrite WTR (flags masked out) to reload the countdown */
		cur &= ~WTR_FLAGS;
		ret = i2c_smbus_write_byte_data(data->client, a->reg, cur);
		break;
	default:
		ret = -EPERM;
		break;
	}

	mutex_unlock(&data->lock);
	return ret ? ret : count;
}

/* ---- Attribute definitions --------------------------------------------- */
#define CPLD_ATTR_DEF(_name, _mode, _store, _reg, _shift, _type, _al, _ci) \
	static struct cpld_attr cpld_attr_##_name = {			\
		.dev_attr = __ATTR(_name, _mode, cpld_show, _store),	\
		.reg = (_reg), .shift = (_shift), .type = (_type),	\
		.active_low = (_al), .cache_idx = (_ci),		\
	}

/* Variant with a distinct C symbol so two devices can share a sysfs name. */
#define CPLD_ATTR_DEF2(_sym, _name, _mode, _store, _reg, _shift, _type, _al, _ci) \
	static struct cpld_attr cpld_attr_##_sym = {			\
		.dev_attr = __ATTR(_name, _mode, cpld_show, _store),	\
		.reg = (_reg), .shift = (_shift), .type = (_type),	\
		.active_low = (_al), .cache_idx = (_ci),		\
	}

/* Identification */
CPLD_ATTR_DEF(cpld_version, 0444, NULL, CPLD_REG_CCHWVR, 0, ATTR_VERSION, 0, 0);
CPLD_ATTR_DEF(hw_revision, 0444, NULL, CPLD_REG_CCHWVR, 0, ATTR_HWREV, 0, 0);

/* Fan / PSU status */
CPLD_ATTR_DEF(fan_present, 0444, NULL, CPLD_REG_MCR1, 1, ATTR_BIT_RO, 1, 0);
CPLD_ATTR_DEF(psu1_present, 0444, NULL, CPLD_REG_PSR1, 4, ATTR_BIT_RO, 1, 0);
CPLD_ATTR_DEF(psu2_present, 0444, NULL, CPLD_REG_PSR1, 5, ATTR_BIT_RO, 1, 0);
CPLD_ATTR_DEF(psu1_ac_ok, 0444, NULL, CPLD_REG_PSR1, 0, ATTR_BIT_RO, 0, 0);
CPLD_ATTR_DEF(psu2_ac_ok, 0444, NULL, CPLD_REG_PSR1, 1, ATTR_BIT_RO, 0, 0);
CPLD_ATTR_DEF(psu1_dc_ok, 0444, NULL, CPLD_REG_PSR1, 2, ATTR_BIT_RO, 0, 0);
CPLD_ATTR_DEF(psu2_dc_ok, 0444, NULL, CPLD_REG_PSR1, 3, ATTR_BIT_RO, 0, 0);

/* LED control (raw 0-6) */
CPLD_ATTR_DEF(led_system, 0644, cpld_store, CPLD_REG_LEDCR1, 5, ATTR_LED, 0, 0);
CPLD_ATTR_DEF(led_power, 0644, cpld_store, CPLD_REG_LEDCR1, 2, ATTR_LED, 0, 0);
CPLD_ATTR_DEF(led_fan, 0644, cpld_store, CPLD_REG_LEDCR2, 0, ATTR_LED, 0, 0);
CPLD_ATTR_DEF(led_gnss, 0644, cpld_store, CPLD_REG_LEDCR3, 3, ATTR_LED, 0, 0);
CPLD_ATTR_DEF(led_sync, 0644, cpld_store, CPLD_REG_LEDCR3, 0, ATTR_LED, 0, 0);

/* PTP status */
CPLD_ATTR_DEF(ptp_status, 0444, NULL, CPLD_REG_PTPCSR1, 0, ATTR_PTP, 0, 0);

/* SFP28 ports: present, interrupt, rx_los, tx_fault, tx_disable, speed_led */
#define SFP_ATTRS(n, preg, ireg, ici, lreg, freg, treg, sreg, bit)	    \
	CPLD_ATTR_DEF(sfp##n##_present, 0444, NULL, preg, bit,		    \
		      ATTR_BIT_RO, 1, 0);				    \
	CPLD_ATTR_DEF(sfp##n##_interrupt, 0444, NULL, ireg, bit,		    \
		      ATTR_INTERRUPT, 1, ici);				    \
	CPLD_ATTR_DEF(sfp##n##_rx_los, 0444, NULL, lreg, bit,		    \
		      ATTR_BIT_RO, 0, 0);				    \
	CPLD_ATTR_DEF(sfp##n##_tx_fault, 0444, NULL, freg, bit,		    \
		      ATTR_BIT_RO, 0, 0);				    \
	CPLD_ATTR_DEF(sfp##n##_tx_disable, 0644, cpld_store, treg, bit,	    \
		      ATTR_BIT_RW, 0, 0);				    \
	CPLD_ATTR_DEF(sfp##n##_speed_led, 0644, cpld_store, sreg, bit,	    \
		      ATTR_BIT_RW, 0, 0)

SFP_ATTRS(1, CPLD_REG_ZMPSCR1, CPLD_REG_MISRR2, 1, CPLD_REG_ZMLSCR1,
	  CPLD_REG_ZMTFSCR1, CPLD_REG_ZTDSCR1, CPLD_REG_ZLSR1, 0);
SFP_ATTRS(2, CPLD_REG_ZMPSCR1, CPLD_REG_MISRR2, 1, CPLD_REG_ZMLSCR1,
	  CPLD_REG_ZMTFSCR1, CPLD_REG_ZTDSCR1, CPLD_REG_ZLSR1, 1);
SFP_ATTRS(3, CPLD_REG_ZMPSCR1, CPLD_REG_MISRR2, 1, CPLD_REG_ZMLSCR1,
	  CPLD_REG_ZMTFSCR1, CPLD_REG_ZTDSCR1, CPLD_REG_ZLSR1, 2);
SFP_ATTRS(4, CPLD_REG_ZMPSCR1, CPLD_REG_MISRR2, 1, CPLD_REG_ZMLSCR1,
	  CPLD_REG_ZMTFSCR1, CPLD_REG_ZTDSCR1, CPLD_REG_ZLSR1, 3);
SFP_ATTRS(5, CPLD_REG_ZMPSCR1, CPLD_REG_MISRR2, 1, CPLD_REG_ZMLSCR1,
	  CPLD_REG_ZMTFSCR1, CPLD_REG_ZTDSCR1, CPLD_REG_ZLSR1, 4);
SFP_ATTRS(6, CPLD_REG_ZMPSCR1, CPLD_REG_MISRR2, 1, CPLD_REG_ZMLSCR1,
	  CPLD_REG_ZMTFSCR1, CPLD_REG_ZTDSCR1, CPLD_REG_ZLSR1, 5);
SFP_ATTRS(7, CPLD_REG_ZMPSCR1, CPLD_REG_MISRR2, 1, CPLD_REG_ZMLSCR1,
	  CPLD_REG_ZMTFSCR1, CPLD_REG_ZTDSCR1, CPLD_REG_ZLSR1, 6);
SFP_ATTRS(8, CPLD_REG_ZMPSCR1, CPLD_REG_MISRR2, 1, CPLD_REG_ZMLSCR1,
	  CPLD_REG_ZMTFSCR1, CPLD_REG_ZTDSCR1, CPLD_REG_ZLSR1, 7);
SFP_ATTRS(9, CPLD_REG_ZMPSCR2, CPLD_REG_MISRR3, 2, CPLD_REG_ZMLSCR2,
	  CPLD_REG_ZMTFSCR2, CPLD_REG_ZTDSCR2, CPLD_REG_ZLSR2, 0);
SFP_ATTRS(10, CPLD_REG_ZMPSCR2, CPLD_REG_MISRR3, 2, CPLD_REG_ZMLSCR2,
	  CPLD_REG_ZMTFSCR2, CPLD_REG_ZTDSCR2, CPLD_REG_ZLSR2, 1);
SFP_ATTRS(11, CPLD_REG_ZMPSCR2, CPLD_REG_MISRR3, 2, CPLD_REG_ZMLSCR2,
	  CPLD_REG_ZMTFSCR2, CPLD_REG_ZTDSCR2, CPLD_REG_ZLSR2, 2);
SFP_ATTRS(12, CPLD_REG_ZMPSCR2, CPLD_REG_MISRR3, 2, CPLD_REG_ZMLSCR2,
	  CPLD_REG_ZMTFSCR2, CPLD_REG_ZTDSCR2, CPLD_REG_ZLSR2, 3);

/* QSFP28 ports: present, interrupt, lpmode, speed_led, reset */
#define QSFP_ATTRS(n, bit)						    \
	CPLD_ATTR_DEF(qsfp##n##_present, 0444, NULL, CPLD_REG_ZSQCR2, bit,   \
		      ATTR_BIT_RO, 1, 0);				    \
	CPLD_ATTR_DEF(qsfp##n##_interrupt, 0444, NULL, CPLD_REG_MISRR1, bit, \
		      ATTR_INTERRUPT, 1, 0);				    \
	CPLD_ATTR_DEF(qsfp##n##_lpmode, 0644, cpld_store, CPLD_REG_ZSQCR3,   \
		      bit, ATTR_BIT_RW, 0, 0);				    \
	CPLD_ATTR_DEF(qsfp##n##_speed_led, 0644, cpld_store, CPLD_REG_ZQLSR, \
		      bit, ATTR_BIT_RW, 0, 0);				    \
	CPLD_ATTR_DEF(qsfp##n##_reset, 0644, cpld_store, CPLD_REG_ZSQCR1,    \
		      (bit) * 2, ATTR_QSFP_RESET, 0, 0)

QSFP_ATTRS(13, 0);
QSFP_ATTRS(14, 1);

/* ---- Attribute group ---------------------------------------------------- */
#define AP(name) (&cpld_attr_##name.dev_attr.attr)
#define SFP_AP(n)							\
	AP(sfp##n##_present), AP(sfp##n##_interrupt),			\
	AP(sfp##n##_rx_los), AP(sfp##n##_tx_fault),			\
	AP(sfp##n##_tx_disable), AP(sfp##n##_speed_led)
#define QSFP_AP(n)							\
	AP(qsfp##n##_present), AP(qsfp##n##_interrupt),			\
	AP(qsfp##n##_lpmode), AP(qsfp##n##_speed_led),			\
	AP(qsfp##n##_reset)

static struct attribute *cpld_attrs[] = {
	AP(cpld_version), AP(hw_revision),
	AP(fan_present),
	AP(psu1_present), AP(psu2_present),
	AP(psu1_ac_ok), AP(psu2_ac_ok),
	AP(psu1_dc_ok), AP(psu2_dc_ok),
	AP(led_system), AP(led_power), AP(led_fan),
	AP(led_gnss), AP(led_sync),
	AP(ptp_status),
	SFP_AP(1), SFP_AP(2), SFP_AP(3), SFP_AP(4),
	SFP_AP(5), SFP_AP(6), SFP_AP(7), SFP_AP(8),
	SFP_AP(9), SFP_AP(10), SFP_AP(11), SFP_AP(12),
	QSFP_AP(13), QSFP_AP(14),
	NULL,
};
ATTRIBUTE_GROUPS(cpld);

/* ---- cpldA (CPU board) attributes --------------------------------------- */
/* Same sysfs names as cpldB (cpld_version/hw_revision) but distinct C symbols
 * and cpldA-specific registers/parsing.
 */
CPLD_ATTR_DEF2(cpua_version, cpld_version, 0444, NULL,
	       CPLDA_REG_CCBHVR, 0, ATTR_CPU_VERSION, 0, 0);
CPLD_ATTR_DEF2(cpua_hwrev, hw_revision, 0444, NULL,
	       CPLDA_REG_CCBHVR, 0, ATTR_HWREV, 0, 0);

/* CPU watchdog timer (WTR 0x08, WTD 0x09) */
CPLD_ATTR_DEF(wd_enable, 0644, cpld_store, CPLDA_REG_WTR, 0, ATTR_WD_ENABLE, 0, 0);
CPLD_ATTR_DEF(wd_timeout, 0644, cpld_store, CPLDA_REG_WTR, 0, ATTR_WD_TIMEOUT, 0, 0);
CPLD_ATTR_DEF(wd_kick, 0200, cpld_store, CPLDA_REG_WTR, 0, ATTR_WD_KICK, 0, 0);
CPLD_ATTR_DEF(wd_timeout_flag, 0444, NULL, CPLDA_REG_WTR, 0, ATTR_WD_FLAG, 0, 0);
CPLD_ATTR_DEF(wd_triggered, 0444, NULL, CPLDA_REG_WTR, 0, ATTR_WD_TRIGGERED, 0, 0);
CPLD_ATTR_DEF(wd_remaining, 0444, NULL, CPLDA_REG_WTD, 0, ATTR_WD_REMAINING, 0, 0);

static struct attribute *cpld_cpua_attrs[] = {
	AP(cpua_version), AP(cpua_hwrev),
	AP(wd_enable), AP(wd_timeout), AP(wd_kick),
	AP(wd_timeout_flag), AP(wd_triggered), AP(wd_remaining),
	NULL,
};
ATTRIBUTE_GROUPS(cpld_cpua);

/* ---- I2C mux ------------------------------------------------------------ */
static int cpld_mux_select(struct i2c_mux_core *muxc, u32 chan)
{
	struct virgo_cpld_data *data = i2c_mux_priv(muxc);

	/* ZIMCR: 0x01 = SFP-P1 .. 0x0E = QSFP-P14 */
	return i2c_smbus_write_byte_data(data->client, CPLD_REG_ZIMCR,
					 chan + 1);
}

static int cpld_mux_deselect(struct i2c_mux_core *muxc, u32 chan)
{
	struct virgo_cpld_data *data = i2c_mux_priv(muxc);

	/* idle-disconnect: all module SCL disconnected */
	return i2c_smbus_write_byte_data(data->client, CPLD_REG_ZIMCR, 0x00);
}

/* ---- Probe / remove ----------------------------------------------------- */
static int cpld_apply_defaults(struct i2c_client *client)
{
	int ret;

	/* Release both QSFP from reset (01 = normal operation). */
	ret = i2c_smbus_write_byte_data(client, CPLD_REG_ZSQCR1, 0x05);
	if (ret)
		return ret;
	/* Disable low power mode on both QSFP. */
	ret = i2c_smbus_write_byte_data(client, CPLD_REG_ZSQCR3, 0x00);
	if (ret)
		return ret;
	/* Enable TX on all 12 SFP ports. */
	ret = i2c_smbus_write_byte_data(client, CPLD_REG_ZTDSCR1, 0x00);
	if (ret)
		return ret;
	ret = i2c_smbus_write_byte_data(client, CPLD_REG_ZTDSCR2, 0x00);
	if (ret)
		return ret;
	/* Enable serial LED interface (preserve other LEDCR2 bits). */
	ret = i2c_smbus_read_byte_data(client, CPLD_REG_LEDCR2);
	if (ret < 0)
		return ret;
	ret = i2c_smbus_write_byte_data(client, CPLD_REG_LEDCR2,
					ret | LEDCR2_SLED_EN);
	if (ret)
		return ret;
	/* Disconnect the module I2C mux. */
	return i2c_smbus_write_byte_data(client, CPLD_REG_ZIMCR, 0x00);
}

static int virgo_cpld_probe_cpua(struct i2c_client *client,
				 struct virgo_cpld_data *data)
{
	struct device *dev = &client->dev;
	int ver;

	/* Confirm the CPU CPLD responds; never write any register. */
	ver = i2c_smbus_read_byte_data(client, CPLDA_REG_CCBHVR);
	if (ver < 0) {
		dev_err(dev, "CPU CPLD not responding (CCBHVR read: %d)\n", ver);
		return -ENODEV;
	}
	dev_info(dev, "Virgo CPU CPLD V%02u, HW rev code 0x%x\n",
		 ver & 0x1F, (ver >> 5) & 0x7);

	/* Hands-off: expose version/revision + watchdog, touch no registers. */
	return sysfs_create_groups(&dev->kobj, cpld_cpua_groups);
}

static int virgo_cpld_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct virgo_cpld_data *data;
	int ret, i, ver;

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	/*
	 * cpldA (0x18, CPU board) and cpldB (0x75, main board) share the same
	 * DT compatible; dispatch on the I2C address.
	 */
	if (client->addr != CPLDA_I2C_ADDR &&
	    client->addr != CPLDB_I2C_ADDR) {
		dev_info(dev, "ignoring CPLD at unexpected addr 0x%02x\n",
			 client->addr);
		return -ENODEV;
	}

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->client = client;
	mutex_init(&data->lock);
	i2c_set_clientdata(client, data);

	if (client->addr == CPLDA_I2C_ADDR)
		return virgo_cpld_probe_cpua(client, data);

	/* cpldB (main board) path. */

	/* Probe: confirm the CPLD responds. */
	ver = i2c_smbus_read_byte_data(client, CPLD_REG_CCHWVR);
	if (ver < 0) {
		dev_err(dev, "CPLD not responding (CCHWVR read: %d)\n", ver);
		return -ENODEV;
	}
	dev_info(dev, "Virgo CPLD %c%02u, HW rev code 0x%x\n",
		 (ver & BIT(4)) ? 'T' : 'V', ver & 0x0F, (ver >> 5) & 0x7);

	ret = cpld_apply_defaults(client);
	if (ret) {
		dev_err(dev, "failed to apply default config: %d\n", ret);
		return ret;
	}

	ret = sysfs_create_groups(&dev->kobj, cpld_groups);
	if (ret)
		return ret;

	data->muxc = i2c_mux_alloc(client->adapter, dev, CPLD_MUX_NCHANS, 0,
				   I2C_MUX_LOCKED, cpld_mux_select,
				   cpld_mux_deselect);
	if (!data->muxc) {
		ret = -ENOMEM;
		goto err_group;
	}
	data->muxc->priv = data;

	for (i = 0; i < CPLD_MUX_NCHANS; i++) {
		ret = i2c_mux_add_adapter(data->muxc, CPLD_MUX_BASE_BUS + i, i);
		if (ret) {
			dev_err(dev, "failed to add mux channel %d (bus %d): %d\n",
				i, CPLD_MUX_BASE_BUS + i, ret);
			goto err_mux;
		}
	}

	return 0;

err_mux:
	i2c_mux_del_adapters(data->muxc);
err_group:
	sysfs_remove_groups(&dev->kobj, cpld_groups);
	return ret;
}

static void virgo_cpld_remove(struct i2c_client *client)
{
	struct virgo_cpld_data *data = i2c_get_clientdata(client);

	/* cpldA has no mux; cpldB registers one. */
	if (data->muxc) {
		i2c_mux_del_adapters(data->muxc);
		sysfs_remove_groups(&client->dev.kobj, cpld_groups);
	} else {
		sysfs_remove_groups(&client->dev.kobj, cpld_cpua_groups);
	}
}

static const struct i2c_device_id virgo_cpld_id[] = {
	{ "virgo2_cpld", 0 },
	{ "cpldA", CPLDA },
	{ "cpldB", CPLDB },
	{ }
};
MODULE_DEVICE_TABLE(i2c, virgo_cpld_id);

static const struct of_device_id virgo_cpld_of_match[] = {
	{ .compatible = "custom  ,virgo2_cpld" },
	{ }
};
MODULE_DEVICE_TABLE(of, virgo_cpld_of_match);

static struct i2c_driver virgo_cpld_driver = {
	.driver = {
		.name = "virgo2_cpld",
		.of_match_table = virgo_cpld_of_match,
	},
	.probe = virgo_cpld_probe,
	.remove = virgo_cpld_remove,
	.id_table = virgo_cpld_id,
};
module_i2c_driver(virgo_cpld_driver);

MODULE_AUTHOR("Larch Networks");
MODULE_DESCRIPTION("Virgo MB (cpldB) and CPU (cpldA) CPLD driver");
MODULE_LICENSE("GPL");

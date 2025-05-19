/*
 * emc2305.c - hwmon driver for SMSC EMC2305 fan controller
 * (C) Copyright 2013
 * Reinhard Pfau, Guntermann & Drunck GmbH <pfau@gdsys.de>
 *
 * Based on emc2103 driver by SMSC.
 *
 * Datasheet available at:
 * http://www.smsc.com/Downloads/SMSC/Downloads_Public/Data_Sheets/2305.pdf
 *
 * Also supports the EMC2303 fan controller which has the same functionality
 * and register layout as EMC2305, but supports only up to 3 fans instead of 5.
 *
 * Also supports EMC2302 (up to 2 fans) and EMC2301 (1 fan) fan controller.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/time.h>

/*
 * Addresses scanned.
 * Listed in the same order as they appear in the EMC2305, EMC2303 data sheets.
 *
 * Note: these are the I2C adresses which are possible for EMC2305 and EMC2303
 * chips.
 * The EMC2302 supports only 0x2e (EMC2302-1) and 0x2f (EMC2302-2).
 * The EMC2301 supports only 0x2f.
 */
static const unsigned short i2c_adresses[] = {
    0x2E,
    0x2F,
    0x2C,
    0x2D,
    0x4C,
    0x4D,
    I2C_CLIENT_END
};

/*
 * global registers
 */
enum {
    REG_INVALID = 0x0,
    REG_CONFIGURATION = 0x20,
    REG_FAN_STATUS = 0x24,
    REG_FAN_STALL_STATUS = 0x25,
    REG_FAN_SPIN_STATUS = 0x26,
    REG_DRIVE_FAIL_STATUS = 0x27,
    REG_FAN_INTERRUPT_ENABLE = 0x29,
    REG_PWM_POLARITY_CONFIG = 0x2a,
    REG_PWM_OUTPUT_CONFIG = 0x2b,
    REG_PWM_BASE_FREQ_1 = 0x2c,
    REG_PWM_BASE_FREQ_2 = 0x2d,
    REG_SOFTWARE_LOCK = 0xef,
    REG_PRODUCT_FEATURES = 0xfc,
    REG_PRODUCT_ID = 0xfd,
    REG_MANUFACTURER_ID = 0xfe,
    REG_REVISION = 0xff
};

#define MAX_COMMON_REGS 16

/*
 * fan specific registers
 */
enum {
    REG_FAN_SETTING = 0x30,
    REG_PWM_DIVIDE = 0x31,
    REG_FAN_CONFIGURATION_1 = 0x32,
    REG_FAN_CONFIGURATION_2 = 0x33,
    REG_GAIN = 0x35,
    REG_FAN_SPIN_UP_CONFIG = 0x36,
    REG_FAN_MAX_STEP = 0x37,
    REG_FAN_MINIMUM_DRIVE = 0x38,
    REG_FAN_VALID_TACH_COUNT = 0x39,
    REG_FAN_DRIVE_FAIL_BAND_LOW = 0x3a,
    REG_FAN_DRIVE_FAIL_BAND_HIGH = 0x3b,
    REG_TACH_TARGET_LOW = 0x3c,
    REG_TACH_TARGET_HIGH = 0x3d,
    REG_TACH_READ_HIGH = 0x3e,
    REG_TACH_READ_LOW = 0x3f,
};

#define SEL_FAN(fan, reg) (reg + fan * 0x10)

#define MAX_REG_PER_FAN 16

/*
 * Factor by equations [2] and [3] from data sheet; valid for fans where the
 * number of edges equals (poles * 2 + 1).
 */

#define EMC2305_TACH_FREQ             32768 /* 32.768KHz */
#define EMC2305_RPM_CONST_VAL         60

#define EMC2305_FAN_CONFIG_RANGE_MASK 0x60
#define TACH_READING_DEFAULT_VAL      0xFFF8
#define EMC2305_PERCENT_VAL           100
#define EMC2305_FAN_SETTING_MAX_VAL   0xff
#define EMC2305_FAN_WATCHDOG_STATUS_MASK 0x80

#define FAN_INIT_RPM_SPEED    12000
#define FAN_MAX_RPM_SPEED     28600
#define TACH_VALUE_SHIFT_BITS 3
#define SPEED_PWM_LEN_STRING  7

#define MAX_NUM_FANS 5
#define DEFAULT_NUM_FANS 3
#define DEFAULT_TACH_COUNT_MULTIPLIER 4 /* 2000 RPM : TACH count multiplier = 4*/
#define DEFAULT_POLES 2                 /* 2 pole FANs : 5 edges samples */
#define MAX_FAN_FAULT_REG_COUNT 4

time64_t timestamp = 0;
bool fan_fault_status[MAX_NUM_FANS] = {false};
static unsigned short mode = 1;
static unsigned short num_fans = DEFAULT_NUM_FANS;
static unsigned short tach_count_multiplier = DEFAULT_TACH_COUNT_MULTIPLIER;
static unsigned short poles = DEFAULT_POLES;
module_param(mode, ushort, 0);
module_param(num_fans, ushort, 0);
module_param(tach_count_multiplier, ushort, 0);
module_param(poles, ushort, 0);

struct emc2305_fan_data {
    bool        enabled;
    bool        valid;
    unsigned long   last_updated;
    bool        rpm_control;		/* RPM(FSC) mode or Direct Setting mode */
    uint8_t     multiplier;
    uint8_t     poles;
    uint32_t    tach_target_rpm;
    uint32_t    driver_setting_rpm;
    uint8_t     pwm;
    uint8_t     ranges;
    uint8_t     edges;
    uint16_t    fanstop;
};

struct emc2305_data {
    struct device		*hwmon_dev;
    struct mutex		update_lock;
    int			fans;
    struct emc2305_fan_data	fan[MAX_NUM_FANS];
};

/* Fault status registers */
static const uint8_t emc2305_fault_status_reg[] = { REG_FAN_STATUS,
    REG_FAN_STALL_STATUS,
    REG_FAN_SPIN_STATUS,
    REG_DRIVE_FAIL_STATUS };

/**
 * @brief This function calculates the tach count based on the passed parameters.
 * @param[in] fan_data - The fan details of the fan
 * @param[in] rpm - The speed of the fan in RPM
 * @returns tach value in tach counts
 */
uint16_t calculate_tach_count_emc23xx(struct emc2305_fan_data *fan_data, uint32_t rpm)
{
    uint16_t tachval = 0;

    if ((rpm == 0) || (fan_data->poles == 0)) {
        return 0;
    }
    tachval = (((fan_data->edges - 1) * EMC2305_TACH_FREQ * EMC2305_RPM_CONST_VAL * fan_data->multiplier) /
            (fan_data->poles * rpm));
    tachval = tachval << TACH_VALUE_SHIFT_BITS;

    return tachval;
}

/**
 * @brief This function calculates the fan rpm based on the TACH value (RPM based FSC Mode)
 * @param[in] fan - fan details
 * @param[in] tachval - The tach counts of the fan
 * @returns speed of the fan in RPM
 */
uint32_t calculate_rpm_from_tach_emc23xx(struct emc2305_fan_data *fan, uint16_t tachval)
{
    uint32_t speed = 0;

    if ((tachval == 0) || (fan->poles == 0)) {
        return 0;
    }

    speed = (((fan->edges - 1) * EMC2305_TACH_FREQ * fan->multiplier * EMC2305_RPM_CONST_VAL)
            / (tachval * fan->poles));

    return speed;
}

/**
 * @brief This function calculates the fan rpm based on Driver Setting (Direct Setting Mode)
 * @param[in] fan - fan details
 * @param[in] setting - The direct setting value
 * @returns speed of the fan in RPM
 */
uint32_t calculate_rpm_from_driver_setting_emc23xx(struct emc2305_fan_data *fan, uint8_t setting)
{
    uint8_t speed_percent = 0;

    speed_percent = (setting * EMC2305_PERCENT_VAL) / (EMC2305_FAN_SETTING_MAX_VAL);
    return (FAN_MAX_RPM_SPEED * speed_percent) / EMC2305_PERCENT_VAL;
}

static int read_u8_from_i2c(struct i2c_client *client, uint8_t i2c_reg, uint8_t *output)
{
    int status = i2c_smbus_read_byte_data(client, i2c_reg);

    if (status < 0) {
        dev_warn(&client->dev, "reg 0x%02x, err %d\n",
                i2c_reg, status);
    } else {
        *output = status;
    }
    return status;
}

/* Clear the fan fault status in the EMC2305 Controller.
 */
static void emc2305_clear_fan_fault(struct i2c_client *client)
{
    uint8_t buf = 0, index = 0;
    int rc = 0;

    for (index = 0; index < sizeof(emc2305_fault_status_reg); index++) {
        /* All the status register are Read-On-Clear */
        rc = read_u8_from_i2c(client, emc2305_fault_status_reg[index], &buf);
        if (rc < 0) {
            return;
        }
    }
}

/*
 * TACH reading functions : common to both Direct Setting & FSC (RPM control) modes
 */
static void read_tach_registers(struct i2c_client *client, uint16_t *output,
        uint8_t hi_addr, uint8_t lo_addr)
{
    uint8_t high_byte = 0, lo_byte = 0;

    if (read_u8_from_i2c (client, hi_addr, &high_byte) < 0)
        return;

    if (read_u8_from_i2c (client, lo_addr, &lo_byte) < 0)
        return;

    *output = (((uint16_t) high_byte << 8) | (lo_byte));
}

/*
 * TACH Target write: applicable for FSC (RPM control) mode only
 */
static void write_fan_target(struct i2c_client *client, int fan,
        uint32_t rpm)
{
    struct emc2305_data *data = i2c_get_clientdata (client);
    struct emc2305_fan_data *fan_data = &data->fan[fan];
    const uint8_t lo_reg = SEL_FAN (fan, REG_TACH_TARGET_LOW);
    const uint8_t hi_reg = SEL_FAN (fan, REG_TACH_TARGET_HIGH);
    uint16_t tachval = 0;
    uint8_t tach[2] = { 0, 0 };

    // The Tach Target are in registers 3c/3d, 4c/4d, 5c/5d, 6c/6d & 7c/7d
    tachval = calculate_tach_count_emc23xx (fan_data, rpm);
    tach[1] = (tachval & 0xff00) >> 8;	// High Byte
    tach[0] = (tachval & 0xff);	// Low Byte

    i2c_smbus_write_byte_data (client, lo_reg, tach[0]);
    i2c_smbus_write_byte_data (client, hi_reg, tach[1]);

    fan_data->tach_target_rpm = rpm;
}

/*
 * FAN Driver Setting write: applicable for Direct Setting mode only
 */
static void write_fan_setting(struct i2c_client *client, int fan,
        uint32_t rpm)
{
    struct emc2305_data *data = i2c_get_clientdata (client);
    struct emc2305_fan_data *fan_data = &data->fan[fan];
    uint8_t speed_percent = 0, reg = 0, setting = 0;

    speed_percent = ((rpm * EMC2305_PERCENT_VAL) / FAN_MAX_RPM_SPEED);
    setting = ((speed_percent * EMC2305_FAN_SETTING_MAX_VAL) / EMC2305_PERCENT_VAL);
    reg = SEL_FAN (fan, REG_FAN_SETTING);
    i2c_smbus_write_byte_data (client, reg, setting);

    fan_data->driver_setting_rpm = rpm;
}

static struct emc2305_fan_data *emc2305_update_fan(struct i2c_client *client, int fan_idx)
{
    struct emc2305_data *data = i2c_get_clientdata(client);
    struct emc2305_fan_data *fan_data = &data->fan[fan_idx];

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, fan_data->last_updated + HZ + HZ / 2)
            || !fan_data->valid) {
        fan_data->valid = true;
        fan_data->last_updated = jiffies;
    }

    mutex_unlock(&data->update_lock);
    return fan_data;
}

static void read_fan_fault(struct device *dev, struct device_attribute *da, bool *fault)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct emc2305_data *data = i2c_get_clientdata(client);
    int fan_idx = to_sensor_dev_attr(da)->index;
    uint8_t fault_regs[MAX_FAN_FAULT_REG_COUNT] = {0};
    int index = 0;
    int rc = 0;
    time64_t current_time = 0;
    int fan_id = 0;

    *fault = false;
    current_time = ktime_get_real_seconds();
    time64_t time_diff = current_time - timestamp;

    if ((time_diff >= 1) || (timestamp == 0)) {
        for (index = 0; index < MAX_FAN_FAULT_REG_COUNT; index++) {
            rc = read_u8_from_i2c(client, emc2305_fault_status_reg[index], &fault_regs[index]);
            if (rc < 0) {
                printk(KERN_INFO "Failed to read reg 0x%02x\n", emc2305_fault_status_reg[index]);
                return;
            }
        }
        timestamp = current_time;
        for (fan_id = 0; fan_id < data->fans; ++fan_id) {
            fan_fault_status[fan_id] = ((fault_regs[0] & EMC2305_FAN_WATCHDOG_STATUS_MASK) ||
                                  ((fault_regs[1] | fault_regs[2] | fault_regs[3]) & (1 << fan_id))) ? true : false;
        }
    }

    *fault = fan_fault_status[fan_idx];

    return;
}

/*
 * set/ config functions
 */

/*
 * Note: we also update the fan target here, because its value is
 * determined in part by the fan clock divider.  This follows the principle
 * of least surprise; the user doesn't expect the fan target to change just
 * because the divider changed.
 */
static int emc2305_set_fan_div(struct i2c_client *client, int fan_idx, long new_div)
{
    struct emc2305_data *data = i2c_get_clientdata(client);
    struct emc2305_fan_data *fan = &data->fan[fan_idx];
    const uint8_t reg_conf1 = SEL_FAN(fan_idx, REG_FAN_CONFIGURATION_1);
    int new_range_bits, old_div = 8 / fan->multiplier;
    int status = 0;

    if (new_div == old_div) /* No change */
        return 0;

    switch (new_div) {
        case 1:
            new_range_bits = 3;
            break;
        case 2:
            new_range_bits = 2;
            break;
        case 4:
            new_range_bits = 1;
            break;
        case 8:
            new_range_bits = 0;
            break;
        default:
            return -EINVAL;
    }

    mutex_lock(&data->update_lock);

    status = i2c_smbus_read_byte_data(client, reg_conf1);
    if (status < 0) {
        dev_dbg(&client->dev, "reg 0x%02x, err %d\n",
                reg_conf1, status);
        status = -EIO;
        goto exit_unlock;
    }
    status &= 0x9F;
    status |= (new_range_bits << 5);
    status = i2c_smbus_write_byte_data(client, reg_conf1, status);
    if (status < 0) {
        status = -EIO;
        goto exit_invalidate;
    }

    fan->multiplier = 8 / new_div;

    /* update fan target if high byte is not disabled */
    if ((fan->tach_target_rpm & 0x7fff) != 0x7fff) {
        uint16_t new_target = (fan->tach_target_rpm * old_div) / new_div;
        write_fan_target(client, fan_idx, min_t(uint16_t, new_target, FAN_MAX_RPM_SPEED));
    }

exit_invalidate:
    /* invalidate fan data to force re-read from hardware */
    fan->valid = false;
exit_unlock:
    mutex_unlock(&data->update_lock);
    return status;
}

static int emc2305_set_fan_target(struct i2c_client *client, int fan_idx, long rpm_target)
{
    struct emc2305_data *data = i2c_get_clientdata (client);
    struct emc2305_fan_data *fan = &data->fan[fan_idx];
    int status = 0;

    /*
     * Datasheet states 16000 as maximum RPM target
     * (table 2.2 and section 4.3)
     */
    if (rpm_target < 0) {
        return -EINVAL;
    }

    mutex_lock (&data->update_lock);

    if ((rpm_target == 0) || (rpm_target > FAN_MAX_RPM_SPEED)) {
        rpm_target = FAN_MAX_RPM_SPEED;
    }

    if (fan->rpm_control) {
        /* RPM mode */
        write_fan_target (client, fan_idx, (uint16_t) rpm_target);
    } else {
        /* Direct Method */
        write_fan_setting (client, fan_idx, (uint16_t) rpm_target);
    }

    mutex_unlock (&data->update_lock);
    return status;
}

static int emc2305_set_pwm_enable(struct i2c_client *client, int fan_idx, long enable)
{
    struct emc2305_data *data = i2c_get_clientdata(client);
    struct emc2305_fan_data *fan = &data->fan[fan_idx];
    const uint8_t reg_fan_conf1 = SEL_FAN(fan_idx, REG_FAN_CONFIGURATION_1);
    int status = 0;
    uint8_t conf_reg;

    mutex_lock(&data->update_lock);
    switch (enable) {
        case 0:
            fan->rpm_control = false;
            break;
        case 3:
            fan->rpm_control = true;
            break;
        default:
            status = -EINVAL;
            goto exit_unlock;
    }

    status = read_u8_from_i2c(client, reg_fan_conf1, &conf_reg);
    if (status < 0) {
        status = -EIO;
        goto exit_unlock;
    }

    if (fan->rpm_control)
        conf_reg |= 0x80;
    else
        conf_reg &= ~0x80;

    status = i2c_smbus_write_byte_data(client, reg_fan_conf1, conf_reg);
    if (status < 0)
        status = -EIO;

exit_unlock:
    mutex_unlock(&data->update_lock);
    return status;
}

static int emc2305_set_pwm(struct i2c_client *client, int fan_idx, long pwm)
{
    struct emc2305_data *data = i2c_get_clientdata(client);
    struct emc2305_fan_data *fan = &data->fan[fan_idx];
    const uint8_t reg_fan_setting = SEL_FAN(fan_idx, REG_FAN_SETTING);
    int status = 0;

    /*
     * Datasheet states 255 as maximum PWM
     * (section 5.7)
     */
    if ((pwm < 0) || (pwm > 255)) {
        return -EINVAL;
    }

    fan->pwm = pwm;

    mutex_lock(&data->update_lock);

    status = i2c_smbus_write_byte_data(client, reg_fan_setting, fan->pwm);

    mutex_unlock(&data->update_lock);
    return status;
}

/*
 * sysfs callback functions
 *
 * Note:
 * Naming of the funcs is modelled after the naming scheme described in
 * Documentation/hwmon/sysfs-interface:
 *
 * For a sysfs file <type><number>_<item> the functions are named like this:
 *    the show function: show_<type>_<item>
 *    the store function: set_<type>_<item>
 * For read only (RO) attributes of course only the show func is required.
 *
 * This convention allows us to define the sysfs attributes by using macros.
 */
static ssize_t show_fan_input(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client (dev);
    int fan_idx = to_sensor_dev_attr (da)->index;
    struct emc2305_data *data = i2c_get_clientdata(client);
    struct emc2305_fan_data *fan = &data->fan[fan_idx];
    uint32_t speed = 0;
    uint16_t tachval =0;

    mutex_lock(&data->update_lock);
    read_tach_registers(client, &tachval,
            SEL_FAN(fan_idx, REG_TACH_READ_HIGH),
            SEL_FAN(fan_idx, REG_TACH_READ_LOW));

    // Mask off the lower bits which are indeterminate
    tachval = tachval >> TACH_VALUE_SHIFT_BITS;

    /* Disable the fan speed in case of fanstop value */
    if (tachval == fan->fanstop) {
        speed = 0;
    } else {
        speed = calculate_rpm_from_tach_emc23xx(fan, tachval);
    }

    mutex_unlock(&data->update_lock);
    return snprintf(buf, SPEED_PWM_LEN_STRING, "%d\n", speed);
}

static ssize_t show_fan_fault(struct device *dev, struct device_attribute *da, char *buf)
{
    bool fault;
    read_fan_fault(dev, da, &fault);
    return snprintf(buf, SPEED_PWM_LEN_STRING, "%d\n", fault ? 1 : 0);
}

static ssize_t show_fan_div(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client (dev);
    struct emc2305_data *data = i2c_get_clientdata(client);
    int fan_idx = to_sensor_dev_attr(da)->index;
    struct emc2305_fan_data *fan = &data->fan[fan_idx];

    int fan_div = 8 / fan->multiplier;
    return snprintf(buf, SPEED_PWM_LEN_STRING, "%d\n", fan_div);
}

static ssize_t set_fan_div(struct device *dev, struct device_attribute *da,
        const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    int fan_idx = to_sensor_dev_attr(da)->index;
    long new_div;
    int status;

    status = kstrtol(buf, 10, &new_div);
    if (status < 0) {
        return -EINVAL;
    }

    status = emc2305_set_fan_div(client, fan_idx, new_div);
    if (status < 0)
        return status;

    return count;
}

static ssize_t show_fan_target(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client (dev);
    struct emc2305_data *data = i2c_get_clientdata(client);
    int fan_idx = to_sensor_dev_attr (da)->index;
    struct emc2305_fan_data *fan = &data->fan[fan_idx];
    uint8_t setting = 0;
    uint32_t speed = 0;
    uint16_t tachval =0;

    if (fan->rpm_control) {
        /* RPM mode */
        read_tach_registers(client, &tachval,
                SEL_FAN(fan_idx, REG_TACH_TARGET_HIGH),
                SEL_FAN(fan_idx, REG_TACH_TARGET_LOW));
        tachval = tachval >> TACH_VALUE_SHIFT_BITS;
        speed = calculate_rpm_from_tach_emc23xx(fan, tachval);
        return snprintf(buf, SPEED_PWM_LEN_STRING, "%d\n", speed);
    } else {
        /* Direct Method */
        if (read_u8_from_i2c(client, SEL_FAN (fan_idx, REG_FAN_SETTING), &setting) < 0) {
            return 0;
        }
        speed = calculate_rpm_from_driver_setting_emc23xx(fan, setting);
        return snprintf (buf, SPEED_PWM_LEN_STRING, "%d\n", speed);
    }
}

static ssize_t set_fan_target(struct device *dev, struct device_attribute *da,
        const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    int fan_idx = to_sensor_dev_attr(da)->index;
    long rpm_target = 0;
    int status = 0;

    status = kstrtol(buf, 10, &rpm_target);
    if (status < 0) {
        return -EINVAL;
    }

    status = emc2305_set_fan_target(client, fan_idx, rpm_target);
    if (status < 0)
        return status;

    return count;
}

static ssize_t show_pwm_enable(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client (dev);
    struct emc2305_data *data = i2c_get_clientdata(client);
    int fan_idx = to_sensor_dev_attr (da)->index;
    struct emc2305_fan_data *fan = &data->fan[fan_idx];

    return snprintf(buf, SPEED_PWM_LEN_STRING, "%d\n", fan->rpm_control ? 3 : 0);
}

static ssize_t set_pwm_enable(struct device *dev, struct device_attribute *da,
        const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    int fan_idx = to_sensor_dev_attr(da)->index;
    long new_value;
    int status;

    status = kstrtol(buf, 10, &new_value);
    if (status < 0) {
        return -EINVAL;
    }
    status = emc2305_set_pwm_enable(client, fan_idx, new_value);
    return count;
}

static ssize_t show_pwm(struct device *dev, struct device_attribute *da,
        char *buf)
{
    struct i2c_client *client = to_i2c_client (dev);
    struct emc2305_data *data = i2c_get_clientdata(client);
    int fan_idx = to_sensor_dev_attr (da)->index;
    struct emc2305_fan_data *fan = &data->fan[fan_idx];

    return snprintf(buf, SPEED_PWM_LEN_STRING, "%d\n", fan->pwm);
}

static ssize_t set_pwm(struct device *dev, struct device_attribute *da,
        const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    int fan_idx = to_sensor_dev_attr(da)->index;
    unsigned long val = 0;
    int ret = 0;
    int status = 0;

    ret = kstrtoul(buf, 10, &val);
    if (ret)
        return ret;
    if (val > 255) {
        return -EINVAL;
    }

    status = emc2305_set_pwm(client, fan_idx, val);
    return count;
}

static ssize_t show_fan_dump(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client (dev);
    int fan_idx = to_sensor_dev_attr (da)->index;
    uint8_t reg[MAX_REG_PER_FAN];
    int i, rc;

    for(i=0;i<MAX_REG_PER_FAN;i++) {
        if ((rc = read_u8_from_i2c(client, REG_FAN_SETTING + (fan_idx*0x10) + i, &reg[i])) < 0) {
            printk(KERN_ERR "i2c read failed for reg 0x%02x. rc %d\n",REG_FAN_SETTING + ((fan_idx-1)*0x10),rc);
            return 0;
        }
    }

    for(i=0;i<MAX_REG_PER_FAN;i+=4) {
        printk(KERN_NOTICE "[0x%02x]=0x%02x [0x%02x]=0x%02x [0x%02x]=0x%02x [0x%02x]=0x%02x",
                REG_FAN_SETTING + (fan_idx*0x10) + i  ,reg[i],
                REG_FAN_SETTING + (fan_idx*0x10) + i+1,reg[i+1],
                REG_FAN_SETTING + (fan_idx*0x10) + i+2,reg[i+2],
                REG_FAN_SETTING + (fan_idx*0x10) + i+3,reg[i+3]);
    }
    printk(KERN_NOTICE "\n");

    return snprintf(buf, SPEED_PWM_LEN_STRING, "%s", "");
}

static ssize_t show_debug_dump(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client (dev);
    struct emc2305_data *data = i2c_get_clientdata(client);
    uint8_t globals[] = {REG_CONFIGURATION, REG_FAN_STATUS, REG_FAN_STALL_STATUS, REG_FAN_SPIN_STATUS,
                         REG_DRIVE_FAIL_STATUS, REG_FAN_INTERRUPT_ENABLE, REG_PWM_POLARITY_CONFIG, REG_PWM_OUTPUT_CONFIG,
                         REG_PWM_BASE_FREQ_1, REG_PWM_BASE_FREQ_2, REG_SOFTWARE_LOCK, REG_PRODUCT_FEATURES,
                         REG_PRODUCT_ID, REG_MANUFACTURER_ID, REG_REVISION, REG_INVALID};
    uint8_t reg[MAX_COMMON_REGS] = {0};
    int i, rc, fan_idx;
    uint16_t tachval = 0, tach_reading = 0;

    for(i=0;i<MAX_COMMON_REGS;i++) {
        if (REG_INVALID != globals[i]) {
            if ((rc = read_u8_from_i2c(client, globals[i], &reg[i])) < 0) {
                printk(KERN_ERR "i2c read failed for reg 0x%02x. rc %d\n",globals[i],rc);
                return 0;
            }
        }
    }

    for(i=0;i<MAX_COMMON_REGS;i+=4) {
        printk(KERN_NOTICE "[0x%02x]=0x%02x [0x%02x]=0x%02x [0x%02x]=0x%02x [0x%02x]=0x%02x",
                globals[i],reg[i],globals[i+1],reg[i+1],globals[i+2],reg[i+2],globals[i+3],reg[i+3]);
    }

    printk(KERN_NOTICE "\n");

    for (fan_idx = 0; fan_idx < data->fans; ++fan_idx) {
        read_tach_registers(client, &tachval,
                SEL_FAN(fan_idx, REG_TACH_READ_HIGH),
                SEL_FAN(fan_idx, REG_TACH_READ_LOW));
        tach_reading = calculate_rpm_from_tach_emc23xx(&data->fan[fan_idx],tachval >> TACH_VALUE_SHIFT_BITS);

        printk(KERN_NOTICE "FAN %d : Enabled %d Valid %d RPM Control %d multiplier %d poles %d", fan_idx,
                data->fan[fan_idx].enabled, data->fan[fan_idx].valid, data->fan[fan_idx].rpm_control,
                data->fan[fan_idx].multiplier,data->fan[fan_idx].poles); 
        printk(KERN_NOTICE "        Tach Count (RPM) %5d ranges %d Edges %d fanstop 0x%04x",
                tach_reading,data->fan[fan_idx].ranges,data->fan[fan_idx].edges,data->fan[fan_idx].fanstop);
        printk(KERN_NOTICE "        Tach Target (RPM) %5d Driver Setting (RPM) %5d pwm 0x%02x",
                data->fan[fan_idx].tach_target_rpm, data->fan[fan_idx].driver_setting_rpm, data->fan[fan_idx].pwm);
    }

    printk(KERN_NOTICE "\n");

    return snprintf(buf, SPEED_PWM_LEN_STRING, "%s", "");
}

/* define a read only attribute */
#define EMC2305_ATTR_RO(_type, _item, _num)			\
    SENSOR_ATTR(_type ## _num ## _ ## _item, S_IRUGO,	\
            show_## _type ## _ ## _item, NULL, _num - 1)

/* define a read/write attribute */
#define EMC2305_ATTR_RW(_type, _item, _num)				\
    SENSOR_ATTR(_type ## _num ## _ ## _item, S_IRUGO | S_IWUSR,	\
            show_## _type ##_ ## _item,				\
            set_## _type ## _ ## _item, _num - 1)

/*
 * TODO: Ugly hack, but temporary as this whole logic needs
 * to be rewritten as per standard HWMON sysfs registration
 */

/* define a read/write attribute */
#define EMC2305_ATTR_RW2(_type, _num)				\
    SENSOR_ATTR(_type ## _num, S_IRUGO | S_IWUSR,	\
            show_## _type, set_## _type, _num - 1)

/* defines the attributes for a single fan */
#define EMC2305_DEFINE_FAN_ATTRS(_num)					\
    static const							\
struct sensor_device_attribute emc2305_attr_fan ## _num[] = {	\
    EMC2305_ATTR_RO(fan, input, _num),			\
    EMC2305_ATTR_RO(fan, fault, _num),			\
    EMC2305_ATTR_RW(fan, div, _num),			\
    EMC2305_ATTR_RW(fan, target, _num),			\
    EMC2305_ATTR_RW(pwm, enable, _num),			\
    EMC2305_ATTR_RW2(pwm, _num),			\
    EMC2305_ATTR_RO(fan, dump, _num),			\
}

#define EMC2305_NUM_FAN_ATTRS ARRAY_SIZE(emc2305_attr_fan1)

/* fan attributes for the single fans */
EMC2305_DEFINE_FAN_ATTRS(1);
EMC2305_DEFINE_FAN_ATTRS(2);
EMC2305_DEFINE_FAN_ATTRS(3);
EMC2305_DEFINE_FAN_ATTRS(4);
EMC2305_DEFINE_FAN_ATTRS(5);
EMC2305_DEFINE_FAN_ATTRS(6);

/* fan attributes */
static const struct sensor_device_attribute *emc2305_fan_attrs[] = {
    emc2305_attr_fan1,
    emc2305_attr_fan2,
    emc2305_attr_fan3,
    emc2305_attr_fan4,
    emc2305_attr_fan5,
};

/* debug attributes for all fans */
static struct sensor_device_attribute emc2305_debug_attrs[] = {
	SENSOR_ATTR(debug_dump, S_IRUGO, show_debug_dump, NULL, 0),
};

#define MAX_DEBUG_ATTRS ARRAY_SIZE(emc2305_debug_attrs)

static const struct sensor_device_attribute emc2305_attr_common[] = {
};

/*
 * driver interface
 */

static void emc2305_remove(struct i2c_client *client)
{
    struct emc2305_data *data = i2c_get_clientdata(client);
    int fan_idx, i;

    hwmon_device_unregister(data->hwmon_dev);

    for (fan_idx = 0; fan_idx < data->fans; ++fan_idx)
        for (i = 0; i < EMC2305_NUM_FAN_ATTRS; ++i)
            device_remove_file(
                    &client->dev,
                    &emc2305_fan_attrs[fan_idx][i].dev_attr);

    for (i = 0; i < ARRAY_SIZE(emc2305_attr_common); ++i)
        device_remove_file(&client->dev,
                &emc2305_attr_common[i].dev_attr);

    for (i = 0; i < MAX_DEBUG_ATTRS; ++i)
        device_remove_file(&client->dev,
                &emc2305_debug_attrs[i].dev_attr);

    mutex_destroy(&data->update_lock);
    kfree(data);
}


#ifdef CONFIG_OF
/*
 * device tree support
 */

struct of_fan_attribute {
    const char *name;
    int (*set)(struct i2c_client*, int, long);
};

struct of_fan_attribute of_fan_attributes[] = {
    {"fan-div", emc2305_set_fan_div},
    {"fan-target", emc2305_set_fan_target},
    {"pwm-enable", emc2305_set_pwm_enable},
    {NULL, NULL}
};

static int emc2305_config_of(struct i2c_client *client)
{
    struct emc2305_data *data = i2c_get_clientdata(client);
    struct device_node *node;
    unsigned int fan_idx;

    if (!client->dev.of_node)
    {
        return -EINVAL;
    }
    if (!of_get_next_child(client->dev.of_node, NULL))
        return  0;

    for (fan_idx = 0; fan_idx < data->fans; ++fan_idx)
        data->fan[fan_idx].enabled = false;

    for_each_child_of_node(client->dev.of_node, node) {
        const __be32 *property;
        int len;
        struct of_fan_attribute *attr;

        property = of_get_property(node, "reg", &len);
        if (!property || len != sizeof(int)) {
            dev_err(&client->dev, "invalid reg on %s\n",
                    node->full_name);
            continue;
        }

        fan_idx = be32_to_cpup(property);
        if (fan_idx >= data->fans) {
            dev_err(&client->dev,
                    "invalid fan index %d on %s\n",
                    fan_idx, node->full_name);
            continue;
        }

        data->fan[fan_idx].enabled = true;

        for (attr = of_fan_attributes; attr->name; ++attr) {
            int status = 0;
            long value;
            property = of_get_property(node, attr->name, &len);
            if (!property)
                continue;
            if (len != sizeof(int)) {
                dev_err(&client->dev, "invalid %s on %s\n",
                        attr->name, node->full_name);
                continue;
            }
            value = be32_to_cpup(property);
            status = attr->set(client, fan_idx, value);
            if (status == -EINVAL) {
                dev_err(&client->dev,
                        "invalid value for %s on %s\n",
                        attr->name, node->full_name);
            }
        }
    }

    return 0;
}

#endif

static void emc2305_device_init(struct i2c_client *client)
{
    int fan_index;
    uint16_t range = 0, edges = 0, fanstop = 0;
    uint8_t conf = 0, value = 0;
    struct emc2305_data *data = i2c_get_clientdata (client);
    bool rpm_control;

    switch (tach_count_multiplier) {
        case 1:
            range = 0;
            fanstop = (uint16_t)0xFFF8;
            break;
        case 2:
            range = 1;
            fanstop = (uint16_t)0xFFF0;
            break;
        case 4:
            range = 2;
            fanstop = (uint16_t)0xFFE0;
            break;
        case 8:
            range = 3;
            fanstop = (uint16_t)0xFFC0;
            break;
        default:
            printk(KERN_ERR "Invalid tach_count_multiplier specified. Using default (%d).\n",DEFAULT_TACH_COUNT_MULTIPLIER);
            tach_count_multiplier = DEFAULT_TACH_COUNT_MULTIPLIER;
            range = 2;
            fanstop = (uint16_t)0xFFE0;
    }

    if(poles > 4) {
        printk(KERN_ERR "Invalid poles specified. Using default (%d).\n",DEFAULT_POLES);
        poles = DEFAULT_POLES;
        edges = (poles * 2) + 1;
    }
    edges = (poles * 2) + 1;

    rpm_control = (mode == 0) ? false : true;

    for (fan_index = 0; fan_index < data->fans; ++fan_index) {
        data->fan[fan_index].rpm_control = rpm_control;
        data->fan[fan_index].ranges = range;
        data->fan[fan_index].multiplier = tach_count_multiplier;
        data->fan[fan_index].edges = edges;
        data->fan[fan_index].poles = poles;
        data->fan[fan_index].fanstop = fanstop;
    }

    mutex_lock(&data->update_lock);

    /* Clear status registers */
    emc2305_clear_fan_fault (client);

    conf = REG_PWM_OUTPUT_CONFIG;
    value = 0x1f;
    i2c_smbus_write_byte_data (client, conf, value);

    conf = REG_PWM_BASE_FREQ_1;
    value = 0x0f;
    i2c_smbus_write_byte_data (client, conf, value);

    conf = REG_PWM_BASE_FREQ_2;
    value = 0x3f;
    i2c_smbus_write_byte_data (client, conf, value);


    for (fan_index = 0; fan_index < data->fans; ++fan_index) {
        conf = SEL_FAN (fan_index, REG_FAN_MAX_STEP);
        value = 0x01;
        i2c_smbus_write_byte_data (client, conf, value);

        conf = SEL_FAN (fan_index, REG_FAN_VALID_TACH_COUNT);
        value = 0xfe;
        i2c_smbus_write_byte_data (client, conf, value);

        if (data->fan[fan_index].rpm_control) {
            write_fan_target (client, fan_index, FAN_INIT_RPM_SPEED);
        } else {
            write_fan_setting (client, fan_index, FAN_INIT_RPM_SPEED);
        }

        conf = SEL_FAN (fan_index, REG_FAN_MINIMUM_DRIVE);
        value = 0x01;
        i2c_smbus_write_byte_data (client, conf, value);

        conf = SEL_FAN (fan_index, REG_PWM_DIVIDE);
        value = 0x0;
        i2c_smbus_write_byte_data (client, conf, value);

        /*  Change to RPM Mode */
        conf = SEL_FAN (fan_index, REG_FAN_CONFIGURATION_1);
        value = ((data->fan[fan_index].rpm_control & 0x1) << 7) |
                (range << 5) |
                ((data->fan[fan_index].poles - 1) << 3) |
                0x3; // 400 ms update interval. Refer to REGISTER 6-13: FAN CONFIG - FAN CONFIGURATION REGISTERS 
        i2c_smbus_write_byte_data (client, conf, value);
    }

    mutex_unlock(&data->update_lock);
}

static void emc2305_get_config(struct i2c_client *client)
{
    int i;
    struct emc2305_data *data = i2c_get_clientdata(client);

    for (i = 0; i < data->fans; ++i) {
        data->fan[i].enabled = true;
        emc2305_update_fan(client, i);
    }

#ifdef CONFIG_OF
    emc2305_config_of(client);
#endif

}

static int emc2305_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct emc2305_data *data;
    int status;
    int i;
    int fan_idx, debug_idx;
    unsigned short max_fans_supported;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
        return -EIO;

    data = kzalloc(sizeof(struct emc2305_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = i2c_smbus_read_byte_data(client, REG_PRODUCT_ID);
    switch (status) {
        case 0x34: /* EMC2305 */
            max_fans_supported = 5;
            break;
        case 0x35: /* EMC2303 */
            max_fans_supported = 3;
            break;
        case 0x36: /* EMC2302 */
            max_fans_supported = 2;
            break;
        case 0x37: /* EMC2301 */
            max_fans_supported = 1;
            break;
        default:
            if (status >= 0) {

                status = -EINVAL;
            }
            goto exit_free;
    }

    data->fans = max_fans_supported;
    if(num_fans && (num_fans < max_fans_supported)) {
        data->fans = num_fans;
    }

    /* initialise EMC2305 driver */
    emc2305_device_init(client);

    emc2305_get_config(client);

    for (i = 0; i < ARRAY_SIZE(emc2305_attr_common); ++i) {
        status = device_create_file(&client->dev,
                &emc2305_attr_common[i].dev_attr);
        if (status)
            goto exit_remove;
    }
    for (fan_idx = 0; fan_idx < data->fans; ++fan_idx)
        for (i = 0; i < EMC2305_NUM_FAN_ATTRS; ++i) {
            if (!data->fan[fan_idx].enabled)
                continue;
            status = device_create_file(
                    &client->dev,
                    &emc2305_fan_attrs[fan_idx][i].dev_attr);
            if (status)
                goto exit_remove_fans;
        }

    for (debug_idx = 0; debug_idx < MAX_DEBUG_ATTRS; ++debug_idx) {
            status = device_create_file(
                    &client->dev,
                    &emc2305_debug_attrs[debug_idx].dev_attr);
            if (status)
                goto exit_remove;
    }

    data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(data->hwmon_dev)) {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove_fans;
    }

    dev_info(&client->dev, "%s: sensor '%s'\n",
            dev_name(data->hwmon_dev), client->name);

    return 0;

exit_remove_fans:
    for (fan_idx = 0; fan_idx < data->fans; ++fan_idx)
        for (i = 0; i < EMC2305_NUM_FAN_ATTRS; ++i)
            device_remove_file(
                    &client->dev,
                    &emc2305_fan_attrs[fan_idx][i].dev_attr);

exit_remove:
    for (i = 0; i < ARRAY_SIZE(emc2305_attr_common); ++i)
        device_remove_file(&client->dev,
                &emc2305_attr_common[i].dev_attr);

    for (i = 0; i < MAX_DEBUG_ATTRS; ++i)
        device_remove_file(&client->dev,
                &emc2305_debug_attrs[i].dev_attr);

exit_free:
    mutex_destroy(&data->update_lock);
    kfree(data);
    return status;
}

static const struct i2c_device_id emc2305_id[] = {
    { "emc2305", 0 },
    { "emc2303", 0 },
    { "emc2302", 0 },
    { "emc2301", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, emc2305_id);

static struct i2c_driver emc2305_driver = {
    .class		= I2C_CLASS_HWMON,
    .driver = {
        .name	= "emc2305",
    },
    .probe		= emc2305_probe,
    .remove		= emc2305_remove,
    .id_table	= emc2305_id,
};

module_i2c_driver(emc2305_driver);

MODULE_PARM_DESC(mode, "FAN Control Operating Mode: 0=Direct Setting, 1=RPM based FSC (default)");
MODULE_PARM_DESC(num_fans, "Number of FANs in this platform <1..5>");
MODULE_PARM_DESC(tach_count_multiplier, "FAN TACH count multiplier <1/2/4/8>");
MODULE_PARM_DESC(poles, "FAN poles <1..4>");

MODULE_AUTHOR("Reinhard Pfau <pfau@gdsys.de>");
MODULE_DESCRIPTION("SMSC EMC2305 hwmon driver");
MODULE_LICENSE("GPL");

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

#define FAN_MAX_RPM_SPEED     28600
#define TACH_VALUE_SHIFT_BITS 3
#define SPEED_PWM_LEN_STRING  7

struct emc2305_fan_data {
    bool        enabled;
    bool        valid;
    unsigned long   last_updated;
    bool        rpm_control;
    uint8_t     multiplier;
    uint8_t     poles;
    uint16_t    target;
    uint16_t    tach;
    uint16_t    rpm_factor;
    uint8_t     pwm;
    uint8_t     ranges;
    uint8_t     edges;
};

struct emc2305_data {
    struct device		*hwmon_dev;
    struct mutex		update_lock;
    int			fans;
    struct emc2305_fan_data	fan[5];
};

/* Fault status registers */
static const uint8_t emc2305_fault_status_reg[] = { REG_FAN_STATUS,
    REG_FAN_STALL_STATUS,
    REG_FAN_SPIN_STATUS,
    REG_DRIVE_FAIL_STATUS };

/*
 * @brief Convert speed percentage value to volt.
 * @param percent[in] - RPM speed in percent
 * Return      - corresponding voltage value.
 */
static uint8_t emc2305_speed_percent_to_drv_volt_get(uint8_t percent)
{
    return ((percent * EMC2305_FAN_SETTING_MAX_VAL) / EMC2305_PERCENT_VAL);
}

/*
 * @brief Convert RPM to speed percentage.
 * @param max_speed[in] - Max Fan speed.
 * @param rpm[in]       - RPM value.
 * Return        - corresponding speed_percent value.
 */
static uint8_t emc2305_rpm_to_speed_percent_get(uint32_t max_speed, long rpm)
{
    return ((rpm * EMC2305_PERCENT_VAL) / max_speed);
}

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
 * @brief This function calculates the fan rpm based on the passed parameters.
 * @param[in] fan - fan details
 * @param[in] tachval - The tach counts of the fan
 * @returns speed of the fan in RPM
 */
uint32_t calculate_rpm_emc23xx(struct emc2305_fan_data *fan, uint16_t tachval)
{
    uint32_t speed = 0;

    if ((tachval == 0) || (fan->poles == 0)) {
        return 0;
    }

    speed = (((fan->edges - 1) * EMC2305_TACH_FREQ * fan->multiplier * EMC2305_RPM_CONST_VAL)
            / (tachval * fan->poles));

    return speed;
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

static void read_fan_from_i2c(struct i2c_client *client, uint16_t *output,
        uint8_t hi_addr, uint8_t lo_addr)
{
    uint8_t high_byte = 0, lo_byte = 0;

    if (read_u8_from_i2c (client, hi_addr, &high_byte) < 0)
        return;

    if (read_u8_from_i2c (client, lo_addr, &lo_byte) < 0)
        return;

    *output = (((uint16_t) high_byte << 8) | (lo_byte));
}

static void write_fan_target_to_i2c(struct i2c_client *client, int fan,
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
}

static void read_fan_config_from_i2c(struct i2c_client *client, int fan)

{
    struct emc2305_data *data = i2c_get_clientdata(client);
    uint8_t conf = 0;

    if (read_u8_from_i2c(client, SEL_FAN(fan, REG_FAN_CONFIGURATION_1),
                &conf) < 0) {
        return;
    }
    data->fan[fan].rpm_control = (conf & 0x80) != 0;
    data->fan[fan].ranges = ((conf & EMC2305_FAN_CONFIG_RANGE_MASK) >> 5);
    data->fan[fan].multiplier = 1 << (data->fan[fan].ranges);
    data->fan[fan].poles = ((conf & 0x18) >> 3) + 1;
    data->fan[fan].edges = (data->fan[fan].poles * 2) + 1;
}

static void read_fan_setting(struct i2c_client *client, int fan)
{
    struct emc2305_data *data = i2c_get_clientdata(client);
    uint8_t setting = 0;

    if (read_u8_from_i2c(client, SEL_FAN(fan, REG_FAN_SETTING),
                &setting) < 0)
        return;

    data->fan[fan].pwm = setting;
}

static void read_fan_data(struct i2c_client *client, int fan_idx)
{
    struct emc2305_data *data = i2c_get_clientdata(client);

    read_fan_from_i2c(client, &data->fan[fan_idx].target,
            SEL_FAN(fan_idx, REG_TACH_TARGET_HIGH),
            SEL_FAN(fan_idx, REG_TACH_TARGET_LOW));

    read_fan_from_i2c(client, &data->fan[fan_idx].tach,
            SEL_FAN(fan_idx, REG_TACH_READ_HIGH),
            SEL_FAN(fan_idx, REG_TACH_READ_LOW));
}

    static struct emc2305_fan_data *
emc2305_update_fan(struct i2c_client *client, int fan_idx)
{
    struct emc2305_data *data = i2c_get_clientdata(client);
    struct emc2305_fan_data *fan_data = &data->fan[fan_idx];

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, fan_data->last_updated + HZ + HZ / 2)
            || !fan_data->valid) {
        read_fan_config_from_i2c(client, fan_idx);
        read_fan_data(client, fan_idx);
        read_fan_setting(client, fan_idx);
        fan_data->valid = true;
        fan_data->last_updated = jiffies;
    }

    mutex_unlock(&data->update_lock);
    return fan_data;
}

    static struct emc2305_fan_data *
emc2305_update_device_fan(struct device *dev, struct device_attribute *da)
{
    struct i2c_client *client = to_i2c_client(dev);
    int fan_idx = to_sensor_dev_attr(da)->index;

    return emc2305_update_fan(client, fan_idx);
}

static void read_fan_status(struct device *dev, struct device_attribute *da, bool *fault)
{
    struct i2c_client *client = to_i2c_client(dev);
    int fan_idx = to_sensor_dev_attr(da)->index;
    uint8_t reg = 0, index = 0;
    int rc = 0;
    *fault = false;
    for (index = 1; index < sizeof(emc2305_fault_status_reg); index++) {
        rc = read_u8_from_i2c(client, emc2305_fault_status_reg[index], &reg);
        if (rc < 0) {
            return;
        }
        *fault = (reg & (1 << fan_idx)) ? true : false;
        if (*fault == true)
             return;
    }
    rc = read_u8_from_i2c(client, emc2305_fault_status_reg[0], &reg);
    if (rc < 0) {
        return;
    }
    *fault = (reg & EMC2305_FAN_WATCHDOG_STATUS_MASK) ? true : false;
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
    static int
emc2305_set_fan_div(struct i2c_client *client, int fan_idx, long new_div)
{
    struct emc2305_data *data = i2c_get_clientdata(client);
    struct emc2305_fan_data *fan = emc2305_update_fan(client, fan_idx);
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
    if ((fan->target & 0x7fff) != 0x7fff) {
        uint16_t new_target = (fan->target * old_div) / new_div;
        fan->target = min_t(uint16_t, new_target, FAN_MAX_RPM_SPEED);
        write_fan_target_to_i2c(client, fan_idx, fan->target);
    }

exit_invalidate:
    /* invalidate fan data to force re-read from hardware */
    fan->valid = false;
exit_unlock:
    mutex_unlock(&data->update_lock);
    return status;
}

    static int
emc2305_set_fan_target(struct i2c_client *client, int fan_idx, long rpm_target)
{
    struct emc2305_data *data = i2c_get_clientdata (client);
    struct emc2305_fan_data *fan = emc2305_update_fan (client, fan_idx);
    const uint8_t reg_fan_conf1 = SEL_FAN (fan_idx, REG_FAN_CONFIGURATION_1);
    int status = 0;
    uint8_t conf_reg = 0, speed_percent = 0, reg = 0, setting = 0;

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
        write_fan_target_to_i2c (client, fan_idx, (uint16_t) rpm_target);
        /* Set RPM mode */
        status = read_u8_from_i2c (client, reg_fan_conf1, &conf_reg);
        if (status < 0) {
            status = -EIO;
        }

        fan->rpm_control = true;
        conf_reg |= 0x80;

        status = i2c_smbus_write_byte_data (client, reg_fan_conf1, conf_reg);
        if (status < 0) {
            status = -EIO;
        }

    } else {
        /* Direct Method */
        /* Set the fan speed */
        speed_percent = emc2305_rpm_to_speed_percent_get (FAN_MAX_RPM_SPEED, rpm_target);
        setting = emc2305_speed_percent_to_drv_volt_get (speed_percent);
        reg = SEL_FAN (fan_idx, REG_FAN_SETTING);
        i2c_smbus_write_byte_data (client, reg, setting);
    }

    mutex_unlock (&data->update_lock);
    return status;
}

    static int
emc2305_set_pwm_enable(struct i2c_client *client, int fan_idx, long enable)
{
    struct emc2305_data *data = i2c_get_clientdata(client);
    struct emc2305_fan_data *fan = emc2305_update_fan(client, fan_idx);
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

    static int
emc2305_set_pwm(struct i2c_client *client, int fan_idx, long pwm)
{
    struct emc2305_data *data = i2c_get_clientdata(client);
    struct emc2305_fan_data *fan = emc2305_update_fan(client, fan_idx);
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

    static ssize_t
read_fan_input_target(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct emc2305_data *data = i2c_get_clientdata(client);
    struct emc2305_fan_data *fan = emc2305_update_device_fan(dev, da);
    int fan_idx = to_sensor_dev_attr(da)->index;
    uint32_t speed = 0;
    uint16_t tach_fanstop_mask = 0, fanstop = 0, tachval =0;
    uint8_t index = 0, conf = 0, value = 0, range = 0;

    mutex_lock(&data->update_lock);
    read_fan_from_i2c(client, &tachval,
            SEL_FAN(fan_idx, REG_TACH_READ_HIGH),
            SEL_FAN(fan_idx, REG_TACH_READ_LOW));

    // Get Fan Configuration value
    conf = SEL_FAN (fan_idx, REG_FAN_CONFIGURATION_1);
    value = i2c_smbus_read_byte_data (client, conf);

    // Get Range from Configuration value
    range = ((value & EMC2305_FAN_CONFIG_RANGE_MASK) >> 5);

    switch (range) {
        case 0:
            fanstop = (uint16_t)0xFFF8;
            break;
        case 1:
            fanstop = (uint16_t)0xFFF0;
            break;
        case 2:
            fanstop = (uint16_t)0xFFE0;
            break;
        case 3:
            fanstop = (uint16_t)0xFFC0;
            break;
        default:
            //shouldn't reach here, as the mask 0x60
            return 0;
    }

    // TACH_READING_DEFAULT_VAL: 0xFFF8, tach_fanstop_mask: 0x7
    tach_fanstop_mask = (uint16_t)~TACH_READING_DEFAULT_VAL;

    for (index = 1; index <= range; index++) {
        tach_fanstop_mask = (tach_fanstop_mask << 1);
        tach_fanstop_mask |=  0x1;
    }

    // Mask off the lower bits which are indeterminate
    tachval &= ~tach_fanstop_mask;
    tachval = tachval >> TACH_VALUE_SHIFT_BITS;

    /* Disable the fan speed in case of fanstop value */
    if (tachval == fanstop) {
        speed = 0;
    } else {
        speed = calculate_rpm_emc23xx(fan, tachval);
    }

    mutex_unlock(&data->update_lock);
    return snprintf(buf, SPEED_PWM_LEN_STRING, "%d\n", speed);
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
    static ssize_t
show_fan_input(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client (dev);
    int fan_idx = to_sensor_dev_attr (da)->index;
    struct emc2305_data *data = i2c_get_clientdata (client);
    struct emc2305_fan_data *fan = emc2305_update_fan (client, fan_idx);
    uint8_t setting = 0, speed_percent = 0;
    uint32_t speed = 0;

    if (fan->rpm_control) {
        return read_fan_input_target (dev, da, buf);
    } else {
        if (read_u8_from_i2c(client, SEL_FAN (fan_idx, REG_FAN_SETTING), &setting) < 0) {
            return 0;
        }
        speed_percent = (setting * EMC2305_PERCENT_VAL) / (EMC2305_FAN_SETTING_MAX_VAL);
        speed = (FAN_MAX_RPM_SPEED * speed_percent) / EMC2305_PERCENT_VAL;
        return snprintf (buf, SPEED_PWM_LEN_STRING, "%d\n", speed);
    }

}

    static ssize_t
show_fan_fault(struct device *dev, struct device_attribute *da, char *buf)
{
    struct emc2305_fan_data *fan = emc2305_update_device_fan(dev, da);
    bool fault;
    read_fan_status(dev, da, &fault);
    return snprintf(buf, SPEED_PWM_LEN_STRING, "%d\n", fault ? 1 : 0);
}

    static ssize_t
show_fan_div(struct device *dev, struct device_attribute *da, char *buf)
{
    struct emc2305_fan_data *fan = emc2305_update_device_fan(dev, da);
    int fan_div = 8 / fan->multiplier;
    return snprintf(buf, SPEED_PWM_LEN_STRING, "%d\n", fan_div);
}

    static ssize_t
set_fan_div(struct device *dev, struct device_attribute *da,
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

    static ssize_t
show_fan_target(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client (dev);
    int fan_idx = to_sensor_dev_attr (da)->index;
    struct emc2305_data *data = i2c_get_clientdata (client);
    struct emc2305_fan_data *fan = emc2305_update_fan (client, fan_idx);
    uint8_t setting = 0, speed_percent = 0;
    uint32_t speed = 0;

    if (fan->rpm_control) {
        /* RPM mode */
        return read_fan_input_target (dev, da, buf);
    } else {
        /* Direct Method */
        if (read_u8_from_i2c(client, SEL_FAN (fan_idx, REG_FAN_SETTING), &setting) < 0) {
            return 0;
        }
        speed_percent = (setting * EMC2305_PERCENT_VAL) / (EMC2305_FAN_SETTING_MAX_VAL);
        speed = (FAN_MAX_RPM_SPEED * speed_percent) / EMC2305_PERCENT_VAL;
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

    static ssize_t
show_pwm_enable(struct device *dev, struct device_attribute *da, char *buf)
{
    struct emc2305_fan_data *fan = emc2305_update_device_fan(dev, da);
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
    struct emc2305_fan_data *fan = emc2305_update_device_fan(dev, da);
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
    EMC2305_ATTR_RW2(pwm, _num)			\
}

#define EMC2305_NUM_FAN_ATTRS ARRAY_SIZE(emc2305_attr_fan1)

/* common attributes for EMC2303 and EMC2305 */
static const struct sensor_device_attribute emc2305_attr_common[] = {
};

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

/*
 * driver interface
 */

static int emc2305_remove(struct i2c_client *client)
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

    kfree(data);
    return 0;
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
    uint16_t rpm = 0;
    uint8_t conf = 0;
    uint8_t value = 0;
    struct emc2305_data *data = i2c_get_clientdata (client);

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
        conf = SEL_FAN (fan_index, REG_FAN_CONFIGURATION_1);
        value = 0x4b;
        i2c_smbus_write_byte_data (client, conf, value);

        conf = SEL_FAN (fan_index, REG_FAN_MAX_STEP);
        value = 0x01;
        i2c_smbus_write_byte_data (client, conf, value);

        conf = SEL_FAN (fan_index, REG_FAN_VALID_TACH_COUNT);
        value = 0xfe;
        i2c_smbus_write_byte_data (client, conf, value);

        /* Set default speed (12000 RPM) */
        rpm = 12000;
        write_fan_target_to_i2c (client, fan_index, rpm);

        conf = SEL_FAN(fan_index, REG_FAN_SETTING);
        value = 0x0;
        i2c_smbus_write_byte_data (client, conf, value);

        conf = SEL_FAN (fan_index, REG_FAN_MINIMUM_DRIVE);
        value = 0x01;
        i2c_smbus_write_byte_data (client, conf, value);

        conf = SEL_FAN(fan_index, REG_FAN_SETTING);
        value = 0x0;
        i2c_smbus_write_byte_data (client, conf, value);

        /*  Change to RPM Mode */
        conf = SEL_FAN (fan_index, REG_FAN_CONFIGURATION_1);
        value = i2c_smbus_read_byte_data (client, conf);
        value |= 0x80;
        i2c_smbus_write_byte_data (client, conf, value);

        conf = SEL_FAN (fan_index, REG_PWM_DIVIDE);
        value = 0x0;
        i2c_smbus_write_byte_data (client, conf, value);

        conf = SEL_FAN(fan_index, REG_FAN_SETTING);
        value = 0x0;
        i2c_smbus_write_byte_data (client, conf, value);

        conf = SEL_FAN (fan_index, REG_FAN_CONFIGURATION_1);
        value = 0x4b;
        i2c_smbus_write_byte_data (client, conf, value);
    }

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

    static int
emc2305_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct emc2305_data *data;
    int status;
    int i;
    int fan_idx;

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
            data->fans = 5;
            break;
        case 0x35: /* EMC2303 */
            data->fans = 3;
            break;
        case 0x36: /* EMC2302 */
            data->fans = 2;
            break;
        case 0x37: /* EMC2301 */
            data->fans = 1;
            break;
        default:
            if (status >= 0) {

                status = -EINVAL;
            }
            goto exit_free;
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
exit_free:
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

/* Return 0 if detection is successful, -ENODEV otherwise */
    static int
emc2305_detect(struct i2c_client *new_client, struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = new_client->adapter;
    int manufacturer, product;

    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
        return -ENODEV;

    manufacturer =
        i2c_smbus_read_byte_data(new_client, REG_MANUFACTURER_ID);
    if (manufacturer != 0x5D)
        return -ENODEV;

    product = i2c_smbus_read_byte_data(new_client, REG_PRODUCT_ID);

    switch (product) {
        case 0x34:
            strlcpy(info->type, "emc2305", I2C_NAME_SIZE);
            break;
        case 0x35:
            strlcpy(info->type, "emc2303", I2C_NAME_SIZE);
            break;
        case 0x36:
            strlcpy(info->type, "emc2302", I2C_NAME_SIZE);
            break;
        case 0x37:
            strlcpy(info->type, "emc2301", I2C_NAME_SIZE);
            break;
        default:
            return -ENODEV;
    }

    return 0;
}

static struct i2c_driver emc2305_driver = {
    .class		= I2C_CLASS_HWMON,
    .driver = {
        .name	= "emc2305",
    },
    .probe		= emc2305_probe,
    .remove		= emc2305_remove,
    .id_table	= emc2305_id,
    /*
       .detect		= emc2305_detect,
       .address_list	= i2c_adresses,
     */
};

module_i2c_driver(emc2305_driver);

MODULE_AUTHOR("Reinhard Pfau <pfau@gdsys.de>");
MODULE_DESCRIPTION("SMSC EMC2305 hwmon driver");
MODULE_LICENSE("GPL");

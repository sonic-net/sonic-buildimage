/*
 * A hwmon driver for the wistron_switch_cpld
 *
 */

/* ----------------------------------------------------------------------------
 * Include files
 * ----------------------------------------------------------------------------
 */
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>

/* ---------------------------------------------------------------------------
 * Constant
 * ----------------------------------------------------------------------------
 */
#include "wistron_cpld.h"

static LIST_HEAD(cpld_client_list);
static struct mutex list_lock;

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

/* ----------------------------------------------------------------------------
 *
 * Module attribute functions
 * ----------------------------------------------------------------------------
 */
/*
static ssize_t cpld_show_board_version(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0, ver = 0, ver2 = 0;
    unsigned char command = CPLD_HW_REVISION_REG;
    struct i2c_client *client = to_i2c_client(dev);

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    ver = (val & 0xf0) >> 4;
    ver2 = (val & 0xf);
    return sprintf(buf, "%s %s\n", (ver == 0) ? "EVT" : (ver == 1) ? "DVT" : "PVT", (ver2 == 0) ? "with CPU" : "without CPU" );
}
*/

static ssize_t cpld_show_board_id(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, ver = 0;
    unsigned char command = CPLD_HW_REVISION_REG;
    struct i2c_client *client = to_i2c_client(dev);

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    ver = (val & 0x7);

    return sprintf(buf, "%d\n", ver);
}

static ssize_t cpld_show_fan_direction(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, ver = 0;
    unsigned char command = CPLD_HW_REVISION_REG;
    struct i2c_client *client = to_i2c_client(dev);

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    ver = (val & 0x4) >> 2;

    return sprintf(buf, "%d\n", ver);
}

static ssize_t cpld_show_version(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, mjr_ver = 0, mnr_ver = 0;
    unsigned char command = CPLD_REVISION_REG;
    struct i2c_client *client = to_i2c_client(dev);

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    mjr_ver = (val & 0xf0) >> 4;
    mnr_ver = (val & 0xf) ;

    return sprintf(buf, "%d.%d\n", mjr_ver, mnr_ver);
}

static ssize_t cpld_show_build_date(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, val2 = 0, b_year = 0, b_month = 0, b_day = 0;
    unsigned char command = CPLD_BUILD_YEAR_REG;
    unsigned char command2 = CPLD_BUILD_DATE_REG;
    struct i2c_client *client = to_i2c_client(dev);

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val2 = i2c_smbus_read_byte_data(client, command2);
    if (val2 < 0) {
        mutex_unlock(&list_lock);
        return val2;
    }

    mutex_unlock(&list_lock);

    b_year = (val & 0xfe) >> 1;
    b_month = ((val & 0x1) << 3) + ((val2 & 0xe0) >> 5);
    b_day = val2 & 0x1f;
    return sprintf(buf, "20%02d %02d %02d\n", b_year, b_month, b_day);
}

static ssize_t cpld_read_psu_status(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0, bit = 0;
    unsigned char command = CPLD_PSU_CHECK_REG;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);

    bit = sda->index;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    res = (val & (1 << bit)) >> bit;
    //0 : present
    //1 : not present
    //0 : power on
    //1 : power off
    if ( (bit == PSU2_PRESENT) || (bit == PSU1_PRESENT) || (bit == PSU2_ON) || (bit == PSU1_ON) )
        res = (res) ? 0 : 1;

    return sprintf(buf, "%d\n", res);
}

static ssize_t cpld_write_psu_status(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    unsigned char command = CPLD_PSU_CHECK_REG;
    int val = 0, res = 0, bit = 0, mask = 0;

    val = kstrtoint(buf, 10, &res);

    if (val)
        return val;

    if (res < 0 || res > 1)
        return -EINVAL;

    bit = sda->index;
    mask = (0x1) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val &= ~(mask << bit);

    val |= (~(res) << bit);

    i2c_smbus_write_byte_data(client, command, val);

    mutex_unlock(&list_lock);

    return count;
}

static ssize_t cpld_show_power_ctrl(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, val2 = 0, i = 0;
    unsigned char command = CPLD_POWER_CTL1_REG;
    unsigned char command2 = CPLD_POWER_CTL2_REG;
    struct i2c_client *client = to_i2c_client(dev);

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val2 = i2c_smbus_read_byte_data(client, command2);
    if (val2 < 0) {
        mutex_unlock(&list_lock);
        return val2;
    }

    mutex_unlock(&list_lock);

    for (i = 0; i < 8; i++) {
        sprintf(buf + strlen(buf), "%s: %s\n", power_ctrl_str1[i], ((val & (1 << i)) >> i) ? "Enable" : "Disable");
    }
    for (i = 0; i < 8; i++) {
        sprintf(buf + strlen(buf), "%s: %s\n", power_ctrl_str2[i], ((val2 & (1 << i)) >> i) ? "Enable" : "Disable");
    }

    return (ssize_t)strlen(buf);;
}

static ssize_t cpld_show_power_good(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, val2 = 0, i = 0;
    unsigned char command = CPLD_POWER_GOOD_REG;
    unsigned char command2 = CPLD_POWER_GOOD2_REG;
    struct i2c_client *client = to_i2c_client(dev);

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val2 = i2c_smbus_read_byte_data(client, command2);
    if (val2 < 0) {
        mutex_unlock(&list_lock);
        return val2;
    }
    mutex_unlock(&list_lock);

    for (i = 0; i < 8; i++) {
        sprintf(buf + strlen(buf), "%s: %s\n", power_good_str1[i], ((val & (1 << i)) >> i) ? "OK" : "Failed");
    }
    for (i = 0; i < 8; i++) {
        sprintf(buf + strlen(buf), "%s: %s\n", power_good_str2[i], ((val2 & (1 << i)) >> i) ? "OK" : "Failed");
    }
    return (ssize_t)strlen(buf);;
}

static ssize_t port_led_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0, bit = 0, mask = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    unsigned char command = 0;

    if ((sda->index == PORT1) || (sda->index == PORT2))
        command = CPLD_1G_LED_CTL1_REG;
    else if ((sda->index == PORT3) || (sda->index == PORT4))
        command = CPLD_1G_LED_CTL2_REG;
    else if ((sda->index == PORT5) || (sda->index == PORT6))
        command = CPLD_1G_LED_CTL3_REG;
    else if ((sda->index == PORT7) || (sda->index == PORT8))
        command = CPLD_1G_LED_CTL4_REG;
    else if ((sda->index == PORT9) || (sda->index == PORT10))
        command = CPLD_1G_LED_CTL5_REG;
    else if ((sda->index == PORT11) || (sda->index == PORT12))
        command = CPLD_1G_LED_CTL6_REG;
    else if ((sda->index == PORT13) || (sda->index == PORT14))
        command = CPLD_1G_LED_CTL7_REG;
    else if ((sda->index == PORT15) || (sda->index == PORT16))
        command = CPLD_1G_LED_CTL8_REG;
    else if ((sda->index == PORT17) || (sda->index == PORT18))
        command = CPLD_1G_LED_CTL9_REG;
    else if ((sda->index == PORT19) || (sda->index == PORT20))
        command = CPLD_1G_LED_CTL10_REG;
    else if ((sda->index == PORT21) || (sda->index == PORT22))
        command = CPLD_1G_LED_CTL11_REG;
    else if ((sda->index == PORT23) || (sda->index == PORT24))
        command = CPLD_1G_LED_CTL12_REG;
    else if ((sda->index == PORT25) || (sda->index == PORT26))
        command = CPLD_1G_LED_CTL13_REG;
    else if ((sda->index == PORT27) || (sda->index == PORT28))
        command = CPLD_1G_LED_CTL14_REG;
    else if ((sda->index == PORT29) || (sda->index == PORT30))
        command = CPLD_1G_LED_CTL15_REG;
    else if ((sda->index == PORT31) || (sda->index == PORT32))
        command = CPLD_1G_LED_CTL16_REG;
    else if ((sda->index == PORT33) || (sda->index == PORT34))
        command = CPLD_2_5G_LED_CTL1_REG;
    else if ((sda->index == PORT35) || (sda->index == PORT36))
        command = CPLD_2_5G_LED_CTL2_REG;
    else if ((sda->index == PORT37) || (sda->index == PORT38))
        command = CPLD_2_5G_LED_CTL3_REG;
    else if ((sda->index == PORT39) || (sda->index == PORT40))
        command = CPLD_2_5G_LED_CTL4_REG;
    else if ((sda->index == PORT41) || (sda->index == PORT42))
        command = CPLD_2_5G_LED_CTL5_REG;
    else if ((sda->index == PORT43) || (sda->index == PORT44))
        command = CPLD_2_5G_LED_CTL6_REG;
    else if ((sda->index == PORT45) || (sda->index == PORT46))
        command = CPLD_2_5G_LED_CTL7_REG;
    else if ((sda->index == PORT47) || (sda->index == PORT48))
        command = CPLD_2_5G_LED_CTL8_REG;
    else if ((sda->index == PORT49) || (sda->index == PORT50))
        command = CPLD_SFP_LED_CTL1_REG;
    else if ((sda->index == PORT51) || (sda->index == PORT52))
        command = CPLD_SFP_LED_CTL2_REG;
    else if ((sda->index == PORT53) || (sda->index == PORT54))
        command = CPLD_SFP_LED_CTL3_REG;

    if (sda->index < PORT33) {
        bit = ((sda->index) % 2) * 4;
        mask = (0x7) << bit;
    } else{
        bit = ((sda->index + 1) % 2) * 4;
        mask = (0x7) << bit;
    }

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    res = (val & mask) >> bit;

    return sprintf(buf, "%d\n", res);
}

static ssize_t port_led_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    int val = 0, res = 0, bit = 0, mask = 0;
    unsigned char command = 0;

    if ((sda->index == PORT1) || (sda->index == PORT2))
        command = CPLD_1G_LED_CTL1_REG;
    else if ((sda->index == PORT3) || (sda->index == PORT4))
        command = CPLD_1G_LED_CTL2_REG;
    else if ((sda->index == PORT5) || (sda->index == PORT6))
        command = CPLD_1G_LED_CTL3_REG;
    else if ((sda->index == PORT7) || (sda->index == PORT8))
        command = CPLD_1G_LED_CTL4_REG;
    else if ((sda->index == PORT9) || (sda->index == PORT10))
        command = CPLD_1G_LED_CTL5_REG;
    else if ((sda->index == PORT11) || (sda->index == PORT12))
        command = CPLD_1G_LED_CTL6_REG;
    else if ((sda->index == PORT13) || (sda->index == PORT14))
        command = CPLD_1G_LED_CTL7_REG;
    else if ((sda->index == PORT15) || (sda->index == PORT16))
        command = CPLD_1G_LED_CTL8_REG;
    else if ((sda->index == PORT17) || (sda->index == PORT18))
        command = CPLD_1G_LED_CTL9_REG;
    else if ((sda->index == PORT19) || (sda->index == PORT20))
        command = CPLD_1G_LED_CTL10_REG;
    else if ((sda->index == PORT21) || (sda->index == PORT22))
        command = CPLD_1G_LED_CTL11_REG;
    else if ((sda->index == PORT23) || (sda->index == PORT24))
        command = CPLD_1G_LED_CTL12_REG;
    else if ((sda->index == PORT25) || (sda->index == PORT26))
        command = CPLD_1G_LED_CTL13_REG;
    else if ((sda->index == PORT27) || (sda->index == PORT28))
        command = CPLD_1G_LED_CTL14_REG;
    else if ((sda->index == PORT29) || (sda->index == PORT30))
        command = CPLD_1G_LED_CTL15_REG;
    else if ((sda->index == PORT31) || (sda->index == PORT32))
        command = CPLD_1G_LED_CTL16_REG;
    else if ((sda->index == PORT33) || (sda->index == PORT34))
        command = CPLD_2_5G_LED_CTL1_REG;
    else if ((sda->index == PORT35) || (sda->index == PORT36))
        command = CPLD_2_5G_LED_CTL2_REG;
    else if ((sda->index == PORT37) || (sda->index == PORT38))
        command = CPLD_2_5G_LED_CTL3_REG;
    else if ((sda->index == PORT39) || (sda->index == PORT40))
        command = CPLD_2_5G_LED_CTL4_REG;
    else if ((sda->index == PORT41) || (sda->index == PORT42))
        command = CPLD_2_5G_LED_CTL5_REG;
    else if ((sda->index == PORT43) || (sda->index == PORT44))
        command = CPLD_2_5G_LED_CTL6_REG;
    else if ((sda->index == PORT45) || (sda->index == PORT46))
        command = CPLD_2_5G_LED_CTL7_REG;
    else if ((sda->index == PORT47) || (sda->index == PORT48))
        command = CPLD_2_5G_LED_CTL8_REG;
    else if ((sda->index == PORT49) || (sda->index == PORT50))
        command = CPLD_SFP_LED_CTL1_REG;
    else if ((sda->index == PORT51) || (sda->index == PORT52))
        command = CPLD_SFP_LED_CTL2_REG;
    else if ((sda->index == PORT53) || (sda->index == PORT54))
        command = CPLD_SFP_LED_CTL3_REG;

    val = kstrtoint(buf, 10, &res);
    if (val)
        return val;
    if (res < 0 || res > 4)
        return -EINVAL;

    if (sda->index < PORT33) {
        bit = ((sda->index) % 2) * 4;
        mask = (0x7) << bit;
    } else{
        bit = ((sda->index + 1) % 2) * 4;
        mask = (0x7) << bit;
    }

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val &= ~(mask);
    val |= (res << bit);

    i2c_smbus_write_byte_data(client, command, val);

    mutex_unlock(&list_lock);

    return count;
}
/*
static ssize_t port_led_act_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0, bit = 0, mask = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    unsigned char command = 0;

    if ((sda->index == PORT0) || (sda->index == PORT1))
        command = CPLD_1G_LED_CTL1_REG;
    else if ((sda->index == PORT2) || (sda->index == PORT3))
        command = CPLD_1G_LED_CTL2_REG;
    else if ((sda->index == PORT4) || (sda->index == PORT5))
        command = CPLD_1G_LED_CTL3_REG;
    else if ((sda->index == PORT6) || (sda->index == PORT7))
        command = CPLD_1G_LED_CTL4_REG;
    else if ((sda->index == PORT8) || (sda->index == PORT9))
        command = CPLD_1G_LED_CTL5_REG;
    else if ((sda->index == PORT10) || (sda->index == PORT11))
        command = CPLD_1G_LED_CTL6_REG;
    else if ((sda->index == PORT12) || (sda->index == PORT13))
        command = CPLD_1G_LED_CTL7_REG;
    else if ((sda->index == PORT14) || (sda->index == PORT15))
        command = CPLD_1G_LED_CTL8_REG;
    else if ((sda->index == PORT16) || (sda->index == PORT17))
        command = CPLD_1G_LED_CTL9_REG;
    else if ((sda->index == PORT18) || (sda->index == PORT19))
        command = CPLD_1G_LED_CTL10_REG;
    else if ((sda->index == PORT20) || (sda->index == PORT21))
        command = CPLD_1G_LED_CTL11_REG;
    else if ((sda->index == PORT22) || (sda->index == PORT23))
        command = CPLD_1G_LED_CTL12_REG;
    else if ((sda->index == PORT24) || (sda->index == PORT25))
        command = CPLD_1G_LED_CTL13_REG;
    else if ((sda->index == PORT26) || (sda->index == PORT27))
        command = CPLD_1G_LED_CTL14_REG;
    else if ((sda->index == PORT28) || (sda->index == PORT29))
        command = CPLD_1G_LED_CTL15_REG;
    else if ((sda->index == PORT30) || (sda->index == PORT31))
        command = CPLD_1G_LED_CTL16_REG;
    else if ((sda->index == PORT32) || (sda->index == PORT33))
        command = CPLD_2_5G_LED_CTL1_REG;
    else if ((sda->index == PORT34) || (sda->index == PORT35))
        command = CPLD_2_5G_LED_CTL2_REG;
    else if ((sda->index == PORT36) || (sda->index == PORT37))
        command = CPLD_2_5G_LED_CTL3_REG;
    else if ((sda->index == PORT38) || (sda->index == PORT39))
        command = CPLD_2_5G_LED_CTL4_REG;
    else if ((sda->index == PORT40) || (sda->index == PORT41))
        command = CPLD_2_5G_LED_CTL5_REG;
    else if ((sda->index == PORT42) || (sda->index == PORT43))
        command = CPLD_2_5G_LED_CTL6_REG;
    else if ((sda->index == PORT44) || (sda->index == PORT45))
        command = CPLD_2_5G_LED_CTL7_REG;
    else if ((sda->index == PORT46) || (sda->index == PORT47))
        command = CPLD_2_5G_LED_CTL8_REG;
    else if ((sda->index == PORT48) || (sda->index == PORT49))
        command = CPLD_SFP_LED_CTL1_REG;
    else if ((sda->index == PORT50) || (sda->index == PORT51))
        command = CPLD_SFP_LED_CTL2_REG;
    else if ((sda->index == PORT52) || (sda->index == PORT53))
        command = CPLD_SFP_LED_CTL3_REG;

    bit = (((sda->index) % 2) * 4) + 2;
    mask = (0x3) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    res = (val & mask) >> bit;

    return sprintf(buf, "%d\n", res);
}

static ssize_t port_led_act_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    int val = 0, res = 0, bit = 0, mask = 0;
    unsigned char command = 0;

    if ((sda->index == PORT0) || (sda->index == PORT1))
        command = CPLD_1G_LED_CTL1_REG;
    else if ((sda->index == PORT2) || (sda->index == PORT3))
        command = CPLD_1G_LED_CTL2_REG;
    else if ((sda->index == PORT4) || (sda->index == PORT5))
        command = CPLD_1G_LED_CTL3_REG;
    else if ((sda->index == PORT6) || (sda->index == PORT7))
        command = CPLD_1G_LED_CTL4_REG;
    else if ((sda->index == PORT8) || (sda->index == PORT9))
        command = CPLD_1G_LED_CTL5_REG;
    else if ((sda->index == PORT10) || (sda->index == PORT11))
        command = CPLD_1G_LED_CTL6_REG;
    else if ((sda->index == PORT12) || (sda->index == PORT13))
        command = CPLD_1G_LED_CTL7_REG;
    else if ((sda->index == PORT14) || (sda->index == PORT15))
        command = CPLD_1G_LED_CTL8_REG;
    else if ((sda->index == PORT16) || (sda->index == PORT17))
        command = CPLD_1G_LED_CTL9_REG;
    else if ((sda->index == PORT18) || (sda->index == PORT19))
        command = CPLD_1G_LED_CTL10_REG;
    else if ((sda->index == PORT20) || (sda->index == PORT21))
        command = CPLD_1G_LED_CTL11_REG;
    else if ((sda->index == PORT22) || (sda->index == PORT23))
        command = CPLD_1G_LED_CTL12_REG;
    else if ((sda->index == PORT24) || (sda->index == PORT25))
        command = CPLD_1G_LED_CTL13_REG;
    else if ((sda->index == PORT26) || (sda->index == PORT27))
        command = CPLD_1G_LED_CTL14_REG;
    else if ((sda->index == PORT28) || (sda->index == PORT29))
        command = CPLD_1G_LED_CTL15_REG;
    else if ((sda->index == PORT30) || (sda->index == PORT31))
        command = CPLD_1G_LED_CTL16_REG;
    else if ((sda->index == PORT32) || (sda->index == PORT33))
        command = CPLD_2_5G_LED_CTL1_REG;
    else if ((sda->index == PORT34) || (sda->index == PORT35))
        command = CPLD_2_5G_LED_CTL2_REG;
    else if ((sda->index == PORT36) || (sda->index == PORT37))
        command = CPLD_2_5G_LED_CTL3_REG;
    else if ((sda->index == PORT38) || (sda->index == PORT39))
        command = CPLD_2_5G_LED_CTL4_REG;
    else if ((sda->index == PORT40) || (sda->index == PORT41))
        command = CPLD_2_5G_LED_CTL5_REG;
    else if ((sda->index == PORT42) || (sda->index == PORT43))
        command = CPLD_2_5G_LED_CTL6_REG;
    else if ((sda->index == PORT44) || (sda->index == PORT45))
        command = CPLD_2_5G_LED_CTL7_REG;
    else if ((sda->index == PORT46) || (sda->index == PORT47))
        command = CPLD_2_5G_LED_CTL8_REG;
    else if ((sda->index == PORT48) || (sda->index == PORT49))
        command = CPLD_SFP_LED_CTL1_REG;
    else if ((sda->index == PORT50) || (sda->index == PORT51))
        command = CPLD_SFP_LED_CTL2_REG;
    else if ((sda->index == PORT52) || (sda->index == PORT53))
        command = CPLD_SFP_LED_CTL3_REG;

    val = kstrtoint(buf, 10, &res);
    if (val)
        return val;

    if (sda->index < PORT48){
        if (res < 0 || res > 3)
            return -EINVAL;
    }else{
        if (res < 0 || res > 2)
            return -EINVAL;
    }

    bit = (((sda->index) % 2) * 4) + 2;
    mask = (0x3) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val &= ~(mask);
    val |= (res << bit);

    i2c_smbus_write_byte_data(client, command, val);

    mutex_unlock(&list_lock);

    return count;
}
*/
static ssize_t port_poe_led_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0, bit = 0, mask = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    unsigned char command = 0;

    if (sda->index < PORT5)
        command = CPLD_POE_PRT_LED_CTL1_REG;
    else if (sda->index < PORT9)
        command = CPLD_POE_PRT_LED_CTL2_REG;
    else if (sda->index < PORT13)
        command = CPLD_POE_PRT_LED_CTL3_REG;
    else if (sda->index < PORT17)
        command = CPLD_POE_PRT_LED_CTL4_REG;
    else if (sda->index < PORT21)
        command = CPLD_POE_PRT_LED_CTL5_REG;
    else if (sda->index < PORT25)
        command = CPLD_POE_PRT_LED_CTL6_REG;
    else if (sda->index < PORT29)
        command = CPLD_POE_PRT_LED_CTL7_REG;
    else if (sda->index < PORT33)
        command = CPLD_POE_PRT_LED_CTL8_REG;
    else if (sda->index < PORT37)
        command = CPLD_POE_PRT_LED_CTL9_REG;
    else if (sda->index < PORT41)
        command = CPLD_POE_PRT_LED_CTL10_REG;
    else if (sda->index < PORT45)
        command = CPLD_POE_PRT_LED_CTL11_REG;
    else if (sda->index < PORT49)
        command = CPLD_POE_PRT_LED_CTL12_REG;

    if (sda->index < PORT33) {
        bit = ((sda->index) % 4) * 2;
        mask = (0x3) << bit;
    } else {
        switch( (sda->index) % 4 ){
            case 0:
                bit = 2;
                mask = (0x3) << bit;
                break;
            case 1:
                bit = 0;
                mask = (0x3) << bit;
                break;
            case 2:
                bit = 6;
                mask = (0x3) << bit;
                break;
            case 3:
                bit = 4;
                mask = (0x3) << bit;
                break;
        }
    }
    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    res = (val & mask) >> bit;

    return sprintf(buf, "%d\n", res);
}

static ssize_t port_poe_led_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    int val = 0, res = 0, bit = 0, mask = 0;
    unsigned char command = 0;

    if (sda->index < PORT5)
        command = CPLD_POE_PRT_LED_CTL1_REG;
    else if (sda->index < PORT9)
        command = CPLD_POE_PRT_LED_CTL2_REG;
    else if (sda->index < PORT13)
        command = CPLD_POE_PRT_LED_CTL3_REG;
    else if (sda->index < PORT17)
        command = CPLD_POE_PRT_LED_CTL4_REG;
    else if (sda->index < PORT21)
        command = CPLD_POE_PRT_LED_CTL5_REG;
    else if (sda->index < PORT25)
        command = CPLD_POE_PRT_LED_CTL6_REG;
    else if (sda->index < PORT29)
        command = CPLD_POE_PRT_LED_CTL7_REG;
    else if (sda->index < PORT33)
        command = CPLD_POE_PRT_LED_CTL8_REG;
    else if (sda->index < PORT37)
        command = CPLD_POE_PRT_LED_CTL9_REG;
    else if (sda->index < PORT41)
        command = CPLD_POE_PRT_LED_CTL10_REG;
    else if (sda->index < PORT45)
        command = CPLD_POE_PRT_LED_CTL11_REG;
    else if (sda->index < PORT49)
        command = CPLD_POE_PRT_LED_CTL12_REG;

    val = kstrtoint(buf, 10, &res);
    if (val)
        return val;

    if (res < 0 || res > 2)
        return -EINVAL;

    if (sda->index < PORT33) {
        bit = ((sda->index) % 4) * 2;
        mask = (0x3) << bit;
    } else {
        switch( (sda->index) % 4 ){
            case 0:
                bit = 2;
                mask = (0x3) << bit;
                break;
            case 1:
                bit = 0;
                mask = (0x3) << bit;
                break;
            case 2:
                bit = 6;
                mask = (0x3) << bit;
                break;
            case 3:
                bit = 4;
                mask = (0x3) << bit;
                break;
        }
    }

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val &= ~(mask);

    val |= (res << bit);

    i2c_smbus_write_byte_data(client, command, val);

    mutex_unlock(&list_lock);

    return count;
}

static ssize_t fan_led_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0, bit = 0, mask = 0;
    unsigned char command = CPLD_FAN_LED_CTL_REG;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);

    bit = sda->index * 2;
    mask = (0x3) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    res = (val & mask) >> bit;

    return sprintf(buf, "%d\n", res);
}

static ssize_t fan_led_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    unsigned char command = CPLD_FAN_LED_CTL_REG;
    int val = 0, res = 0, bit = 0, mask = 0;

    val = kstrtoint(buf, 10, &res);

    if (val)
        return val;

    if (res < 0 || res > 2)
        return -EINVAL;

    bit = sda->index * 2;
    mask = (0x3) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val &= ~(mask);

    val |= (res << bit);

    i2c_smbus_write_byte_data(client, command, val);

    mutex_unlock(&list_lock);

    return count;
}

static ssize_t sys_led_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0, bit = 0, mask = 0;
    unsigned char command = CPLD_SYS_LED_CTL_REG;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);

    bit = sda->index;
    mask = (0x3) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    res = (val & mask) >> bit;

    return sprintf(buf, "%d\n", res);
}

static ssize_t sys_led_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    unsigned char command = CPLD_SYS_LED_CTL_REG;
    int val = 0, res = 0, bit = 0, mask = 0;

    val = kstrtoint(buf, 10, &res);
    if (val)
        return val;

    if (res < 0 || res > 3)
        return -EINVAL;

    bit = sda->index;
    mask = (0x3) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val &= ~(mask);

    val |= (res << bit);

    i2c_smbus_write_byte_data(client, command, val);

    mutex_unlock(&list_lock);

    return count;
}

static ssize_t rst_ctrl_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0, bit = 0, mask = 0;

    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    unsigned char command = 0;
    if (sda->index > RST_MAC_JTAG){
        command = CPLD_RESET_CTL2_REG;
        bit = (sda->index - 8);
    } else {
        command = CPLD_RESET_CTL1_REG;
        bit = sda->index;
    }
    mask = (0x1) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    res = (val & mask) >> bit;

    return sprintf(buf, "%d\n", res);
}
/*
static ssize_t rst_ctrl_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    int val = 0, res = 0, bit = 0, mask = 0;
    unsigned char command = 0;
    val = kstrtoint(buf, 10, &res);
    if (val)
        return val;

    if (res < 0 || res > 1)
        return -EINVAL;

    if (sda->index > RST_MAC_JTAG){
        command = CPLD_RESET_CTL2_REG;
        bit = (sda->index - 8);
    } else {
        command = CPLD_RESET_CTL1_REG;
        bit = sda->index;
    }
    mask = (0x1) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val &= ~(mask);

    val |= (res << bit);

    i2c_smbus_write_byte_data(client, command, val);

    mutex_unlock(&list_lock);

    return count;
}

static ssize_t int_ctrl_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0, bit = 0, mask = 0;

    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    unsigned char command = 0;
    if (sda->index < INT_OOB_IOIEXP){
        command = CPLD_INT_CTL1_REG;
        bit = sda->index;
    } else if (sda->index < DEV_INIT_DONE){
        command = CPLD_INT_CTL2_REG;
        bit = sda->index - 8;
    } else if (sda->index < MB_SYSTEM_INT){
        command = CPLD_INT_CTL3_REG;
        bit = sda->index - 16;
    } else {
        command = CPLD_INT_CTL4_REG;
        bit = sda->index - 19;
    }
    mask = (0x1) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    res = (val & mask) >> bit;

    return sprintf(buf, "%d\n", res);
}

static ssize_t int_ctrl_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    int val = 0, res = 0, bit = 0, mask = 0;
    unsigned char command = 0;
    val = kstrtoint(buf, 10, &res);
    if (val)
        return val;

    if (res < 0 || res > 1)
        return -EINVAL;

    if (sda->index < INT_OOB_IOIEXP){
        command = CPLD_INT_CTL1_REG;
        bit = sda->index;
    } else if (sda->index < DEV_INIT_DONE){
        command = CPLD_INT_CTL2_REG;
        bit = sda->index - 8;
    } else if (sda->index < MB_SYSTEM_INT){
        command = CPLD_INT_CTL3_REG;
        bit = sda->index - 16;
    } else {
        command = CPLD_INT_CTL4_REG;
        bit = sda->index - 19;
    }
    mask = (0x1) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val &= ~(mask);

    val |= (res << bit);

    i2c_smbus_write_byte_data(client, command, val);

    mutex_unlock(&list_lock);

    return count;
}
*/
static ssize_t fan_ctrl_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0, bit = 0, mask = 0;

    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    unsigned char command = 0;
    if (sda->index < PWRGD_VR_P12V_FAN3){
        command = CPLD_FAN_CTL1_REG;
        bit = sda->index;
    } else {
        command = CPLD_FAN_CTL2_REG;
        bit = sda->index - PWRGD_VR_P12V_FAN3;
    }
    mask = (0x1) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    res = (val & mask) >> bit;

    // fan present bit is active low
    if ( (sda->index == CNFAN1_PRESENT) || (sda->index == CNFAN2_PRESENT) || (sda->index == CNFAN3_PRESENT) )
        res = (res) ? 0 : 1;

    // fan enable bit is active low
    //if ( (sda->index == FM_FAN1_EN) || (sda->index == FM_FAN2_EN) || (sda->index == FM_FAN3_EN) )
    //    res = (res) ? 0 : 1;

    return sprintf(buf, "%d\n", res);
}

static ssize_t fan_ctrl_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    int val = 0, res = 0, bit = 0, mask = 0;
    unsigned char command = 0;
    val = kstrtoint(buf, 10, &res);
    if (val)
        return val;

    if (res < 0 || res > 1)
        return -EINVAL;

    command = CPLD_FAN_CTL1_REG;

    bit = sda->index;
    mask = (0x1) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val &= ~(mask);

    val |= (res << bit);
    i2c_smbus_write_byte_data(client, command, val);

    mutex_unlock(&list_lock);

    return count;
}

static ssize_t port_led_auto_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0, bit = 0, mask = 0;
    unsigned char command = CPLD_PORT_LED_MANUAL_REG;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);

    mutex_lock(&list_lock);

    bit = sda->index;
    mask = (0x1) << bit;

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    res = (val & mask) >> bit;

    return sprintf(buf, "%d\n", res);
}

static ssize_t port_led_auto_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    unsigned char command = CPLD_PORT_LED_MANUAL_REG;
    int val = 0, res = 0, bit = 0, mask = 0;

    val = kstrtoint(buf, 10, &res);

    if (val)
        return val;

    if (res < 0 || res > 1)
        return -EINVAL;

    mutex_lock(&list_lock);

    bit = sda->index;
    mask = (0x1) << bit;

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val &= ~(mask);

    val |= (res << bit);

    i2c_smbus_write_byte_data(client, command, val);

    mutex_unlock(&list_lock);

    return count;
}

static ssize_t poe_enable_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0, bit = 0, mask = 0;
    unsigned char command = CPLD_MSCI_CTL_REG;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);

    bit = sda->index;
    mask = (0x1) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    res = (val & mask) >> bit;

    return sprintf(buf, "%d\n", res);
}

static ssize_t poe_enable_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    unsigned char command = CPLD_MSCI_CTL_REG;
    int val = 0, res = 0, bit = 0, mask = 0;

    val = kstrtoint(buf, 10, &res);
    if (val)
        return val;

    if (res < 0 || res > 1)
        return -EINVAL;

    bit = sda->index;
    mask = (0x1) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val &= ~(mask);

    val |= (res << bit);

    i2c_smbus_write_byte_data(client, command, val);

    mutex_unlock(&list_lock);

    return count;
}

static ssize_t msci_ctl2_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0, bit = 0, mask = 0;
    unsigned char command = CPLD_MSCI_CTL2_REG;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);

    bit = sda->index;
    mask = (0x1) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    res = (val & mask) >> bit;

    return sprintf(buf, "%d\n", res);
}

static ssize_t msci_ctl2_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    unsigned char command = CPLD_MSCI_CTL2_REG;
    int val = 0, res = 0, bit = 0, mask = 0;

    val = kstrtoint(buf, 10, &res);
    if (val)
        return val;

    if (res < 0 || res > 1)
        return -EINVAL;

    bit = sda->index;
    mask = (0x1) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val &= ~(mask);

    val |= (res << bit);

    i2c_smbus_write_byte_data(client, command, val);

    mutex_unlock(&list_lock);

    return count;
}

static ssize_t watchdog_enable_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0, bit = 0, mask = (0x1) << bit;
    unsigned char command = CPLD_WATCHDOG_REG;
    struct i2c_client *client = to_i2c_client(dev);

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    res = (val & mask) >> bit;

    return sprintf(buf, "%d\n", res);
}

static ssize_t watchdog_enable_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    unsigned char command = CPLD_WATCHDOG_REG;
    int val = 0, res = 0, bit = 0, mask = (0x1) << bit;

    val = kstrtoint(buf, 10, &res);
    if (val)
        return val;

    if (res < 0 || res > 1)
        return -EINVAL;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val &= ~(mask);

    val |= (res << bit);

    i2c_smbus_write_byte_data(client, command, val);

    mutex_unlock(&list_lock);

    return count;
}

static ssize_t watchdog_refresh(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    unsigned char command = CPLD_WATCHDOG_REG;
    int val = 0, res = 1, bit = 1, mask = (0x1) << bit;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val &= ~(mask);

    val |= (res << bit);

    i2c_smbus_write_byte_data(client, command, val);

    mutex_unlock(&list_lock);

    return count;
}

static ssize_t watchdog_timer_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0, bit = 2, mask = (0x7) << bit;
    unsigned char command = CPLD_WATCHDOG_REG;
    struct i2c_client *client = to_i2c_client(dev);

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    res = (val & mask) >> bit;

    return sprintf(buf, "%d\n", res);
}

static ssize_t watchdog_timer_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    unsigned char command = CPLD_WATCHDOG_REG;
    int val = 0, res = 0, bit = 2, mask = (0x7) << bit;

    val = kstrtoint(buf, 10, &res);
    if (val)
        return val;

    if (res < WATCHDOG_180S || res > WATCHDOG_300S)
        return -EINVAL;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val &= ~(mask);

    val |= (res << bit);

    i2c_smbus_write_byte_data(client, command, val);

    mutex_unlock(&list_lock);

    return count;
}

static ssize_t power_down_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0, bit = 0, mask = (0x1) << bit;
    unsigned char command = CPLD_SYS_POWER_DOWN_REG;
    struct i2c_client *client = to_i2c_client(dev);

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    mutex_unlock(&list_lock);

    res = (val & mask) >> bit;

    return sprintf(buf, "%d\n", res);
}

static ssize_t power_down_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    unsigned char command = CPLD_SYS_POWER_DOWN_REG;
    int val = 0, res = 0, bit = 0, mask = (0x1) << bit;

    val = kstrtoint(buf, 10, &res);
    if (val)
        return val;

    if (res < 0 || res > 1)
        return -EINVAL;

    mutex_lock(&list_lock);

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        mutex_unlock(&list_lock);
        return val;
    }

    val &= ~(mask);

    val |= (res << bit);

    i2c_smbus_write_byte_data(client, command, val);

    mutex_unlock(&list_lock);

    return count;
}

//static SENSOR_DEVICE_ATTR(board_rev, S_IRUGO, cpld_show_board_version, NULL, 0);
static SENSOR_DEVICE_ATTR(board_id, S_IRUGO, cpld_show_board_id, NULL, 0);
static SENSOR_DEVICE_ATTR(fan_direction, S_IRUGO, cpld_show_fan_direction, NULL, 0);
static SENSOR_DEVICE_ATTR(cpld_rev, S_IRUGO, cpld_show_version, NULL, 0);
static SENSOR_DEVICE_ATTR(build_date, S_IRUGO, cpld_show_build_date, NULL, 0);

static SENSOR_DEVICE_ATTR(psu2_on, S_IRUGO | S_IWUSR, cpld_read_psu_status, cpld_write_psu_status, PSU2_ON);
static SENSOR_DEVICE_ATTR(psu1_on, S_IRUGO | S_IWUSR, cpld_read_psu_status, cpld_write_psu_status, PSU1_ON);
static SENSOR_DEVICE_ATTR(psu2_ac_ok, S_IRUGO, cpld_read_psu_status, NULL, PSU2_AC_OK);
static SENSOR_DEVICE_ATTR(psu1_ac_ok, S_IRUGO, cpld_read_psu_status, NULL, PSU1_AC_OK);
static SENSOR_DEVICE_ATTR(psu2_present, S_IRUGO, cpld_read_psu_status, NULL, PSU2_PRESENT);
static SENSOR_DEVICE_ATTR(psu1_present, S_IRUGO, cpld_read_psu_status, NULL, PSU1_PRESENT);
static SENSOR_DEVICE_ATTR(psu2_power_good, S_IRUGO, cpld_read_psu_status, NULL, PSU2_PWRGD);
static SENSOR_DEVICE_ATTR(psu1_power_good, S_IRUGO, cpld_read_psu_status, NULL, PSU1_PWRGD);


static SENSOR_DEVICE_ATTR(power_ctrl, S_IRUGO, cpld_show_power_ctrl, NULL, 0);
static SENSOR_DEVICE_ATTR(power_good, S_IRUGO, cpld_show_power_good, NULL, 0);

static SENSOR_DEVICE_ATTR(fan_full_speed, S_IRUGO | S_IWUSR, fan_ctrl_read, fan_ctrl_write, FAN_FULL_SPEED);
static SENSOR_DEVICE_ATTR(fm_fan3_en, S_IRUGO | S_IWUSR, fan_ctrl_read, fan_ctrl_write, FM_FAN3_EN);
static SENSOR_DEVICE_ATTR(fm_fan2_en, S_IRUGO | S_IWUSR, fan_ctrl_read, fan_ctrl_write, FM_FAN2_EN);
static SENSOR_DEVICE_ATTR(fm_fan1_en, S_IRUGO | S_IWUSR, fan_ctrl_read, fan_ctrl_write, FM_FAN1_EN);
static SENSOR_DEVICE_ATTR(pwrgd_vr_p12v_fan3, S_IRUGO , fan_ctrl_read, NULL, PWRGD_VR_P12V_FAN3);
static SENSOR_DEVICE_ATTR(pwrgd_vr_p12v_fan2, S_IRUGO, fan_ctrl_read, NULL, PWRGD_VR_P12V_FAN2);
static SENSOR_DEVICE_ATTR(pwrgd_vr_p12v_fan1, S_IRUGO, fan_ctrl_read, NULL, PWRGD_VR_P12V_FAN1);
static SENSOR_DEVICE_ATTR(cnfan3_present, S_IRUGO, fan_ctrl_read, NULL, CNFAN3_PRESENT);
static SENSOR_DEVICE_ATTR(cnfan2_present, S_IRUGO, fan_ctrl_read, NULL, CNFAN2_PRESENT);
static SENSOR_DEVICE_ATTR(cnfan1_present, S_IRUGO, fan_ctrl_read, NULL, CNFAN1_PRESENT);

static SENSOR_DEVICE_ATTR(port1_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT1);
static SENSOR_DEVICE_ATTR(port2_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT2);
static SENSOR_DEVICE_ATTR(port3_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT3);
static SENSOR_DEVICE_ATTR(port4_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT4);
static SENSOR_DEVICE_ATTR(port5_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT5);
static SENSOR_DEVICE_ATTR(port6_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT6);
static SENSOR_DEVICE_ATTR(port7_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT7);
static SENSOR_DEVICE_ATTR(port8_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT8);
static SENSOR_DEVICE_ATTR(port9_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT9);
static SENSOR_DEVICE_ATTR(port10_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT10);
static SENSOR_DEVICE_ATTR(port11_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT11);
static SENSOR_DEVICE_ATTR(port12_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT12);
static SENSOR_DEVICE_ATTR(port13_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT13);
static SENSOR_DEVICE_ATTR(port14_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT14);
static SENSOR_DEVICE_ATTR(port15_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT15);
static SENSOR_DEVICE_ATTR(port16_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT16);
static SENSOR_DEVICE_ATTR(port17_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT17);
static SENSOR_DEVICE_ATTR(port18_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT18);
static SENSOR_DEVICE_ATTR(port19_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT19);
static SENSOR_DEVICE_ATTR(port20_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT20);
static SENSOR_DEVICE_ATTR(port21_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT21);
static SENSOR_DEVICE_ATTR(port22_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT22);
static SENSOR_DEVICE_ATTR(port23_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT23);
static SENSOR_DEVICE_ATTR(port24_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT24);
static SENSOR_DEVICE_ATTR(port25_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT25);
static SENSOR_DEVICE_ATTR(port26_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT26);
static SENSOR_DEVICE_ATTR(port27_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT27);
static SENSOR_DEVICE_ATTR(port28_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT28);
static SENSOR_DEVICE_ATTR(port29_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT29);
static SENSOR_DEVICE_ATTR(port30_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT30);
static SENSOR_DEVICE_ATTR(port31_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT31);
static SENSOR_DEVICE_ATTR(port32_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT32);
static SENSOR_DEVICE_ATTR(port33_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT33);
static SENSOR_DEVICE_ATTR(port34_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT34);
static SENSOR_DEVICE_ATTR(port35_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT35);
static SENSOR_DEVICE_ATTR(port36_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT36);
static SENSOR_DEVICE_ATTR(port37_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT37);
static SENSOR_DEVICE_ATTR(port38_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT38);
static SENSOR_DEVICE_ATTR(port39_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT39);
static SENSOR_DEVICE_ATTR(port40_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT40);
static SENSOR_DEVICE_ATTR(port41_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT41);
static SENSOR_DEVICE_ATTR(port42_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT42);
static SENSOR_DEVICE_ATTR(port43_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT43);
static SENSOR_DEVICE_ATTR(port44_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT44);
static SENSOR_DEVICE_ATTR(port45_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT45);
static SENSOR_DEVICE_ATTR(port46_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT46);
static SENSOR_DEVICE_ATTR(port47_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT47);
static SENSOR_DEVICE_ATTR(port48_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT48);
static SENSOR_DEVICE_ATTR(port49_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT49);
static SENSOR_DEVICE_ATTR(port50_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT50);
static SENSOR_DEVICE_ATTR(port51_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT51);
static SENSOR_DEVICE_ATTR(port52_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT52);
static SENSOR_DEVICE_ATTR(port53_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT53);
static SENSOR_DEVICE_ATTR(port54_led, S_IRUGO | S_IWUSR, port_led_read, port_led_write, PORT54);
/*
static SENSOR_DEVICE_ATTR(port0_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT0);
static SENSOR_DEVICE_ATTR(port1_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT1);
static SENSOR_DEVICE_ATTR(port2_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT2);
static SENSOR_DEVICE_ATTR(port3_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT3);
static SENSOR_DEVICE_ATTR(port4_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT4);
static SENSOR_DEVICE_ATTR(port5_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT5);
static SENSOR_DEVICE_ATTR(port6_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT6);
static SENSOR_DEVICE_ATTR(port7_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT7);
static SENSOR_DEVICE_ATTR(port8_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT8);
static SENSOR_DEVICE_ATTR(port9_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT9);
static SENSOR_DEVICE_ATTR(port10_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT10);
static SENSOR_DEVICE_ATTR(port11_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT11);
static SENSOR_DEVICE_ATTR(port12_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT12);
static SENSOR_DEVICE_ATTR(port13_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT13);
static SENSOR_DEVICE_ATTR(port14_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT14);
static SENSOR_DEVICE_ATTR(port15_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT15);
static SENSOR_DEVICE_ATTR(port16_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT16);
static SENSOR_DEVICE_ATTR(port17_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT17);
static SENSOR_DEVICE_ATTR(port18_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT18);
static SENSOR_DEVICE_ATTR(port19_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT19);
static SENSOR_DEVICE_ATTR(port20_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT20);
static SENSOR_DEVICE_ATTR(port21_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT21);
static SENSOR_DEVICE_ATTR(port22_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT22);
static SENSOR_DEVICE_ATTR(port23_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT23);
static SENSOR_DEVICE_ATTR(port24_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT24);
static SENSOR_DEVICE_ATTR(port25_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT25);
static SENSOR_DEVICE_ATTR(port26_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT26);
static SENSOR_DEVICE_ATTR(port27_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT27);
static SENSOR_DEVICE_ATTR(port28_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT28);
static SENSOR_DEVICE_ATTR(port29_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT29);
static SENSOR_DEVICE_ATTR(port30_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT30);
static SENSOR_DEVICE_ATTR(port31_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT31);
static SENSOR_DEVICE_ATTR(port32_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT32);
static SENSOR_DEVICE_ATTR(port33_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT33);
static SENSOR_DEVICE_ATTR(port34_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT34);
static SENSOR_DEVICE_ATTR(port35_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT35);
static SENSOR_DEVICE_ATTR(port36_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT36);
static SENSOR_DEVICE_ATTR(port37_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT37);
static SENSOR_DEVICE_ATTR(port38_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT38);
static SENSOR_DEVICE_ATTR(port39_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT39);
static SENSOR_DEVICE_ATTR(port40_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT40);
static SENSOR_DEVICE_ATTR(port41_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT41);
static SENSOR_DEVICE_ATTR(port42_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT42);
static SENSOR_DEVICE_ATTR(port43_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT43);
static SENSOR_DEVICE_ATTR(port44_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT44);
static SENSOR_DEVICE_ATTR(port45_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT45);
static SENSOR_DEVICE_ATTR(port46_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT46);
static SENSOR_DEVICE_ATTR(port47_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT47);
static SENSOR_DEVICE_ATTR(port48_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT48);
static SENSOR_DEVICE_ATTR(port49_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT49);
static SENSOR_DEVICE_ATTR(port50_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT50);
static SENSOR_DEVICE_ATTR(port51_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT51);
static SENSOR_DEVICE_ATTR(port52_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT52);
static SENSOR_DEVICE_ATTR(port53_act_led, S_IRUGO | S_IWUSR, port_led_act_read, port_led_act_write, PORT53);
*/

static SENSOR_DEVICE_ATTR(port1_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT1);
static SENSOR_DEVICE_ATTR(port2_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT2);
static SENSOR_DEVICE_ATTR(port3_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT3);
static SENSOR_DEVICE_ATTR(port4_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT4);
static SENSOR_DEVICE_ATTR(port5_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT5);
static SENSOR_DEVICE_ATTR(port6_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT6);
static SENSOR_DEVICE_ATTR(port7_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT7);
static SENSOR_DEVICE_ATTR(port8_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT8);
static SENSOR_DEVICE_ATTR(port9_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT9);
static SENSOR_DEVICE_ATTR(port10_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT10);
static SENSOR_DEVICE_ATTR(port11_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT11);
static SENSOR_DEVICE_ATTR(port12_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT12);
static SENSOR_DEVICE_ATTR(port13_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT13);
static SENSOR_DEVICE_ATTR(port14_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT14);
static SENSOR_DEVICE_ATTR(port15_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT15);
static SENSOR_DEVICE_ATTR(port16_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT16);
static SENSOR_DEVICE_ATTR(port17_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT17);
static SENSOR_DEVICE_ATTR(port18_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT18);
static SENSOR_DEVICE_ATTR(port19_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT19);
static SENSOR_DEVICE_ATTR(port20_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT20);
static SENSOR_DEVICE_ATTR(port21_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT21);
static SENSOR_DEVICE_ATTR(port22_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT22);
static SENSOR_DEVICE_ATTR(port23_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT23);
static SENSOR_DEVICE_ATTR(port24_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT24);
static SENSOR_DEVICE_ATTR(port25_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT25);
static SENSOR_DEVICE_ATTR(port26_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT26);
static SENSOR_DEVICE_ATTR(port27_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT27);
static SENSOR_DEVICE_ATTR(port28_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT28);
static SENSOR_DEVICE_ATTR(port29_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT29);
static SENSOR_DEVICE_ATTR(port30_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT30);
static SENSOR_DEVICE_ATTR(port31_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT31);
static SENSOR_DEVICE_ATTR(port32_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT32);
static SENSOR_DEVICE_ATTR(port33_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT33);
static SENSOR_DEVICE_ATTR(port34_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT34);
static SENSOR_DEVICE_ATTR(port35_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT35);
static SENSOR_DEVICE_ATTR(port36_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT36);
static SENSOR_DEVICE_ATTR(port37_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT37);
static SENSOR_DEVICE_ATTR(port38_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT38);
static SENSOR_DEVICE_ATTR(port39_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT39);
static SENSOR_DEVICE_ATTR(port40_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT40);
static SENSOR_DEVICE_ATTR(port41_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT41);
static SENSOR_DEVICE_ATTR(port42_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT42);
static SENSOR_DEVICE_ATTR(port43_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT43);
static SENSOR_DEVICE_ATTR(port44_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT44);
static SENSOR_DEVICE_ATTR(port45_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT45);
static SENSOR_DEVICE_ATTR(port46_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT46);
static SENSOR_DEVICE_ATTR(port47_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT47);
static SENSOR_DEVICE_ATTR(port48_poe_led, S_IRUGO | S_IWUSR, port_poe_led_read, port_poe_led_write, PORT48);

static SENSOR_DEVICE_ATTR(fan1_led, S_IRUGO | S_IWUSR, fan_led_read, fan_led_write, FAN1);
static SENSOR_DEVICE_ATTR(fan2_led, S_IRUGO | S_IWUSR, fan_led_read, fan_led_write, FAN2);
static SENSOR_DEVICE_ATTR(fan3_led, S_IRUGO | S_IWUSR, fan_led_read, fan_led_write, FAN3);

static SENSOR_DEVICE_ATTR(loc_led, S_IRUGO | S_IWUSR, sys_led_read, sys_led_write, LOC_LED);
static SENSOR_DEVICE_ATTR(psu_led, S_IRUGO | S_IWUSR, sys_led_read, sys_led_write, PSU_LED);
static SENSOR_DEVICE_ATTR(sys_led, S_IRUGO | S_IWUSR, sys_led_read, sys_led_write, SYS_LED);
static SENSOR_DEVICE_ATTR(fan_led, S_IRUGO | S_IWUSR, sys_led_read, sys_led_write, FAN_LED);

static SENSOR_DEVICE_ATTR(port_led_auto, S_IRUGO | S_IWUSR, port_led_auto_read, port_led_auto_write, FRONT_PORT_LED_EN);
static SENSOR_DEVICE_ATTR(sysled_auto, S_IRUGO | S_IWUSR, port_led_auto_read, port_led_auto_write, FRONT_SYS_LED_EN);
static SENSOR_DEVICE_ATTR(psuled_auto, S_IRUGO | S_IWUSR, port_led_auto_read, port_led_auto_write, FRONT_PSU_LED_EN);
static SENSOR_DEVICE_ATTR(fanled_auto, S_IRUGO | S_IWUSR, port_led_auto_read, port_led_auto_write, FRONT_FAN_LED_EN);
static SENSOR_DEVICE_ATTR(locled_auto, S_IRUGO | S_IWUSR, port_led_auto_read, port_led_auto_write, FRONT_LOC_LED_EN);

static SENSOR_DEVICE_ATTR(poe_en_ctrl, S_IRUGO | S_IWUSR, poe_enable_read, poe_enable_write, 0);

static SENSOR_DEVICE_ATTR(psu_vsense, S_IRUGO | S_IWUSR, msci_ctl2_read, msci_ctl2_write, PSU_VSENSE);
static SENSOR_DEVICE_ATTR(psu_pwrgd, S_IRUGO, msci_ctl2_read, NULL, PSU_PWRGD);
static SENSOR_DEVICE_ATTR(rst_button_10s, S_IRUGO, rst_ctrl_read, NULL, RST_BUTTON_10S);

static SENSOR_DEVICE_ATTR(watchdog_timer, S_IRUGO | S_IWUSR, watchdog_timer_read, watchdog_timer_write, 0);
static SENSOR_DEVICE_ATTR(watchdog_kick, S_IWUSR, NULL, watchdog_refresh, 0);
static SENSOR_DEVICE_ATTR(watchdog_en, S_IRUGO | S_IWUSR, watchdog_enable_read, watchdog_enable_write, 0);


static SENSOR_DEVICE_ATTR(system_power_down, S_IRUGO | S_IWUSR, power_down_read, power_down_write, 0);
/*
static SENSOR_DEVICE_ATTR(rst_mb_ioiexp, S_IRUGO | S_IWUSR, rst_ctrl_read, rst_ctrl_write, RST_MB_IOIEXP);
static SENSOR_DEVICE_ATTR(rst_phy0_5, S_IRUGO | S_IWUSR, rst_ctrl_read, rst_ctrl_write, RST_PHY0_5);
static SENSOR_DEVICE_ATTR(rst_poe_ctr, S_IRUGO | S_IWUSR, rst_ctrl_read, rst_ctrl_write, RST_POE_CTR);
static SENSOR_DEVICE_ATTR(cpld_sysrst_mac_rst, S_IRUGO | S_IWUSR, rst_ctrl_read, rst_ctrl_write, CPLD_SYSRST_MAC_RST);
static SENSOR_DEVICE_ATTR(rst_cpu_sysrst_out, S_IRUGO | S_IWUSR, rst_ctrl_read, rst_ctrl_write, RST_CPU_SYSRST_OUT);
static SENSOR_DEVICE_ATTR(rst_gphy_rst, S_IRUGO | S_IWUSR, rst_ctrl_read, rst_ctrl_write, RST_GPHY_RST);
static SENSOR_DEVICE_ATTR(rst_i2c_mux, S_IRUGO | S_IWUSR, rst_ctrl_read, rst_ctrl_write, RST_I2C_MUX);
static SENSOR_DEVICE_ATTR(rst_mac_jtag, S_IRUGO | S_IWUSR, rst_ctrl_read, rst_ctrl_write, RST_MAC_JTAG);
static SENSOR_DEVICE_ATTR(rst_button_cpld_cpu, S_IRUGO | S_IWUSR, rst_ctrl_read, rst_ctrl_write, RST_BUTTON_CPLD_CPU);
static SENSOR_DEVICE_ATTR(rst_button, S_IRUGO | S_IWUSR, rst_ctrl_read, rst_ctrl_write, RST_BUTTON);
static SENSOR_DEVICE_ATTR(rst_cpld_i2c_mux, S_IRUGO | S_IWUSR, rst_ctrl_read, rst_ctrl_write, RST_CPLD_I2C_MUX);
static SENSOR_DEVICE_ATTR(rst_mac_sys, S_IRUGO | S_IWUSR, rst_ctrl_read, rst_ctrl_write, RST_MAC_SYS);

static SENSOR_DEVICE_ATTR(int_phy0, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, INT_PHY0);
static SENSOR_DEVICE_ATTR(int_phy1, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, INT_PHY1);
static SENSOR_DEVICE_ATTR(int_phy2, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, INT_PHY2);
static SENSOR_DEVICE_ATTR(int_phy3, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, INT_PHY3);
static SENSOR_DEVICE_ATTR(int_phy4_0, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, INT_PHY4_0);
static SENSOR_DEVICE_ATTR(int_phy4_1, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, INT_PHY4_1);
static SENSOR_DEVICE_ATTR(int_phy5_0, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, INT_PHY5_0);
static SENSOR_DEVICE_ATTR(int_phy5_1, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, INT_PHY5_1);
static SENSOR_DEVICE_ATTR(int_oob_ioiexp, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, INT_OOB_IOIEXP);
static SENSOR_DEVICE_ATTR(int_mb_ioiexp, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, INT_MB_IOIEXP);
static SENSOR_DEVICE_ATTR(usb_ocp_alert, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, USB_OCP_ALERT);
static SENSOR_DEVICE_ATTR(temp_alert, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, TEMP_ALERT);
static SENSOR_DEVICE_ATTR(rtc_irq, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, RTC_IRQ);
static SENSOR_DEVICE_ATTR(psu2_int_alert, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, PSU2_INT_ALERT);
static SENSOR_DEVICE_ATTR(psu1_int_alert, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, PSU1_INT_ALERT);
static SENSOR_DEVICE_ATTR(int_pd69210, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, INT_PD69210);
static SENSOR_DEVICE_ATTR(dev_init_done, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, DEV_INIT_DONE);
static SENSOR_DEVICE_ATTR(fan_ctl_fail, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, FAN_CTL_FAIL);
static SENSOR_DEVICE_ATTR(memhot, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, MEMHOT);
static SENSOR_DEVICE_ATTR(mb_system_int, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, MB_SYSTEM_INT);
static SENSOR_DEVICE_ATTR(mb_mod_pre_int, S_IRUGO | S_IWUSR, int_ctrl_read, int_ctrl_write, MB_MOD_PRE_INT);
*/
static struct attribute *wistron_switch_cpld_attributes[] = {
//    &sensor_dev_attr_board_rev.dev_attr.attr,
    &sensor_dev_attr_board_id.dev_attr.attr,
    &sensor_dev_attr_fan_direction.dev_attr.attr,
    &sensor_dev_attr_cpld_rev.dev_attr.attr,
    &sensor_dev_attr_build_date.dev_attr.attr,

    &sensor_dev_attr_psu2_on.dev_attr.attr,
    &sensor_dev_attr_psu1_on.dev_attr.attr,
    &sensor_dev_attr_psu2_ac_ok.dev_attr.attr,
    &sensor_dev_attr_psu1_ac_ok.dev_attr.attr,
    &sensor_dev_attr_psu2_present.dev_attr.attr,
    &sensor_dev_attr_psu1_present.dev_attr.attr,
    &sensor_dev_attr_psu2_power_good.dev_attr.attr,
    &sensor_dev_attr_psu1_power_good.dev_attr.attr,

    &sensor_dev_attr_power_ctrl.dev_attr.attr,
    &sensor_dev_attr_power_good.dev_attr.attr,

    &sensor_dev_attr_fan_full_speed.dev_attr.attr,
    &sensor_dev_attr_fm_fan3_en.dev_attr.attr,
    &sensor_dev_attr_fm_fan2_en.dev_attr.attr,
    &sensor_dev_attr_fm_fan1_en.dev_attr.attr,
    &sensor_dev_attr_pwrgd_vr_p12v_fan3.dev_attr.attr,
    &sensor_dev_attr_pwrgd_vr_p12v_fan2.dev_attr.attr,
    &sensor_dev_attr_pwrgd_vr_p12v_fan1.dev_attr.attr,
    &sensor_dev_attr_cnfan3_present.dev_attr.attr,
    &sensor_dev_attr_cnfan2_present.dev_attr.attr,
    &sensor_dev_attr_cnfan1_present.dev_attr.attr,

    &sensor_dev_attr_port1_led.dev_attr.attr,
    &sensor_dev_attr_port2_led.dev_attr.attr,
    &sensor_dev_attr_port3_led.dev_attr.attr,
    &sensor_dev_attr_port4_led.dev_attr.attr,
    &sensor_dev_attr_port5_led.dev_attr.attr,
    &sensor_dev_attr_port6_led.dev_attr.attr,
    &sensor_dev_attr_port7_led.dev_attr.attr,
    &sensor_dev_attr_port8_led.dev_attr.attr,
    &sensor_dev_attr_port9_led.dev_attr.attr,
    &sensor_dev_attr_port10_led.dev_attr.attr,
    &sensor_dev_attr_port11_led.dev_attr.attr,
    &sensor_dev_attr_port12_led.dev_attr.attr,
    &sensor_dev_attr_port13_led.dev_attr.attr,
    &sensor_dev_attr_port14_led.dev_attr.attr,
    &sensor_dev_attr_port15_led.dev_attr.attr,
    &sensor_dev_attr_port16_led.dev_attr.attr,
    &sensor_dev_attr_port17_led.dev_attr.attr,
    &sensor_dev_attr_port18_led.dev_attr.attr,
    &sensor_dev_attr_port19_led.dev_attr.attr,
    &sensor_dev_attr_port20_led.dev_attr.attr,
    &sensor_dev_attr_port21_led.dev_attr.attr,
    &sensor_dev_attr_port22_led.dev_attr.attr,
    &sensor_dev_attr_port23_led.dev_attr.attr,
    &sensor_dev_attr_port24_led.dev_attr.attr,
    &sensor_dev_attr_port25_led.dev_attr.attr,
    &sensor_dev_attr_port26_led.dev_attr.attr,
    &sensor_dev_attr_port27_led.dev_attr.attr,
    &sensor_dev_attr_port28_led.dev_attr.attr,
    &sensor_dev_attr_port29_led.dev_attr.attr,
    &sensor_dev_attr_port30_led.dev_attr.attr,
    &sensor_dev_attr_port31_led.dev_attr.attr,
    &sensor_dev_attr_port32_led.dev_attr.attr,
    &sensor_dev_attr_port33_led.dev_attr.attr,
    &sensor_dev_attr_port34_led.dev_attr.attr,
    &sensor_dev_attr_port35_led.dev_attr.attr,
    &sensor_dev_attr_port36_led.dev_attr.attr,
    &sensor_dev_attr_port37_led.dev_attr.attr,
    &sensor_dev_attr_port38_led.dev_attr.attr,
    &sensor_dev_attr_port39_led.dev_attr.attr,
    &sensor_dev_attr_port40_led.dev_attr.attr,
    &sensor_dev_attr_port41_led.dev_attr.attr,
    &sensor_dev_attr_port42_led.dev_attr.attr,
    &sensor_dev_attr_port43_led.dev_attr.attr,
    &sensor_dev_attr_port44_led.dev_attr.attr,
    &sensor_dev_attr_port45_led.dev_attr.attr,
    &sensor_dev_attr_port46_led.dev_attr.attr,
    &sensor_dev_attr_port47_led.dev_attr.attr,
    &sensor_dev_attr_port48_led.dev_attr.attr,
    &sensor_dev_attr_port49_led.dev_attr.attr,
    &sensor_dev_attr_port50_led.dev_attr.attr,
    &sensor_dev_attr_port51_led.dev_attr.attr,
    &sensor_dev_attr_port52_led.dev_attr.attr,
    &sensor_dev_attr_port53_led.dev_attr.attr,
    &sensor_dev_attr_port54_led.dev_attr.attr,
/*
    &sensor_dev_attr_port0_act_led.dev_attr.attr,
    &sensor_dev_attr_port1_act_led.dev_attr.attr,
    &sensor_dev_attr_port2_act_led.dev_attr.attr,
    &sensor_dev_attr_port3_act_led.dev_attr.attr,
    &sensor_dev_attr_port4_act_led.dev_attr.attr,
    &sensor_dev_attr_port5_act_led.dev_attr.attr,
    &sensor_dev_attr_port6_act_led.dev_attr.attr,
    &sensor_dev_attr_port7_act_led.dev_attr.attr,
    &sensor_dev_attr_port8_act_led.dev_attr.attr,
    &sensor_dev_attr_port9_act_led.dev_attr.attr,
    &sensor_dev_attr_port10_act_led.dev_attr.attr,
    &sensor_dev_attr_port11_act_led.dev_attr.attr,
    &sensor_dev_attr_port12_act_led.dev_attr.attr,
    &sensor_dev_attr_port13_act_led.dev_attr.attr,
    &sensor_dev_attr_port14_act_led.dev_attr.attr,
    &sensor_dev_attr_port15_act_led.dev_attr.attr,
    &sensor_dev_attr_port16_act_led.dev_attr.attr,
    &sensor_dev_attr_port17_act_led.dev_attr.attr,
    &sensor_dev_attr_port18_act_led.dev_attr.attr,
    &sensor_dev_attr_port19_act_led.dev_attr.attr,
    &sensor_dev_attr_port20_act_led.dev_attr.attr,
    &sensor_dev_attr_port21_act_led.dev_attr.attr,
    &sensor_dev_attr_port22_act_led.dev_attr.attr,
    &sensor_dev_attr_port23_act_led.dev_attr.attr,
    &sensor_dev_attr_port24_act_led.dev_attr.attr,
    &sensor_dev_attr_port25_act_led.dev_attr.attr,
    &sensor_dev_attr_port26_act_led.dev_attr.attr,
    &sensor_dev_attr_port27_act_led.dev_attr.attr,
    &sensor_dev_attr_port28_act_led.dev_attr.attr,
    &sensor_dev_attr_port29_act_led.dev_attr.attr,
    &sensor_dev_attr_port30_act_led.dev_attr.attr,
    &sensor_dev_attr_port31_act_led.dev_attr.attr,
    &sensor_dev_attr_port32_act_led.dev_attr.attr,
    &sensor_dev_attr_port33_act_led.dev_attr.attr,
    &sensor_dev_attr_port34_act_led.dev_attr.attr,
    &sensor_dev_attr_port35_act_led.dev_attr.attr,
    &sensor_dev_attr_port36_act_led.dev_attr.attr,
    &sensor_dev_attr_port37_act_led.dev_attr.attr,
    &sensor_dev_attr_port38_act_led.dev_attr.attr,
    &sensor_dev_attr_port39_act_led.dev_attr.attr,
    &sensor_dev_attr_port40_act_led.dev_attr.attr,
    &sensor_dev_attr_port41_act_led.dev_attr.attr,
    &sensor_dev_attr_port42_act_led.dev_attr.attr,
    &sensor_dev_attr_port43_act_led.dev_attr.attr,
    &sensor_dev_attr_port44_act_led.dev_attr.attr,
    &sensor_dev_attr_port45_act_led.dev_attr.attr,
    &sensor_dev_attr_port46_act_led.dev_attr.attr,
    &sensor_dev_attr_port47_act_led.dev_attr.attr,
    &sensor_dev_attr_port48_act_led.dev_attr.attr,
    &sensor_dev_attr_port49_act_led.dev_attr.attr,
    &sensor_dev_attr_port50_act_led.dev_attr.attr,
    &sensor_dev_attr_port51_act_led.dev_attr.attr,
    &sensor_dev_attr_port52_act_led.dev_attr.attr,
    &sensor_dev_attr_port53_act_led.dev_attr.attr,
*/

    &sensor_dev_attr_port1_poe_led.dev_attr.attr,
    &sensor_dev_attr_port2_poe_led.dev_attr.attr,
    &sensor_dev_attr_port3_poe_led.dev_attr.attr,
    &sensor_dev_attr_port4_poe_led.dev_attr.attr,
    &sensor_dev_attr_port5_poe_led.dev_attr.attr,
    &sensor_dev_attr_port6_poe_led.dev_attr.attr,
    &sensor_dev_attr_port7_poe_led.dev_attr.attr,
    &sensor_dev_attr_port8_poe_led.dev_attr.attr,
    &sensor_dev_attr_port9_poe_led.dev_attr.attr,
    &sensor_dev_attr_port10_poe_led.dev_attr.attr,
    &sensor_dev_attr_port11_poe_led.dev_attr.attr,
    &sensor_dev_attr_port12_poe_led.dev_attr.attr,
    &sensor_dev_attr_port13_poe_led.dev_attr.attr,
    &sensor_dev_attr_port14_poe_led.dev_attr.attr,
    &sensor_dev_attr_port15_poe_led.dev_attr.attr,
    &sensor_dev_attr_port16_poe_led.dev_attr.attr,
    &sensor_dev_attr_port17_poe_led.dev_attr.attr,
    &sensor_dev_attr_port18_poe_led.dev_attr.attr,
    &sensor_dev_attr_port19_poe_led.dev_attr.attr,
    &sensor_dev_attr_port20_poe_led.dev_attr.attr,
    &sensor_dev_attr_port21_poe_led.dev_attr.attr,
    &sensor_dev_attr_port22_poe_led.dev_attr.attr,
    &sensor_dev_attr_port23_poe_led.dev_attr.attr,
    &sensor_dev_attr_port24_poe_led.dev_attr.attr,
    &sensor_dev_attr_port25_poe_led.dev_attr.attr,
    &sensor_dev_attr_port26_poe_led.dev_attr.attr,
    &sensor_dev_attr_port27_poe_led.dev_attr.attr,
    &sensor_dev_attr_port28_poe_led.dev_attr.attr,
    &sensor_dev_attr_port29_poe_led.dev_attr.attr,
    &sensor_dev_attr_port30_poe_led.dev_attr.attr,
    &sensor_dev_attr_port31_poe_led.dev_attr.attr,
    &sensor_dev_attr_port32_poe_led.dev_attr.attr,
    &sensor_dev_attr_port33_poe_led.dev_attr.attr,
    &sensor_dev_attr_port34_poe_led.dev_attr.attr,
    &sensor_dev_attr_port35_poe_led.dev_attr.attr,
    &sensor_dev_attr_port36_poe_led.dev_attr.attr,
    &sensor_dev_attr_port37_poe_led.dev_attr.attr,
    &sensor_dev_attr_port38_poe_led.dev_attr.attr,
    &sensor_dev_attr_port39_poe_led.dev_attr.attr,
    &sensor_dev_attr_port40_poe_led.dev_attr.attr,
    &sensor_dev_attr_port41_poe_led.dev_attr.attr,
    &sensor_dev_attr_port42_poe_led.dev_attr.attr,
    &sensor_dev_attr_port43_poe_led.dev_attr.attr,
    &sensor_dev_attr_port44_poe_led.dev_attr.attr,
    &sensor_dev_attr_port45_poe_led.dev_attr.attr,
    &sensor_dev_attr_port46_poe_led.dev_attr.attr,
    &sensor_dev_attr_port47_poe_led.dev_attr.attr,
    &sensor_dev_attr_port48_poe_led.dev_attr.attr,

    &sensor_dev_attr_fan1_led.dev_attr.attr,
    &sensor_dev_attr_fan2_led.dev_attr.attr,
    &sensor_dev_attr_fan3_led.dev_attr.attr,

    &sensor_dev_attr_loc_led.dev_attr.attr,
    &sensor_dev_attr_psu_led.dev_attr.attr,
    &sensor_dev_attr_sys_led.dev_attr.attr,
    &sensor_dev_attr_fan_led.dev_attr.attr,

    &sensor_dev_attr_port_led_auto.dev_attr.attr,
    &sensor_dev_attr_sysled_auto.dev_attr.attr,
    &sensor_dev_attr_fanled_auto.dev_attr.attr,
    &sensor_dev_attr_psuled_auto.dev_attr.attr,
    &sensor_dev_attr_locled_auto.dev_attr.attr,

    &sensor_dev_attr_poe_en_ctrl.dev_attr.attr,

    &sensor_dev_attr_psu_vsense.dev_attr.attr,
    &sensor_dev_attr_psu_pwrgd.dev_attr.attr,
    &sensor_dev_attr_rst_button_10s.dev_attr.attr,

    &sensor_dev_attr_watchdog_timer.dev_attr.attr,
    &sensor_dev_attr_watchdog_kick.dev_attr.attr,
    &sensor_dev_attr_watchdog_en.dev_attr.attr,

    &sensor_dev_attr_system_power_down.dev_attr.attr,
/*
    &sensor_dev_attr_rst_mb_ioiexp.dev_attr.attr,
    &sensor_dev_attr_rst_phy0_5.dev_attr.attr,
    &sensor_dev_attr_rst_poe_ctr.dev_attr.attr,
    &sensor_dev_attr_cpld_sysrst_mac_rst.dev_attr.attr,
    &sensor_dev_attr_rst_cpu_sysrst_out.dev_attr.attr,
    &sensor_dev_attr_rst_gphy_rst.dev_attr.attr,
    &sensor_dev_attr_rst_i2c_mux.dev_attr.attr,
    &sensor_dev_attr_rst_mac_jtag.dev_attr.attr,
    &sensor_dev_attr_rst_button_cpld_cpu.dev_attr.attr,
    &sensor_dev_attr_rst_button.dev_attr.attr,
    &sensor_dev_attr_rst_cpld_i2c_mux.dev_attr.attr,
    &sensor_dev_attr_rst_mac_sys.dev_attr.attr,

    &sensor_dev_attr_int_phy0.dev_attr.attr,
    &sensor_dev_attr_int_phy1.dev_attr.attr,
    &sensor_dev_attr_int_phy2.dev_attr.attr,
    &sensor_dev_attr_int_phy3.dev_attr.attr,
    &sensor_dev_attr_int_phy4_0.dev_attr.attr,
    &sensor_dev_attr_int_phy4_1.dev_attr.attr,
    &sensor_dev_attr_int_phy5_0.dev_attr.attr,
    &sensor_dev_attr_int_phy5_1.dev_attr.attr,
    &sensor_dev_attr_int_oob_ioiexp.dev_attr.attr,
    &sensor_dev_attr_int_mb_ioiexp.dev_attr.attr,
    &sensor_dev_attr_usb_ocp_alert.dev_attr.attr,
    &sensor_dev_attr_temp_alert.dev_attr.attr,
    &sensor_dev_attr_rtc_irq.dev_attr.attr,
    &sensor_dev_attr_psu2_int_alert.dev_attr.attr,
    &sensor_dev_attr_psu1_int_alert.dev_attr.attr,
    &sensor_dev_attr_int_pd69210.dev_attr.attr,
    &sensor_dev_attr_dev_init_done.dev_attr.attr,
    &sensor_dev_attr_fan_ctl_fail.dev_attr.attr,
    &sensor_dev_attr_memhot.dev_attr.attr,
    &sensor_dev_attr_mb_system_int.dev_attr.attr,
    &sensor_dev_attr_mb_mod_pre_int.dev_attr.attr,
*/
    NULL
};

static const struct attribute_group wistron_switch_cpld_group = {
    .attrs = wistron_switch_cpld_attributes,
};


/* ----------------------------------------------------------------------------
 * Module probe/remove functions
 * ----------------------------------------------------------------------------
 */
static int _add_client(struct i2c_client *client)
{
    struct cpld_client_node *node = kzalloc(sizeof(struct cpld_client_node), GFP_KERNEL);
    if (!node) {
        dev_dbg(&client->dev, "Can't allocate cpld_client_node (0x%x)\n", client->addr);
        return -ENOMEM;
    }

    node->client = client;

    mutex_lock(&list_lock);
    list_add(&node->list, &cpld_client_list);
    mutex_unlock(&list_lock);

    return 0;
}

static int _remove_client(struct i2c_client *client)
{
    struct list_head *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int found = 0;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list) {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client == client) {
            found = 1;
            break;
        }
    }

    if (found) {
        list_del(list_node);
        kfree(cpld_node);
    }

    mutex_unlock(&list_lock);

    return 0;
}

/* Probe I2C driver */
static int wistron_i2c_cpld_probe(struct i2c_client *client,
                                 const struct i2c_device_id *dev_id)
{
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_dbg(&client->dev, "i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    status = _add_client(client);
    if (status < 0) {
        dev_err(&client->dev, "Failed to add client\n");
        goto exit;
    }

    status = sysfs_create_group(&client->dev.kobj, &wistron_switch_cpld_group);

    if (status) {
        goto exit;
    }

    dev_info(&client->dev, "chip found\n");

    return 0;

exit:
    return status;
}

static void wistron_i2c_cpld_remove(struct i2c_client *client)
{
    _remove_client(client);

    sysfs_remove_group(&client->dev.kobj, &wistron_switch_cpld_group);

}

/* ----------------------------------------------------------------------------
 * Module main functions
 * ----------------------------------------------------------------------------
 */
static const struct i2c_device_id wistron_i2c_cpld_id[] = {
    {"wistron_cpld",  0},
    {}
};

MODULE_DEVICE_TABLE(i2c, wistron_i2c_cpld_id);

static struct i2c_driver wistron_i2c_cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "wistron_cpld",
    },
    .probe      = wistron_i2c_cpld_probe,
    .remove     = wistron_i2c_cpld_remove,
    .id_table   = wistron_i2c_cpld_id,
};

static int __init wistron_i2c_cpld_init(void)
{
    mutex_init(&list_lock);

    return i2c_add_driver(&wistron_i2c_cpld_driver);
}

static void __exit wistron_i2c_cpld_exit(void)
{
    i2c_del_driver(&wistron_i2c_cpld_driver);
}


MODULE_AUTHOR("Wistron");
MODULE_DESCRIPTION("SONiC platform driver for Wistron CPLD");
MODULE_LICENSE("GPL");

module_init(wistron_i2c_cpld_init);
module_exit(wistron_i2c_cpld_exit);

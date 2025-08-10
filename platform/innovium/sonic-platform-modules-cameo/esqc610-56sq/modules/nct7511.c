/*

 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/regmap.h>
#include <linux/slab.h>

#define DRVNAME "nct7511"


#define REG_BANK		0x00
#define REG_TEMP_LSB		0x05
#define REG_FANCOUNT_LOW	0x13
#define REG_START		0x21
#define REG_MODE		0x22 /* 7.2.32 Mode Selection Register */
#define REG_FAN_ENABLE		0x24
#define REG_PWM(x)		(0x60 + (x))
#define REG_SMARTFAN_EN(x)      (0x64 + (x) / 2)
#define SMARTFAN_EN_SHIFT(x)    ((x) % 2 * 4)
#define REG_VENDOR_ID		0xfd
#define REG_CHIP_ID		0xfe
#define REG_VERSION_ID		0xff

/*
 * Data structures and manipulation thereof
 */

struct nct7511_data {
	struct regmap *regmap;
	struct mutex access_lock; /* for multi-byte read and write operations */
};

static ssize_t show_temp_type(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct nct7511_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	unsigned int mode;
	int ret;

	ret = regmap_read(data->regmap, REG_MODE, &mode);
	if (ret < 0)
		return ret;

	return sprintf(buf, "%u\n", (mode >> (2 * sattr->index) & 3) + 2);
}

static ssize_t store_temp_type(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct nct7511_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	unsigned int type;
	int err;

	err = kstrtouint(buf, 0, &type);
	if (err < 0)
		return err;
	if (sattr->index == 2 && type != 4) /* RD3 */
		return -EINVAL;
	if (type < 3 || type > 4)
		return -EINVAL;
	err = regmap_update_bits(data->regmap, REG_MODE,
			3 << 2 * sattr->index, (type - 2) << 2 * sattr->index);
	return err ? : count;
}

static ssize_t show_pwm_mode(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	struct nct7511_data *data = dev_get_drvdata(dev);
	unsigned int regval;
	int ret;

	if (sattr->index > 1)
		return sprintf(buf, "1\n");

	ret = regmap_read(data->regmap, 0x5E, &regval);
	if (ret < 0)
		return ret;

	return sprintf(buf, "%u\n", !(regval & (1 << sattr->index)));
}

static ssize_t show_pwm(struct device *dev, struct device_attribute *devattr,
			char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct nct7511_data *data = dev_get_drvdata(dev);
	unsigned int val;
	int ret;

	if (!attr->index)
		return sprintf(buf, "255\n");

	ret = regmap_read(data->regmap, attr->index, &val);
	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", val);
}

static ssize_t store_pwm(struct device *dev, struct device_attribute *devattr,
			 const char *buf, size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct nct7511_data *data = dev_get_drvdata(dev);
	int err;
	u8 val;

	err = kstrtou8(buf, 0, &val);
	if (err < 0)
		return err;

	err = regmap_write(data->regmap, attr->index, val);
	return err ? : count;
}

static ssize_t show_pwm_enable(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct nct7511_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	unsigned int reg, enabled;
	int ret;

	ret = regmap_read(data->regmap, REG_SMARTFAN_EN(sattr->index), &reg);
	if (ret < 0)
		return ret;
	enabled = reg >> SMARTFAN_EN_SHIFT(sattr->index) & 1;
	return sprintf(buf, "%u\n", enabled + 1);
}

static ssize_t store_pwm_enable(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct nct7511_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	u8 val;
	int ret;

	ret = kstrtou8(buf, 0, &val);
	if (ret < 0)
		return ret;
	if (val < 1 || val > 2)
		return -EINVAL;
	ret = regmap_update_bits(data->regmap, REG_SMARTFAN_EN(sattr->index),
				 1 << SMARTFAN_EN_SHIFT(sattr->index),
				 (val - 1) << SMARTFAN_EN_SHIFT(sattr->index));
	return ret ? : count;
}

static int nct7511_read_temp(struct nct7511_data *data,
			     u8 reg_temp, u8 reg_temp_low, int *temp)
{
	unsigned int t1, t2 = 0;
	int err;

	*temp = 0;

	mutex_lock(&data->access_lock);
	err = regmap_read(data->regmap, reg_temp, &t1);
	if (err < 0)
		goto abort;
	t1 <<= 8;
	if (reg_temp_low) {	/* 11 bit data */
		err = regmap_read(data->regmap, reg_temp_low, &t2);
		if (err < 0)
			goto abort;
	}
	t1 |= t2 & 0xe0;
	*temp = (s16)t1 / 32 * 125;
abort:
	mutex_unlock(&data->access_lock);
	return err;
}

static int nct7511_read_fan(struct nct7511_data *data, u8 reg_fan)
{
	unsigned int f1, f2;
	int ret;

	mutex_lock(&data->access_lock);
	ret = regmap_read(data->regmap, reg_fan, &f1);
	if (ret < 0)
		goto abort;
	ret = regmap_read(data->regmap, REG_FANCOUNT_LOW, &f2);
	if (ret < 0)
		goto abort;
	ret = (f1 << 5) | (f2 >> 3);
	/* convert fan count to rpm */
	if (ret == 0x1fff)	/* maximum value, assume fan is stopped */
		ret = 0;
	else if (ret)
		ret = DIV_ROUND_CLOSEST(1350000U, ret);
abort:
	mutex_unlock(&data->access_lock);
	return ret;
}

static int nct7511_read_fan_min(struct nct7511_data *data, u8 reg_fan_low,
				u8 reg_fan_high)
{
	unsigned int f1, f2;
	int ret;

	mutex_lock(&data->access_lock);
	ret = regmap_read(data->regmap, reg_fan_low, &f1);
	if (ret < 0)
		goto abort;
	ret = regmap_read(data->regmap, reg_fan_high, &f2);
	if (ret < 0)
		goto abort;
	ret = f1 | ((f2 & 0xf8) << 5);
	/* convert fan count to rpm */
	if (ret == 0x1fff)	/* maximum value, assume no limit */
		ret = 0;
	else if (ret)
		ret = DIV_ROUND_CLOSEST(1350000U, ret);
	else
		ret = 1350000U;
abort:
	mutex_unlock(&data->access_lock);
	return ret;
}

static int nct7511_write_fan_min(struct nct7511_data *data, u8 reg_fan_low,
				 u8 reg_fan_high, unsigned long limit)
{
	int err;

	if (limit)
		limit = DIV_ROUND_CLOSEST(1350000U, limit);
	else
		limit = 0x1fff;
	limit = clamp_val(limit, 0, 0x1fff);

	mutex_lock(&data->access_lock);
	err = regmap_write(data->regmap, reg_fan_low, limit & 0xff);
	if (err < 0)
		goto abort;

	err = regmap_write(data->regmap, reg_fan_high, (limit & 0x1f00) >> 5);
abort:
	mutex_unlock(&data->access_lock);
	return err;
}

static ssize_t show_temp(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct nct7511_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int err, temp;

	err = nct7511_read_temp(data, sattr->nr, sattr->index, &temp);
	if (err < 0)
		return err;

	return sprintf(buf, "%d\n", temp);
}

static ssize_t store_temp(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	struct nct7511_data *data = dev_get_drvdata(dev);
	int nr = sattr->nr;
	long val;
	int err;

	err = kstrtol(buf, 10, &val);
	if (err < 0)
		return err;

	val = DIV_ROUND_CLOSEST(clamp_val(val, -128000, 127000), 1000);

	err = regmap_write(data->regmap, nr, val & 0xff);
	return err ? : count;
}

static ssize_t show_fan(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	struct nct7511_data *data = dev_get_drvdata(dev);
	int speed;

	speed = nct7511_read_fan(data, sattr->index);
	if (speed < 0)
		return speed;

	return sprintf(buf, "%d\n", speed);
}

static ssize_t show_fan_min(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	struct nct7511_data *data = dev_get_drvdata(dev);
	int speed;

	speed = nct7511_read_fan_min(data, sattr->nr, sattr->index);
	if (speed < 0)
		return speed;

	return sprintf(buf, "%d\n", speed);
}

static ssize_t store_fan_min(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	struct nct7511_data *data = dev_get_drvdata(dev);
	unsigned long val;
	int err;

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;

	err = nct7511_write_fan_min(data, sattr->nr, sattr->index, val);
	return err ? : count;
}

static ssize_t show_alarm(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct nct7511_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int bit = sattr->index;
	unsigned int val;
	int ret;

	ret = regmap_read(data->regmap, sattr->nr, &val);
	if (ret < 0)
		return ret;

	return sprintf(buf, "%u\n", !!(val & (1 << bit)));
}

static ssize_t
show_beep(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	struct nct7511_data *data = dev_get_drvdata(dev);
	unsigned int regval;
	int err;

	err = regmap_read(data->regmap, sattr->nr, &regval);
	if (err)
		return err;

	return sprintf(buf, "%u\n", !!(regval & (1 << sattr->index)));
}

static ssize_t
store_beep(struct device *dev, struct device_attribute *attr, const char *buf,
	   size_t count)
{
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	struct nct7511_data *data = dev_get_drvdata(dev);
	unsigned long val;
	int err;

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;
	if (val > 1)
		return -EINVAL;

	err = regmap_update_bits(data->regmap, sattr->nr, 1 << sattr->index,
				 val ? 1 << sattr->index : 0);
	return err ? : count;
}

static SENSOR_DEVICE_ATTR(temp1_type, S_IRUGO | S_IWUSR,
			  show_temp_type, store_temp_type, 0);
static SENSOR_DEVICE_ATTR_2(temp1_input, S_IRUGO, show_temp, NULL, 0x01,
			    REG_TEMP_LSB);
static SENSOR_DEVICE_ATTR_2(temp1_min, S_IRUGO | S_IWUSR, show_temp,
			    store_temp, 0x31, 0);
static SENSOR_DEVICE_ATTR_2(temp1_max, S_IRUGO | S_IWUSR, show_temp,
			    store_temp, 0x30, 0);
static SENSOR_DEVICE_ATTR_2(temp1_crit, S_IRUGO | S_IWUSR, show_temp,
			    store_temp, 0x3a, 0);

static SENSOR_DEVICE_ATTR(temp2_type, S_IRUGO | S_IWUSR,
			  show_temp_type, store_temp_type, 1);
static SENSOR_DEVICE_ATTR_2(temp2_input, S_IRUGO, show_temp, NULL, 0x02,
			    REG_TEMP_LSB);
static SENSOR_DEVICE_ATTR_2(temp2_min, S_IRUGO | S_IWUSR, show_temp,
			    store_temp, 0x33, 0);
static SENSOR_DEVICE_ATTR_2(temp2_max, S_IRUGO | S_IWUSR, show_temp,
			    store_temp, 0x32, 0);
static SENSOR_DEVICE_ATTR_2(temp2_crit, S_IRUGO | S_IWUSR, show_temp,
			    store_temp, 0x3b, 0);

static SENSOR_DEVICE_ATTR(temp3_type, S_IRUGO | S_IWUSR,
			  show_temp_type, store_temp_type, 2);
static SENSOR_DEVICE_ATTR_2(temp3_input, S_IRUGO, show_temp, NULL, 0x03,
			    REG_TEMP_LSB);
static SENSOR_DEVICE_ATTR_2(temp3_min, S_IRUGO | S_IWUSR, show_temp,
			    store_temp, 0x35, 0);
static SENSOR_DEVICE_ATTR_2(temp3_max, S_IRUGO | S_IWUSR, show_temp,
			    store_temp, 0x34, 0);
static SENSOR_DEVICE_ATTR_2(temp3_crit, S_IRUGO | S_IWUSR, show_temp,
			    store_temp, 0x3c, 0);

static SENSOR_DEVICE_ATTR_2(temp4_input, S_IRUGO, show_temp, NULL, 0x04, 0);
static SENSOR_DEVICE_ATTR_2(temp4_min, S_IRUGO | S_IWUSR, show_temp,
			    store_temp, 0x37, 0);
static SENSOR_DEVICE_ATTR_2(temp4_max, S_IRUGO | S_IWUSR, show_temp,
			    store_temp, 0x36, 0);
static SENSOR_DEVICE_ATTR_2(temp4_crit, S_IRUGO | S_IWUSR, show_temp,
			    store_temp, 0x3d, 0);


static SENSOR_DEVICE_ATTR_2(temp1_min_alarm, S_IRUGO, show_alarm, NULL,
			    0x18, 0);
static SENSOR_DEVICE_ATTR_2(temp2_min_alarm, S_IRUGO, show_alarm, NULL,
			    0x18, 1);
static SENSOR_DEVICE_ATTR_2(temp3_min_alarm, S_IRUGO, show_alarm, NULL,
			    0x18, 2);
static SENSOR_DEVICE_ATTR_2(temp4_min_alarm, S_IRUGO, show_alarm, NULL,
			    0x18, 3);

static SENSOR_DEVICE_ATTR_2(temp1_max_alarm, S_IRUGO, show_alarm, NULL,
			    0x19, 0);
static SENSOR_DEVICE_ATTR_2(temp2_max_alarm, S_IRUGO, show_alarm, NULL,
			    0x19, 1);
static SENSOR_DEVICE_ATTR_2(temp3_max_alarm, S_IRUGO, show_alarm, NULL,
			    0x19, 2);
static SENSOR_DEVICE_ATTR_2(temp4_max_alarm, S_IRUGO, show_alarm, NULL,
			    0x19, 3);


static SENSOR_DEVICE_ATTR_2(temp1_crit_alarm, S_IRUGO, show_alarm, NULL,
			    0x1b, 0);
static SENSOR_DEVICE_ATTR_2(temp2_crit_alarm, S_IRUGO, show_alarm, NULL,
			    0x1b, 1);
static SENSOR_DEVICE_ATTR_2(temp3_crit_alarm, S_IRUGO, show_alarm, NULL,
			    0x1b, 2);
static SENSOR_DEVICE_ATTR_2(temp4_crit_alarm, S_IRUGO, show_alarm, NULL,
			    0x1b, 3);

static SENSOR_DEVICE_ATTR_2(temp1_fault, S_IRUGO, show_alarm, NULL, 0x17, 0);
static SENSOR_DEVICE_ATTR_2(temp2_fault, S_IRUGO, show_alarm, NULL, 0x17, 1);
static SENSOR_DEVICE_ATTR_2(temp3_fault, S_IRUGO, show_alarm, NULL, 0x17, 2);

static SENSOR_DEVICE_ATTR_2(temp1_beep, S_IRUGO | S_IWUSR, show_beep,
			    store_beep, 0x5c, 0);
static SENSOR_DEVICE_ATTR_2(temp2_beep, S_IRUGO | S_IWUSR, show_beep,
			    store_beep, 0x5c, 1);
static SENSOR_DEVICE_ATTR_2(temp3_beep, S_IRUGO | S_IWUSR, show_beep,
			    store_beep, 0x5c, 2);
static SENSOR_DEVICE_ATTR_2(temp4_beep, S_IRUGO | S_IWUSR, show_beep,
			    store_beep, 0x5c, 3);

static struct attribute *nct7511_temp_attrs[] = {
	&sensor_dev_attr_temp1_type.dev_attr.attr,
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp1_min.dev_attr.attr,
	&sensor_dev_attr_temp1_max.dev_attr.attr,
	&sensor_dev_attr_temp1_crit.dev_attr.attr,
	&sensor_dev_attr_temp1_min_alarm.dev_attr.attr,
	&sensor_dev_attr_temp1_max_alarm.dev_attr.attr,
	&sensor_dev_attr_temp1_crit_alarm.dev_attr.attr,
	&sensor_dev_attr_temp1_fault.dev_attr.attr,
	&sensor_dev_attr_temp1_beep.dev_attr.attr,

	&sensor_dev_attr_temp2_type.dev_attr.attr,		/* 10 */
	&sensor_dev_attr_temp2_input.dev_attr.attr,
	&sensor_dev_attr_temp2_min.dev_attr.attr,
	&sensor_dev_attr_temp2_max.dev_attr.attr,
	&sensor_dev_attr_temp2_crit.dev_attr.attr,
	&sensor_dev_attr_temp2_min_alarm.dev_attr.attr,
	&sensor_dev_attr_temp2_max_alarm.dev_attr.attr,
	&sensor_dev_attr_temp2_crit_alarm.dev_attr.attr,
	&sensor_dev_attr_temp2_fault.dev_attr.attr,
	&sensor_dev_attr_temp2_beep.dev_attr.attr,

	&sensor_dev_attr_temp3_type.dev_attr.attr,		/* 20 */
	&sensor_dev_attr_temp3_input.dev_attr.attr,
	&sensor_dev_attr_temp3_min.dev_attr.attr,
	&sensor_dev_attr_temp3_max.dev_attr.attr,
	&sensor_dev_attr_temp3_crit.dev_attr.attr,
	&sensor_dev_attr_temp3_min_alarm.dev_attr.attr,
	&sensor_dev_attr_temp3_max_alarm.dev_attr.attr,
	&sensor_dev_attr_temp3_crit_alarm.dev_attr.attr,
	&sensor_dev_attr_temp3_fault.dev_attr.attr,
	&sensor_dev_attr_temp3_beep.dev_attr.attr,

	&sensor_dev_attr_temp4_input.dev_attr.attr,		/* 30 */
	&sensor_dev_attr_temp4_min.dev_attr.attr,
	&sensor_dev_attr_temp4_max.dev_attr.attr,
	&sensor_dev_attr_temp4_crit.dev_attr.attr,
	&sensor_dev_attr_temp4_min_alarm.dev_attr.attr,
	&sensor_dev_attr_temp4_max_alarm.dev_attr.attr,
	&sensor_dev_attr_temp4_crit_alarm.dev_attr.attr,
	&sensor_dev_attr_temp4_beep.dev_attr.attr,

	NULL
};

static umode_t nct7511_temp_is_visible(struct kobject *kobj,
				       struct attribute *attr, int index)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct nct7511_data *data = dev_get_drvdata(dev);
	unsigned int reg;
	int err;

	err = regmap_read(data->regmap, REG_MODE, &reg);
	if (err < 0)
		return 0;

	if (index < 10 &&
	    (reg & 03) != 0x01 && (reg & 0x03) != 0x02)		/* RD1 */
		return 0;

	if (index >= 10 && index < 20 &&
	    (reg & 0x0c) != 0x04 && (reg & 0x0c) != 0x08)	/* RD2 */
		return 0;
	if (index >= 20 && index < 30 && (reg & 0x30) != 0x20)	/* RD3 */
		return 0;

	if (index >= 30 && index < 38)				/* local */
		return attr->mode;

	return attr->mode;
}

static struct attribute_group nct7511_temp_group = {
	.attrs = nct7511_temp_attrs,
	.is_visible = nct7511_temp_is_visible,
};


static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, show_fan, NULL, 0x10);
static SENSOR_DEVICE_ATTR_2(fan1_min, S_IRUGO | S_IWUSR, show_fan_min,
			    store_fan_min, 0x49, 0x4c);
static SENSOR_DEVICE_ATTR_2(fan1_alarm, S_IRUGO, show_alarm, NULL, 0x1a, 0);
static SENSOR_DEVICE_ATTR_2(fan1_beep, S_IRUGO | S_IWUSR, show_beep, store_beep,
			    0x5b, 0);

/* 7.2.89 Fan Control Output Type */
static SENSOR_DEVICE_ATTR(pwm1_mode, S_IRUGO, show_pwm_mode, NULL, 0);

/* 7.2.91... Fan Control Output Value */
static SENSOR_DEVICE_ATTR(pwm1, S_IRUGO | S_IWUSR, show_pwm, store_pwm,
			  REG_PWM(0));


/* 7.2.95... Temperature to Fan mapping Relationships Register */
static SENSOR_DEVICE_ATTR(pwm1_enable, S_IRUGO | S_IWUSR, show_pwm_enable,
			  store_pwm_enable, 0);


static struct attribute *nct7511_fan_attrs[] = {
	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_fan1_min.dev_attr.attr,
	&sensor_dev_attr_fan1_alarm.dev_attr.attr,
	&sensor_dev_attr_fan1_beep.dev_attr.attr,
	NULL
};

static umode_t nct7511_fan_is_visible(struct kobject *kobj,
				      struct attribute *attr, int index)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct nct7511_data *data = dev_get_drvdata(dev);
	int fan = index / 4;	/* 4 attributes per fan */
	unsigned int reg;
	int err;

	err = regmap_read(data->regmap, REG_FAN_ENABLE, &reg);
	if (err < 0 || !(reg & (1 << fan)))
		return 0;

	return attr->mode;
}

static struct attribute_group nct7511_fan_group = {
	.attrs = nct7511_fan_attrs,
	.is_visible = nct7511_fan_is_visible,
};

static struct attribute *nct7511_pwm_attrs[] = {
	&sensor_dev_attr_pwm1_enable.dev_attr.attr,
	&sensor_dev_attr_pwm1_mode.dev_attr.attr,
	&sensor_dev_attr_pwm1.dev_attr.attr,
	NULL
};

static struct attribute_group nct7511_pwm_group = {
	.attrs = nct7511_pwm_attrs,
};

/* 7.2.115... 0x80-0x83, 0x84 Temperature (X-axis) transition */
static SENSOR_DEVICE_ATTR_2(pwm1_auto_point1_temp, S_IRUGO | S_IWUSR,
			    show_temp, store_temp, 0x80, 0);
static SENSOR_DEVICE_ATTR_2(pwm1_auto_point2_temp, S_IRUGO | S_IWUSR,
			    show_temp, store_temp, 0x81, 0);
static SENSOR_DEVICE_ATTR_2(pwm1_auto_point3_temp, S_IRUGO | S_IWUSR,
			    show_temp, store_temp, 0x82, 0);
static SENSOR_DEVICE_ATTR_2(pwm1_auto_point4_temp, S_IRUGO | S_IWUSR,
			    show_temp, store_temp, 0x83, 0);
static SENSOR_DEVICE_ATTR_2(pwm1_auto_point5_temp, S_IRUGO | S_IWUSR,
			    show_temp, store_temp, 0x84, 0);

/* 7.2.120... 0x85-0x88 PWM (Y-axis) transition */
static SENSOR_DEVICE_ATTR(pwm1_auto_point1_pwm, S_IRUGO | S_IWUSR,
			  show_pwm, store_pwm, 0x85);
static SENSOR_DEVICE_ATTR(pwm1_auto_point2_pwm, S_IRUGO | S_IWUSR,
			  show_pwm, store_pwm, 0x86);
static SENSOR_DEVICE_ATTR(pwm1_auto_point3_pwm, S_IRUGO | S_IWUSR,
			  show_pwm, store_pwm, 0x87);
static SENSOR_DEVICE_ATTR(pwm1_auto_point4_pwm, S_IRUGO | S_IWUSR,
			  show_pwm, store_pwm, 0x88);
static SENSOR_DEVICE_ATTR(pwm1_auto_point5_pwm, S_IRUGO, show_pwm, NULL, 0);


static struct attribute *nct7511_auto_point_attrs[] = {
	&sensor_dev_attr_pwm1_auto_point1_temp.dev_attr.attr,
	&sensor_dev_attr_pwm1_auto_point2_temp.dev_attr.attr,
	&sensor_dev_attr_pwm1_auto_point3_temp.dev_attr.attr,
	&sensor_dev_attr_pwm1_auto_point4_temp.dev_attr.attr,
	&sensor_dev_attr_pwm1_auto_point5_temp.dev_attr.attr,

	&sensor_dev_attr_pwm1_auto_point1_pwm.dev_attr.attr,
	&sensor_dev_attr_pwm1_auto_point2_pwm.dev_attr.attr,
	&sensor_dev_attr_pwm1_auto_point3_pwm.dev_attr.attr,
	&sensor_dev_attr_pwm1_auto_point4_pwm.dev_attr.attr,
	&sensor_dev_attr_pwm1_auto_point5_pwm.dev_attr.attr,

	NULL
};

static struct attribute_group nct7511_auto_point_group = {
	.attrs = nct7511_auto_point_attrs,
};

static const struct attribute_group *nct7511_groups[] = {
	&nct7511_temp_group,
	&nct7511_fan_group,
	&nct7511_pwm_group,
	&nct7511_auto_point_group,
	NULL
};

static int nct7511_detect(struct i2c_client *client,
			  struct i2c_board_info *info)
{
	int reg;

	reg = i2c_smbus_read_byte_data(client, REG_VENDOR_ID);
	if (reg != 0x50)
		return -ENODEV;

	reg = i2c_smbus_read_byte_data(client, REG_CHIP_ID);
	if (reg != 0xc3)
		return -ENODEV;

	reg = i2c_smbus_read_byte_data(client, REG_VERSION_ID);
	if (reg < 0 || (reg & 0xf0) != 0x20)
		return -ENODEV;

	/* Also validate lower bits of voltage and temperature registers */
	reg = i2c_smbus_read_byte_data(client, REG_TEMP_LSB);
	if (reg < 0 || (reg & 0x1f))
		return -ENODEV;

	strlcpy(info->type, "nct7511", I2C_NAME_SIZE);
	return 0;
}

static bool nct7511_regmap_is_volatile(struct device *dev, unsigned int reg)
{
	return (reg != REG_BANK && reg <= 0x20) ||
		(reg >= REG_PWM(0) && reg <= REG_PWM(2));
}

static const struct regmap_config nct7511_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.cache_type = REGCACHE_RBTREE,
	.volatile_reg = nct7511_regmap_is_volatile,
};

static int nct7511_init_chip(struct nct7511_data *data)
{
	int err;

	/* Enable ADC */
	err = regmap_update_bits(data->regmap, REG_START, 0x01, 0x01);
	if (err)
		return err;
       /* Enable local temperature sensor */
	return regmap_update_bits(data->regmap, REG_MODE, 0x40, 0x40);
}

static int nct7511_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct nct7511_data *data;
	struct device *hwmon_dev;
	int ret;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;

	data->regmap = devm_regmap_init_i2c(client, &nct7511_regmap_config);
	if (IS_ERR(data->regmap))
		return PTR_ERR(data->regmap);

	mutex_init(&data->access_lock);

	ret = nct7511_init_chip(data);
	if (ret < 0)
		return ret;

	hwmon_dev = devm_hwmon_device_register_with_groups(dev, client->name,
							   data,
							   nct7511_groups);
	return PTR_ERR_OR_ZERO(hwmon_dev);
}


static const struct i2c_device_id nct7511_idtable[] = {
	{ "nct7511", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, nct7511_idtable);

static struct i2c_driver nct7511_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
		.name = DRVNAME,
	},
	.detect = nct7511_detect,
	.probe = nct7511_probe,
	.id_table = nct7511_idtable,
};

module_i2c_driver(nct7511_driver);

MODULE_AUTHOR("Cameo <cameo@nettech-global.net>");
MODULE_DESCRIPTION("NCT7511Y Hardware Monitoring Driver");
MODULE_LICENSE("GPL v2");

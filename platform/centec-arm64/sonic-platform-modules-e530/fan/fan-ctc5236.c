/*
 * Centec FAN driver
 *
 * based by pwm-fan.c
 * Author: shil <shil@centecnetworks.com>
 * Copyright 2005-2018, Centec Networks (Suzhou) Co., Ltd.
 *
 */

#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/sysfs.h>
#include <linux/thermal.h>

#define CTC_MAX_PWM 255
#define CTC_MAX_FAN 4
char *pwmnames[CTC_MAX_FAN] = { "pwm1", "pwm2", "pwm3", "pwm4" };

struct fan_ctc5236_data {
    struct mutex lock;
    struct pwm_device *pwm[CTC_MAX_FAN];
    unsigned int pwm_value[CTC_MAX_FAN];
    long temp;
};

static int __ctc_set_pwm(struct fan_ctc5236_data *data, int index, unsigned long pwm)
{
    struct pwm_args pargs;
    unsigned long duty;
    int ret = 0;

    pwm_get_args(data->pwm[index], &pargs);

    mutex_lock(&data->lock);
    if (data->pwm_value[index] == pwm)
        goto exit_set_pwm_err;

    duty = DIV_ROUND_UP(pwm * (pargs.period - 1), CTC_MAX_PWM);
    ret = pwm_config(data->pwm[index], duty, pargs.period);
    if (ret)
        goto exit_set_pwm_err;

    if (pwm == 0)
        pwm_disable(data->pwm[index]);

    if (data->pwm_value[index] == 0) {
        ret = pwm_enable(data->pwm[index]);
        if (ret)
            goto exit_set_pwm_err;
    }

    data->pwm_value[index] = pwm;
exit_set_pwm_err:
    mutex_unlock(&data->lock);
    return ret;
}

static ssize_t ctc_set_pwm(struct device *dev, struct device_attribute *devattr,
               const char *buf, size_t count)
{
    struct fan_ctc5236_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    unsigned long pwm;
    int ret;

    if (kstrtoul(buf, 10, &pwm) || pwm > CTC_MAX_PWM)
        return -EINVAL;

    ret = __ctc_set_pwm(data, attr->index, pwm);
    if (ret)
        return ret;

    return count;
}

static ssize_t ctc_show_pwm(struct device *dev,
            struct device_attribute *devattr, char *buf)
{
    struct fan_ctc5236_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);

    return sprintf(buf, "%u\n", data->pwm_value[attr->index]);
}

static ssize_t ctc_show_fan(struct device *dev, struct device_attribute *devattr,
            char *buf)
{
    struct fan_ctc5236_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pwm_capture result;
    int ret = 0;
    int ratio = 0;
    
    mutex_lock(&data->lock);

    ret = pwm_capture(data->pwm[attr->index], &result, jiffies_to_msecs(HZ));
    if (ret)
        goto exit_show_fan_err;

    if (result.period == 0 || result.duty_cycle == 0)
        goto exit_show_fan_err;

    ratio = result.period * 10 / result.duty_cycle;
    if (ratio >= 19 && ratio <= 21)
    {
        mutex_unlock(&data->lock);
        return sprintf(buf, "%d\n", (int)(30000000000ll / result.period));
    }

exit_show_fan_err:
    mutex_unlock(&data->lock);
    return sprintf(buf, "0\n");
}

static ssize_t ctc_show_temp(struct device *dev,
            struct device_attribute *devattr, char *buf)
{
    struct fan_ctc5236_data *data = dev_get_drvdata(dev);
    
    return sprintf(buf, "%ld\n", data->temp);
}

static ssize_t ctc_set_temp(struct device *dev, struct device_attribute *devattr,
               const char *buf, size_t count)
{
    struct fan_ctc5236_data *data = dev_get_drvdata(dev);

    if (kstrtol(buf, 10, &(data->temp)))
        return -EINVAL;

    return count;
}


static SENSOR_DEVICE_ATTR(pwm1, S_IRUGO | S_IWUSR, ctc_show_pwm, ctc_set_pwm, 0);
static SENSOR_DEVICE_ATTR(pwm2, S_IRUGO | S_IWUSR, ctc_show_pwm, ctc_set_pwm, 1);
static SENSOR_DEVICE_ATTR(pwm3, S_IRUGO | S_IWUSR, ctc_show_pwm, ctc_set_pwm, 2);
static SENSOR_DEVICE_ATTR(pwm4, S_IRUGO | S_IWUSR, ctc_show_pwm, ctc_set_pwm, 3);
static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, ctc_show_fan, NULL, 0);
static SENSOR_DEVICE_ATTR(fan2_input, S_IRUGO, ctc_show_fan, NULL, 1);
static SENSOR_DEVICE_ATTR(fan3_input, S_IRUGO, ctc_show_fan, NULL, 2);
static SENSOR_DEVICE_ATTR(fan4_input, S_IRUGO, ctc_show_fan, NULL, 3);
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO | S_IWUSR, ctc_show_temp, ctc_set_temp, 0);

static struct attribute *fan_ctc5236_attrs[] = {
    &sensor_dev_attr_pwm1.dev_attr.attr,
    &sensor_dev_attr_pwm2.dev_attr.attr,
    &sensor_dev_attr_pwm3.dev_attr.attr,
    &sensor_dev_attr_pwm4.dev_attr.attr,
    &sensor_dev_attr_fan1_input.dev_attr.attr,
    &sensor_dev_attr_fan2_input.dev_attr.attr,
    &sensor_dev_attr_fan3_input.dev_attr.attr,
    &sensor_dev_attr_fan4_input.dev_attr.attr,
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    NULL,
};

ATTRIBUTE_GROUPS(fan_ctc5236);

static int fan_ctc5236_probe(struct platform_device *pdev)
{
    struct fan_ctc5236_data *data;
    struct pwm_args pargs;
    struct device *hwmon;
    int duty_cycle;
    int ret;
    int idx;

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    mutex_init(&data->lock);
    data->temp = 35000;

    for (idx = 0; idx < CTC_MAX_FAN; idx++)
    {
        data->pwm[idx] = devm_pwm_get(&pdev->dev, pwmnames[idx]);
        if (IS_ERR(data->pwm[idx])) {
            dev_err(&pdev->dev, "Could not get PWM\n");
            return PTR_ERR(data->pwm[idx]);
        }
    }

    platform_set_drvdata(pdev, data);

    for (idx = 0; idx < CTC_MAX_FAN; idx++)
    {
        /*
         * FIXME: pwm_apply_args() should be removed when switching to the
         * atomic PWM API.
         */
        pwm_apply_args(data->pwm[idx]);

        /* Set duty cycle to maximum allowed */
        pwm_get_args(data->pwm[idx], &pargs);

        duty_cycle = pargs.period - 1;
        data->pwm_value[idx] = CTC_MAX_PWM;

        ret = pwm_config(data->pwm[idx], duty_cycle, pargs.period);
        if (ret) {
            dev_err(&pdev->dev, "Failed to configure PWM\n");
            return ret;
        }

        /* Enbale PWM output */
        ret = pwm_enable(data->pwm[idx]);
        if (ret) {
            dev_err(&pdev->dev, "Failed to enable PWM\n");
            return ret;
        }
    }

    hwmon = devm_hwmon_device_register_with_groups(&pdev->dev, "ctc5236fan",
                               data, fan_ctc5236_groups);

    if (IS_ERR(hwmon)) {
        dev_err(&pdev->dev, "Failed to register hwmon device\n");
        for (idx = 0; idx < CTC_MAX_FAN; idx++)
        {
            pwm_disable(data->pwm[idx]);
        }
        return PTR_ERR(hwmon);
    }

    return 0;
}

static int fan_ctc5236_remove(struct platform_device *pdev)
{
    struct fan_ctc5236_data *data = platform_get_drvdata(pdev);
    int idx;

    for (idx = 0; idx < CTC_MAX_FAN; idx++)
    {
        if (data->pwm_value[idx])
            pwm_disable(data->pwm[idx]);
    }
    return 0;
}

static const struct of_device_id of_fan_ctc5236_match[] = {
    { .compatible = "fan-ctc5236", },
    {},
};
MODULE_DEVICE_TABLE(of, of_fan_ctc5236_match);

static struct platform_driver fan_ctc5236_driver = {
    .probe        = fan_ctc5236_probe,
    .remove        = fan_ctc5236_remove,
    .driver    = {
        .name        = "fan-ctc5236",
        .of_match_table    = of_fan_ctc5236_match,
    },
};

module_platform_driver(fan_ctc5236_driver);

MODULE_AUTHOR("Shi Lei <shil@centecnetworks.com>");
MODULE_ALIAS("platform:fan-ctc5236");
MODULE_DESCRIPTION("CTC5236 FAN driver");
MODULE_LICENSE("GPL");

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * RTC client/driver for the PT7C4337 Real-Time Clock
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/slab.h>
#include <linux/regmap.h>

#include <wb_bsp_kernel_debug.h>


#define PT7C4337_REG_SECONDS      0x00
#define PT7C4337_REG_MINUTES      0x01
#define PT7C4337_REG_HOURS        0x02
#define PT7C4337_REG_AMPM         0x02
#define PT7C4337_REG_DAY          0x03
#define PT7C4337_REG_DATE         0x04
#define PT7C4337_REG_MONTH        0x05
#define PT7C4337_REG_CENTURY      0x05
#define PT7C4337_REG_YEAR         0x06
#define PT7C4337_REG_ALARM1       0x07       /* Alarm 1 BASE */
#define PT7C4337_REG_ALARM2       0x0B       /* Alarm 2 BASE */
#define PT7C4337_REG_CR           0x0E       /* Control register */
#       define PT7C4337_REG_CR_nEOSC   0x80
#       define PT7C4337_REG_CR_RS2     0x10
#       define PT7C4337_REG_CR_RS1     0x08
#       define PT7C4337_REG_CR_INTCN   0x04
#       define PT7C4337_REG_CR_A2IE    0x02
#       define PT7C4337_REG_CR_A1IE    0x01

#define PT7C4337_REG_SR           0x0F       /* control/status register */
#       define PT7C4337_REG_SR_OSF     0x80
#       define PT7C4337_REG_SR_A2F     0x02
#       define PT7C4337_REG_SR_A1F     0x01



struct pt7c4337 {
    struct device *dev;
    struct regmap *regmap;
    int irq;
    struct rtc_device *rtc;

    bool suspended;
};

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);


static int pt7c4337_check_rtc_status(struct device *dev)
{
    struct pt7c4337 *pt7c4337 = dev_get_drvdata(dev);
    int ret = 0;
    int control, stat;

    stat = 0;
    ret = regmap_read(pt7c4337->regmap, PT7C4337_REG_SR, &stat);
    if (ret) {
        dev_err(dev, "rd sr reg fail, ret: %d\n", ret);
        return ret;
    }

    if (stat & PT7C4337_REG_SR_OSF) {
        dev_warn(dev,
                "oscillator discontinuity flagged, "
                "time unreliable\n");
    }

    stat &= ~(PT7C4337_REG_SR_OSF | PT7C4337_REG_SR_A1F | PT7C4337_REG_SR_A2F);

    ret = regmap_write(pt7c4337->regmap, PT7C4337_REG_SR, stat);
    if (ret) {
        dev_err(dev, "wr val 0x%x to sr reg fail, ret: %d\n", stat, ret);
        return ret;
    }

    /* If the alarm is pending, clear it before requesting
     * the interrupt, so an interrupt event isn't reported
     * before everything is initialized.
     */
    control = 0;
    ret = regmap_read(pt7c4337->regmap, PT7C4337_REG_CR, &control);
    if (ret) {
        dev_err(dev, "rd cr reg fail, ret: %d\n", ret);
        return ret;
    }

    control &= ~(PT7C4337_REG_CR_A1IE | PT7C4337_REG_CR_A2IE);
    control |= PT7C4337_REG_CR_INTCN;

    return regmap_write(pt7c4337->regmap, PT7C4337_REG_CR, control);
}

static int pt7c4337_read_time(struct device *dev, struct rtc_time *time)
{
    struct pt7c4337 *pt7c4337 = dev_get_drvdata(dev);
    int ret;
    u8 buf[7];
    unsigned int year, month, day, hour, minute, second;
    unsigned int week, twelve_hr, am_pm;
    unsigned int century, add_century = 0;

    mem_clear(buf, sizeof(buf));
    ret = regmap_bulk_read(pt7c4337->regmap, PT7C4337_REG_SECONDS, buf, 7);
    if (ret) {
        DEBUG_ERROR("regmap_bulk_read fail, ret:%d\n", ret);
        return ret;
    }

    DEBUG_INFO("buf: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
               buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);

    second = buf[0];
    minute = buf[1];
    hour = buf[2];
    week = buf[3];
    day = buf[4];
    month = buf[5];
    year = buf[6];

    /* Extract additional information for AM/PM and century */

    twelve_hr = hour & 0x40;
    am_pm = hour & 0x20;
    century = month & 0x80;

    /* Write to rtc_time structure */

    time->tm_sec = bcd2bin(second);
    time->tm_min = bcd2bin(minute);
    if (twelve_hr) {
        /* Convert to 24 hr */
        if (am_pm)
            time->tm_hour = bcd2bin(hour & 0x1F) + 12;
        else
            time->tm_hour = bcd2bin(hour & 0x1F);
    } else {
        time->tm_hour = bcd2bin(hour);
    }

    /* Day of the week in linux range is 0~6 while 1~7 in RTC chip */
    time->tm_wday = bcd2bin(week) - 1;
    time->tm_mday = bcd2bin(day);
    /* linux tm_mon range:0~11, while month range is 1~12 in RTC chip */
    time->tm_mon = bcd2bin(month & 0x7F) - 1;
    if (century)
        add_century = 100;

    time->tm_year = bcd2bin(year) + add_century;

    DEBUG_INFO("time: %d-%d-%d %d:%d:%d, week:%d\n", time->tm_year, time->tm_mon, time->tm_mday,
               time->tm_hour, time->tm_min, time->tm_sec, time->tm_wday);

    return 0;
}

static int pt7c4337_set_time(struct device *dev, struct rtc_time *time)
{
    struct pt7c4337 *pt7c4337 = dev_get_drvdata(dev);
    u8 buf[7];
    int ret;

    /* Extract time from rtc_time and load into pt7c4337*/
    DEBUG_INFO("time: %d-%d-%d %d:%d:%d, week:%d\n", time->tm_year, time->tm_mon, time->tm_mday,
               time->tm_hour, time->tm_min, time->tm_sec, time->tm_wday);

    buf[0] = bin2bcd(time->tm_sec);
    buf[1] = bin2bcd(time->tm_min);
    buf[2] = bin2bcd(time->tm_hour);
    /* Day of the week in linux range is 0~6 while 1~7 in RTC chip */
    buf[3] = bin2bcd(time->tm_wday + 1);
    buf[4] = bin2bcd(time->tm_mday); /* Date */
    /* linux tm_mon range:0~11, while month range is 1~12 in RTC chip */
    buf[5] = bin2bcd(time->tm_mon + 1);
    buf[6] = bin2bcd(time->tm_year);

    DEBUG_INFO("buf: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
               buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);

    ret = regmap_bulk_write(pt7c4337->regmap, PT7C4337_REG_SECONDS, buf, 7);
    DEBUG_INFO("regmap_bulk_write ret:%d\n", ret);

    return ret;
}

/*
 * PT7C4337 has two alarm, we only use alarm1
 * According to linux specification, only support one-shot alarm
 * no periodic alarm mode
 */
static int pt7c4337_read_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
    struct pt7c4337 *pt7c4337 = dev_get_drvdata(dev);
    int control, stat;
    int ret;
    u8 buf[4];

    stat = 0;
    control = 0;
    ret = regmap_read(pt7c4337->regmap, PT7C4337_REG_SR, &stat);
    if (ret) {
        DEBUG_ERROR("rd reg sr:0x%x fail, ret:%d\n", PT7C4337_REG_SR, ret);
        return ret;
    }
    DEBUG_INFO("stat: 0x%x\n", stat);
    
    ret = regmap_read(pt7c4337->regmap, PT7C4337_REG_CR, &control);
    if (ret) {
        DEBUG_ERROR("rd reg cr:0x%x fail, ret:%d\n", PT7C4337_REG_CR, ret);
        return ret;
    }
    DEBUG_INFO("control: 0x%x\n", control);

    mem_clear(buf, sizeof(buf));
    ret = regmap_bulk_read(pt7c4337->regmap, PT7C4337_REG_ALARM1, buf, 4);
    if (ret) {
        DEBUG_ERROR("bulk rd reg fail, ret:%d\n", ret);
        return ret;
    }
    DEBUG_INFO("buf: 0x%x, 0x%x, 0x%x, 0x%x\n", buf[0], buf[1], buf[2], buf[3]);

    alarm->time.tm_sec = bcd2bin(buf[0] & 0x7F);
    alarm->time.tm_min = bcd2bin(buf[1] & 0x7F);
    alarm->time.tm_hour = bcd2bin(buf[2] & 0x7F);
    alarm->time.tm_mday = bcd2bin(buf[3] & 0x7F);

    alarm->enabled = !!(control & PT7C4337_REG_CR_A1IE);
    alarm->pending = !!(stat & PT7C4337_REG_SR_A1F);

    return 0;
}

/*
 * linux rtc-module does not support wday alarm
 * and only 24h time mode supported indeed
 */
static int pt7c4337_set_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
    struct pt7c4337 *pt7c4337 = dev_get_drvdata(dev);
    int control, stat;
    int ret;
    u8 buf[4];

    if (pt7c4337->irq <= 0) {
        DEBUG_ERROR("irq:%d is <= 0\n", pt7c4337->irq);
        return -EINVAL;
    }

    buf[0] = bin2bcd(alarm->time.tm_sec);
    buf[1] = bin2bcd(alarm->time.tm_min);
    buf[2] = bin2bcd(alarm->time.tm_hour);
    buf[3] = bin2bcd(alarm->time.tm_mday);

    /* clear alarm interrupt enable bit */
    control = 0;
    ret = regmap_read(pt7c4337->regmap, PT7C4337_REG_CR, &control);
    if (ret) {
        DEBUG_ERROR("read cr reg fail, ret:%d\n", ret);
        return ret;
    }
    DEBUG_INFO("control: 0x%x\n", control);

    control &= ~(PT7C4337_REG_CR_A1IE | PT7C4337_REG_CR_A2IE);
    ret = regmap_write(pt7c4337->regmap, PT7C4337_REG_CR, control);
    if (ret) {
        DEBUG_ERROR("wr val 0x%x to cr reg fail, ret:%d\n", control, ret);
        return ret;
    }

    /* clear any pending alarm flag */
    stat = 0;
    ret = regmap_read(pt7c4337->regmap, PT7C4337_REG_SR, &stat);
    if (ret) {
        DEBUG_ERROR("rd sr reg fail, ret:%d\n", ret);
        return ret;
    }
    DEBUG_INFO("stat: 0x%x\n", stat);

    stat &= ~(PT7C4337_REG_SR_A1F | PT7C4337_REG_SR_A2F);
    ret = regmap_write(pt7c4337->regmap, PT7C4337_REG_SR, stat);
    if (ret) {
        DEBUG_ERROR("wr val 0x%x to sr reg fail, ret:%d\n", stat, ret);
        return ret;
    }

    DEBUG_INFO("buf %u %u %u %u\n", buf[0], buf[1], buf[2], buf[3]);
    ret = regmap_bulk_write(pt7c4337->regmap, PT7C4337_REG_ALARM1, buf, 4);
    if (ret) {
        DEBUG_ERROR("bulk wr fail, ret:%d\n", ret);
        return ret;
    }

    if (alarm->enabled) {
        control |= PT7C4337_REG_CR_A1IE;
        ret = regmap_write(pt7c4337->regmap, PT7C4337_REG_CR, control);
    }

    DEBUG_INFO("ret:%d\n", ret);
    return ret;
}

static int pt7c4337_update_alarm(struct device *dev, unsigned int enabled)
{
    struct pt7c4337 *pt7c4337 = dev_get_drvdata(dev);
    int control;
    int ret;

    control = 0;
    ret = regmap_read(pt7c4337->regmap, PT7C4337_REG_CR, &control);
    if (ret) {
        DEBUG_ERROR("read cr reg fail, ret:%d\n", ret);
        return ret;
    }
    DEBUG_INFO("control: 0x%x\n", control);

    if (enabled)
        /* enable alarm1 interrupt */
        control |= PT7C4337_REG_CR_A1IE;
    else
        /* disable alarm1 interrupt */
        control &= ~(PT7C4337_REG_CR_A1IE);
    ret = regmap_write(pt7c4337->regmap, PT7C4337_REG_CR, control);

    DEBUG_INFO("ret:%d\n", ret);
    return ret;
}

static int pt7c4337_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
    struct pt7c4337 *pt7c4337 = dev_get_drvdata(dev);

    if (pt7c4337->irq <= 0) {
        DEBUG_ERROR("irq:%d is <= 0\n", pt7c4337->irq);
        return -EINVAL;
    }

    return pt7c4337_update_alarm(dev, enabled);
}

static irqreturn_t pt7c4337_irq(int irq, void *dev_id)
{
    struct device *dev = dev_id;
    struct pt7c4337 *pt7c4337 = dev_get_drvdata(dev);
    int ret;
    int stat, control;

    rtc_lock(pt7c4337->rtc);

    stat = 0;
    ret = regmap_read(pt7c4337->regmap, PT7C4337_REG_SR, &stat);
    if (ret) {
        DEBUG_ERROR("rd sr reg fail, ret:%d\n", ret);
        goto unlock;
    }
    DEBUG_INFO("stat:0x%x\n", stat);

    if (stat & PT7C4337_REG_SR_A1F) {
        control = 0;
        ret = regmap_read(pt7c4337->regmap, PT7C4337_REG_CR, &control);
        if (ret) {
            dev_warn(pt7c4337->dev,
                 "Read Control Register error %d\n", ret);
        } else {
            /* disable alarm1 interrupt */
            control &= ~(PT7C4337_REG_CR_A1IE);
            ret = regmap_write(pt7c4337->regmap, PT7C4337_REG_CR,
                       control);
            if (ret) {
                dev_warn(pt7c4337->dev,
                     "Write Control Register error %d\n",
                     ret);
                goto unlock;
            }

            /* clear the alarm pend flag */
            stat &= ~PT7C4337_REG_SR_A1F;
            ret = regmap_write(pt7c4337->regmap, PT7C4337_REG_SR, stat);
            if (ret) {
                dev_warn(pt7c4337->dev,
                     "Write Status Register error %d\n",
                     ret);
                goto unlock;
            }

            rtc_update_irq(pt7c4337->rtc, 1, RTC_AF | RTC_IRQF);
        }
    }

unlock:
    rtc_unlock(pt7c4337->rtc);

    return IRQ_HANDLED;
}

static const struct rtc_class_ops pt7c4337_rtc_ops = {
    .read_time = pt7c4337_read_time,
    .set_time = pt7c4337_set_time,
    .read_alarm = pt7c4337_read_alarm,
    .set_alarm = pt7c4337_set_alarm,
    .alarm_irq_enable = pt7c4337_alarm_irq_enable,
};

static int pt7c4337_probe(struct device *dev, struct regmap *regmap, int irq,
            const char *name)
{
    struct pt7c4337 *pt7c4337;
    int ret;

    pt7c4337 = devm_kzalloc(dev, sizeof(*pt7c4337), GFP_KERNEL);
    if (!pt7c4337) {
        dev_err(dev, "%s: mem allocation failed\n", __func__);
        return -ENOMEM;
    }

    pt7c4337->regmap = regmap;
    pt7c4337->irq = irq;
    pt7c4337->dev = dev;
    dev_set_drvdata(dev, pt7c4337);

    ret = pt7c4337_check_rtc_status(dev);
    if (ret) {
        dev_err(dev, "pt7c4337_check_rtc_status fail, ret:%d\n", ret);
        return ret;
    }
    DEBUG_INFO("check status ret: %d, irq: %d\n", ret, pt7c4337->irq);

    if (pt7c4337->irq > 0) {
        device_init_wakeup(dev, 1);
    }

    pt7c4337->rtc = devm_rtc_device_register(dev, name, &pt7c4337_rtc_ops,
                        THIS_MODULE);
    if (IS_ERR(pt7c4337->rtc)) {
        dev_err(dev, "devm_rtc_device_register fail \n");
        return PTR_ERR(pt7c4337->rtc);
    }

    if (pt7c4337->irq > 0) {
        ret = devm_request_threaded_irq(dev, pt7c4337->irq, NULL,
                        pt7c4337_irq,
                        IRQF_SHARED | IRQF_ONESHOT,
                        name, dev);
        if (ret) {
            device_set_wakeup_capable(dev, 0);
            pt7c4337->irq = 0;
            dev_err(dev, "unable to request IRQ\n");
        }
    }

    return 0;
}

#ifdef CONFIG_PM_SLEEP
static int pt7c4337_suspend(struct device *dev)
{
    struct pt7c4337 *pt7c4337 = dev_get_drvdata(dev);

    if (device_may_wakeup(dev)) {
        if (enable_irq_wake(pt7c4337->irq))
            dev_warn_once(dev, "Cannot set wakeup source\n");
    }

    return 0;
}

static int pt7c4337_resume(struct device *dev)
{
    struct pt7c4337 *pt7c4337 = dev_get_drvdata(dev);

    if (device_may_wakeup(dev)) {
        disable_irq_wake(pt7c4337->irq);
    }

    return 0;
}
#endif

static const struct dev_pm_ops pt7c4337_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(pt7c4337_suspend, pt7c4337_resume)
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
static int pt7c4337_i2c_probe(struct i2c_client *client,
                const struct i2c_device_id *id)
#else
static int pt7c4337_i2c_probe(struct i2c_client *client)
#endif
{
    struct regmap *regmap;
    static const struct regmap_config config = {
        .reg_bits = 8,
        .val_bits = 8,
        .max_register = PT7C4337_REG_SR,
    };

    regmap = devm_regmap_init_i2c(client, &config);
    if (IS_ERR(regmap)) {
        dev_err(&client->dev, "%s: regmap allocation failed: %ld\n",
            __func__, PTR_ERR(regmap));
        return PTR_ERR(regmap);
    }

    return pt7c4337_probe(&client->dev, regmap, client->irq, client->name);
}

static const struct i2c_device_id pt7c4337_id[] = {
    { "pt7c4337", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, pt7c4337_id);

static const  __maybe_unused struct of_device_id pt7c4337_of_match[] = {
    { .compatible = "pt7c4337" },
    { }
};
MODULE_DEVICE_TABLE(of, pt7c4337_of_match);

static struct i2c_driver pt7c4337_driver = {
    .driver = {
        .name = "rtc-pt7c4337",
        .of_match_table = of_match_ptr(pt7c4337_of_match),
        .pm    = &pt7c4337_pm_ops,
    },
    .probe = pt7c4337_i2c_probe,
    .id_table = pt7c4337_id,
};

module_i2c_driver(pt7c4337_driver);

MODULE_DESCRIPTION("PT7C4337 RTC Driver");
MODULE_LICENSE("GPL");


#include <linux/version.h>
#include "wb_rtc_inst85053.h"

static int inst85053a_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
    struct inst85053a *inst = dev_get_drvdata(dev);
    unsigned char regs[INST85053A_TIME_REG_NUM];
    int ret;

    ret = regmap_bulk_read(inst->regmap, INST85053A_REG_SEC, regs, INST85053A_TIME_REG_NUM);
    if (ret) {
        dev_err(dev, "%s: error %d.\n", __func__, ret);
        return ret;
    }

    
    if ((regs[INST85053A_REG_WEEKDAY] & 0x07) == 0 || (regs[INST85053A_REG_MONTH] & 0x1F) == 0) {
        dev_err(dev, "%s: rtc date reg error.\n", __func__);
        return -EINVAL;
    }

    tm->tm_sec  = bcd2bin(regs[INST85053A_REG_SEC] & 0x7F);
    tm->tm_min  = bcd2bin(regs[INST85053A_REG_MIN] & 0x7F);
    tm->tm_hour = bcd2bin(regs[INST85053A_REG_HOUR] & 0x3F);
    tm->tm_wday = bcd2bin(regs[INST85053A_REG_WEEKDAY] & 0x07) - 1;
    tm->tm_mday = bcd2bin(regs[INST85053A_REG_DAY] & 0x3F);
    tm->tm_mon  = bcd2bin(regs[INST85053A_REG_MONTH] & 0x1F) - 1;
    tm->tm_year = bcd2bin(regs[INST85053A_REG_YEAR]) + 100;

    return 0;
}

/*
 * inst85053a_rtc_set_time - Set the RTC time
 * @dev: Device structure pointer
 * @tm: Pointer to the rtc_time structure containing the time to set
 *
 * Sets the given time into the INST85053a RTC device.
 * Returns 0 on success, negative value on error.
 */

static int inst85053a_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
    struct inst85053a *inst = dev_get_drvdata(dev);
    unsigned char regs[INST85053A_TIME_REG_NUM] = {0};
    int ret;

    ret = regmap_bulk_read(inst->regmap, INST85053A_REG_SEC, regs, INST85053A_TIME_REG_NUM);
    if (ret) {
        dev_err(dev, "%s: error %d.\n", __func__, ret);
        return ret;
    }

    if (tm->tm_year < 100 || tm->tm_year >= 200) {
                dev_err(dev, "%s: year %d out of range. Only 2000-2099 supported.\n", __func__, 1900 + tm->tm_year);
                return -EINVAL;
        }

    regs[INST85053A_REG_SEC]     = bin2bcd(tm->tm_sec & (0x7F));
    regs[INST85053A_REG_MIN]     = bin2bcd(tm->tm_min & (0x7F));
    regs[INST85053A_REG_HOUR]    = bin2bcd(tm->tm_hour & (0x3F));
    regs[INST85053A_REG_WEEKDAY] = bin2bcd((tm->tm_wday & (0x07)) + 1);
    regs[INST85053A_REG_DAY]     = bin2bcd(tm->tm_mday & (0x3F));
    regs[INST85053A_REG_MONTH]   = bin2bcd((tm->tm_mon & (0x1F)) + 1);
    regs[INST85053A_REG_YEAR]    = bin2bcd(tm->tm_year - 100);

    ret = regmap_update_bits(inst->regmap, INST85053A_REG_CTRL1,
                            CTRL1_STOP, CTRL1_STOP);
    if (ret) {
        dev_err(dev, "%s: stop rtc error %d.\n", __func__, ret);
        return ret;
    }

    ret = regmap_bulk_write(inst->regmap, INST85053A_REG_SEC, regs, INST85053A_TIME_REG_NUM);
    if (ret) {
        dev_err(dev, "%s: write rtc time error %d.\n", __func__, ret);
        return ret;
    }

    return regmap_update_bits(inst->regmap, INST85053A_REG_CTRL1,
                             CTRL1_STOP, 0);
}

/*
 * inst85053a_read_alarm - Read the RTC alarm information
 * @dev: Device structure pointer
 * @alarm: Pointer to the rtc_wkalrm structure to store the read alarm information
 *
 * Reads the current alarm time and status from the INST85053a RTC device.
 * Returns 0 on success, negative value on error.
 */
static int inst85053a_read_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
    struct inst85053a *inst = dev_get_drvdata(dev);
    unsigned char regs[INST85053A_TIME_REG_NUM] = {0};
    unsigned int ctrl1;
    int ret;

    ret = regmap_bulk_read(inst->regmap, INST85053A_REG_SEC_ALARM, regs, INST85053A_TIME_REG_NUM);
    if (ret) {
        dev_err(dev, "%s: read rtc error %d.\n", __func__, ret);
        return ret;
    }

    alarm->time.tm_sec  = bcd2bin(regs[INST85053A_REG_SEC_ALARM] & 0x7F);
    alarm->time.tm_min  = bcd2bin(regs[INST85053A_REG_MIN_ALARM] & 0x7F);
    alarm->time.tm_hour = bcd2bin(regs[INST85053A_REG_HOUR_ALARM] & 0x3F);

    ret = regmap_read(inst->regmap, INST85053A_REG_CTRL1, &ctrl1);
    if (ret) {
        dev_err(dev, "%s: read rtc error %d.\n", __func__, ret);
        return ret;
    }

    alarm->enabled = !!(ctrl1 & CTRL1_AIE);
    return 0;
}

/*
 * inst85053a_rtc_alarm_irq_enable - Enable or disable RTC alarm interrupt
 * @dev: Device structure pointer
 * @enable: Enable or disable flag
 *
 * Enables or disables the alarm interrupt of the INST85053a RTC device based on the enable parameter.
 * Returns 0 on success, negative value on error.
 */
static int inst85053a_rtc_alarm_irq_enable(struct device *dev, unsigned int enable){
    unsigned int ctrl1_flag, status_flag;
    int ret;
    struct inst85053a *inst = dev_get_drvdata(dev);

    switch(enable) {
        case INST85053A_DISABLE:
            ctrl1_flag = ~CTRL1_AIE;
            status_flag = ~STATUS_AF;
            break;

        case INST85053A_ENABLE:
            ctrl1_flag = CTRL1_AIE;
            status_flag = ~STATUS_AF;
            break;

        default:
            dev_err(dev, "%s: input enable val error %u.\n", __func__, enable);
            return -EINVAL;
    }

    ret = regmap_update_bits(inst->regmap, INST85053A_REG_CTRL1, CTRL1_AIE, ctrl1_flag);
    ret |= regmap_update_bits(inst->regmap, INST85053A_REG_STATUS, STATUS_AF, status_flag);
    if(ret) {
        dev_err(dev, "%s: update status register error %d.\n", __func__,  ret);
        return -EIO;
    }

    return 0;
}

/*
 * inst85053a_set_alarm - Set the RTC alarm time
 * @dev: Device structure pointer
 * @alarm: Pointer to the rtc_wkalrm structure containing the alarm time to set
 *
 * Sets the given alarm time into the INST85053a RTC device and enables the alarm interrupt.
 * Returns 0 on success, negative value on error.
 */
static int inst85053a_set_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
    struct inst85053a *inst = dev_get_drvdata(dev);
    unsigned char regs[4];
    int ret;

    regs[0] = bin2bcd(alarm->time.tm_sec);
    regs[1] = bin2bcd(alarm->time.tm_min);
    regs[2] = bin2bcd(alarm->time.tm_hour);

    inst85053a_rtc_alarm_irq_enable(dev, INST85053A_DISABLE);

    ret = regmap_write(inst->regmap, INST85053A_REG_SEC_ALARM, regs[0]);
    ret |= regmap_write(inst->regmap, INST85053A_REG_MIN_ALARM, regs[1]);
    ret |= regmap_write(inst->regmap, INST85053A_REG_HOUR_ALARM, regs[2]);
    if (ret) {
        dev_err(dev, "%s: write rtc error %d.\n", __func__, ret);
        return ret;
    }

    return inst85053a_rtc_alarm_irq_enable(dev, INST85053A_ENABLE);
}

/*
 * inst85053a_read_offset - Read the RTC clock offset
 * @dev: Device structure pointer
 * @offset: Pointer to store the read offset value
 *
 * Reads the clock offset value from the INST85053a RTC device and stores it in the provided pointer.
 * Returns 0 on success, negative value on error.
 */
static int inst85053a_read_offset(struct device *dev, long *offset)
{
    struct inst85053a *inst = dev_get_drvdata(dev);
    int ret;
    unsigned int reg, offm;

    ret = regmap_read(inst->regmap, INST85053A_REG_OFFSET, &reg);
    if (ret) {
        dev_err(dev, "%s: write rtc offset error %d.\n", __func__, ret);
        return ret;
    }

    ret = regmap_read(inst->regmap, INST85053A_REG_OSC, &offm);
    if (ret) {
        dev_err(dev, "%s: write rtc offset error %d.\n", __func__, ret);
        return ret;
    }

    if ((offm & BIT(6)) != 0) {
        *offset = reg * INST85053A_MODE_STEP0;
    } else if (offm == INST85053A_NORMAL_MODE) {
        *offset = reg * INST85053A_MODE_STEP1;
    } else {
        dev_err(dev, "%s: read rtc offset error.\n", __func__);
        return -EINVAL;
    }

    return 0;
}

/*
 * inst85053a_set_offset - Set the RTC clock offset
 * @dev: Device structure pointer
 * @offset: Offset value to set
 *
 * Sets the given offset value into the INST85053a RTC device.
 * Returns 0 on success, negative value on error.
 */
static int inst85053a_set_offset(struct device *dev, long offset)
{
    int ret;
    unsigned int reg, reg_mode0, reg_mode1, offm;
    struct inst85053a *inst = dev_get_drvdata(dev);

    if (offset > INST85053A_MODE_STEP0 * 127 || offset < INST85053A_MODE_STEP0 * -128) {
        return -ERANGE;
    }

    reg_mode0 = clamp(DIV_ROUND_CLOSEST(offset, INST85053A_MODE_STEP0), -128L, 127L);
    reg_mode1 = clamp(DIV_ROUND_CLOSEST(offset, INST85053A_MODE_STEP1), -128L, 127L);

    if (abs(reg_mode0 * INST85053A_MODE_STEP0 - offset) >
        abs(reg_mode1 * INST85053A_MODE_STEP1 - offset)) {
        reg = reg_mode1;
        offm = INST85053A_FAST_MODE;
    } else {
        reg = reg_mode0;
        offm = INST85053A_NORMAL_MODE;
    }

    ret = regmap_write(inst->regmap, INST85053A_REG_OFFSET, reg);
    ret |= regmap_update_bits(inst->regmap, INST85053A_REG_OSC, OSC_OFFM, offm);
    if (ret) {
        dev_err(dev, "%s: write rtc offset error %d.\n", __func__, ret);
        return ret;
    }

    return 0;
}

/*
 * inst85053a_nvmem_read - Read NVMEM data
 * @priv: Private data pointer, typically a regmap pointer
 * @offset: Starting offset for reading data
 * @val: Buffer to store the read data
 * @bytes: Number of bytes to read
 *
 * Reads data from the SRAM of the INST85053a device.
 * Returns the number of bytes read, or negative value on error.
 */
static int inst85053a_nvsram_read(void *priv, unsigned int offset, void *val,
            size_t bytes)
{
    int ret;
    struct regmap *inst85053a_sregmap = (struct regmap *)priv;

    if (offset + bytes > INST85053A_SRAM_SIZE) {
        return -EINVAL;
    }

    ret = regmap_bulk_read(inst85053a_sregmap, INST85053A_SRAM_BASE + offset,
                          val, bytes);

    return ret ? ret : bytes;
}

/*
 * inst85053a_nvsram_write - Write NVMEM data
 * @priv: Private data pointer, typically a regmap pointer
 * @offset: Starting offset for writing data
 * @val: Buffer containing the data to write
 * @bytes: Number of bytes to write
 *
 * Writes data to the SRAM of the INST85053a device.
 * Returns the number of bytes written, or negative value on error.
 */
static int inst85053a_nvsram_write(void *priv, unsigned int offset, void *val,
            size_t bytes)
{
    int ret;
    struct regmap *inst85053a_sregmap = (struct regmap *)priv;

    if (offset + bytes > INST85053A_SRAM_SIZE) {
        return -EINVAL;
    }

    ret = regmap_bulk_write(inst85053a_sregmap, INST85053A_SRAM_BASE + offset,
                           val, bytes);

    return ret ? ret : bytes;
}

/*
 * inst85053a_alarm_irq - RTC alarm interrupt handler
 * @irq: Interrupt number
 * @dev_id: Device ID
 *
 * Handles the alarm interrupt for the INST85053a RTC device, reads the alarm status and clears the alarm flag.
 * Returns IRQ_HANDLED if the interrupt was handled, or IRQ_NONE if not.
 */
static irqreturn_t inst85053a_alarm_irq(int irq, void *dev_id)
{
    struct inst85053a *inst = i2c_get_clientdata(dev_id);
    unsigned int val;
    int ret;

    ret = regmap_read(inst->regmap, INST85053A_REG_STATUS, &val);
    if (ret) {
        return IRQ_NONE;
    }

    if (val & STATUS_AF) {
        rtc_update_irq(inst->rtc, 1, RTC_IRQF | RTC_AF);
        regmap_update_bits(inst->regmap, INST85053A_REG_STATUS, STATUS_AF, 0);
        return IRQ_HANDLED;
    }

    return IRQ_NONE;
}

static const struct rtc_class_ops inst85053a_rtc_ops = {
    .read_time = inst85053a_rtc_read_time,
    .set_time = inst85053a_rtc_set_time,
    .read_alarm = inst85053a_read_alarm,
    .set_alarm = inst85053a_set_alarm,
    .set_offset = inst85053a_set_offset,
    .read_offset = inst85053a_read_offset,
    .alarm_irq_enable = inst85053a_rtc_alarm_irq_enable,
};

static const struct inst85053a_config inst_85053a_config = {
    .regmap = {
        .reg_bits = 8,
        .val_bits = 8,
        .max_register = 0x1D,
    },
    .alarm_support = 1,
};

static const struct inst85053a_config pcf_85053a_config = {
    .regmap = {
        .reg_bits = 8,
        .val_bits = 8,
        .max_register = 0x1D,
    },
    .alarm_support = 1,
};

static const struct inst85053a_sram_config inst_85053a_sconfig = {
    .regmap = {
        .reg_bits = 8,
        .val_bits = 8,
        .max_register = 0x7F,
    },
};

static ssize_t inst85053a_version_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct inst85053a *inst = dev_get_drvdata(dev);
    unsigned int version;
    int ret;

    if (!inst) {
        dev_err(dev, "inst is null.\n");
        return -EINVAL;
    }

    ret = regmap_read(inst->regmap, INST85053A_REG_VERSION, &version);
    if (ret) {
        return ret;
    }

    return snprintf(buf, PAGE_SIZE, "0x%x\n", version);
}

static ssize_t inst85053a_vendor_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct inst85053a *inst = dev_get_drvdata(dev);
    unsigned int vendor;
    int ret;

    if (!inst) {
        dev_err(dev, "inst is null.\n");
        return -EINVAL;
    }

    ret = regmap_read(inst->regmap, INST85053A_REG_VENDOR, &vendor);
    if (ret) {
        return ret;
    }

    return snprintf(buf, PAGE_SIZE, "0x%x\n", vendor);
}

static ssize_t inst85053a_model_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct inst85053a *inst = dev_get_drvdata(dev);
    unsigned int model;
    int ret;
    
    if (!inst) {
        dev_err(dev, "inst is null.\n");
        return -EINVAL;
    }

    ret = regmap_read(inst->regmap, INST85053A_REG_MODEL, &model);
    if (ret) {
        return ret;
    }

    return snprintf(buf, PAGE_SIZE, "0x%x\n", model);
}

static DEVICE_ATTR(version, S_IRUGO, inst85053a_version_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, inst85053a_vendor_show, NULL);
static DEVICE_ATTR(model, S_IRUGO, inst85053a_model_show, NULL);

static struct attribute *inst85053a_attrs[] = {
    &dev_attr_version.attr,
    &dev_attr_vendor.attr,
    &dev_attr_model.attr,
    NULL
};

static const struct attribute_group inst85053a_sysfs_group = {
    .attrs = inst85053a_attrs,
};

static int inst85053a_dts_config_init(struct inst85053a *inst)
{
    int ret;
    struct i2c_client *client;
    struct device_node *of_node;

    client = inst->client;
    of_node = client->dev.of_node;

    ret = of_property_read_u8(of_node, "irq_enable", &inst->irq_enable);
    if (ret != 0) {
        dev_info(&client->dev, "inst85053a does not define the irq_enable field");
        inst->irq_enable = 0;
    }

    return 0;
}

static int inst85053a_hw_init(struct inst85053a *inst)
{
    int ret;

    ret = inst->irq_enable == 1 ? 
        regmap_update_bits(inst->regmap, INST85053A_REG_CTRL1, CTRL1_AIE, CTRL1_AIE) :
        0;

    return ret;
}
/*
 * inst85053a_probe - Device probe function
 * @client: I2C client structure pointer
 *
 * Initializes the INST85053a RTC device, including registering the RTC device, setting up the interrupt handler, and creating sysfs attributes.
 * Returns 0 on success, negative value on error.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
static int inst85053a_probe(struct i2c_client *client,
    const struct i2c_device_id *id)
#else
static int inst85053a_probe(struct i2c_client *client)
#endif
{
    struct inst85053a *inst;
    int ret;
    const struct inst85053a_config *config = &inst_85053a_config;
    const void *data = of_device_get_match_data(&client->dev);
    unsigned int val;
    struct i2c_client *sram_client;
    struct regmap *sram_regmap;

    struct nvmem_config nvmem_cfg = {
                .name = "inst85053a_sram",
                .size = INST85053A_SRAM_SIZE,
                .word_size = 1,
                .reg_read = inst85053a_nvsram_read,
                .reg_write = inst85053a_nvsram_write,
                .type = INST85053A_SRAM_SIZE
        };


    inst = devm_kzalloc(&client->dev, sizeof(*inst), GFP_KERNEL);
    if (inst == NULL) {
        dev_err(&client->dev, "devm_kzalloc inst failed.\n");
        return -EINVAL;
    }

    if (data) {
        config = data;
    }

    inst->regmap = devm_regmap_init_i2c(client, &config->regmap);
    if (IS_ERR(inst->regmap)) {
        return dev_err_probe(&client->dev, PTR_ERR(inst->regmap),
                            "devm_regmap_init_i2c failed.\n");
    }

    sram_client = devm_i2c_new_dummy_device(&client->dev, client->adapter, INST85053A_SRAM_ADDR);
    if (IS_ERR(sram_client)) {
        return dev_err_probe(&client->dev, PTR_ERR(sram_client),
                            "Failed to create SRAM I2C client.\n");
    }

    sram_regmap = devm_regmap_init_i2c(sram_client, &inst_85053a_sconfig.regmap);
    if (IS_ERR(sram_regmap)) {
        return dev_err_probe(&client->dev, PTR_ERR(sram_regmap),
                            "devm_regmap_init_i2c failed for SRAM.\n");
    }

    i2c_set_clientdata(client, inst);
    inst->client = client;

    ret = regmap_read(inst->regmap, INST85053A_REG_CTRL1, &val);
    if (ret) {
        return dev_err_probe(&client->dev, ret, "RTC chip is not present\n");
    }

    inst->rtc = devm_rtc_allocate_device(&client->dev);
    if (IS_ERR(inst->rtc)) {
        return PTR_ERR(inst->rtc);
    }

    ret = inst85053a_dts_config_init(inst);
    if (ret) {
        dev_err(&client->dev, "parse dts failed, ret: %d\n", ret);
        return ret;
    }

    ret = inst85053a_hw_init(inst);
    if (ret) {
        dev_err(&client->dev, "inst85053a_hw_init failed, ret: %d\n", ret);
        return ret;
    }

    inst->rtc->ops = &inst85053a_rtc_ops;
    inst->rtc->range_min = RTC_TIMESTAMP_BEGIN_2000;
    inst->rtc->range_max = RTC_TIMESTAMP_END_2099;
    clear_bit(RTC_FEATURE_ALARM, inst->rtc->features);

    if (config->alarm_support == 1 && client->irq > 0) {
        ret = devm_request_threaded_irq(&client->dev, client->irq,
            NULL, inst85053a_alarm_irq,
            IRQF_ONESHOT, "inst85053-alarm", inst);
            if (ret) {
                return dev_err_probe(&client->dev, ret, "unable to request IRQ, alarms disabled.\n");
            }
    }
    mutex_init(&inst->sram_lock);

    ret = sysfs_create_group(&client->dev.kobj, &inst85053a_sysfs_group);
    if (ret) {
        return dev_err_probe(&client->dev, ret, "Failed to create attribute\n");
    }

    nvmem_cfg.priv = sram_regmap;
    ret = devm_rtc_nvmem_register(inst->rtc, &nvmem_cfg);
    if (ret) {
        return dev_err_probe(&client->dev, ret, "RTC chip is not present\n");
    }

    ret = devm_rtc_register_device(inst->rtc);
    if (ret) {
        dev_err(&client->dev, "devm_rtc_register_device failed, ret: %d\n", ret);
        return ret;
    }

    dev_info(&client->dev, "registered rtc-inst85053a sueccessfully.\n");

    return 0;
}

static const struct i2c_device_id inst85053a_id[] = {
    { "inst85053a", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, inst85053a_id);

static const struct of_device_id inst85053a_of_match[] = {
    { .compatible = "dp,inst85053a", .data = &inst_85053a_config },
    { .compatible = "nxp,pcf85053a", .data = &pcf_85053a_config},
    { }
};
MODULE_DEVICE_TABLE(of, inst85053a_of_match);

static struct i2c_driver inst85053a_driver = {
    .driver = {
        .name = "rtc-inst85053a",
        .of_match_table = inst85053a_of_match,
    },
    .probe = inst85053a_probe,
    .id_table = inst85053a_id,
};
module_i2c_driver(inst85053a_driver);

MODULE_DESCRIPTION("rtc 85053 driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
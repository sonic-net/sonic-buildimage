/*
 * Aspeed AST2400/2500 ADC
 *
 * Copyright (C) 2017 Google, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/delay.h>

#define ASPEED_ADC_ADDR                    (0x1e6e9000)
#define SCU_BASE                           (0x1E6E2000)
#define SCU_SYSTEM_RESET                   (0x04)
#define SCU_ADC_RESET_MASK                 (1 << 23)

#define ASPEED_RESOLUTION_BITS             (10)
#define ASPEED_CLOCKS_PER_SAMPLE           (12)

#define ASPEED_REG_ENGINE_CONTROL          (0x00)
#define ASPEED_REG_INTERRUPT_CONTROL       (0x04)
#define ASPEED_REG_VGA_DETECT_CONTROL      (0x08)
#define ASPEED_REG_CLOCK_CONTROL           (0x0C)
#define ASPEED_ADC_REG_ADC10               (0x10)
#define ASPEED_REG_COMPENSATING            (0xC4)

#define ASPEED_ADC_CHANNEL_P1V05_VCCSCSUS  (0)
#define ASPEED_ADC_CHANNEL_P1V05_PCH       (1)
#define ASPEED_ADC_CHANNEL_P1V05_VCCGBE    (2)
#define ASPEED_ADC_CHANNEL_PVCCIN          (3)
#define ASPEED_ADC_CHANNEL_P1V05_VCCIOIN   (4)
#define ASPEED_ADC_CHANNEL_P3V3            (5)
#define ASPEED_ADC_CHANNEL_P3V3_AUX        (6)
#define ASPEED_ADC_CHANNEL_P5V_AUX         (7)
#define ASPEED_ADC_CHANNEL_VIN_12V         (8)
#define ASPEED_ADC_CHANNEL_P1V2_VDDQ       (9)

#define PVCCIN_REAL                        (180)
#define PVCCIN_DIVIDE                      (134)
#define P3V3_REAL                          (330)
#define P3V3_DIVIDE                        (126)
#define P3V3_AUX_REAL                      (330)
#define P3V3_AUX_DIVIDE                    (126)
#define P5V_AUX_REAL                       (500)
#define P5V_AUX_DIVIDE                     (116)
#define VIN_12V_REAL                       (1200)
#define VIN_12V_DIVIDE                     (123)

#define ASPEED_OPERATION_MODE_POWER_DOWN   (0x0 << 1)
#define ASPEED_OPERATION_MODE_STANDBY      (0x1 << 1)
#define ASPEED_OPERATION_MODE_NORMAL       (0x7 << 1)

#define ASPEED_ENGINE_ENABLE               BIT(0)
#define ASPEED_ENGINE_ACSM                 BIT(5)
#define ASPEED_ENGINE_INIT_READY           BIT(8)
#define ASPEED_ENGINE_INIT_TIMEOUT         (100)  /* The initialization timeout period is 100ms */
#define ASPEED_ENGINE_ACS_TIMEOUT          (50)   /* Automatic voltage compensation timeout time 50ms */
#define ASPEED_ENGINE_INIT_SLEEP_TIME      (10)   /* 10ms Polling status */

#define ASPEED_ADC_REF_VOLTAGE             (1800) /* millivolts */
#define ASPEED_ADC_CV_CALC(val)            (((val) >> 16) & 0x3FF)
#define DEBUG_LEN_MAX                      (1024 * 20)
#define ASPEED_ADC_CV_BASE                 (0x200)
#define ASPEED_ADC_CV_MAX                  (0x300)
#define ASPEED_ADC_CV_MIN                  (0x100)

/* Debug level */
typedef enum {
    DBG_START,
    DBG_VERBOSE,
    DBG_KEY,
    DBG_WARN,
    DBG_ERROR,
    DBG_END,
} dbg_level_t;

static int debuglevel=DBG_START;
module_param(debuglevel, int, S_IRUGO | S_IWUSR);
#define DBG_DEBUG(fmt, arg...)  do { \
    if ( debuglevel > DBG_START && debuglevel < DBG_ERROR) { \
          printk(KERN_INFO "[DEBUG]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
    } else if ( debuglevel >= DBG_ERROR ) {   \
        printk(KERN_ERR "[DEBUG]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
    } else {    } \
} while (0)

struct aspeed_adc_model_data {
    const char *model_name;
    unsigned int min_sampling_rate;  /* Hz */
    unsigned int max_sampling_rate;  /* Hz */
    unsigned int vref_voltage;       /* mV */
};

struct aspeed_adc_data {
    struct device    *dev;
    void __iomem    *base;
    spinlock_t    clk_lock;
    struct clk_hw    *clk_prescaler;
    struct clk_hw    *clk_scaler;
    char data_zjy[10];
    char data_inlet[10];
    int adc_cv;
};

#define ASPEED_CHAN(_idx, _data_reg_addr) {                \
    .type = IIO_VOLTAGE,                                   \
    .indexed = 1,                                          \
    .channel = (_idx),                                     \
    .address = (_data_reg_addr),                           \
    .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),          \
    .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) | \
                BIT(IIO_CHAN_INFO_SAMP_FREQ),              \
}

static ssize_t show_p_zjy_value(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_p_zjy_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t show_p_inlet_value(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_p_inlet_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count);

/* add dev attr */
static ssize_t show_ADC_VALUE_value(struct device *dev, struct device_attribute *da, char *buf);
/** P1V05_VCCSCSUS = in21_input
    P1V05_PCH      = in22_input
    P1V05_VCCGB    = in23_input
    PVCCCIN        = in8_input
    P1V05_VCCIOIN  = in24_input
    P3V3           = in4_input
    P3V3_AUX       = in25_input
    P5V_AUX        = in3_input
    12V_VIN        = in12_input
    P1V2_VDDQ      = in11_input
**/
static SENSOR_DEVICE_ATTR(in21_input, S_IRUGO, show_ADC_VALUE_value, NULL, ASPEED_ADC_CHANNEL_P1V05_VCCSCSUS);
static SENSOR_DEVICE_ATTR(in22_input, S_IRUGO, show_ADC_VALUE_value, NULL, ASPEED_ADC_CHANNEL_P1V05_PCH);
static SENSOR_DEVICE_ATTR(in23_input, S_IRUGO, show_ADC_VALUE_value, NULL, ASPEED_ADC_CHANNEL_P1V05_VCCGBE);
static SENSOR_DEVICE_ATTR(in8_input,  S_IRUGO, show_ADC_VALUE_value, NULL, ASPEED_ADC_CHANNEL_PVCCIN);       /* CPU_VCORE */
static SENSOR_DEVICE_ATTR(in24_input, S_IRUGO, show_ADC_VALUE_value, NULL, ASPEED_ADC_CHANNEL_P1V05_VCCIOIN);
static SENSOR_DEVICE_ATTR(in4_input,  S_IRUGO, show_ADC_VALUE_value, NULL, ASPEED_ADC_CHANNEL_P3V3);         /* SYS_3V3  */
static SENSOR_DEVICE_ATTR(in3_input,  S_IRUGO, show_ADC_VALUE_value, NULL, ASPEED_ADC_CHANNEL_P5V_AUX);      /* SYS_5V */
static SENSOR_DEVICE_ATTR(in25_input, S_IRUGO, show_ADC_VALUE_value, NULL, ASPEED_ADC_CHANNEL_P3V3_AUX);
static SENSOR_DEVICE_ATTR(in12_input, S_IRUGO, show_ADC_VALUE_value, NULL, ASPEED_ADC_CHANNEL_VIN_12V);      /* SYS_12V */
static SENSOR_DEVICE_ATTR(in11_input, S_IRUGO, show_ADC_VALUE_value, NULL, ASPEED_ADC_CHANNEL_P1V2_VDDQ);
static SENSOR_DEVICE_ATTR(power3_input, S_IRUGO| S_IWUSR, show_p_zjy_value, set_p_zjy_value, 0);
static SENSOR_DEVICE_ATTR(temp1_input,  S_IRUGO| S_IWUSR, show_p_inlet_value, set_p_inlet_value, 0);

static struct attribute *adc_hwmon_attrs[] = {
    &sensor_dev_attr_in21_input.dev_attr.attr,
    &sensor_dev_attr_in22_input.dev_attr.attr,
    &sensor_dev_attr_in23_input.dev_attr.attr,
    &sensor_dev_attr_in8_input.dev_attr.attr,
    &sensor_dev_attr_in24_input.dev_attr.attr,
    &sensor_dev_attr_in4_input.dev_attr.attr,
    &sensor_dev_attr_in25_input.dev_attr.attr,
    &sensor_dev_attr_in3_input.dev_attr.attr,
    &sensor_dev_attr_in12_input.dev_attr.attr,
    &sensor_dev_attr_in11_input.dev_attr.attr,
    &sensor_dev_attr_power3_input.dev_attr.attr,
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    NULL
};
ATTRIBUTE_GROUPS(adc_hwmon);

/* File system dependent */
static struct attribute *adc_hwmon_sysfs_attrs[] = {
    &sensor_dev_attr_in11_input.dev_attr.attr,
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    NULL
};
static const struct attribute_group adc_hwmon_sysfs_attrs_group = {
    .attrs = adc_hwmon_sysfs_attrs,
};

static int aspeed_adc_get_channel_reading(struct aspeed_adc_data *data, int channel, long *val)
{
    u32 data_reg;
    u32 data_reg_val;
    long adc_val_origin, adc_val_adjust;

    /* Each 32-bit data register contains 2 10-bit ADC values. */
    data_reg = ASPEED_ADC_REG_ADC10 + (channel / 2) * 4;
    data_reg_val = readl(data->base + data_reg);

    DBG_DEBUG("adc channel %d, reg addr:0x%x, val:0x%x\n",
        channel, data_reg, data_reg_val);

    if (channel % 2 == 0) {
        adc_val_origin = data_reg_val & 0x3FF;
    } else {
        adc_val_origin = (data_reg_val >> 16) & 0x3FF;
    }
    adc_val_adjust = adc_val_origin + data->adc_cv;

    DBG_DEBUG("adc origin val:0x%lx, cv:%d, adjust val:0x%lx\n",
        adc_val_origin, data->adc_cv, adc_val_adjust);
    /* Scale 10-bit input reading to millivolts. */
    *val = adc_val_adjust * ASPEED_ADC_REF_VOLTAGE / 1024;
    return 0;
}

static ssize_t show_ADC_VALUE_value(struct device *dev, struct device_attribute *da, char *buf)
{
    int adc_devide_para = 100;
    long adc_val = 0;
    int adc_index = to_sensor_dev_attr_2(da)->index;
    struct aspeed_adc_data *data = dev_get_drvdata(dev);
    /* adc_devide_para indicates the actual voltage *100/ partial voltage. *100 is to prevent precision loss */
    switch(adc_index){
    case ASPEED_ADC_CHANNEL_PVCCIN:
        adc_devide_para = (100 * PVCCIN_REAL) / PVCCIN_DIVIDE;
        break;
    case ASPEED_ADC_CHANNEL_P3V3:
        adc_devide_para = (100 * P3V3_REAL) / P3V3_DIVIDE;
        break;
    case ASPEED_ADC_CHANNEL_P3V3_AUX:
        adc_devide_para = (100 * P3V3_AUX_REAL) / P3V3_AUX_DIVIDE;
        break;
    case ASPEED_ADC_CHANNEL_P5V_AUX:
        adc_devide_para = (100 * P5V_AUX_REAL) / P5V_AUX_DIVIDE;
        break;
    case ASPEED_ADC_CHANNEL_VIN_12V:
        adc_devide_para = (100 * VIN_12V_REAL) / VIN_12V_DIVIDE;
        break;
    default:
        adc_devide_para = 100;
        break;
    }

    aspeed_adc_get_channel_reading(data,adc_index,&adc_val);
    return snprintf(buf, 256, "%ld\n", adc_val*adc_devide_para/100);
}

static ssize_t show_p_zjy_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct aspeed_adc_data *data = dev_get_drvdata(dev);
    return snprintf(buf, 256, "%s\n", data->data_zjy);
}

static ssize_t show_p_inlet_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct aspeed_adc_data *data = dev_get_drvdata(dev);
    return snprintf(buf, 256, "%s\n", data->data_inlet);
}

static ssize_t set_p_zjy_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct aspeed_adc_data *data = dev_get_drvdata(dev);

    if (strlen(buf) > 9) {
        DBG_DEBUG("lenth must less then 9\n");
        return -1;
    }
    memset(data->data_zjy, 0, 10);
    memcpy(data->data_zjy, buf, strlen(buf));

    return count;
}

static ssize_t set_p_inlet_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct aspeed_adc_data *data = dev_get_drvdata(dev);

    if (strlen(buf) > 9) {
        DBG_DEBUG("lenth must less then 9\n");
        return -1;
    }
    memset(data->data_inlet, 0, 10);
    memcpy(data->data_inlet, buf, strlen(buf));

    return count;
}

static void aspeed_toggle_scu_reset(void)
{
    u32 val;

    val = readl(ioremap(SCU_BASE + SCU_SYSTEM_RESET, 4));
    DBG_DEBUG("read val: %x\n", val);

    if (!(val & SCU_ADC_RESET_MASK)) {
        val = val | SCU_ADC_RESET_MASK;
        writel(val, ioremap(SCU_BASE + SCU_SYSTEM_RESET, 4));
        msleep(1);
    }

    val = val & ~SCU_ADC_RESET_MASK;
    writel(val, ioremap(SCU_BASE + SCU_SYSTEM_RESET, 4));
}

static int aspeed_adc_init_complete(struct aspeed_adc_data *data)
{
    u32 data_reg_val;

    data_reg_val = readl(data->base + ASPEED_REG_ENGINE_CONTROL);
    if (data_reg_val & ASPEED_ENGINE_INIT_READY) {
        return 1;
    }

    return 0;
}

static int aspeed_adc_init_wait(struct aspeed_adc_data *data)
{
    int retry_cnt;

    retry_cnt = ASPEED_ENGINE_INIT_TIMEOUT / ASPEED_ENGINE_INIT_SLEEP_TIME;
    while (retry_cnt--) {
        if (aspeed_adc_init_complete(data)) {
            return 0;
        }
        msleep(ASPEED_ENGINE_INIT_SLEEP_TIME);
    }

    return -1;
}

static int aspeed_adc_acs_complete(struct aspeed_adc_data *data)
{
    u32 data_reg_val;

    data_reg_val = readl(data->base + ASPEED_REG_ENGINE_CONTROL);
    if (data_reg_val & ASPEED_ENGINE_ACSM) {
        return 0;
    }

    return 1;
}

static int aspeed_adc_compensate_wait(struct aspeed_adc_data *data)
{
    int retry_cnt;

    retry_cnt = ASPEED_ENGINE_ACS_TIMEOUT / ASPEED_ENGINE_INIT_SLEEP_TIME;
    while (retry_cnt--) {
        if (aspeed_adc_acs_complete(data)) {
            return 0;
        }
        msleep(ASPEED_ENGINE_INIT_SLEEP_TIME);
    }

    return -1;
}

static int aspeed_adc_probe(struct platform_device *pdev)
{
    struct aspeed_adc_data *data;
    struct resource *res;
    const char *clk_parent_name;
    int ret;
    u32 adc_engine_control_reg_val;
    int adc_compensating_val;
    int status;

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data) {
        dev_err(&pdev->dev, "devm_kzalloc failed.\n");
        return -ENOMEM;
    }

    data->dev = &pdev->dev;
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    data->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(data->base)){
        dev_err(&pdev->dev, "devm_ioremap_resource failed.\n");
        return PTR_ERR(data->base);
    }

    aspeed_toggle_scu_reset();  /* Reset ADC controller */

    /* Register ADC clock prescaler with source specified by device tree. */
    spin_lock_init(&data->clk_lock);
    clk_parent_name = of_clk_get_parent_name(pdev->dev.of_node, 0);
    data->clk_prescaler = clk_hw_register_divider(
                &pdev->dev, "prescaler", clk_parent_name, 0,
                data->base + ASPEED_REG_CLOCK_CONTROL,
                17, 15, 0, &data->clk_lock);
    if (IS_ERR(data->clk_prescaler)) {
        dev_err(&pdev->dev, "prescaler, clk_hw_register_divider failed.\n");
        return PTR_ERR(data->clk_prescaler);
    }
    /*
     * Register ADC clock scaler downstream from the prescaler. Allow rate
     * setting to adjust the prescaler as well.
     */
    data->clk_scaler = clk_hw_register_divider(
                &pdev->dev, "scaler", "prescaler",
                CLK_SET_RATE_PARENT,
                data->base + ASPEED_REG_CLOCK_CONTROL,
                0, 10, 0, &data->clk_lock);
    if (IS_ERR(data->clk_scaler)) {
        ret = PTR_ERR(data->clk_scaler);
        dev_err(&pdev->dev, "scaler, clk_hw_register_divider failed.\n");
        goto scaler_error;
    }

    /* Start 8 9 in normal mode. */
    clk_prepare_enable(data->clk_scaler->clk);

    /* Initializes the ADC controller */
    adc_engine_control_reg_val = ASPEED_OPERATION_MODE_NORMAL | ASPEED_ENGINE_ENABLE;
    writel(adc_engine_control_reg_val, data->base + ASPEED_REG_ENGINE_CONTROL);
    ret = aspeed_adc_init_wait(data);
    if (ret) {
        dev_err(&pdev->dev, "Aspeed adc controller initial sequence timeout.\n");
        goto iio_register_error;
    }
    /* Configure automatic voltage compensation mode */
    adc_engine_control_reg_val = ASPEED_ENGINE_ACSM | ASPEED_OPERATION_MODE_NORMAL | ASPEED_ENGINE_ENABLE;
    writel(adc_engine_control_reg_val, data->base + ASPEED_REG_ENGINE_CONTROL);
    ret = aspeed_adc_compensate_wait(data);
    if (ret) {
        dev_warn(&pdev->dev, "Aspeed adc controller set auto compensating sensing mode timeout.\n");
        data->adc_cv = 0;
        ret = 0;
    } else {
        /* Save CV values */
        adc_compensating_val = ASPEED_ADC_CV_CALC(readl(data->base + ASPEED_REG_COMPENSATING));
        if ((adc_compensating_val > ASPEED_ADC_CV_MAX) || (adc_compensating_val < ASPEED_ADC_CV_MIN)) {
            dev_warn(&pdev->dev, "Aspeed adc controller auto compensating sensing value[0x%x] invalid.\n",
                adc_compensating_val);
            data->adc_cv = 0;
        } else {
            data->adc_cv = ASPEED_ADC_CV_BASE - adc_compensating_val;
        }
    }

    /* Configuring sampling channels */
    adc_engine_control_reg_val = (GENMASK(9, 0)<<16) | ASPEED_OPERATION_MODE_NORMAL | ASPEED_ENGINE_ENABLE;
    writel(adc_engine_control_reg_val, data->base + ASPEED_REG_ENGINE_CONTROL);

    status = sysfs_create_group(&pdev->dev.kobj, &adc_hwmon_sysfs_attrs_group);
    if (status != 0) {
        dev_err(&pdev->dev, "ADC hwmon sysfs create failed.\n");
        goto iio_register_error;
    }
    /* Make this driver part of hwmon class. */
    data->dev = hwmon_device_register_with_groups(&pdev->dev, pdev->name, data, adc_hwmon_groups);
    if (IS_ERR(data->dev)) {
        sysfs_remove_group(&pdev->dev.kobj, &adc_hwmon_sysfs_attrs_group);
        dev_err(&pdev->dev, "ADC hwmon groups register failed.\n");
        goto iio_register_error;
    }
    dev_info(&pdev->dev, "Aspeed adc controller init success, cv:%d.\n", data->adc_cv);
    return 0;
iio_register_error:
    writel(ASPEED_OPERATION_MODE_POWER_DOWN, data->base + ASPEED_REG_ENGINE_CONTROL);
    clk_disable_unprepare(data->clk_scaler->clk);
    clk_hw_unregister_divider(data->clk_scaler);
scaler_error:
    clk_hw_unregister_divider(data->clk_prescaler);
    return ret;
}

static void aspeed_adc_remove(struct platform_device *pdev)
{
    struct aspeed_adc_data *data = dev_get_drvdata(&pdev->dev);
    /* Resource release */
    writel(ASPEED_OPERATION_MODE_POWER_DOWN, data->base + ASPEED_REG_ENGINE_CONTROL);
    clk_disable_unprepare(data->clk_scaler->clk);
    clk_hw_unregister_divider(data->clk_scaler);
    clk_hw_unregister_divider(data->clk_prescaler);

    hwmon_device_unregister(data->dev);
    sysfs_remove_group(&pdev->dev.kobj, &adc_hwmon_sysfs_attrs_group);
}

static const struct aspeed_adc_model_data ast2400_model_data = {
    .model_name = "ast2400-adc",
    .vref_voltage = 2500, /* mV */
    .min_sampling_rate = 10000,
    .max_sampling_rate = 500000,
};

static const struct aspeed_adc_model_data ast2500_model_data = {
    .model_name = "ast2500-adc",
    .vref_voltage = 1800, /* mV */
    .min_sampling_rate = 1,
    .max_sampling_rate = 1000000,
};

static const struct of_device_id aspeed_adc_matches[] = {
    { .compatible = "aspeed,ast2400-adc", .data = &ast2400_model_data },
    { .compatible = "aspeed,ast2500-adc", .data = &ast2500_model_data },
    {},
};
MODULE_DEVICE_TABLE(of, aspeed_adc_matches);

static struct platform_driver aspeed_adc_hwmon_driver = {
    .probe = aspeed_adc_probe,
    .remove = aspeed_adc_remove,
    .driver = {
        .name = KBUILD_MODNAME,
        .of_match_table = aspeed_adc_matches,
    }
};

module_platform_driver(aspeed_adc_hwmon_driver);

MODULE_AUTHOR("Rick Altherr <raltherr@google.com>");
MODULE_DESCRIPTION("Aspeed AST2400/2500 ADC Driver");
MODULE_LICENSE("GPL");

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Jozsef Horvath <info@ministro.hu>
 *
 */
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/iio/iio.h>
#include <linux/iio/events.h>
#include <linux/iio/buffer.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/triggered_buffer.h>

#define TI_ADS7142_NAME					"ads7142"

#define TI_ADS7142_DATA_VALID_TIMEOUT			100

/* Opcodes for commands */
/* General */
#define TI_ADS7142_OC_GENERAL				0x00
/* Single Register Read */
#define TI_ADS7142_OC_SINGLE_REG_READ			0x10
/* Single Register Write */
#define TI_ADS7142_OC_SINGLE_REG_WRITE			0x08
/* Single Bit Set */
#define TI_ADS7142_OC_SET_BIT				0x18
/* Single Bit Clear */
#define TI_ADS7142_OC_CLEAR_BIT				0x20
/* Block Register Read */
#define TI_ADS7142_OC_BLOCK_READ			0x30
/* Block Register Write */
#define TI_ADS7142_OC_BLOCK_WRITE			0x28

/* Registers */
/* Reset registers */
#define TI_ADS7142_WKEY					0x17
#define TI_ADS7142_DEVICE_RESET				0x14
/* Functional mode select registers */
#define TI_ADS7142_OFFSET_CAL				0x15
#define TI_ADS7142_OPMODE_SEL				0x1C
#define TI_ADS7142_OPMODE_SEL_MANUALCH0			(0)
#define TI_ADS7142_OPMODE_SEL_MANUALSEQ			(4)
#define TI_ADS7142_OPMODE_SEL_MONITORING		(6)
#define TI_ADS7142_OPMODE_SEL_HIGHPREC			(7)
#define TI_ADS7142_OPMODE_STATUS			0x00
#define TI_ADS7142_OPMODE_STATUS_OPMODE_MSK		(3)
#define TI_ADS7142_OPMODE_STATUS_OPMODE_MANUAL		(0)
#define TI_ADS7142_OPMODE_STATUS_OPMODE_AUTO		(2)
#define TI_ADS7142_OPMODE_STATUS_OPMODE_HIGHPREC	(3)
#define TI_ADS7142_OPMODE_STATUS_HS_MODE		BIT(2)

/* Input config register */
#define TI_ADS7142_CH_INPUT_CFG				0x24
#define TI_ADS7142_CH_INPUT_CFG_TCSE			(0)
#define TI_ADS7142_CH_INPUT_CFG_SCSE			(1)
#define TI_ADS7142_CH_INPUT_CFG_SCPD			(2)
/* Analog mux and sequencer registers */
#define TI_ADS7142_AUTO_SEQ_CHEN			0x20
#define TI_ADS7142_AUTO_SEQ_CHEN_CH0			BIT(0)
#define TI_ADS7142_AUTO_SEQ_CHEN_CH1			BIT(1)
#define TI_ADS7142_START_SEQUENCE			0x1E
#define TI_ADS7142_START_SEQUENCE_SEQ_START		BIT(0)
#define TI_ADS7142_ABORT_SEQUENCE			0x1F
#define TI_ADS7142_ABORT_SEQUENCE_SEQ_ABORT		BIT(0)
#define TI_ADS7142_SEQUENCE_STATUS			0x04
#define TI_ADS7142_SEQUENCE_STATUS_SEQ_ERR_ST_MSK	(0x06)
#define TI_ADS7142_SEQUENCE_STATUS_SEQ_DISABLED		(0x00)
#define TI_ADS7142_SEQUENCE_STATUS_SEQ_ENABLED		(0x02)
#define TI_ADS7142_SEQUENCE_STATUS_SEQ_ERROR		(0x06)
/* Oscillator and timing control registers */
#define TI_ADS7142_OSC_SEL				0x18
#define TI_ADS7142_OSC_SEL_HSZ_LP			BIT(0)
#define TI_ADS7142_NCLK_SEL				0x19
#define TI_ADS7142_NCLK_SEL_MSK				0xFF
/* Data buffer control register */
#define TI_ADS7142_DATA_BUFFER_OPMODE			0x2C
#define TI_ADS7142_DATA_BUFFER_OPMODE_STOP_BURST	(0)
#define TI_ADS7142_DATA_BUFFER_OPMODE_START_BURST	(1)
#define TI_ADS7142_DATA_BUFFER_OPMODE_PRE_ALERT		(4)
#define TI_ADS7142_DATA_BUFFER_OPMODE_POST_ALERT	(6)
#define TI_ADS7142_DOUT_FORMAT_CFG			0x28
#define TI_ADS7142_DOUT_FORMAT_CFG_12B			(0)
#define TI_ADS7142_DOUT_FORMAT_CFG_12BCH		(1)
#define TI_ADS7142_DOUT_FORMAT_CFG_12BCHDV		(2)
#define TI_ADS7142_DATA_BUFFER_STATUS			0x01
/* Accumulator control register */
#define TI_ADS7142_ACC_EN				0x30
#define TI_ADS7142_ACC_CH0_LSB				0x08
#define TI_ADS7142_ACC_CH0_MSB				0x09
#define TI_ADS7142_ACC_CH1_LSB				0x0A
#define TI_ADS7142_ACC_CH1_MSB				0x0B
#define TI_ADS7142_ACC_STATUS				0x02
/* Digital window comparator registers */
#define TI_ADS7142_ALERT_DWC_EN				0x37
#define TI_ADS7142_ALERT_DWC_EN_BLOCK_EN		BIT(0)
#define TI_ADS7142_ALERT_CHEN				0x34
#define TI_ADS7142_DWC_HTH_CH0_LSB			0x38
#define TI_ADS7142_DWC_HTH_CH0_MSB			0x39
#define TI_ADS7142_DWC_LTH_CH0_LSB			0x3A
#define TI_ADS7142_DWC_LTH_CH0_MSB			0x3B
#define TI_ADS7142_DWC_HYS_CH0				0x40
#define TI_ADS7142_DWC_HTH_CH1_LSB			0x3C
#define TI_ADS7142_DWC_HTH_CH1_MSB			0x3D
#define TI_ADS7142_DWC_LTH_CH1_LSB			0x3E
#define TI_ADS7142_DWC_LTH_CH1_MSB			0x3F
#define TI_ADS7142_DWC_HYS_CH1				0x41
#define TI_ADS7142_PRE_ALT_EVT_CNT			0x36
#define TI_ADS7142_ALT_TRIG_CHID			0x03
#define TI_ADS7142_ALT_LOW_FLAGS			0x0C
#define TI_ADS7142_ALT_LOW_FLAGS_CH0			BIT(0)
#define TI_ADS7142_ALT_LOW_FLAGS_CH1			BIT(1)
#define TI_ADS7142_ALT_HIGH_FLAGS			0x0E
#define TI_ADS7142_ALT_HIGH_FLAGS_CH0			BIT(0)
#define TI_ADS7142_ALT_HIGH_FLAGS_CH1			BIT(1)

#define TI_ADS7142_THRESHOLD_MSK			0xFFF
#define TI_ADS7142_HYSTERESIS_MSK			0x3F

#define TI_ADS7142_HSO_FREQ				20000000
#define TI_ADS7142_HSO_NCLK_MIN				21
#define TI_ADS7142_HSO_NCLK_MAX				255
#define TI_ADS7142_HSO_MIN_SS				78431
#define TI_ADS7142_HSO_MAX_SS				952380
#define TI_ADS7142_LPO_FREQ				10504
#define TI_ADS7142_LPO_NCLK_MIN				18
#define TI_ADS7142_LPO_NCLK_MAX				255
#define TI_ADS7142_LPO_MIN_SS				41
#define TI_ADS7142_LPO_MAX_SS				583

#define TI_ADS7142_CHANNEL_COUNT			2

/* Channel count obtained when the module is loaded */
static int g_chn_count = 0;
module_param(g_chn_count, int, 0660);

struct ti_ads7142_channel_config {
    bool alert_low;
    bool alert_high;
    int high_threshold;
    int low_threshold;
    int hysteresis;
};

struct ti_ads7142_channel {
    struct ti_ads7142_channel_config config;
    u32 channel;
};

struct ti_ads7142_config {
    bool osc_sel;
    u32 n_clk;
    int buffer_mode;
};

struct ti_ads7142_priv {
    struct mutex lock; /* For syncing access to device */
    struct regulator *avdd;
    struct regulator *dvdd;
    struct ti_ads7142_config config;
    struct ti_ads7142_channel *channels;
    u16 *scan_data;
    bool irq_present;
    bool monitor_pending;
};

static const struct iio_event_spec ti_ads7142_events[] = {
    {
        .type = IIO_EV_TYPE_THRESH,
        .dir = IIO_EV_DIR_RISING,
        .mask_separate = BIT(IIO_EV_INFO_VALUE)
                        | BIT(IIO_EV_INFO_ENABLE),
    }, {
        .type = IIO_EV_TYPE_THRESH,
        .dir = IIO_EV_DIR_FALLING,
        .mask_separate = BIT(IIO_EV_INFO_VALUE)
                        | BIT(IIO_EV_INFO_ENABLE),
    }, {
        .type = IIO_EV_TYPE_THRESH,
        .dir = IIO_EV_DIR_EITHER,
        .mask_separate = BIT(IIO_EV_INFO_HYSTERESIS),
    },
};

static const char * const ti_ads7142_ain_names[] = {
    "AIN0",
    "AIN1",
};

#define TI_ADS7142_BUFFM_NONE				0
#define TI_ADS7142_BUFFM_STOP_BURST			1
#define TI_ADS7142_BUFFM_START_BURST			2
#define TI_ADS7142_BUFFM_PRE_ALERT			3
#define TI_ADS7142_BUFFM_POST_ALERT			4
static const char * const ti_ads7142_buffer_modes[] = {
    [TI_ADS7142_BUFFM_NONE]		= "none",
    [TI_ADS7142_BUFFM_STOP_BURST]	= "stop_burst",
    [TI_ADS7142_BUFFM_START_BURST]	= "start_burst",
    [TI_ADS7142_BUFFM_PRE_ALERT]	= "pre_alert",
    [TI_ADS7142_BUFFM_POST_ALERT]	= "post_alert"
};

static int ti_ads7142_reg_write(const struct i2c_client *client, u8 reg,
				u8 data)
{
    u8 buf[3] = { TI_ADS7142_OC_SINGLE_REG_WRITE, reg, data };
    int ret;

    ret = i2c_master_send(client, buf, sizeof(buf));
    if (ret >= 0 && ret != sizeof(buf))
        ret = -EIO;

    return ret == sizeof(buf) ? 0 : ret;
}

static int ti_ads7142_reg_read(const struct i2c_client *client, u8 reg,
			       u8 *data)
{
    u8 buf[2] = { TI_ADS7142_OC_SINGLE_REG_READ, reg };
    struct i2c_msg msg[2] = {
        {
            .addr = client->addr,
            .len = sizeof(buf),
            .buf = buf,
        },
        {
            .addr = client->addr,
            .flags = I2C_M_RD,
            .len = 1,
            .buf = data,
        }
    };
    int ret;

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret >= 0 && ret != ARRAY_SIZE(msg))
        ret = -EIO;

    return ret == ARRAY_SIZE(msg) ? 0 : ret;
}

static int ti_ads7142_data_buffer_read(const struct i2c_client *client,
				       int length, void *data)
{
    int ret;

    ret = i2c_master_recv(client, data, length);
    if (ret >= 0 && ret != length)
        ret = -EIO;

    return ret == length ? 0 : ret;
}

static int ti_ads7142_soft_reset(const struct i2c_client *client)
{
    u8 buf[2] = { TI_ADS7142_OC_GENERAL, 0x06 };
    int ret;

    ret = i2c_master_send(client, buf, sizeof(buf));
    if (ret >= 0 && ret != sizeof(buf))
        ret = -EIO;

    return ret == sizeof(buf) ? 0 : ret;
}

static int ti_ads7142_address2channel(struct iio_dev *indio_dev,
				      int address,
				      struct ti_ads7142_channel **channel)
{
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);
    int i;

    for (i = 0; i < indio_dev->num_channels; i++) {
        if (address == priv->channels[i].channel) {
            *channel = &priv->channels[i];
            return 0;
        }
    }
    return -ENODEV;
}

static int ti_ads7142_sequence_start(struct iio_dev *indio_dev)
{
    struct i2c_client *client = to_i2c_client(indio_dev->dev.parent);

    return ti_ads7142_reg_write(client, TI_ADS7142_START_SEQUENCE,
				TI_ADS7142_START_SEQUENCE_SEQ_START);
}

static int ti_ads7142_sequence_abort(struct iio_dev *indio_dev)
{
    struct i2c_client *client = to_i2c_client(indio_dev->dev.parent);

    return ti_ads7142_reg_write(client, TI_ADS7142_ABORT_SEQUENCE,
				TI_ADS7142_ABORT_SEQUENCE_SEQ_ABORT);
}

static int ti_ads7142_osc_set(struct iio_dev *indio_dev)
{
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);
    struct i2c_client *client = to_i2c_client(indio_dev->dev.parent);
    int ret;

    ret = ti_ads7142_reg_write(client, TI_ADS7142_OSC_SEL,
			       priv->config.osc_sel ? TI_ADS7142_OSC_SEL_HSZ_LP : 0);
    if (ret)
        return ret;

    return ti_ads7142_reg_write(client, TI_ADS7142_NCLK_SEL,
				priv->config.n_clk);
}

static int ti_ads7142_input_cfg_set(struct iio_dev *indio_dev)
{
    struct i2c_client *client = to_i2c_client(indio_dev->dev.parent);

    return ti_ads7142_reg_write(client, TI_ADS7142_CH_INPUT_CFG,
				TI_ADS7142_CH_INPUT_CFG_TCSE);
}

static int ti_ads7142_dout_format_set(struct iio_dev *indio_dev)
{
    struct i2c_client *client = to_i2c_client(indio_dev->dev.parent);

    return ti_ads7142_reg_write(client, TI_ADS7142_DOUT_FORMAT_CFG,
				TI_ADS7142_DOUT_FORMAT_CFG_12BCHDV);
}

static int ti_ads7142_osc_calc_set(struct iio_dev *indio_dev, int request)
{
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);
    bool osc_sel;
    u32 osc_freq;

    if (request <= TI_ADS7142_LPO_MAX_SS) {
        osc_sel = true;
        if (request < TI_ADS7142_LPO_MIN_SS)
            request = TI_ADS7142_LPO_MIN_SS;
        osc_freq = TI_ADS7142_LPO_FREQ;
    } else {
        osc_sel = false;
        if (request < TI_ADS7142_HSO_MIN_SS)
            request = TI_ADS7142_HSO_MIN_SS;
        if (request > TI_ADS7142_HSO_MAX_SS)
            request = TI_ADS7142_HSO_MAX_SS;
        osc_freq = TI_ADS7142_HSO_FREQ;
    }

    priv->config.osc_sel = osc_sel;
    priv->config.n_clk = (osc_freq / request) & TI_ADS7142_NCLK_SEL_MSK;
    return 0;
}

static int ti_ads7142_osc_calc_get(struct iio_dev *indio_dev, int *result)
{
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);
    u32 n_clk;
    u32 osc_freq;

    if (priv->config.osc_sel) {
        // Low power oscillator
        n_clk = priv->config.n_clk;
        if (n_clk < TI_ADS7142_LPO_NCLK_MIN)
            n_clk = TI_ADS7142_LPO_NCLK_MIN;
        if (n_clk > TI_ADS7142_LPO_NCLK_MAX)
            n_clk = TI_ADS7142_LPO_NCLK_MAX;
        osc_freq = TI_ADS7142_LPO_FREQ;
    } else {
        // High speed oscillator
        n_clk = priv->config.n_clk;
        if (n_clk < TI_ADS7142_HSO_NCLK_MIN)
            n_clk = TI_ADS7142_HSO_NCLK_MIN;
        if (n_clk > TI_ADS7142_HSO_NCLK_MAX)
            n_clk = TI_ADS7142_HSO_NCLK_MAX;
	osc_freq = TI_ADS7142_HSO_FREQ;
    }
    *result = osc_freq / n_clk;
    return 0;
}

static int ti_ads7142_hth_set(struct iio_dev *indio_dev, int channel,
			      int threshold)
{
    struct i2c_client *client = to_i2c_client(indio_dev->dev.parent);
    int ret;

    if (threshold < 0 || threshold > TI_ADS7142_THRESHOLD_MSK)
        return -EINVAL;

    ret = ti_ads7142_reg_write(client,
			       TI_ADS7142_DWC_HTH_CH0_LSB + channel * 4,
			       threshold & 0xFF);
    if (ret)
        return ret;

    ret = ti_ads7142_reg_write(client,
			       TI_ADS7142_DWC_HTH_CH0_MSB + channel * 4,
			       (threshold >> 8) & 0xF);
    return ret;
}

static int ti_ads7142_lth_set(struct iio_dev *indio_dev, int channel,
			      int threshold)
{
    struct i2c_client *client = to_i2c_client(indio_dev->dev.parent);
    int ret;

    if (threshold < 0 || threshold > TI_ADS7142_THRESHOLD_MSK)
        return -EINVAL;

    ret = ti_ads7142_reg_write(client,
			       TI_ADS7142_DWC_LTH_CH0_LSB + channel * 4,
			       threshold & 0xFF);
    if (ret)
        return ret;

    ret = ti_ads7142_reg_write(client,
			       TI_ADS7142_DWC_LTH_CH0_MSB + channel * 4,
			       (threshold >> 8) & 0xF);
    return ret;
}

static int ti_ads7142_hys_set(struct iio_dev *indio_dev, int channel,
			      int hysteresis)
{
    struct i2c_client *client = to_i2c_client(indio_dev->dev.parent);
    int ret;

    if (hysteresis < 0 || hysteresis > TI_ADS7142_HYSTERESIS_MSK)
        return -EINVAL;

    ret = ti_ads7142_reg_write(client, TI_ADS7142_DWC_HYS_CH0 + channel,
			       hysteresis & TI_ADS7142_HYSTERESIS_MSK);
    return ret;
}

static int ti_ads7142_buffered_setup_and_start(struct iio_dev *indio_dev)
{
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);
    struct i2c_client *client = to_i2c_client(indio_dev->dev.parent);
    struct ti_ads7142_channel *channel;
    int i;
    u8 alert_ch = 0;
    u8 buffer_op_mode;
    u8 seq_channels = 0;
    int ret;

    if (!priv->config.buffer_mode)
        return 0;

    priv->monitor_pending = false;

    ret = ti_ads7142_sequence_abort(indio_dev);
    if (ret)
        return ret;

    ret = ti_ads7142_osc_set(indio_dev);
    if (ret)
        return ret;

    ret = ti_ads7142_input_cfg_set(indio_dev);
    if (ret)
        return ret;

    ret = ti_ads7142_dout_format_set(indio_dev);
    if (ret)
        return ret;

    switch (priv->config.buffer_mode) {
    case TI_ADS7142_BUFFM_STOP_BURST:
        buffer_op_mode = TI_ADS7142_DATA_BUFFER_OPMODE_STOP_BURST;
    break;
    case TI_ADS7142_BUFFM_START_BURST:
        buffer_op_mode = TI_ADS7142_DATA_BUFFER_OPMODE_START_BURST;
    break;
    case TI_ADS7142_BUFFM_PRE_ALERT:
        buffer_op_mode = TI_ADS7142_DATA_BUFFER_OPMODE_PRE_ALERT;
    break;
    case TI_ADS7142_BUFFM_POST_ALERT:
        buffer_op_mode = TI_ADS7142_DATA_BUFFER_OPMODE_POST_ALERT;
    break;
    default:
        return -EINVAL;
    break;
    }
    ret = ti_ads7142_reg_write(client, TI_ADS7142_DATA_BUFFER_OPMODE,
			       buffer_op_mode);
    if (ret)
        return ret;

    ret = ti_ads7142_reg_write(client, TI_ADS7142_OPMODE_SEL,
			       TI_ADS7142_OPMODE_SEL_MONITORING);
    if (ret)
        return ret;

    for_each_set_bit(i, indio_dev->active_scan_mask,
	             indio_dev->masklength) {
        seq_channels |= 1 << i;
    }

    ret = ti_ads7142_reg_write(client, TI_ADS7142_AUTO_SEQ_CHEN,
		               seq_channels);
    if (ret)
        return ret;

    if (priv->config.buffer_mode < TI_ADS7142_BUFFM_PRE_ALERT)
        goto seq_start;

    /*
     * Pre and post alert settings
     */
    ret = ti_ads7142_reg_write(client, TI_ADS7142_PRE_ALT_EVT_CNT, 0);
    if (ret)
        return ret;

    ret = ti_ads7142_reg_write(client, TI_ADS7142_ALT_LOW_FLAGS,
			       TI_ADS7142_ALT_LOW_FLAGS_CH0
			       | TI_ADS7142_ALT_LOW_FLAGS_CH1);
    if (ret)
        return ret;

    ret = ti_ads7142_reg_write(client, TI_ADS7142_ALT_HIGH_FLAGS,
			       TI_ADS7142_ALT_HIGH_FLAGS_CH0
			       | TI_ADS7142_ALT_HIGH_FLAGS_CH1);
    if (ret)
        return ret;

    for_each_set_bit(i, indio_dev->active_scan_mask,
	             indio_dev->masklength) {
        ret = ti_ads7142_address2channel(indio_dev, i,
				         &channel);
        if (ret)
            return ret;

        ret = ti_ads7142_hth_set(indio_dev, channel->channel,
			         channel->config.high_threshold);
        if (ret)
            return ret;

        ret = ti_ads7142_lth_set(indio_dev, channel->channel,
				 channel->config.low_threshold);
        if (ret)
            return ret;

        ret = ti_ads7142_hys_set(indio_dev, channel->channel,
				 channel->config.hysteresis);
        if (ret)
            return ret;

        if (channel->config.alert_low ||
	    channel->config.alert_high) {
            alert_ch |= 1 << channel->channel;
        }
    }

    ret = ti_ads7142_reg_write(client, TI_ADS7142_ALERT_DWC_EN,
			       alert_ch ? TI_ADS7142_ALERT_DWC_EN_BLOCK_EN : 0);
    if (ret)
        return ret;

    ret = ti_ads7142_reg_write(client, TI_ADS7142_ALERT_CHEN,
			       alert_ch);
    if (ret)
        return ret;

    if (!alert_ch)
        return ret;

seq_start:
    ret = ti_ads7142_sequence_start(indio_dev);
    priv->monitor_pending = !ret;

    return ret;
}

static int ti_ads7142_buffered_collect(struct iio_dev *indio_dev,
				       int *channel_collected)
{
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);
    struct i2c_client *client = to_i2c_client(indio_dev->dev.parent);
    int scan_channel_count;
    int have_valid_data;
    int data_valid;
    u16 data_buffer;
    u16 buffer[TI_ADS7142_CHANNEL_COUNT];
    u8 seq_channels = 0;
    int channel_address;
    int value;
    int i, j;
    int ret;

    if (!priv->scan_data)
        return -EINVAL;

    scan_channel_count = bitmap_weight(indio_dev->active_scan_mask,
				       indio_dev->masklength);
    if (!scan_channel_count)
        return -EINVAL;

    for_each_set_bit(i, indio_dev->active_scan_mask,
		     indio_dev->masklength) {
        seq_channels |= 1 << i;
    }

    do {
        memset(priv->scan_data, 0x00, indio_dev->scan_bytes);
        have_valid_data = 0;
        for (i = 0; i < scan_channel_count; i++) {
            ret = ti_ads7142_data_buffer_read(client,
					      sizeof(data_buffer),
					      &data_buffer);
            if (ret)
                return ret;
            data_buffer = be16_to_cpu(data_buffer);
            data_valid = data_buffer & 1;
            if (!data_valid) {
                ret = -ENOENT;
                break;
            }

            channel_address = (data_buffer >> 1) & 0x7;
            if (!(seq_channels & 1 << channel_address)) {
		dev_err(indio_dev->dev.parent,
			"%s: invalid channel address(%d)",
			__func__, channel_address);
                    return -EIO;
            }

            value = data_buffer >> 4;
            buffer[channel_address] = value;
            have_valid_data = 1;
            if (channel_collected)
                *channel_collected |= 1 << channel_address;
        }

        if (!have_valid_data)
            continue;

        j = 0;
        for_each_set_bit(i, indio_dev->active_scan_mask,
	                 indio_dev->masklength) {
            priv->scan_data[j] = buffer[i];
            j++;
        }
        iio_push_to_buffers_with_timestamp(indio_dev, priv->scan_data,
					   iio_get_time_ns(indio_dev));
    } while (data_valid);

    return ret;
}

static int ti_ads7142_buffered_abort(struct iio_dev *indio_dev)
{
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);
    int ret;

    ret = ti_ads7142_sequence_abort(indio_dev);
    if (!ret)
        priv->monitor_pending = false;

    return ret;
}

static int ti_ads7142_manual_read(struct iio_dev *indio_dev,
				  int address, int *val)
{
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);
    struct i2c_client *client = to_i2c_client(indio_dev->dev.parent);
    u16 data_buffer;
    int ret;

    mutex_lock(&priv->lock);
    ret = ti_ads7142_sequence_abort(indio_dev);
    if (ret)
        goto final;

    ret = ti_ads7142_osc_set(indio_dev);
    if (ret)
        goto final;

    ret = ti_ads7142_input_cfg_set(indio_dev);
    if (ret)
        goto final;

    ret = ti_ads7142_dout_format_set(indio_dev);
    if (ret)
        goto final;

    ret = ti_ads7142_reg_write(client, TI_ADS7142_OPMODE_SEL,
			       TI_ADS7142_OPMODE_SEL_MANUALSEQ);
				   
    if (ret)
        goto final;

    ret = ti_ads7142_reg_write(client, TI_ADS7142_AUTO_SEQ_CHEN,
			       1 << address);
    if (ret)
        goto final;

    ret = ti_ads7142_sequence_start(indio_dev);
    if (ret)
        goto final;

    ret = ti_ads7142_data_buffer_read(client, sizeof(data_buffer),
				      &data_buffer);
    if (ret)
        goto abort;

    *val = (be16_to_cpu(data_buffer) >> 4);

abort:
    ret = ti_ads7142_sequence_abort(indio_dev);
final:
    mutex_unlock(&priv->lock);
    return ret;
}

static irqreturn_t ti_ads7142_ist(int irq, void *dev_id)
{
    struct iio_dev *indio_dev = dev_id;
    struct i2c_client *client = to_i2c_client(indio_dev->dev.parent);
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);
    struct ti_ads7142_channel *channel;
    u8 low_flags;
    u8 high_flags;
    u8 seq_st;
    int i;
    int ret;
    int channel_collected;
    s64 timestamp = iio_get_time_ns(indio_dev);

    mutex_lock(&priv->lock);
    if (!priv->config.buffer_mode || !priv->monitor_pending) {
        mutex_unlock(&priv->lock);
        return IRQ_NONE;
    }

    /*
     * BUSY/PDY fires when the sequence stopped in
     * trigger handler(ti_ads7142_trigger_handler),
     * if buffer mode is stop_burst, all the required
     * operations are in trigger handler, so irq handler
     * simple returns at this point.
     */
    if (priv->config.buffer_mode == TI_ADS7142_BUFFM_STOP_BURST) {
        mutex_unlock(&priv->lock);
        return IRQ_NONE;
    }

    ret = ti_ads7142_reg_read(client, TI_ADS7142_SEQUENCE_STATUS, &seq_st);
    if (ret) {
        dev_err(indio_dev->dev.parent,
                "%s: SEQUENCE_STATUS reg read error(%i)",
		__func__, ret);
        goto final;
    }

    if ((seq_st & TI_ADS7142_SEQUENCE_STATUS_SEQ_ERR_ST_MSK)
	!= TI_ADS7142_SEQUENCE_STATUS_SEQ_ENABLED) {
        dev_err(indio_dev->dev.parent,
	        "%s: SEQUENCE_STATUS error(%i)",
		__func__, seq_st);
        goto final;
    }

    ret = ti_ads7142_reg_read(client, TI_ADS7142_ALT_LOW_FLAGS,
			      &low_flags);
    if (ret) {
        dev_err(indio_dev->dev.parent,
		"%s: ALT_LOW_FLAGS reg read error(%i)",
		__func__, ret);
        goto final;
    }

    ret = ti_ads7142_reg_read(client, TI_ADS7142_ALT_HIGH_FLAGS,
			      &high_flags);
    if (ret) {
        dev_err(indio_dev->dev.parent,
	        "%s: ALT_HIGH_FLAGS reg read error(%i)",
		__func__, ret);
        goto final;
    }

    ret = ti_ads7142_sequence_abort(indio_dev);
    if (ret)
        goto final;

    priv->monitor_pending = false;

    channel_collected = 0;
    ret = ti_ads7142_buffered_collect(indio_dev, &channel_collected);
    if (ret && ret != -ENOENT) {
        dev_err(indio_dev->dev.parent,
		"%s: error(%d) when collecting result for %s mode",
		__func__, ret,
		ti_ads7142_buffer_modes[priv->config.buffer_mode]);
        goto final;
    }

    if (ret == -ENOENT)
        ret = 0;

    if (!channel_collected)
        goto final;

    if (priv->config.buffer_mode < TI_ADS7142_BUFFM_PRE_ALERT)
        goto final;

    for_each_set_bit(i, indio_dev->active_scan_mask,
		     indio_dev->masklength) {
        ret = ti_ads7142_address2channel(indio_dev, i,
					 &channel);
        if (ret)
            goto final;

        if (!(channel_collected & (1 << channel->channel)))
            continue;
        if (channel->config.alert_low &&
	    (low_flags & (1 << channel->channel))) {
            iio_push_event(indio_dev,
			   IIO_UNMOD_EVENT_CODE(IIO_VOLTAGE,
						i,
						IIO_EV_TYPE_THRESH,
						IIO_EV_DIR_FALLING),
			   timestamp);
        }

        if (channel->config.alert_high &&
            (high_flags & (1 << channel->channel))) {
            iio_push_event(indio_dev,
			   IIO_UNMOD_EVENT_CODE(IIO_VOLTAGE,
					        i,
						IIO_EV_TYPE_THRESH,
						IIO_EV_DIR_RISING),
			   timestamp);
        }
    }

final:
    if (!ret && priv->config.buffer_mode >= TI_ADS7142_BUFFM_PRE_ALERT) {
        ret = ti_ads7142_buffered_setup_and_start(indio_dev);
        if (ret) {
            dev_err(indio_dev->dev.parent,
		    "%s: error(%d) when starting %s mode",
		    __func__, ret,
		    ti_ads7142_buffer_modes[priv->config.buffer_mode]);
        }
    }
    mutex_unlock(&priv->lock);
    if (ret)
        return IRQ_NONE;

    return IRQ_HANDLED;
}

static int ti_ads7142_read_raw(struct iio_dev *indio_dev,
			       struct iio_chan_spec const *chan,
			       int *val, int *val2, long info)
{
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);
    int ret;

	
    switch (info) {
    case IIO_CHAN_INFO_RAW:
        if (iio_buffer_enabled(indio_dev))
            return -EBUSY;
        ret = iio_device_claim_direct_mode(indio_dev);
        if (ret)
            return ret;
        ret = ti_ads7142_manual_read(indio_dev, chan->address,
				     val);
        if (!ret)
            ret = IIO_VAL_INT;
        iio_device_release_direct_mode(indio_dev);
        return ret;
    case IIO_CHAN_INFO_SAMP_FREQ:
        ti_ads7142_osc_calc_get(indio_dev, val);
        return IIO_VAL_INT;
    case IIO_CHAN_INFO_SCALE:
        if (IS_ERR(priv->avdd)) {
            ret = -EINVAL;
        } else {
            *val = regulator_get_voltage(priv->avdd) / 1000;
            *val2 = chan->scan_type.realbits;
            ret = IIO_VAL_FRACTIONAL_LOG2;
        }
        return ret;
    default:
        return -EINVAL;
    }
    return 0;
}

static int ti_ads7142_write_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int val, int val2, long mask)
{
    switch (mask) {
    case IIO_CHAN_INFO_SAMP_FREQ:
        return ti_ads7142_osc_calc_set(indio_dev, val);
    default:
        return -EINVAL;
    }

    return 0;
}

static int ti_ads7142_read_event_value(struct iio_dev *indio_dev,
				       const struct iio_chan_spec *chan,
				       enum iio_event_type type,
				       enum iio_event_direction dir,
				       enum iio_event_info info,
				       int *val, int *val2)
{
    struct ti_ads7142_channel *channel;
    int ret;

    ret = ti_ads7142_address2channel(indio_dev, chan->address,
				     &channel);
    if (ret)
        return ret;

    switch (info) {
    case IIO_EV_INFO_VALUE:
        if (dir == IIO_EV_DIR_RISING)
            *val = channel->config.high_threshold;
        else
            *val = channel->config.low_threshold;
        return IIO_VAL_INT;
    case IIO_EV_INFO_HYSTERESIS:
        *val = channel->config.hysteresis;
        return IIO_VAL_INT;
    default:
        return -EINVAL;
    }
    return 0;
}

static int ti_ads7142_write_event_value(struct iio_dev *indio_dev,
					const struct iio_chan_spec *chan,
					enum iio_event_type type,
					enum iio_event_direction dir,
					enum iio_event_info info,
					int val, int val2)
{
    struct ti_ads7142_channel *channel;
    int ret;

    ret = ti_ads7142_address2channel(indio_dev, chan->address,
				     &channel);
    if (ret)
        return ret;

    switch (info) {
    case IIO_EV_INFO_VALUE:
        if (val < 0 || val > TI_ADS7142_THRESHOLD_MSK) {
            ret = -EINVAL;
        } else {
            if (dir == IIO_EV_DIR_RISING)
                channel->config.high_threshold = val;
            else
                channel->config.low_threshold = val;
        }
    break;
    case IIO_EV_INFO_HYSTERESIS:
        if (val < 0 || val > TI_ADS7142_HYSTERESIS_MSK)
            ret = -EINVAL;
        else
            channel->config.hysteresis = val;
    break;
    default:
        ret = -EINVAL;
    break;
    }

    return ret;
}

static int ti_ads7142_read_event_config(struct iio_dev *indio_dev,
					const struct iio_chan_spec *chan,
					enum iio_event_type type,
					enum iio_event_direction dir)
{
    struct ti_ads7142_channel *channel;
    int ret;

    if (type != IIO_EV_TYPE_THRESH)
        return -EINVAL;

    ret = ti_ads7142_address2channel(indio_dev, chan->address,
				     &channel);
    if (ret)
        return ret;

    if (dir == IIO_EV_DIR_RISING)
        ret = channel->config.alert_high ? 1 : 0;
    else
        ret = channel->config.alert_low ? 1 : 0;

    return ret;
}

static int ti_ads7142_write_event_config(struct iio_dev *indio_dev,
					 const struct iio_chan_spec *chan,
					 enum iio_event_type type,
					 enum iio_event_direction dir,
					 int state)
{
    struct ti_ads7142_channel *channel;
    int ret;

    if (type != IIO_EV_TYPE_THRESH)
        return -EINVAL;

    ret = ti_ads7142_address2channel(indio_dev, chan->address,
				     &channel);
    if (ret)
        return ret;

    if (dir == IIO_EV_DIR_RISING)
        channel->config.alert_high = state;
    else
        channel->config.alert_low = state;

    return ret;
}

static const struct iio_info ti_ads7142_iio_info = {
    .read_raw		= ti_ads7142_read_raw,
    .write_raw		= ti_ads7142_write_raw,
    .read_event_value	= ti_ads7142_read_event_value,
    .write_event_value	= ti_ads7142_write_event_value,
    .read_event_config	= ti_ads7142_read_event_config,
    .write_event_config	= ti_ads7142_write_event_config,
};

static int ti_ads7142_triggered_buffer_preenable(struct iio_dev *indio_dev)
{
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);
    int ret;

    if (!priv->config.buffer_mode)
        return -EINVAL;

    /*
     * Start/stop burst buffer modes requires trigger
     */
    if (priv->config.buffer_mode <= TI_ADS7142_BUFFM_START_BURST &&
	!indio_dev->trig) {
        dev_err(indio_dev->dev.parent,
		"%s: Start/stop burst buffer modes requires trigger",
		__func__);
        return -EINVAL;
    }

    /*
     * Start burst and pre/post alert modes requires irq
     */
    if (priv->config.buffer_mode >= TI_ADS7142_BUFFM_START_BURST &&
	!priv->irq_present) {
        dev_err(indio_dev->dev.parent,
		"%s: Start burst and pre/post alert modes requires irq",
		__func__);
        return -EINVAL;
    }

    //priv->scan_data = devm_krealloc(indio_dev->dev.parent,
    priv->scan_data = devm_kmalloc(indio_dev->dev.parent,
				   //priv->scan_data,
				   indio_dev->scan_bytes, GFP_KERNEL);
    if (!priv->scan_data)
        return -ENOMEM;

    mutex_lock(&priv->lock);
    /*
     * Start burst mode started in trigger handler.
     * Sequencer aborted here, just for safe.
     */
    if (priv->config.buffer_mode == TI_ADS7142_BUFFM_START_BURST)
        ret = ti_ads7142_buffered_abort(indio_dev);
    else
        ret = ti_ads7142_buffered_setup_and_start(indio_dev);
    mutex_unlock(&priv->lock);

    return ret;
}

static int ti_ads7142_triggered_buffer_predisable(struct iio_dev *indio_dev)
{
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);
    int ret = 0;

    mutex_lock(&priv->lock);
    ti_ads7142_buffered_abort(indio_dev);
    mutex_unlock(&priv->lock);

    return ret;
}

static const struct iio_buffer_setup_ops ti_ads7142_triggered_buffer_ops = {
    .preenable = &ti_ads7142_triggered_buffer_preenable,
    .predisable = &ti_ads7142_triggered_buffer_predisable,
};

static irqreturn_t ti_ads7142_trigger_handler(int irq, void *p)
{
    struct iio_poll_func *pf = p;
    struct iio_dev *indio_dev = pf->indio_dev;
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);
    int ret = 0;

    mutex_lock(&priv->lock);
    if (priv->config.buffer_mode == TI_ADS7142_BUFFM_STOP_BURST) {
        ret = ti_ads7142_buffered_abort(indio_dev);
        if (ret) {
            dev_err(indio_dev->dev.parent,
		    "%s: error(%d) when aborting in %s mode",
		    __func__, ret,
		    ti_ads7142_buffer_modes[priv->config.buffer_mode]);
        }

        ret = ti_ads7142_buffered_collect(indio_dev, NULL);
        if (ret && ret != -ENOENT) {
            dev_err(indio_dev->dev.parent,
		    "%s: error(%d) when collecting result for %s mode",
		    __func__, ret,
		    ti_ads7142_buffer_modes[priv->config.buffer_mode]);
        }

        if (ret == -ENOENT)
            ret = 0;
    }
    if (!ret &&
	(priv->config.buffer_mode == TI_ADS7142_BUFFM_START_BURST ||
	 priv->config.buffer_mode == TI_ADS7142_BUFFM_STOP_BURST)) {
        ret = ti_ads7142_buffered_setup_and_start(indio_dev);
        if (ret) {
            dev_err(indio_dev->dev.parent,
		    "%s: error(%d) when starting %s mode",
		    __func__, ret,
		    ti_ads7142_buffer_modes[priv->config.buffer_mode]);
        }
    }

    mutex_unlock(&priv->lock);

    if (ret)
        return IRQ_NONE;

    iio_trigger_notify_done(indio_dev->trig);

    return IRQ_HANDLED;
}

static int ti_ads7142_get_buffer_mode(struct iio_dev *indio_dev,
				      const struct iio_chan_spec *chan)
{
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);

    return priv->config.buffer_mode;
}

static int ti_ads7142_set_buffer_mode(struct iio_dev *indio_dev,
				      const struct iio_chan_spec *chan,
				      unsigned int mode)
{
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);
    int ret;

    if (mode > TI_ADS7142_BUFFM_STOP_BURST && !priv->irq_present) {
	dev_err(indio_dev->dev.parent,
		"%s: no irq(BUSY/RDY) specified, mode %s is not supported",
		__func__, ti_ads7142_buffer_modes[mode]);
        return -EINVAL;
    }

    ret = iio_device_claim_direct_mode(indio_dev);
    if (ret)
        return ret;

    priv->config.buffer_mode = mode;
    iio_device_release_direct_mode(indio_dev);
    return 0;
}

static const struct iio_enum ti_ads7142_buffer_modes_enum = {
    .items = ti_ads7142_buffer_modes,
    .num_items = ARRAY_SIZE(ti_ads7142_buffer_modes),
    .get = ti_ads7142_get_buffer_mode,
    .set = ti_ads7142_set_buffer_mode,
};

static const struct iio_chan_spec_ext_info ti_ads7142_ext_info[] = {
    IIO_ENUM("buffer_mode", IIO_SHARED_BY_ALL,
	     &ti_ads7142_buffer_modes_enum),
    {
        .name = "buffer_mode_available",
        .shared = IIO_SHARED_BY_ALL,
        .read = iio_enum_available_read,
        .private = (uintptr_t)&ti_ads7142_buffer_modes_enum,
    },
    { },
};

static int ti_ads7142_parse_channel_config(struct device *dev,
					   struct iio_dev *indio_dev)
{
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);
    struct fwnode_handle *channel_node;
    struct iio_chan_spec *iio_channels;
    struct iio_chan_spec *iio_channel;
    struct ti_ads7142_channel *ads_channel;
    int channel_index = 0;
    int channel_count;
    int ret;
    int channel = 0;

    /* assign channel count to local variable which obtained during module load*/
    channel_count = g_chn_count;
    if (!channel_count) {
        dev_err(dev, "dt: there is no channel definition");
        return -ENODEV;
    }

    if (channel_count > TI_ADS7142_CHANNEL_COUNT) {
        dev_err(dev, "dt: invalid number of channel definitions");
        return -ENODEV;
    }

    priv->channels = devm_kcalloc(dev, channel_count,
				  sizeof(*priv->channels),
				  GFP_KERNEL);
    if (!priv->channels)
        return -ENOMEM;

    indio_dev->num_channels = channel_count;
    iio_channels = devm_kcalloc(dev, channel_count, sizeof(*iio_channels),
				GFP_KERNEL);
    if (!iio_channels)
        return -ENOMEM;

    indio_dev->channels = iio_channels;
	
    while(channel < channel_count) {
        ads_channel = &priv->channels[channel_index];

        ads_channel->channel = channel;
        iio_channel = &iio_channels[channel_index];
        iio_channel->datasheet_name = ti_ads7142_ain_names[ads_channel->channel];
        iio_channel->type = IIO_VOLTAGE;
        iio_channel->indexed = 1;		
        iio_channel->info_mask_separate = BIT(IIO_CHAN_INFO_RAW);
        if (!IS_ERR_OR_NULL(priv->avdd))
            iio_channel->info_mask_separate |= BIT(IIO_CHAN_INFO_SCALE);
        iio_channel->info_mask_shared_by_all = BIT(IIO_CHAN_INFO_SAMP_FREQ);
        iio_channel->scan_type.sign = 'u';
        iio_channel->scan_type.realbits = 12;
        iio_channel->scan_type.storagebits = 16;
        iio_channel->scan_type.endianness = IIO_CPU;
        iio_channel->address = ads_channel->channel;
        iio_channel->scan_index = ads_channel->channel;
        iio_channel->channel = ads_channel->channel;
        iio_channel->event_spec = ti_ads7142_events;
        iio_channel->num_event_specs = ARRAY_SIZE(ti_ads7142_events);
        iio_channel->ext_info = ti_ads7142_ext_info;
		
        ads_channel->config.high_threshold = TI_ADS7142_THRESHOLD_MSK;
        channel_index++;
        channel++;
    }

    return 0;
}

static int ti_ads7142_parse_config(struct device *dev,
				   struct iio_dev *indio_dev)
{
    return ti_ads7142_parse_channel_config(dev, indio_dev);
}

static void ti_ads7142_regulators_disable(void *data)
{
    struct ti_ads7142_priv *priv = data;

    if (!IS_ERR_OR_NULL(priv->avdd))
        regulator_disable(priv->avdd);
    if (!IS_ERR_OR_NULL(priv->dvdd))
        regulator_disable(priv->dvdd);
}

static int ti_ads7142_probe(struct i2c_client *client)
{
    struct iio_dev *indio_dev;
    struct ti_ads7142_priv *priv;
    int ret;

    indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*priv));
    if (!indio_dev)
        return -ENOMEM;

    i2c_set_clientdata(client, indio_dev);
    priv = iio_priv(indio_dev);

    /**
     * starting from v5.9-rc1 iio_device_alloc
     *  sets indio_dev->dev.parent, but older versions not :(
     **/
    if (!indio_dev->dev.parent)
        indio_dev->dev.parent = &client->dev;
    indio_dev->name = TI_ADS7142_NAME;
    indio_dev->modes = INDIO_DIRECT_MODE | INDIO_BUFFER_SOFTWARE;
    indio_dev->info = &ti_ads7142_iio_info;

    priv->avdd = devm_regulator_get(&client->dev, "avdd");
    if (!IS_ERR_OR_NULL(priv->avdd)) {
        ret = regulator_enable(priv->avdd);
        if (ret)
            return ret;
    }

    priv->dvdd = devm_regulator_get(&client->dev, "dvdd");
    if (!IS_ERR_OR_NULL(priv->dvdd)) {
        ret = regulator_enable(priv->dvdd);
        if (ret)
            goto final;
    }
    ret = devm_add_action_or_reset(&client->dev,
				   ti_ads7142_regulators_disable,
				   priv);
    if (ret)
        goto final;

    ret = ti_ads7142_soft_reset(client);
    if (ret)
        goto final;

    ret = ti_ads7142_parse_config(&client->dev, indio_dev);
    if (ret)
        goto final;

    mutex_init(&priv->lock);

    if (client->irq) {
        ret = devm_request_threaded_irq(&client->dev, client->irq,
					NULL, ti_ads7142_ist,
					IRQF_ONESHOT | IRQF_SHARED,
					dev_name(&client->dev),
					indio_dev);
        if (ret) {
            dev_err(&client->dev, "Unable to request IRQ %i",
		    client->irq);
            goto final;
        }
        priv->irq_present = true;
    }

    ret = devm_iio_triggered_buffer_setup(&client->dev,
					  indio_dev,
					  &iio_pollfunc_store_time,
					  &ti_ads7142_trigger_handler,
					  &ti_ads7142_triggered_buffer_ops);
    if (ret)
        goto final;

    ret = devm_iio_device_register(&client->dev, indio_dev);
    if (ret) {
        dev_err(&client->dev, "Failed to register iio device");
        goto final;
    }

    dev_info(&client->dev, "%s is a %s device at address 0x%X",
	     dev_name(&indio_dev->dev), indio_dev->name,
	     client->addr);
final:
    return ret;
}

static int __maybe_unused ti_ads7142_suspend(struct device *dev)
{
    struct iio_dev *indio_dev = i2c_get_clientdata(to_i2c_client(dev));
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);

    mutex_lock(&priv->lock);
    do {
        /*
         * Keep all regulators on when the device in autonomous
         *  monitoring mode.
         * The device can wake up the system with ALERT pin
         */
        if (priv->monitor_pending &&
	    (priv->config.buffer_mode == TI_ADS7142_BUFFM_PRE_ALERT ||
	     priv->config.buffer_mode == TI_ADS7142_BUFFM_POST_ALERT))
            continue;

        if (!IS_ERR_OR_NULL(priv->avdd))
            regulator_disable(priv->avdd);
        if (!IS_ERR_OR_NULL(priv->dvdd))
            regulator_disable(priv->dvdd);
    } while (0);
    mutex_unlock(&priv->lock);

    return 0;
}

static int __maybe_unused ti_ads7142_resume(struct device *dev)
{
    struct iio_dev *indio_dev = i2c_get_clientdata(to_i2c_client(dev));
    struct ti_ads7142_priv *priv = iio_priv(indio_dev);
    int ret = 0;

    mutex_lock(&priv->lock);
    do {
        /*
         * Nothing to do when the device in autonomous monitoring mode.
         */
        if (priv->monitor_pending &&
	    (priv->config.buffer_mode == TI_ADS7142_BUFFM_PRE_ALERT ||
	     priv->config.buffer_mode == TI_ADS7142_BUFFM_POST_ALERT))
            continue;

        if (!IS_ERR_OR_NULL(priv->avdd)) {
            ret = regulator_enable(priv->avdd);
            if (ret)
                continue;
        }
        if (!IS_ERR_OR_NULL(priv->dvdd)) {
            ret = regulator_enable(priv->dvdd);
            if (ret)
                continue;
        }
    } while (0);
    mutex_unlock(&priv->lock);

    return ret;
}

static SIMPLE_DEV_PM_OPS(ti_ads7142_pm_ops, ti_ads7142_suspend,
			 ti_ads7142_resume);

static const struct i2c_device_id ti_ads7142_id[] = {
    { TI_ADS7142_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ti_ads7142_id);

static const struct of_device_id ti_ads7142_of_match[] = {
    { .compatible = "ti,ads7142" },
    {}
};
MODULE_DEVICE_TABLE(of, ti_ads7142_of_match);

static struct i2c_driver ti_ads7142_driver = {
    .driver = {
        .name = TI_ADS7142_NAME,
        .of_match_table = ti_ads7142_of_match,
        .pm = &ti_ads7142_pm_ops,
    },
    .probe		= ti_ads7142_probe,
    .id_table	= ti_ads7142_id,
};

module_i2c_driver(ti_ads7142_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jozsef Horvath <info@ministro.hu>");
MODULE_DESCRIPTION("Texas Instruments ADS7142 ADC driver");

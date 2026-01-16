// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Hardware monitoring driver for MPS Multi-phase Digital VR Controllers
 *
 * Copyright (C) 2020 Nvidia Technologies Ltd.
 */

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pmbus.h>
#include "pmbus.h"

/* Vendor specific registers. */

#define MP2888_MFR_SYS_CONFIG   0x44
#define MP2888_MFR_READ_CS1_2   0x73
#define MP2888_MFR_READ_CS3_4   0x74
#define MP2888_MFR_READ_CS5_6   0x75
#define MP2888_MFR_READ_CS7_8   0x76
#define MP2888_MFR_READ_CS9_10  0x77
#define MP2888_MFR_VR_CONFIG1   0xe1

#define MP2888_TOTAL_CURRENT_RESOLUTION BIT(3)
#define MP2888_PHASE_CURRENT_RESOLUTION BIT(4)
#define MP2888_DRMOS_KCS        GENMASK(2, 0)
#define MP2888_TEMP_UNIT        10
#define MP2888_MAX_PHASE        10

struct mp2882_data {
    struct pmbus_driver_info info;
    struct pmbus_sensor *sensor;
    int total_curr_resolution;
    int phase_curr_resolution;
    int curr_sense_gain;
};

#define to_mp2882_data(x)  container_of(x, struct mp2882_data, info)

#define mfr_vout_loop_ctrl_r1 0xb2
#define mfr_vr_cfg1 0xb7
#define mfr_slope_dcm_set 0xc1
#define mfr_trim_sel 0xc2
static unsigned short process_vout(struct i2c_client *client, int page, int phase, int reg)
{
    unsigned short mfr_vout_cfg;
    unsigned int vout;
    int gain_para[4][2] = {
        {6250, 5},
        {6250, 5},
        {1953, 5},
        {3906, 5},
    };
    int gain_sel;
    int vid_step_sel;

    mfr_vout_cfg = pmbus_read_word_data(client, page, phase, mfr_vout_loop_ctrl_r1);
    vout = pmbus_read_word_data(client, page, phase, reg);
    gain_sel = (mfr_vout_cfg >> 10) & 0x3;
    vid_step_sel = (mfr_vout_cfg >> 14) & 0x3;

    if (vid_step_sel > 1)
        return -ENODATA;

    vout = vout * gain_para[gain_sel][vid_step_sel];
    if (vid_step_sel == 0)
        vout = vout/1000;

    return vout;
}

static unsigned short process_power(struct i2c_client *client, int page, int phase, int reg)
{
    unsigned short mfr_vr_cfg;
    unsigned short power;
    unsigned char modulus[] = {2, 3};

    mfr_vr_cfg = pmbus_read_word_data(client, 0, phase, mfr_vr_cfg1);
    mfr_vr_cfg = (mfr_vr_cfg >> 6) & 0x1;
    power = pmbus_read_word_data(client, page, phase, reg);
    power = power >> modulus[mfr_vr_cfg];

    return power;
}

static unsigned int process_iout(struct i2c_client *client, int page, int phase, int reg)
{
    unsigned short data;

    data = pmbus_read_word_data(client, page, phase, reg);
    data &= 0x3ff;

    return data;
}

static unsigned int process_iout_limit(struct i2c_client *client, int page, int phase,  int reg)
{
    unsigned int data;
    unsigned int ocp_tdc_spike;
    struct pmbus_data *pdata;
    static unsigned int kcs[] = {50, 85, 90, 97, 100};
    static unsigned int trim_sel[][3] = {
        {0, 10000, 4},
        {1, 10000, 4},
        {4, 5000, 4},
        {5, 10000, 3},
        {6, 40000, 4},
        {7, 10000, 3},
        {8, 40000, 4},
        {9, 40000, 3},
        {10, 5000, 4},
        {11, 25000, 4},
        {13, 400, 4},
        {14, 400, 2},
        {15, 40000, 3},
        {0, 0, 0},
    };
    int i;

    data = pmbus_read_word_data(client, page, phase, mfr_slope_dcm_set);
    data = data >> 10;
    data &= 0x7;
    if (data < sizeof(kcs)/sizeof(unsigned int)) {
        ocp_tdc_spike = pmbus_read_word_data(client, page, phase, PMBUS_IOUT_OC_FAULT_LIMIT);
        if (reg == PMBUS_IOUT_OC_WARN_LIMIT) {
            ocp_tdc_spike = ocp_tdc_spike >> 8;
        }
        ocp_tdc_spike &= 0xff;
        ocp_tdc_spike *= kcs[data];

        data = pmbus_read_word_data(client, 1, phase, mfr_trim_sel);
        if (page == 1)
            data = data >> 8;
        data &= 0xf;
        for (i=0; trim_sel[i][1] != 0; i++) {
            if (trim_sel[i][0] == data) {
                ocp_tdc_spike = ocp_tdc_spike * trim_sel[i][1];
                ocp_tdc_spike = ocp_tdc_spike >> trim_sel[i][2];
                ocp_tdc_spike /= 62500;
                pdata = i2c_get_clientdata(client);
                ocp_tdc_spike *= pdata->info->m[PSC_CURRENT_OUT];
                return ocp_tdc_spike;
            }
        }
    }

    return -ENODATA;
}

static unsigned int process_temp_warn(struct i2c_client *client, int page, int phase, int reg)
{
    unsigned int data;

    data = pmbus_read_word_data(client, page, phase, reg);

    data &= 0xff;

    return data;
}

static unsigned int process_temp_warn_write(struct i2c_client *client, int page, int reg, unsigned short word)
{
    unsigned short reg_val;

    if (word > 255)
        return -ENOMEM;

    reg_val = pmbus_read_word_data(client, page, 0xFFFF, reg);
    reg_val &= 0xff00;
    reg_val |=  word;

    return pmbus_write_word_data(client, page, reg, reg_val);
}

static int mp2882_read_word_data(struct i2c_client *client, int page, int phase, int reg)
{
    switch (reg) {
        case PMBUS_IOUT_UC_FAULT_LIMIT:
            break;
        case PMBUS_READ_IOUT:
            return process_iout(client, page, phase, reg);
        case PMBUS_IOUT_OC_WARN_LIMIT:
        case PMBUS_IOUT_OC_FAULT_LIMIT:
            return process_iout_limit(client, page, phase, reg);

        case PMBUS_OT_WARN_LIMIT: // _max
        case PMBUS_OT_FAULT_LIMIT: // _crit
            return process_temp_warn(client, page, phase, reg);

        case PMBUS_READ_VOUT:
            return process_vout(client, page, phase, reg);

        case PMBUS_READ_POUT:
            return process_power(client, page, phase, reg);

        default:
            break;
    }
    return -ENODATA;
}

static int mp2882_write_word_data(struct i2c_client *client, int page, int reg, unsigned short word)
{
    switch (reg) {
        case PMBUS_IOUT_UC_FAULT_LIMIT:
        case PMBUS_IOUT_OC_WARN_LIMIT:
        case PMBUS_IOUT_OC_FAULT_LIMIT:
            break;

        case PMBUS_OT_WARN_LIMIT: // _max
        case PMBUS_OT_FAULT_LIMIT: //_crit
            return process_temp_warn_write(client, page, reg, word);

        default:
            break;
    }

    return -EPERM;
}

static struct pmbus_platform_data mp2882_pdata = {0};
static struct pmbus_driver_info mp2882_info = {0};

static int mp2882_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct pmbus_driver_info *info;
    struct mp2882_data *data;
    unsigned short cfg_data;

    data = devm_kzalloc(&client->dev, sizeof(struct mp2882_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    client->dev.platform_data = &mp2882_pdata;
    mp2882_pdata.flags = PMBUS_SKIP_STATUS_CHECK;

    memcpy(&data->info, &mp2882_info, sizeof(*info));
    info = &data->info;

    /* start init param */
    info->pages = 2;

    info->format[PSC_VOLTAGE_IN] = linear;
    info->format[PSC_VOLTAGE_OUT] = linear;
    info->format[PSC_TEMPERATURE] = direct,
    info->format[PSC_CURRENT_OUT] = direct,
    info->format[PSC_POWER] = linear,

    info->m[PSC_TEMPERATURE] = 1;
    info->b[PSC_TEMPERATURE] = 0;
    info->R[PSC_TEMPERATURE] = 0;

    info->m[PSC_CURRENT_OUT] = 4;
    info->b[PSC_CURRENT_OUT] = 0;
    info->R[PSC_CURRENT_OUT] = 0;

    i2c_smbus_write_byte_data(client, PMBUS_PAGE, 0x0);
    cfg_data = i2c_smbus_read_word_data(client, mfr_vr_cfg1);
    cfg_data = cfg_data | (0x3 << 6);
    i2c_smbus_write_word_data(client, mfr_vr_cfg1, cfg_data);

    info->func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_IOUT | PMBUS_HAVE_POUT | PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_VOUT |
                    PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_STATUS_INPUT | PMBUS_HAVE_STATUS_TEMP;
    
    info->func[1] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_IOUT | PMBUS_HAVE_POUT | PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_VOUT |
                    PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_STATUS_INPUT | PMBUS_HAVE_STATUS_TEMP;

    info->read_word_data  = mp2882_read_word_data;
    info->write_word_data  = mp2882_write_word_data;

    return pmbus_do_probe(client, info);
}
static const struct i2c_device_id mp2882_id[] = {
    {"mp2882", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, mp2882_id);

static const struct of_device_id __maybe_unused mp2882_of_match[] = {
    {.compatible = "mps,mp2882"},
    {}
};
MODULE_DEVICE_TABLE(of, mp2882_of_match);

static struct i2c_driver mp2882_driver = {
    .driver = {
        .name = "mp2882",
        .of_match_table = of_match_ptr(mp2882_of_match),
    },
    .probe = mp2882_probe,
    .id_table = mp2882_id,
};

module_i2c_driver(mp2882_driver);

MODULE_AUTHOR("Vadim Pasternak <vadimp@nvidia.com>");
MODULE_DESCRIPTION("PMBus driver for MPS MP2882 device");
MODULE_LICENSE("GPL");

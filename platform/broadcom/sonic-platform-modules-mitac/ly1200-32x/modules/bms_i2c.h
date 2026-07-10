#ifndef _BMS_I2C_H_
#define _BMS_I2C_H_

const char *bms_i2c_adapter_names[] = {
    "SMBus I801 adapter",
    "SMBus iSMT adapter",
};

enum i2c_adapter_type {
    I2C_ADAPTER_I801 = 0,
    I2C_ADAPTER_ISMT,
};

enum bms_module_switch_bus {
    I2C_STAGE1_MUX_CHAN0 = 0,
    I2C_STAGE1_MUX_CHAN1,
    I2C_STAGE1_MUX_CHAN2,
    I2C_STAGE1_MUX_CHAN3,
    I2C_STAGE1_MUX_CHAN4,
    I2C_STAGE1_MUX_CHAN5,
    I2C_STAGE1_MUX_CHAN6,
    I2C_STAGE1_MUX_CHAN7,
};

#endif /* _BMS_I2C_H_ */

